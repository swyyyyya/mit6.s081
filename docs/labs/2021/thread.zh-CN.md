> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab thread: Multithreading](https://pdos.csail.mit.edu/6.828/2021/labs/thread.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](thread.md) · [官方原始 HTML](original/thread.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：多线程

本实验将帮助你熟悉多线程。你将实现用户级线程库中的线程切换，使用多个线程加速程序，并实现屏障（barrier）。

编码前，请确保已阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)第 7 章“调度”，并研究了对应代码。

开始实验前，切换到 `thread` 分支：

```text
  $ git fetch
  $ git checkout thread
  $ make clean
```

## Uthread：在线程间切换

本练习中，你将设计并实现用户级线程系统的上下文切换机制。为帮助开始，xv6 提供了 `user/uthread.c` 和 `user/uthread_switch.S`，Makefile 中也有构建 `uthread` 程序的规则。`uthread.c` 包含用户级线程库的大部分代码，以及三个简单测试线程的代码。线程库缺少创建线程和在线程之间切换的部分实现。

你的任务是设计一个方案，用于创建线程，以及在线程切换时保存／恢复寄存器，然后实现该方案。完成后，`make grade` 应报告通过 `uthread` 测试。

完成后，在 xv6 中运行 `uthread`，应看到以下输出，三个线程的开始顺序可能不同：

```text
$ make qemu
...
$ uthread
thread_a started
thread_b started
thread_c started
thread_c 0
thread_a 0
thread_b 0
thread_c 1
thread_a 1
thread_b 1
...
thread_c 99
thread_a 99
thread_b 99
thread_c: exit after 100
thread_a: exit after 100
thread_b: exit after 100
thread_schedule: no runnable threads
$
```

这些输出来自三个测试线程；每个线程都有一个循环，打印一行后就将 CPU 让给其他线程。

不过，目前没有上下文切换代码，因此不会看到任何输出。

你需要在 `user/uthread.c` 的 `thread_create()`、`thread_schedule()`，以及 `user/uthread_switch.S` 的 `thread_switch` 中添加代码。目标之一是确保 `thread_schedule()` 首次运行某个线程时，该线程在自己的栈上执行传给 `thread_create()` 的函数。另一目标是确保 `thread_switch` 保存被切出线程的寄存器，恢复被切入线程的寄存器，并返回后者上次停止执行的位置。你需要决定在哪里保存／恢复寄存器；修改 `struct thread` 来存放寄存器是一种好方案。还需在 `thread_schedule` 中调用 `thread_switch`；可以传入所需的任意参数，但目的是从线程 `t` 切换到 `next_thread`。

一些提示：

- `thread_switch` 只需保存／恢复被调用者保存寄存器（callee-save registers）。为什么？
- `user/uthread.asm` 中有 `uthread` 的汇编代码，可能有助于调试。
- 使用 `riscv64-linux-gnu-gdb` 单步执行 `thread_switch` 可能有助于测试。可以这样开始：

```text
  (gdb) file user/_uthread
  Reading symbols from user/_uthread...
  (gdb) b uthread.c:60
  ```

  这会在 `uthread.c` 第 60 行设置断点。断点可能在你运行 `uthread` 之前就触发，也可能不会。为什么会这样？

  xv6 shell 启动后，输入 `uthread`，GDB 会在第 60 行停住。此时可以使用以下命令检查 `uthread` 的状态：

```text
    (gdb) p/x *next_thread
  ```

  使用 `x` 可以检查某个内存位置的内容：

```text
    (gdb) x/x next_thread->stack
  ```

  可以这样运行到 `thread_switch` 开头：

```text
     (gdb) b thread_switch
     (gdb) c
  ```

  可以使用以下命令单步执行汇编指令：

