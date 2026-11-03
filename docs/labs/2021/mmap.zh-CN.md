> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab mmap: Mmap](https://pdos.csail.mit.edu/6.828/2021/labs/mmap.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](mmap.md) · [官方原始 HTML](original/mmap.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：mmap

`mmap` 和 `munmap` 系统调用使 UNIX 程序能够精细控制自己的地址空间。它们可用于在进程间共享内存、将文件映射到进程地址空间，以及作为用户级缺页处理方案的一部分，例如课堂讨论的垃圾回收算法。本实验将向 xv6 添加 `mmap` 和 `munmap`，重点是内存映射文件。

获取实验源码，并检出 `mmap` 分支：

```text
  $ git fetch
  $ git checkout mmap
  $ make clean
```

手册页（运行 `man 2 mmap`）给出了以下声明：

```text
void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
```

`mmap` 的用法很多，但本实验只要求实现与文件内存映射有关的部分功能。可以假定 `addr` 总为零，意味着由内核决定映射文件的虚拟地址。`mmap` 返回该地址，失败时返回 `0xffffffffffffffff`。`length` 为映射字节数，可能与文件长度不同。`prot` 指定映射内存是否可读、可写、可执行；可以假定其为 `PROT_READ`、`PROT_WRITE` 或二者组合。`flags` 只会是 `MAP_SHARED` 或 `MAP_PRIVATE`：前者表示对映射内存的修改应写回文件，后者表示不写回。不需要实现其他标志位。`fd` 是待映射文件的已打开文件描述符。可以假定 `offset` 为零，它表示映射在文件中的起始位置。

对于以 `MAP_SHARED` 映射同一文件的进程，即使它们**不共享**物理页，也可以接受。

`munmap(addr, length)` 应删除指定地址范围内的 mmap 映射。如果进程修改过内存，且使用 `MAP_SHARED` 映射，应先将修改写回文件。一次 `munmap` 可能仅覆盖映射区域的一部分，但可以假定取消映射的范围位于区域开头、末尾，或覆盖整个区域，不会在中间挖洞。

只需实现足以让 `mmaptest` 正常工作的 `mmap` 和 `munmap` 功能。`mmaptest` 未使用的功能不需要实现。

完成后，应看到以下输出：

```text
$ mmaptest
mmap_test starting
test mmap f
test mmap f: OK
test mmap private
test mmap private: OK
test mmap read-only
test mmap read-only: OK
test mmap read/write
test mmap read/write: OK
test mmap dirty
test mmap dirty: OK
test not-mapped unmap
test not-mapped unmap: OK
test mmap two files
test mmap two files: OK
mmap_test: ALL OK
fork_test starting
fork_test OK
mmaptest: all tests succeeded
$ usertests
usertests starting
...
ALL TESTS PASSED
$
```

一些提示：

- 先将 `_mmaptest` 加入 `UPROGS`，添加 `mmap` 和 `munmap` 系统调用，使 `user/mmaptest.c` 编译通过。目前让两个调用直接返回错误即可。`kernel/fcntl.h` 已定义 `PROT_READ` 等常量。运行 `mmaptest`，它会在第一次 mmap 调用处失败。
- 响应缺页异常，惰性填充页表。也就是说，`mmap` 不应分配物理内存或读取文件，而是在 `usertrap` 内或其调用的缺页处理代码中完成，类似惰性页分配实验。这样可以让大文件 mmap 足够快，也能映射比物理内存更大的文件。
- 为每个进程记录 `mmap` 创建的映射。定义与第 15 讲介绍的 VMA（虚拟内存区域）对应的结构，记录每个映射范围的地址、长度、权限、文件等。由于 xv6 内核没有相应的内存分配器，可声明固定大小的 VMA 数组，按需从中分配，16 项应足够。
- 实现 `mmap`：在进程地址空间寻找未使用区域映射文件，并向进程映射区域表添加 VMA。VMA 应保存指向被映射文件的 `struct file` 的指针。`mmap` 应增加文件引用计数，防止文件关闭后结构消失，提示：参见 `filedup`。运行 `mmaptest`，第一次 `mmap` 应成功，但首次访问映射内存时会触发缺页异常，导致 `mmaptest` 被终止。
- 添加代码，使映射区域中的缺页异常分配一页物理内存，从对应文件读取 4096 字节到该页，并映射进用户地址空间。使用 `readi` 读取文件，它接收文件内的读取偏移参数，但需要对传给 `readi` 的 inode 加锁／解锁。不要忘记正确设置页面权限。运行 `mmaptest`，此时应能执行到第一次 `munmap`。
- 实现 `munmap`：找到地址范围对应的 VMA，取消指定页面的映射，提示：使用 `uvmunmap`。如果移除了此前某次 mmap 的全部页面，应减少对应 `struct file` 的引用计数。如果被取消映射的页面已被修改且映射为 `MAP_SHARED`，将页面写回文件。可参考 `filewrite`。
- 理想实现只应写回程序实际修改过的 `MAP_SHARED` 页面。RISC-V PTE 的脏位 `D` 表示页面是否被写过。不过，`mmaptest` 不检查未修改页面是否被写回，因此不检查 `D` 位、直接写回也能通过。
- 修改 `exit`，使其像调用 `munmap` 一样取消进程的全部映射区域。运行 `mmaptest`，`mmap_test` 应通过，但 `fork_test` 可能仍失败。
- 修改 `fork`，确保子进程具有与父进程相同的映射区域。不要忘记增加 VMA 对应的 `struct file` 引用计数。在子进程缺页处理程序中，可以分配新物理页，而不与父进程共享页面。共享会更好，但实现工作更多。运行 `mmaptest`，`mmap_test` 和 `fork_test` 都应通过。

运行 `usertests`，确保其他功能仍正常。

## 提交实验

**本实验到此完成。** 请确保通过所有 `make grade` 测试。如果本实验包含问题，不要忘记将答案写入 `answers-*lab-name*.txt`。提交你的改动（包括添加 `answers-*lab-name*.txt`），然后在实验目录中运行 `make handin` 交付实验。

### 花费时间

新建文件 `time.txt`，只写入一个整数，表示你在本实验上花费的小时数。不要忘记对该文件执行 `git add` 和 `git commit`。

### 提交

你将通过[提交网站](https://6828.scripts.mit.edu/2021/handin.py/)提交作业。在提交任何作业或实验之前，需要先到提交网站申请一次 API 密钥。

提交实验的最终代码改动后，运行 `make handin` 交付实验。

```text
$ git commit -am "ready to submit my lab"
[util c2e3c8b] ready to submit my lab
 2 files changed, 18 insertions(+), 2 deletions(-)

$ make handin
tar: Removing leading `/' from member names
Get an API key for yourself by visiting https://6828.scripts.mit.edu/2021/handin.py/
Please enter your API key: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 79258  100   239  100 79019    853   275k --:--:-- --:--:-- --:--:--  276k
$
```

`make handin` 会将你的 API 密钥保存在 `myapi.key` 中。需要更换密钥时，删除此文件，让 `make handin` 重新生成即可（`myapi.key` 不能包含换行符）。

如果运行 `make handin` 时还有未提交的改动或未跟踪的文件，将看到类似以下输出：

```text
 M hello.c
?? bar.c
?? foo.pyc
Untracked files will not be handed in.  Continue? [y/N]
```

检查上述各行，确保实验解答需要的所有文件都已被 Git 跟踪，即没有列在以 `??` 开头的行中。可以用 `git add filename` 让 Git 跟踪你新建的文件。

如果 `make handin` 无法正常工作，请尝试排查所涉及的 curl 或 Git 命令。也可以运行 `make tarball`；它会生成一个 tar 文件，随后可通过[网页界面](https://6828.scripts.mit.edu/2021/handin.py/)上传。

- 请运行 `make grade`，确保代码通过全部测试。
- 运行 `make handin` 前，提交所有修改过的源代码。
- 你可以在 <https://6828.scripts.mit.edu/2021/handin.py/> 查看提交状态并下载已提交的代码。

## 可选挑战

- 两个进程映射同一文件时，例如 `fork_test`，共享它们的物理页。需要为物理页维护引用计数。
- 你的实现可能对从映射文件读取的每一页都分配新物理页，尽管数据已在内核缓冲区缓存中。修改实现，直接使用缓存中的物理内存，而不是分配新页。这要求文件块与页面大小相同，将 `BSIZE` 设为 4096。还需将映射块固定在缓冲区缓存中，并处理引用计数。
- 消除惰性分配和文件映射实现中的重复部分。提示：为惰性分配区域创建 VMA。
- 修改 `exec`，为可执行文件的不同段使用 VMA，实现按需分页的可执行文件。这会加快程序启动，因为 `exec` 不必从文件系统读取任何数据。
- 实现换出和换入：物理内存不足时，内核将进程的部分内存移到磁盘；进程访问时，再将被换出的内存调入。
