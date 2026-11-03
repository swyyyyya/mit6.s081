> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab syscall: System calls](https://pdos.csail.mit.edu/6.828/2021/labs/syscall.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](syscall.md) · [官方原始 HTML](original/syscall.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：系统调用

上一个实验中，你使用系统调用编写了一些实用工具。本实验将向 xv6 添加新的系统调用，帮助你理解其工作方式，并接触 xv6 内核的一些内部实现。后续实验还会添加更多系统调用。

编码前，请阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev1.pdf)第 2 章、第 4 章的 4.3 和 4.4 节，以及相关源码：

- 系统调用的用户空间代码位于 `user/user.h` 和 `user/usys.pl`。
- 内核空间代码位于 `kernel/syscall.h` 和 `kernel/syscall.c`。
- 与进程有关的代码位于 `kernel/proc.h` 和 `kernel/proc.c`。

开始实验前，切换到 `syscall` 分支：

```tex
  $ git fetch
  $ git checkout syscall
  $ make clean
```

运行 `make grade` 时，会发现评分脚本无法执行 `trace` 和 `sysinfotest`。你的任务是添加必要的系统调用及桩代码，让它们能够工作。

## 系统调用跟踪

本练习中，你将添加系统调用跟踪功能，它可能有助于调试后续实验。你将创建新的 `trace` 系统调用来控制跟踪。它应接收一个整数“掩码”参数，掩码的各个位指定要跟踪哪些系统调用。例如，要跟踪 fork 系统调用，程序可以调用 `trace(1 << SYS_fork)`，其中 `SYS_fork` 是 `kernel/syscall.h` 中定义的系统调用编号。你需要修改 xv6 内核：某个系统调用即将返回时，如果掩码中与其编号对应的位已设置，就打印一行信息。该行应包含进程 ID、系统调用名称和返回值，不需要打印系统调用参数。`trace` 应启用调用进程及其随后通过 fork 创建的所有子进程的跟踪，但不应影响其他进程。

我们提供了用户程序 `trace`，它会启用跟踪并运行另一个程序，参见 `user/trace.c`。完成后，应看到类似以下输出：

```text
$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0
$
$ trace 2147483647 grep hello README
4: syscall trace -> 0
4: syscall exec -> 3
4: syscall open -> 3
4: syscall read -> 1023
4: syscall read -> 966
4: syscall read -> 70
4: syscall read -> 0
4: syscall close -> 0
$
$ grep hello README
$
$ trace 2 usertests forkforkfork
usertests starting
test forkforkfork: 407: syscall fork -> 408
408: syscall fork -> 409
409: syscall fork -> 410
410: syscall fork -> 411
409: syscall fork -> 412
410: syscall fork -> 413
409: syscall fork -> 414
411: syscall fork -> 415
...
$
```

第一个例子中，`trace` 调用 `grep`，仅跟踪 `read` 系统调用，32 即 `1<<SYS_read`。第二个例子中，`trace` 运行 `grep`，跟踪所有系统调用；2147483647 的低 31 位全为 1。第三个例子中，程序没有被跟踪，因此没有跟踪输出。第四个例子中，`usertests` 中 `forkforkfork` 测试的所有后代进程的 fork 系统调用都被跟踪。如果程序表现与上面一致，你的实现就是正确的，不过进程 ID 可能不同。

一些提示：

- 将 `$U/_trace` 加入 Makefile 的 `UPROGS`。
- 运行 `make qemu`，会发现编译器无法编译 `user/trace.c`，因为系统调用的用户空间桩代码还不存在。需要在 `user/user.h` 添加系统调用原型，在 `user/usys.pl` 添加桩代码生成项，并在 `kernel/syscall.h` 添加系统调用编号。Makefile 会调用 Perl 脚本 `user/usys.pl` 生成真正的系统调用桩代码 `user/usys.S`，通过 RISC-V 的 `ecall` 指令进入内核。修复编译问题后，运行 `trace 32 grep hello README`；它仍会失败，因为内核中尚未实现该系统调用。
- 在 `kernel/sysproc.c` 中添加 `sys_trace()` 函数，通过在 `proc` 结构体（见 `kernel/proc.h`）的新字段中记住参数，实现新系统调用。从用户空间获取系统调用参数的函数位于 `kernel/syscall.c`，使用示例见 `kernel/sysproc.c`。
- 修改 `fork()`（见 `kernel/proc.c`），将跟踪掩码从父进程复制给子进程。
- 修改 `kernel/syscall.c` 中的 `syscall()`，打印跟踪输出。需要添加一个系统调用名称数组，以便按编号索引。

## Sysinfo

本练习中，你将添加系统调用 `sysinfo`，收集运行中的系统信息。它接收一个参数：指向 `struct sysinfo` 的指针，参见 `kernel/sysinfo.h`。内核应填写结构体字段：`freemem` 为可用内存的字节数，`nproc` 为 `state` 不等于 `UNUSED` 的进程数量。我们提供了测试程序 `sysinfotest`；如果它打印 `"sysinfotest: OK"`，你就通过了本练习。

一些提示：

- 将 `$U/_sysinfotest` 加入 Makefile 的 `UPROGS`。
- 运行 `make qemu`，`user/sysinfotest.c` 会编译失败。按照上一练习的相同步骤添加 `sysinfo` 系统调用。要在 `user/user.h` 中声明 `sysinfo()` 原型，需要先对 `struct sysinfo` 作前置声明：

```text
      struct sysinfo;
      int sysinfo(struct sysinfo *);
  ```

  修复编译问题后，运行 `sysinfotest`；它会失败，因为内核尚未实现该系统调用。
- `sysinfo` 需要将 `struct sysinfo` 复制回用户空间。如何使用 `copyout()` 完成这一操作，可参考 `sys_fstat()`（`kernel/sysfile.c`）和 `filestat()`（`kernel/file.c`）。
- 为统计空闲内存量，在 `kernel/kalloc.c` 中添加一个函数。
- 为统计进程数量，在 `kernel/proc.c` 中添加一个函数。

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

- 为被跟踪的系统调用打印其参数。
- 计算平均负载，并通过 `sysinfo` 导出。
