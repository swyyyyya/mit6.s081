> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab util: Unix utilities](https://pdos.csail.mit.edu/6.828/2021/labs/util.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](util.md) · [官方原始 HTML](original/util.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：xv6 与 Unix 实用工具

本实验将帮助你熟悉 xv6 及其系统调用。

## 启动 xv6

你可以在 Athena 机器或自己的电脑上完成这些实验。如果使用自己的电脑，请查看[实验工具页面](tools.zh-CN.md)，了解环境配置建议。

如果使用 Athena，必须使用 x86 机器，也就是说，`uname -a` 的输出应包含 `i386 GNU/Linux`、`i686 GNU/Linux` 或 `x86_64 GNU/Linux`。可以使用 `ssh -X athena.dialup.mit.edu` 登录公共 Athena 主机。我们已经在 Athena 上为你配置了适当的编译器和模拟器。运行 `add -f 6.828` 即可使用。每次登录时都必须运行此命令，也可以将其添加到 `~/.environment` 文件中。如果编译或运行 `qemu` 时出现难以理解的错误，请检查是否已添加课程软件目录（course locker）。

获取实验的 xv6 源码，并检出 `util` 分支：

```text
$ git clone git://g.csail.mit.edu/xv6-labs-2021
Cloning into 'xv6-labs-2021'...
...
$ cd xv6-labs-2021
$ git checkout util
Branch 'util' set up to track remote branch 'util' from 'origin'.
Switched to a new branch 'util'
```

`xv6-labs-2021` 仓库与教材使用的 `xv6-riscv` 略有不同，主要是增加了一些文件。如果好奇，可以查看 Git 日志：

```text
$ git log
```

本实验以及后续实验所需的文件通过 [Git](http://www.git-scm.com/) 版本控制系统分发。上面你已经切换到一个包含本实验专用 xv6 版本的分支（`git checkout util`）。要进一步了解 Git，请参阅 [Git 用户手册](http://www.kernel.org/pub/software/scm/git/docs/user-manual.html)，或者这篇[面向计算机科学背景读者的 Git 概述](http://eagain.net/articles/git-for-computer-scientists/)。Git 可以跟踪你对代码所做的修改。例如，完成某个练习、希望保存当前进度时，可以运行以下命令提交改动：

```text
$ git commit -am 'my solution for util lab exercise 1'
Created commit 60d2135: my solution for util lab exercise 1
 1 files changed, 1 insertions(+), 0 deletions(-)
$
```

可以使用 `git diff` 跟踪改动。`git diff` 显示自上次提交以来的代码变化，`git diff origin/util` 显示相对于最初的 `xv6-labs-2021` 代码的变化。这里，`origin/xv6-labs-2021` 是你为本课程下载的初始代码所在的 Git 分支名称。

构建并运行 xv6：

```text
$ make qemu
riscv64-unknown-elf-gcc    -c -o kernel/entry.o kernel/entry.S
riscv64-unknown-elf-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -DSOL_UTIL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o kernel/start.o kernel/start.c
...  
riscv64-unknown-elf-ld -z max-page-size=4096 -N -e main -Ttext 0 -o user/_zombie user/zombie.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-unknown-elf-objdump -S user/_zombie > user/zombie.asm
riscv64-unknown-elf-objdump -t user/_zombie | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/zombie.sym
mkfs/mkfs fs.img README  user/xargstest.sh user/_cat user/_echo user/_forktest user/_grep user/_init user/_kill user/_ln user/_ls user/_mkdir user/_rm user/_sh user/_stressfs user/_usertests user/_grind user/_wc user/_zombie 
nmeta 46 (boot, super, log blocks 30 inode blocks 13, bitmap blocks 1) blocks 954 total 1000
balloc: first 591 blocks have been allocated
balloc: write bitmap block at sector 45
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

xv6 kernel is booting

hart 2 starting
hart 1 starting
init: starting sh
$
```

在提示符后输入 `ls`，应看到类似以下输出：

```text
$ ls
.              1 1 1024
..             1 1 1024
README         2 2 2059
xargstest.sh   2 3 93
cat            2 4 24256
echo           2 5 23080
forktest       2 6 13272
grep           2 7 27560
init           2 8 23816
kill           2 9 23024
ln             2 10 22880
ls             2 11 26448
mkdir          2 12 23176
rm             2 13 23160
sh             2 14 41976
stressfs       2 15 24016
usertests      2 16 148456
grind          2 17 38144
wc             2 18 25344
zombie         2 19 22408
console        3 20 0
```

这些是 `mkfs` 放入初始文件系统的文件，大部分是可以运行的程序。你刚刚已经运行了其中一个：`ls`。

xv6 没有 `ps` 命令，但按下 `Ctrl-p` 后，内核会打印每个进程的信息。现在试一下，应看到两行，分别对应 `init` 和 `sh`。

退出 QEMU：输入 `Ctrl-a x`。

## 评分与提交流程

可以运行 `make grade`，使用评分程序测试你的实现。助教将使用同一个评分程序为你的实验提交评分。此外，我们还会进行实验检查面谈，详见[评分政策](https://pdos.csail.mit.edu/6.828/2021/general.html#grading)。

实验代码附带 GNU Make 规则，便于提交。提交最终代码改动后，运行 `make handin` 交付实验。详细提交方法见[下文提交部分](#提交)。

## sleep

为 xv6 实现 UNIX 程序 `sleep`；它应暂停用户指定数量的时钟滴答（tick）。tick 是 xv6 内核使用的时间概念，即时钟芯片两次中断之间的时间。请将实现写在 `user/sleep.c` 中。

一些提示：

- 编码前先阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)第 1 章。
- 查看 `user/` 中的其他程序，例如 `user/echo.c`、`user/grep.c` 和 `user/rm.c`，了解如何获取传给程序的命令行参数。
- 如果用户忘记传入参数，`sleep` 应打印错误信息。
- 命令行参数以字符串形式传入，可以使用 `atoi` 将其转换为整数，参见 `user/ulib.c`。
- 使用系统调用 `sleep`。
- `kernel/sysproc.c` 包含实现 `sleep` 系统调用的 xv6 内核代码，查找 `sys_sleep`；`user/user.h` 中有用户程序可调用的 `sleep` 的 C 声明；`user/usys.S` 包含调用 `sleep` 时从用户代码跳入内核的汇编代码。
- 确保 `main` 调用 `exit()` 退出程序。
- 将你的 `sleep` 程序加入 Makefile 的 `UPROGS`。之后，`make qemu` 就会编译该程序，并允许你在 xv6 shell 中运行它。
- 要学习 C，请参阅 Kernighan 和 Ritchie 的《C 程序设计语言》第 2 版（K&R）。

在 xv6 shell 中运行程序：

```text
      $ make qemu
      ...
      init: starting sh
      $ sleep 10
      (nothing happens for a little while)
      $
```

如果程序按上面的方式运行时会暂停，你的实现就是正确的。运行 `make grade`，确认是否通过了 `sleep` 测试。

注意，`make grade` 会运行所有测试，包括下列其他练习的测试。要只运行某个练习的评分测试，可以输入：

```text
     $ ./grade-lab-util sleep
```

它会运行名称与“sleep”匹配的评分测试。也可以输入：

```text
     $ make GRADEFLAGS=sleep grade
```

两者作用相同。

## pingpong

编写一个程序，使用 UNIX 系统调用，通过一对管道在两个进程之间往返传递一个字节，每个方向使用一个管道。父进程应向子进程发送一个字节；子进程应打印 `"<pid>: received ping"`，其中 `<pid>` 是它的进程 ID，然后通过管道把该字节写回父进程并退出；父进程应读取子进程发来的字节，打印 `"<pid>: received pong"`，然后退出。请将实现写在 `user/pingpong.c` 中。

一些提示：

- 使用 `pipe` 创建管道。
- 使用 `fork` 创建子进程。
- 使用 `read` 从管道读取，使用 `write` 向管道写入。
- 使用 `getpid` 获取调用进程的进程 ID。
- 将程序加入 Makefile 的 `UPROGS`。
- xv6 用户程序可使用的库函数有限。函数列表见 `user/user.h`；除系统调用之外的实现位于 `user/ulib.c`、`user/printf.c` 和 `user/umalloc.c`。

从 xv6 shell 运行程序，应得到以下输出：

```text
    $ make qemu
    ...
    init: starting sh
    $ pingpong
    4: received ping
    3: received pong
    $
```

如果程序在两个进程之间交换了一个字节，并产生上述输出，你的实现就是正确的。

## primes /

使用管道编写并发版本的素数筛。这一思路来自 Unix 管道的发明者 Doug McIlroy。[这个页面](http://swtch.com/~rsc/thread/)中间的图片及其附近的文字说明了实现方法。请将实现写在 `user/primes.c` 中。

你的目标是使用 `pipe` 和 `fork` 建立流水线。第一个进程把 2 到 35 的整数送入流水线。对于每个素数，应创建一个进程，通过管道从左侧相邻进程读取数据，再通过另一个管道向右侧相邻进程写入数据。由于 xv6 的文件描述符和进程数量有限，第一个进程只需处理到 35。

一些提示：

- 务必关闭进程不需要的文件描述符，否则第一个进程还没处理到 35，程序就会耗尽 xv6 的资源。
- 第一个进程处理完 35 后，应等待整条流水线结束，包括所有子进程、孙进程等。因此，主 `primes` 进程只能在全部输出打印完成、其他所有 `primes` 进程退出之后退出。
- 提示：管道的写端关闭后，`read` 会返回零。
- 最简单的方法是直接向管道写入 32 位（4 字节）`int`，而不是使用格式化的 ASCII 输入输出。
- 只在需要时才创建流水线中的进程。
- 将程序加入 Makefile 的 `UPROGS`。

如果你的实现使用管道完成筛选，并产生以下输出，就是正确的：

```text
    $ make qemu
    ...
    init: starting sh
    $ primes
    prime 2
    prime 3
    prime 5
    prime 7
    prime 11
    prime 13
    prime 17
    prime 19
    prime 23
    prime 29
    prime 31
    $
```

## find

编写 UNIX `find` 程序的简单版本：找出目录树中所有具有指定名称的文件。请将实现写在 `user/find.c` 中。

一些提示：

- 查看 `user/ls.c`，了解如何读取目录。
- 使用递归，使 `find` 能进入子目录。
- 不要递归进入 `"."` 和 `".."`。
- 文件系统的修改会在不同的 QEMU 运行之间保留。要获得干净的文件系统，先运行 `make clean`，再运行 `make qemu`。
- 你需要使用 C 字符串。可以阅读 K&R（C 语言教材），例如第 5.5 节。
- 注意，`==` 不会像 Python 中那样比较字符串，应使用 `strcmp()`。
- 将程序加入 Makefile 的 `UPROGS`。

当文件系统中有 `b` 和 `a/b` 两个文件时，如果程序产生以下输出，你的实现就是正确的：

```text
    $ make qemu
    ...
    init: starting sh
    $ echo > b
    $ mkdir a
    $ echo > a/b
    $ find . b
    ./b
    ./a/b
    $
```

## xargs

编写 UNIX `xargs` 程序的简单版本：从标准输入逐行读取，对每一行执行一条命令，并将该行内容作为参数传给命令。请将实现写在 `user/xargs.c` 中。

以下示例说明了 `xargs` 的行为：

```text
    $ echo hello too | xargs echo bye
    bye hello too
    $
```

这里的命令是 `"echo bye"`，额外参数是 `"hello too"`，组合后的命令为 `"echo bye hello too"`，输出为 `"bye hello too"`。

注意，UNIX 上的 `xargs` 做了优化，一次会向命令传递多个参数。我们不要求你实现这一优化。要使 UNIX 的 `xargs` 按本实验要求的方式运行，请把 `-n` 选项设为 1。例如：

```text
    $ echo "1\n2" | xargs -n 1 echo line
    line 1
    line 2
    $
```

一些提示：

- 使用 `fork` 和 `exec` 对每一行输入执行命令。在父进程中使用 `wait`，等待子进程完成命令。
- 为逐行读取输入，每次读取一个字符，直到遇到换行符（`'\n'`）。
- `kernel/param.h` 声明了 `MAXARG`，需要声明 `argv` 数组时可能用得上。
- 将程序加入 Makefile 的 `UPROGS`。
- 文件系统的修改会在不同的 QEMU 运行之间保留。要获得干净的文件系统，先运行 `make clean`，再运行 `make qemu`。

`xargs`、`find` 和 `grep` 可以很好地组合：

```text
  $ find . b | xargs grep hello
```

这会对 `"."` 目录之下每个名为 `b` 的文件运行 `"grep hello"`。

运行 shell 脚本 `xargstest.sh` 来测试你的 `xargs` 实现。如果产生以下输出，你的实现就是正确的：

```text
  $ make qemu
  ...
  init: starting sh
  $ sh < xargstest.sh
  $ $ $ $ $ $ hello
  hello
  hello
  $ $
```

你可能需要返回去修复 `find` 程序中的错误。输出中有许多 `$`，因为 xv6 shell 不知道自己正在从文件而非控制台读取命令，因此会为文件中的每条命令都打印一个 `$`。

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

- 编写 `uptime` 程序，使用 `uptime` 系统调用，以 tick 为单位打印系统运行时间。
- 让 `find` 在名称匹配时支持正则表达式。`grep.c` 包含一些基本的正则表达式支持。
- xv6 shell（`user/sh.c`）也是普通用户程序，你可以改进它。它是一个最小化 shell，缺少真实 shell 的许多特性。例如：从文件执行 shell 命令时不打印 `$`；支持 `wait`；支持用 `;` 分隔的命令列表；通过实现 `(` 和 `)` 支持子 shell；支持 Tab 补全；保留已执行命令的历史记录；或者实现你希望 shell 具备的其他功能。（如果你的目标很大，可能需要修改内核来支持所需功能；xv6 提供的支持并不多。）
