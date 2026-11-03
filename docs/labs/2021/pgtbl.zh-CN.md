> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab pgtbl: Page tables](https://pdos.csail.mit.edu/6.828/2021/labs/pgtbl.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](pgtbl.md) · [官方原始 HTML](original/pgtbl.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：页表

本实验中，你将探索并修改页表，以加速某些系统调用，并检测哪些页面被访问过。

编码前，请阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)第 3 章及相关文件：

- `kern/memlayout.h`：描述内存布局。
- `kern/vm.c`：包含大部分虚拟内存（VM）代码。
- `kernel/kalloc.c`：包含分配和释放物理内存的代码。

查阅 [RISC-V 特权架构手册](https://github.com/riscv/riscv-isa-manual/releases/download/Ratified-IMFDQC-and-Priv-v1.11/riscv-privileged-20190608.pdf)也可能有所帮助。

开始实验前，切换到 `pgtbl` 分支：

```text
  $ git fetch
  $ git checkout pgtbl
  $ make clean
```

## 加速系统调用

某些操作系统（例如 Linux）通过在用户空间与内核之间共享一个只读区域中的数据，来加速部分系统调用。这样，执行这些系统调用时就不需要进入内核。为帮助你学习如何向页表中插入映射，你的第一个任务是为 xv6 的 `getpid()` 系统调用实现这一优化。

创建每个进程时，在 `USYSCALL`（定义在 `memlayout.h` 中的虚拟地址）映射一个只读页。在该页的起始处存放 `struct usyscall`（同样定义在 `memlayout.h` 中），初始化它以保存当前进程的 PID。本实验已在用户空间提供 `ugetpid()`，它会自动使用 `USYSCALL` 映射。运行 `pgtbltest` 时，如果 `ugetpid` 测试通过，本部分即可获得满分。

一些提示：

- 可以在 `kernel/proc.c` 的 `proc_pagetable()` 中执行映射。
- 选择仅允许用户空间读取该页面的权限位。
- `mappages()` 可能是一个有用的辅助函数。
- 不要忘记在 `allocproc()` 中分配并初始化该页面。
- 确保在 `freeproc()` 中释放该页面。

还有哪些 xv6 系统调用可以通过这一共享页面加速？请解释实现方式。

## 打印页表

为帮助你直观理解 RISC-V 页表，并方便今后的调试，第二个任务是编写一个打印页表内容的函数。

定义函数 `vmprint()`。它应接收一个 `pagetable_t` 参数，并按下述格式打印该页表。在 `exec.c` 的 `return argc` 之前插入 `if(p->pid==1) vmprint(p->pagetable)`，以打印第一个进程的页表。如果通过 `make grade` 的 `pte printout` 测试，本部分即可获得满分。

此时启动 xv6，应打印类似以下内容，描述第一个进程刚刚完成对 `init` 的 `exec()` 时的页表：

```text
page table 0x0000000087f6e000
 ..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
 .. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
 .. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000
 .. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000
 .. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000
 ..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
 .. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
 .. .. ..509: pte 0x0000000021fdd813 pa 0x0000000087f76000
 .. .. ..510: pte 0x0000000021fddc07 pa 0x0000000087f77000
 .. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
```

第一行显示传给 `vmprint` 的参数。之后，每个 PTE 占一行，包括指向页表树更深层页表页的 PTE。每行开头若干个 `" .."` 表示该 PTE 在树中的深度。每行显示 PTE 在其页表页中的索引、PTE 的位值，以及从 PTE 中提取出的物理地址。不要打印无效 PTE。上例中，顶层页表页的第 0 和第 255 项存在映射。第 0 项对应的下一层仅索引 0 存在映射，而该索引 0 对应的最底层页表中，第 0、1、2 项存在映射。

你的代码打印出的物理地址可能与上述不同，但条目数量和虚拟地址应相同。

一些提示：

- 可以把 `vmprint()` 放在 `kernel/vm.c` 中。
- 使用 `kernel/riscv.h` 末尾的宏。
- `freewalk` 函数可提供思路。
- 在 `kernel/defs.h` 中声明 `vmprint` 的原型，以便从 `exec.c` 调用。
- 在 `printf` 中使用 `%p`，像示例那样打印完整的 64 位十六进制 PTE 和地址。

结合教材图 3-4 解释 `vmprint` 的输出。第 0 页包含什么？第 2 页中有什么？进程在用户模式下运行时，能否读写第 1 页映射的内存？倒数第 3 页包含什么？

## 检测哪些页面被访问过

某些垃圾回收器（一种自动内存管理方式）可以利用页面是否被访问过（读取或写入）的信息。本部分中，你将为 xv6 添加一项功能：通过检查 RISC-V 页表中的访问位，检测这些信息并报告给用户空间。RISC-V 硬件页表遍历器每次处理 TLB 未命中时，都会设置 PTE 中的这些位。

你的任务是实现系统调用 `pgaccess()`，报告哪些页面被访问过。它接收三个参数：第一个待检查用户页的起始虚拟地址；待检查的页面数量；用于存放结果位掩码的用户缓冲区地址。位掩码是一种每页使用一位的数据结构，第一个页面对应最低有效位。运行 `pgtbltest` 时，如果 `pgaccess` 测试通过，本部分即可获得满分。

一些提示：

- 从实现 `kernel/sysproc.c` 中的 `sys_pgaccess()` 开始。
- 需要使用 `argaddr()` 和 `argint()` 解析参数。
- 对于输出位掩码，先在内核临时缓冲区中设置好相应的位，再通过 `copyout()` 复制到用户空间，会更简单。
- 可以为可扫描的页面数量设置上限。
- `kernel/vm.c` 中的 `walk()` 对查找相应 PTE 很有帮助。
- 需要在 `kernel/riscv.h` 中定义访问位 `PTE_A`。查阅 RISC-V 手册确定其值。
- 检查 `PTE_A` 是否设置后，务必清除它。否则无法判断从上次调用 `pgaccess()` 以来该页是否再次被访问，因为该位将一直保持置位。
- `vmprint()` 可用于调试页表。

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

- 使用大页（super-page）减少页表中的 PTE 数量。
- 取消用户进程第一页的映射，使解引用空指针触发异常。为此需要让用户代码段从例如 4096 而非 0 开始。
- 添加系统调用，使用 `PTE_D` 报告脏页（修改过的页面）。

---

**译者说明（不属于原文）**：原文准备阅读列表中的 `kern/memlayout.h`、`kern/vm.c` 路径保留原样；本项目对应文件位于 `kernel/` 目录。

