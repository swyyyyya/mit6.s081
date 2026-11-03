> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab lock: Parallelism/locking](https://pdos.csail.mit.edu/6.828/2021/labs/lock.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](lock.md) · [官方原始 HTML](original/lock.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：锁

本实验让你练习通过重新设计代码来提高并行度。多核机器上并行性差的常见表现是锁竞争严重。提高并行度通常需要同时修改数据结构与加锁策略，以减少竞争。你将对 xv6 的内存分配器和块缓存进行这样的改进。

编码前，请阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)的以下部分：

- 第 6 章“锁”及对应代码。
- 第 3.5 节“代码：物理内存分配器”。
- 第 8.1 至 8.3 节“概述”“缓冲区缓存层”“代码：缓冲区缓存”。

```text
  $ git fetch
  $ git checkout lock
  $ make clean
```

## 内存分配器

`user/kalloctest` 会对 xv6 内存分配器进行压力测试：三个进程不断扩大、缩小各自地址空间，从而大量调用 `kalloc` 和 `kfree`。两者都会获取 `kmem.lock`。对于 `kmem` 等几把锁，`kalloctest` 会以 `#fetch-and-add` 的形式，打印 `acquire` 因尝试获取已被其他核心持有的锁而执行的循环次数。这一次数可以粗略衡量锁竞争程度。完成实验前，输出类似：

```text
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 83375 #acquire() 433015
lock: bcache: #fetch-and-add 0 #acquire() 1260
--- top 5 contended locks:
lock: kmem: #fetch-and-add 83375 #acquire() 433015
lock: proc: #fetch-and-add 23737 #acquire() 130718
lock: virtio_disk: #fetch-and-add 11159 #acquire() 114
lock: proc: #fetch-and-add 5937 #acquire() 130786
lock: proc: #fetch-and-add 4080 #acquire() 130786
tot= 83375
test1 FAIL
```

`acquire` 为每把锁维护获取调用次数，以及内部循环尝试设置锁却失败的次数。`kalloctest` 调用一个系统调用，让内核打印 `kmem`、`bcache`（本实验重点）及竞争最激烈的另外 5 把锁的计数。存在竞争时，`acquire` 的循环次数会很大。该系统调用返回 `kmem` 和 `bcache` 锁循环次数的总和。

本实验必须使用一台专用、没有其他负载的多核机器。如果机器同时处理其他任务，`kalloctest` 打印的计数就没有意义。可使用专用 Athena 工作站或自己的笔记本，不要使用拨入共享机器（dialup machine）。

`kalloctest` 中锁竞争的根本原因是 `kalloc()` 只有一个空闲链表，并由一把锁保护。为消除竞争，需要重新设计分配器，避免只有一把锁和一条链表。基本思路是每个 CPU 维护自己的空闲链表，各自使用一把锁。不同 CPU 上的分配和释放可以并行，因为操作的是不同链表。主要挑战是：某 CPU 的空闲链表为空，而另一个 CPU 还有空闲内存时，前者必须“窃取”后者的一部分空闲链表。窃取可能引入锁竞争，但希望这种情况不常发生。

你的任务是实现每 CPU 空闲链表，以及链表为空时的窃取机制。所有锁的名称必须以 `"kmem"` 开头，即对每把锁调用 `initlock`，传入以 `"kmem"` 开头的名字。运行 `kalloctest`，检查是否减少了竞争。运行 `usertests sbrkmuch`，检查是否仍能分配全部内存。输出应类似下方，`kmem` 锁的总竞争大幅减少，但具体数值可能不同。确保 `usertests` 的全部测试通过。`make grade` 应报告 `kalloctest` 测试通过。

```text
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 42843
lock: kmem: #fetch-and-add 0 #acquire() 198674
lock: kmem: #fetch-and-add 0 #acquire() 191534
lock: bcache: #fetch-and-add 0 #acquire() 1242
--- top 5 contended locks:
lock: proc: #fetch-and-add 43861 #acquire() 117281
lock: virtio_disk: #fetch-and-add 5347 #acquire() 114
lock: proc: #fetch-and-add 4856 #acquire() 117312
lock: proc: #fetch-and-add 4168 #acquire() 117316
lock: proc: #fetch-and-add 2797 #acquire() 117266
tot= 0
test1 OK
start test2
total free number of pages: 32499 (out of 32768)
.....
test2 OK
$ usertests sbrkmuch
usertests starting
test sbrkmuch: OK
ALL TESTS PASSED
$ usertests
...
ALL TESTS PASSED
$
```

一些提示：

- 可以使用 `kernel/param.h` 中的常量 `NCPU`。
- 让 `freerange` 将全部空闲内存交给运行 `freerange` 的 CPU。
- `cpuid` 返回当前核心编号，但只有关闭中断时，调用它并使用返回值才安全。使用 `push_off()` 和 `pop_off()` 关闭、恢复中断。
- 可参考 `kernel/sprintf.c` 中的 `snprintf`，了解字符串格式化；不过，将全部锁都命名为 `"kmem"` 也可以。

## 缓冲区缓存

本部分独立于前半部分。无论是否完成前半部分，都可以完成这一部分并通过其测试。