```text
     (gdb) si
  ```

  GDB 的在线文档在[这里](https://sourceware.org/gdb/current/onlinedocs/gdb/)。

## 使用线程

本练习通过哈希表探索使用线程和锁的并行编程。请在具有多个核心的真实 Linux 或 macOS 计算机上完成，不是在 xv6 或 QEMU 内完成。多数较新的笔记本电脑都有多核处理器。

本练习使用 UNIX `pthread` 线程库。可以运行 `man pthreads` 查看手册，也可以查阅网络资料，例如[互斥锁加锁](https://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_mutex_lock.html)、[互斥锁初始化](https://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_mutex_init.html)和[创建线程](https://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_create.html)。

`notxv6/ph.c` 包含一个简单哈希表，单线程使用时正确，多线程使用时不正确。在 xv6 主目录中，例如 `~/xv6-labs-2021`，输入：

```text
$ make ph
$ ./ph 1
```

注意，构建 `ph` 时，Makefile 使用你所运行操作系统的 GCC，而不是 6.S081 工具链。`ph` 的参数指定在哈希表上执行 put 和 get 操作的线程数量。运行一段时间后，`ph 1` 会产生类似以下输出：

```text
100000 puts, 3.991 seconds, 25056 puts/second
0: 0 keys missing
100000 gets, 3.981 seconds, 25118 gets/second
```

实际数值可能与示例相差两倍甚至更多，取决于电脑速度、是否多核，以及是否忙于处理其他任务。

`ph` 运行两项基准测试。首先调用 `put()` 向哈希表添加大量键，打印每秒 put 次数；随后使用 `get()` 从哈希表读取键，打印本应由 put 加入哈希表、实际却缺失的键数量（本例为零），以及每秒 get 次数。

传入大于 1 的参数，可让多个线程同时使用哈希表。尝试 `ph 2`：

```text
$ ./ph 2
100000 puts, 1.885 seconds, 53044 puts/second
1: 16579 keys missing
0: 16579 keys missing
200000 gets, 4.322 seconds, 46274 gets/second
```

该输出第一行表明，两个线程并发向哈希表添加条目时，总速率为每秒 53,044 次插入，约为 `ph 1` 单线程速率的两倍。这是非常好的约 2 倍“并行加速比”，已经达到理想情况，即核心数翻倍，单位时间完成的工作量也翻倍。

不过，两行 `16579 keys missing` 表示大量本应存在的键不在哈希表中。put 本应加入这些键，但某个地方发生了错误。阅读 `notxv6/ph.c`，尤其是 `put()` 和 `insert()`。

为什么两个线程时会丢失键，而一个线程时不会？找出一种两个线程交错执行、导致键丢失的事件顺序。将该顺序及简短解释写入 `answers-thread.txt` 提交。

为避免这种事件顺序，在 `notxv6/ph.c` 的 `put` 和 `get` 中插入加锁、解锁语句，使两个线程时丢失键的数量始终为 0。相关 pthread 调用如下：

```text
pthread_mutex_t lock;            // declare a lock
pthread_mutex_init(&lock, NULL); // initialize the lock
pthread_mutex_lock(&lock);       // acquire lock
pthread_mutex_unlock(&lock);     // release lock
```

当 `make grade` 报告通过 `ph_safe` 测试时，本阶段就完成了；该测试要求两个线程时不丢失键。此时尚未通过 `ph_fast` 测试也没关系。

不要忘记调用 `pthread_mutex_init()`。先用一个线程测试，再用两个线程测试。结果是否正确，即是否消除了键丢失？双线程版本相对于单线程版本是否获得并行加速，即单位时间内完成更多总工作量？

某些情况下，并发的 `put()` 读写哈希表中互不重叠的内存，因此无需用锁相互保护。能否修改 `ph.c`，利用这种情况，让部分 `put()` 获得并行加速？提示：每个哈希桶使用一把锁怎么样？

修改代码，在保持正确性的同时，让部分 put 操作并行执行。当 `make grade` 报告 `ph_safe` 和 `ph_fast` 都通过时，就完成了。`ph_fast` 要求双线程的每秒 put 次数至少为单线程的 1.25 倍。

## 屏障

本练习将实现[屏障](http://en.wikipedia.org/wiki/Barrier_(computer_science))：应用中的一个同步点，所有参与线程都必须等待，直到其他参与线程也到达该点。你将使用 pthread 条件变量，这是一种与 xv6 的 sleep 和 wakeup 类似的执行顺序协调技术。

请在真实计算机上完成本练习，不是在 xv6 或 QEMU 中完成。

`notxv6/barrier.c` 包含一个有问题的屏障实现。

```text
$ make barrier
$ ./barrier 2
barrier: notxv6/barrier.c:42: thread: Assertion `i == t' failed.
```

参数 2 指定在屏障处同步的线程数量，对应 `barrier.c` 中的 `nthread`。每个线程执行循环，每次迭代调用 `barrier()`，再睡眠随机微秒数。断言会触发，因为一个线程在另一个线程到达屏障前就离开了屏障。期望行为是：每个线程都在 `barrier()` 内阻塞，直到全部 `nthreads` 个线程都调用了 `barrier()`。

你的目标是实现上述屏障行为。除了 `ph` 练习中使用的锁原语，还需要以下新的 pthread 原语；详细说明见[条件等待](https://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_cond_wait.html)和[广播唤醒](https://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_cond_broadcast.html)。

```text
pthread_cond_wait(&cond, &mutex);  // go to sleep on cond, releasing lock mutex, acquiring upon wake up
pthread_cond_broadcast(&cond);     // wake up every thread sleeping on cond
```

确保实现通过 `make grade` 的 `barrier` 测试。

`pthread_cond_wait` 在调用时释放 `mutex`，并在返回前重新获取 `mutex`。

我们已提供 `barrier_init()`。你的任务是实现 `barrier()`，使 panic 不再发生。我们已定义 `struct barrier`，其中的字段可供使用。

以下两点使任务更复杂：

- 需要处理连续多次屏障调用，每次称为一轮。`bstate.round` 记录当前轮次。每次所有线程到达屏障后，都应增加 `bstate.round`。
- 需要处理某个线程已经快速进入下一次循环，而其他线程尚未退出屏障的情况。尤其是各轮会复用 `bstate.nthread`。确保一个离开屏障并快速进入下一轮的线程，不会在上一轮还在使用此变量时增加 `bstate.nthread`。

分别用一个、两个和两个以上线程测试代码。

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

## uthread 的可选挑战

用户级线程库与操作系统在多个方面配合得不好。例如，如果一个用户线程在系统调用中阻塞，另一个用户线程也不会运行，因为用户级线程调度器不知道其中一个线程已被 xv6 调度器停止调度。另一个例子是，两个用户线程不会在不同核心上并行运行，因为 xv6 调度器不知道有多个线程可以并行。注意，如果两个用户线程真的并行运行，当前实现还会因为多个竞态而无法正常工作。例如，不同处理器上的两个线程可能并发调用 `thread_schedule`，选中同一个可运行线程，并在两个处理器上同时运行它。

有多种方法解决这些问题。一种是使用[调度器激活](http://en.wikipedia.org/wiki/Scheduler_activations)，另一种是像 Linux 内核那样，为每个用户级线程使用一个内核线程。在 xv6 中实现其中一种方法。要正确实现并不容易；例如，为多线程用户进程更新页表时，需要实现 TLB shootdown，即使其他处理器上对应的 TLB 缓存失效。

为线程库添加锁、条件变量、屏障等功能。
