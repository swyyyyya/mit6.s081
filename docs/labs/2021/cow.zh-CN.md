> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab cow: Copy-on-write fork](https://pdos.csail.mit.edu/6.828/2021/labs/cow.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](cow.md) · [官方原始 HTML](original/cow.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：为 xv6 实现写时复制 fork

虚拟内存提供了一层间接寻址：内核可以把页表项（PTE）标记为无效或只读，使内存访问触发缺页异常，从而拦截访问；还可以通过修改 PTE，改变地址所指向的内容。计算机系统领域有一句话：任何系统问题都可以通过增加一层间接性来解决。惰性分配实验展示了一个例子。本实验将探索另一个例子：写时复制 fork。

开始实验前，切换到 `cow` 分支：

```text
$ git fetch
$ git checkout cow
$ make clean
```

## 问题

xv6 中的 `fork()` 系统调用会将父进程的全部用户空间内存复制给子进程。如果父进程很大，复制会花费很长时间。更糟的是，这些工作通常大部分都被浪费了；例如，子进程在 `fork()` 后执行 `exec()`，就会丢弃复制来的内存，而其中大部分可能根本没有用过。另一方面，如果父子进程都使用某个页面，并且其中一个或两个进程向该页面写入，确实就需要复制。

## 解决办法

写时复制（copy-on-write，COW）`fork()` 的目标是：将为子进程分配和复制物理内存页的工作推迟到真正需要副本时；如果始终不需要，就不做这些工作。

COW `fork()` 只为子进程创建页表，让用户内存的 PTE 指向父进程的物理页，并把父子进程的所有用户 PTE 都标记为不可写。当任一进程尝试写入这些 COW 页面时，CPU 会触发缺页异常。内核的缺页异常处理程序识别出这种情况后，为发生异常的进程分配一个物理页，将原页面复制到新页面，再修改该进程对应的 PTE，使其指向新页面，并将 PTE 标记为可写。缺页异常处理程序返回后，用户进程就可以写入自己的页面副本。

COW `fork()` 会使用户内存所用物理页的释放稍微复杂一些。同一物理页可能被多个进程的页表引用，只有最后一个引用消失后，才能释放它。

## 实现写时复制

你的任务是在 xv6 内核中实现写时复制 fork。如果修改后的内核能够成功执行 `cowtest` 和 `usertests` 两个程序，任务就完成了。

为帮助测试实现，我们提供了 xv6 程序 `cowtest`，源码位于 `user/cowtest.c`。它会运行多种测试，但未修改的 xv6 连第一个测试都无法通过。因此，最初会看到：

```text
$ cowtest
simple: fork() failed
$
```

`simple` 测试会分配超过可用物理内存一半的内存，然后调用 `fork()`。此时 fork 失败，因为剩余物理内存不足以为子进程提供父进程内存的完整副本。

完成后，内核应通过 `cowtest` 和 `usertests` 中的全部测试，即：

```text
$ cowtest
simple: ok
simple: ok
three: zombie!
ok
three: zombie!
ok
three: zombie!
ok
file: ok
ALL COW TESTS PASSED
$ usertests
...
ALL TESTS PASSED
$
```

下面是一种合理的实现步骤。

1. 修改 `uvmcopy()`，将父进程的物理页映射到子进程，而不是分配新页面。清除父子进程 PTE 中的 `PTE_W`。
2. 修改 `usertrap()`，识别缺页异常。如果缺页异常发生在 COW 页面上，使用 `kalloc()` 分配新页面，将旧页面复制到新页面，再把新页面安装到 PTE 中，并设置 `PTE_W`。
3. 确保每个物理页都在最后一个 PTE 引用消失时释放，而且不能提前释放。一种好办法是为每个物理页维护一个“引用计数”，记录引用它的用户页表数量。`kalloc()` 分配页面时，将引用计数设为 1；fork 使子进程共享页面时增加引用计数；任何进程从页表移除该页面时减少引用计数。只有引用计数为零时，`kfree()` 才应把页面放回空闲链表。可以用固定大小的整数数组保存计数。你需要设计数组索引方式和大小。例如，可以用页面物理地址除以 4096 作为索引，并使数组元素数量等于 `kalloc.c` 中 `kinit()` 加入空闲链表的所有页面中的最高物理地址。
4. 修改 `copyout()`，使其遇到 COW 页面时使用与缺页异常相同的处理方案。

一些提示：

- 惰性页分配实验可能已让你熟悉许多与写时复制有关的 xv6 内核代码。不过，不要基于惰性分配实验的解答实现本实验；请按照前面的指示，从一份全新的 xv6 代码开始。
- 为每个 PTE 记录它是否是 COW 映射可能会有帮助。可以使用 RISC-V PTE 中的 RSW（保留给软件使用）位。
- `usertests` 覆盖一些 `cowtest` 没有测试的场景，因此不要忘记确保两者的全部测试都能通过。
- `kernel/riscv.h` 末尾有一些有用的页表标志宏和定义。
- 如果发生 COW 缺页异常时没有空闲内存，应终止该进程。

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

- 修改 xv6，使其同时支持惰性页分配和 COW。
- 测量你的 COW 实现使 xv6 复制的字节数和分配的物理页数量减少了多少。寻找并利用进一步减少这些数值的机会。
