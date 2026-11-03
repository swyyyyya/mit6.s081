> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab traps: Traps](https://pdos.csail.mit.edu/6.828/2021/labs/traps.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](traps.md) · [官方原始 HTML](original/traps.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：陷阱

本实验探索如何使用陷阱实现系统调用。你将先做一些与栈有关的热身练习，然后实现一个用户级陷阱处理示例。

编码前，请阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)第 4 章，以及相关源码：

- `kernel/trampoline.S`：涉及从用户空间进入内核空间及返回的汇编代码。
- `kernel/trap.c`：处理所有中断的代码。

开始实验前，切换到 trap 分支：

```text
  $ git fetch
  $ git checkout traps
  $ make clean
```

## RISC-V 汇编

理解一些 RISC-V 汇编非常重要，你已经在 6.004 中接触过它。xv6 仓库中有文件 `user/call.c`。`make fs.img` 会编译它，并在 `user/call.asm` 中生成可读的汇编版本。

阅读 `call.asm` 中 `g`、`f` 和 `main` 函数的代码。RISC-V 指令手册可在[参考资料页](https://pdos.csail.mit.edu/6.828/2021/reference.html)找到。请回答以下问题，并将答案保存在 `answers-traps.txt` 中：

哪些寄存器存放函数参数？例如，`main` 调用 `printf` 时，哪个寄存器保存 13？

`main` 的汇编代码在哪里调用函数 `f`？在哪里调用 `g`？提示：编译器可能会内联函数。

函数 `printf` 位于哪个地址？

在 `main` 中执行调用 `printf` 的 `jalr` 指令后，寄存器 `ra` 的值是多少？

运行以下代码。

```text
	unsigned int i = 0x00646c72;
	printf("H%x Wo%s", 57616, &i);
```

输出是什么？[这张 ASCII 表](http://web.cs.mun.ca/~michael/c/ascii-table.html)给出了字节与字符之间的映射。

该输出依赖于 RISC-V 使用小端字节序这一事实。如果 RISC-V 改为大端字节序，为得到相同输出，应将 `i` 设为什么值？是否需要将 `57616` 改成另一个值？

这里有[大小端字节序的说明](http://www.webopedia.com/TERM/b/big_endian.html)，以及[一种更诙谐的介绍](http://www.networksorcery.com/enp/ien/ien137.txt)。

下面的代码会在 `'y='` 后打印什么？注意，答案不是某个具体数值。为什么会这样？

```text
	printf("x=%d y=%d", 3);
```

## 栈回溯

调试时，栈回溯通常很有帮助：它列出错误发生位置之上的栈中的函数调用。

在 `kernel/printf.c` 中实现 `backtrace()`。在 `sys_sleep` 中调用它，然后运行会调用 `sys_sleep` 的 `bttest`。输出应如下：

```text
backtrace:
0x0000000080002cda
0x0000000080002bb6
0x0000000080002898
```

运行 `bttest` 后退出 QEMU。在终端中运行 `addr2line -e kernel/kernel`（或 `riscv64-unknown-elf-addr2line -e kernel/kernel`），并像下面这样复制粘贴上述地址；实际地址可能略有不同：

```text
    $ addr2line -e kernel/kernel
    0x0000000080002de2
    0x0000000080002f4a
    0x0000000080002bfc
    Ctrl-D
```

应看到类似以下结果：

```text
    kernel/sysproc.c:74
    kernel/syscall.c:224
    kernel/trap.c:85
```

编译器在每个栈帧中保存一个帧指针，其中存放调用者帧指针的地址。你的 `backtrace` 应使用这些帧指针向上遍历调用栈，并打印每个栈帧中保存的返回地址。

一些提示：

- 在 `kernel/defs.h` 中添加 `backtrace` 的函数原型，以便在 `sys_sleep` 中调用。
- GCC 将当前执行函数的帧指针保存在寄存器 `s0` 中。在 `kernel/riscv.h` 中添加以下函数：

```text
  static inline uint64
  r_fp()
  {
    uint64 x;
    asm volatile("mv %0, s0" : "=r" (x) );
    return x;
  }
  ```

  在 `backtrace` 中调用该函数，读取当前帧指针。该函数使用[内联汇编](https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html)读取 `s0`。
- 这份[讲义](https://pdos.csail.mit.edu/6.828/2021/lec/l-riscv-slides.pdf)包含栈帧布局图。注意，返回地址位于相对栈帧帧指针的固定偏移 -8 处，保存的帧指针位于固定偏移 -16 处。
- xv6 为内核中的每个栈分配一个页，地址按页对齐。可以使用 `PGROUNDDOWN(fp)` 和 `PGROUNDUP(fp)`（见 `kernel/riscv.h`）计算栈所在页面的底部和顶部地址。这些值可帮助 `backtrace` 判断何时结束循环。

栈回溯正常工作后，在 `kernel/printf.c` 的 `panic` 中调用它，使内核发生 panic 时能看到回溯。

## 闹钟

本练习将为 xv6 增加一项功能：根据进程消耗的 CPU 时间周期性地提醒该进程。这可能适用于希望限制 CPU 时间用量的计算密集型进程，或者既需要计算、又需要周期性执行某些动作的进程。更一般地说，你将实现一种初步的用户级中断／异常处理机制；类似机制还可用于让应用程序处理缺页异常等。如果通过 `alarmtest` 和 `usertests`，你的实现就是正确的。

添加系统调用 `sigalarm(interval, handler)`。应用调用 `sigalarm(n, fn)` 后，每消耗 `n` 个 tick 的 CPU 时间，内核就应使应用函数 `fn` 被调用。`fn` 返回后，应用应从之前中断的位置恢复执行。tick 是 xv6 中一个相对任意的时间单位，取决于硬件定时器产生中断的频率。应用调用 `sigalarm(0, 0)` 后，内核应停止产生周期性的闹钟调用。

仓库中有文件 `user/alarmtest.c`，请将它加入 Makefile。在添加 `sigalarm` 和 `sigreturn` 系统调用之前，它无法正确编译，详见下文。

`alarmtest` 在 `test0` 中调用 `sigalarm(2, periodic)`，请求内核每 2 个 tick 强制调用一次 `periodic()`，随后忙循环一段时间。`user/alarmtest.asm` 中的汇编代码可用于调试。当 `alarmtest` 产生类似以下输出，且 `usertests` 也正常运行时，你的实现就是正确的：

```text
$ alarmtest
test0 start
........alarm!
test0 passed
test1 start
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
test1 passed
test2 start
................alarm!
test2 passed
$ usertests
...
ALL TESTS PASSED
$
```

完成后，解答只需要几行代码，但要写对可能不容易。我们会使用原始仓库版本的 `alarmtest.c` 测试你的代码。你可以修改它以便调试，但必须确保原始 `alarmtest` 报告所有测试通过。

### test0：调用处理函数

先修改内核，使其跳到用户空间中的闹钟处理函数，让 `test0` 打印 `"alarm!"`。暂时不必考虑打印之后会怎样；现阶段打印后崩溃也没关系。一些提示：

- 修改 Makefile，使 `alarmtest.c` 被编译为 xv6 用户程序。
- 应在 `user/user.h` 中添加以下声明：

```text
      int sigalarm(int ticks, void (*handler)());
      int sigreturn(void);
  ```

- 更新 `user/usys.pl`（用于生成 `user/usys.S`）、`kernel/syscall.h` 和 `kernel/syscall.c`，使 `alarmtest` 能调用 `sigalarm` 和 `sigreturn`。
- 目前 `sys_sigreturn` 直接返回零即可。
- `sys_sigalarm()` 应在 `proc` 结构体（`kernel/proc.h`）的新字段中保存闹钟间隔和处理函数指针。
- 还需要记录距离上次调用进程的闹钟处理函数已经过去多少 tick，或者距离下次调用还剩多少 tick。因此还需在 `struct proc` 中添加字段。可以在 `proc.c` 的 `allocproc()` 中初始化这些字段。
- 每个 tick，硬件时钟都会触发中断，由 `kernel/trap.c` 的 `usertrap()` 处理。
- 只有在发生定时器中断时，才应修改进程的闹钟 tick 计数。需要类似以下代码：

```text
      if(which_dev == 2) ...
  ```

- 只有进程启用了定时器，才调用闹钟函数。注意，用户闹钟函数的地址可能为 0，例如 `user/alarmtest.asm` 中 `periodic` 就位于地址 0。
- 修改 `usertrap()`，使进程的闹钟间隔到期后，用户进程执行处理函数。RISC-V 从陷阱返回用户空间时，用户代码从哪个指令地址恢复执行，是由什么决定的？
- 让 QEMU 只使用一个 CPU，更便于使用 GDB 查看陷阱，可以运行：

```text
      make CPUS=1 qemu-gdb
  ```

- 如果 `alarmtest` 打印出 `"alarm!"`，你就成功了。

### test1/test2()：恢复被中断的代码

`alarmtest` 很可能在 `test0` 或 `test1` 打印 `"alarm!"` 后崩溃，或最终打印 `"test1 failed"`，或未打印 `"test1 passed"` 就退出。要修复这些问题，必须确保闹钟处理函数完成后，控制流返回到用户程序最初被定时器中断打断的那条指令。还必须将寄存器恢复为中断发生时的值，使用户程序在闹钟处理结束后不受影响地继续执行。最后，每次闹钟触发后，应重新设置计数器，使处理函数能够被周期性调用。

作为起点，我们已作出一项设计决定：用户闹钟处理函数完成后必须调用 `sigreturn` 系统调用。可参考 `alarmtest.c` 中的 `periodic`。这意味着你可以在 `usertrap` 和 `sys_sigreturn` 中添加相互配合的代码，使用户进程处理完闹钟后正确恢复。

一些提示：

- 实现需要保存和恢复寄存器。为正确恢复被中断的代码，需要保存和恢复哪些寄存器？提示：有很多。
- 定时器触发时，让 `usertrap` 在 `struct proc` 中保存足够的状态，以便 `sigreturn` 正确返回被中断的用户代码。
- 防止处理函数重入：如果处理函数尚未返回，内核不应再次调用它。`test2` 会检查这一点。

通过 `test0`、`test1` 和 `test2` 后，运行 `usertests`，确保没有破坏内核的其他部分。

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

- 在 `backtrace()` 中打印函数名称和行号，而不是数值地址。