多个进程密集使用文件系统时，很可能竞争 `bcache.lock`，它保护 `kernel/bio.c` 中的磁盘块缓存。`bcachetest` 创建多个进程，反复读取不同文件，制造 `bcache.lock` 竞争。完成实验前，输出类似：

```text
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 33035
lock: bcache: #fetch-and-add 16142 #acquire() 65978
--- top 5 contended locks:
lock: virtio_disk: #fetch-and-add 162870 #acquire() 1188
lock: proc: #fetch-and-add 51936 #acquire() 73732
lock: bcache: #fetch-and-add 16142 #acquire() 65978
lock: uart: #fetch-and-add 7505 #acquire() 117
lock: proc: #fetch-and-add 6937 #acquire() 73420
tot= 16142
test0: FAIL
start test1
test1 OK
```

你看到的数值可能不同，但 `bcache` 锁的 `acquire` 循环次数会很高。阅读 `kernel/bio.c` 可知，`bcache.lock` 保护缓存块缓冲区链表、每个块缓冲区的引用计数 `b->refcnt`，以及缓存块的标识 `b->dev` 和 `b->blockno`。

修改块缓存，使运行 `bcachetest` 时，bcache 所有锁的 `acquire` 循环次数接近零。理想情况下，块缓存涉及的所有锁的计数之和应为零，但小于 500 也可接受。修改 `bget` 和 `brelse`，使对缓存中不同块的并发查找和释放不易在锁上冲突，例如，不必都等待 `bcache.lock`。必须保持“每个块最多只有一个缓存副本”的不变量。完成后的输出应与下方相似，但不必完全相同。确保 `usertests` 仍通过，且 `make grade` 的所有测试都通过。

```text
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 32954
lock: kmem: #fetch-and-add 0 #acquire() 75
lock: kmem: #fetch-and-add 0 #acquire() 73
lock: bcache: #fetch-and-add 0 #acquire() 85
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4159
lock: bcache.bucket: #fetch-and-add 0 #acquire() 2118
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4274
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4326
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6334
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6321
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6704
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6696
lock: bcache.bucket: #fetch-and-add 0 #acquire() 7757
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6199
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4136
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4136
lock: bcache.bucket: #fetch-and-add 0 #acquire() 2123
--- top 5 contended locks:
lock: virtio_disk: #fetch-and-add 158235 #acquire() 1193
lock: proc: #fetch-and-add 117563 #acquire() 3708493
lock: proc: #fetch-and-add 65921 #acquire() 3710254
lock: proc: #fetch-and-add 44090 #acquire() 3708607
lock: proc: #fetch-and-add 43252 #acquire() 3708521
tot= 128
test0: OK
start test1
test1 OK
$ usertests
  ...
ALL TESTS PASSED
$
```

所有锁的名称请以 `"bcache"` 开头，即为每把锁调用 `initlock`，传入以 `"bcache"` 开头的名字。

块缓存竞争比 `kalloc` 更难减少，因为 bcache 缓冲区确实由多个进程、从而多个 CPU 共享。对于 `kalloc`，每 CPU 一个分配器可消除大部分竞争，但块缓存不能这样处理。建议使用哈希表按块号查找缓存，每个哈希桶一把锁。

以下情况允许出现锁冲突：

- 两个进程并发使用同一个块号。`bcachetest` 的 `test0` 从不这样做。
- 两个进程并发缓存未命中，都需要找一个未使用的块进行替换。`test0` 从不这样做。
- 两个进程使用的块在你的块和锁划分方案中发生冲突。例如，两个块号哈希到同一个桶。视你的设计，`test0` 可能出现这种情况，但应尝试调整方案细节来避免冲突，例如改变哈希表大小。

`bcachetest` 的 `test1` 使用的不同块数量超过缓冲区数量，并执行许多文件系统代码路径。

一些提示：

- 阅读 xv6 教材 8.1—8.3 节对块缓存的说明。
- 可以固定桶数量，无需动态调整哈希表大小。使用素数个桶，例如 13，可降低哈希冲突概率。
- 在哈希表中查找缓冲区，以及找不到时为其分配条目，这两个操作必须作为一个原子操作完成。
- 移除所有缓冲区的全局链表，如 `bcache.head` 等，改为给缓冲区记录最后使用时间戳，使用 `kernel/trap.c` 的 `ticks`。这样 `brelse` 无需获取 bcache 锁，`bget` 可根据时间戳选择最近最少使用的块。
- 可以串行化 `bget` 的淘汰过程，即缓存未命中时选择可复用缓冲区的部分。
- 某些情况可能需要同时持有两把锁，例如淘汰时的 bcache 锁与某个桶的锁。确保避免死锁。
- 替换块时，新块可能哈希到另一个桶，因此需要将 `struct buf` 从一个桶移到另一个桶。棘手情况是新旧块可能哈希到同一桶；确保此时也不会死锁。
- 调试建议：先实现桶锁，但保留 `bget` 开头和结尾对全局 `bcache.lock` 的获取／释放，暂时将代码串行化。确认代码正确、没有竞态后，再移除全局锁，处理并发问题。也可运行 `make CPUS=1 qemu`，在单核下测试。

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

## 可选挑战练习

- 让缓冲区缓存的查找操作无锁化。提示：使用 GCC 的 `__sync_*` 函数。如何确信实现是正确的？
