> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Tools Used in 6.S081](https://pdos.csail.mit.edu/6.828/2021/tools.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](tools.md) · [官方原始 HTML](original/tools.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

切换导航

[6.S081：操作系统工程](https://pdos.csail.mit.edu/6.828/2021/index.html)

- [课程表](https://pdos.csail.mit.edu/6.828/2021/schedule.html)
- [课程](#)
  - [概述](https://pdos.csail.mit.edu/6.828/2021/overview.html)
  - [课程事务](https://pdos.csail.mit.edu/6.828/2021/general.html)
  - [提交网站](https://6828.scripts.mit.edu/2021/handin.py/)
  - [6.S081 2020](http://pdos.csail.mit.edu/6.828/2020)
- [实验](#)
  - [工具](tools.zh-CN.md)
  - [指导](guidance.zh-CN.md)
  - [Unix 实用工具](util.zh-CN.md)
  - [系统调用](syscall.zh-CN.md)
  - [页表](pgtbl.zh-CN.md)
  - [陷阱](traps.zh-CN.md)
  - [写时复制](cow.zh-CN.md)
  - [多线程](thread.zh-CN.md)
  - [网络驱动](net.zh-CN.md)
  - [锁](lock.zh-CN.md)
  - [文件系统](fs.zh-CN.md)
  - [mmap](mmap.zh-CN.md)
- [xv6](#)
  - [xv6](https://pdos.csail.mit.edu/6.828/2021/xv6.html)
  - [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)
- [参考资料](https://pdos.csail.mit.edu/6.828/2021/reference.html)
- [Piazza](https://piazza.com/mit/fall2021/6s081)

2021

# 6.S081 使用的工具

本课程需要以下工具的 RISC-V 版本：QEMU 5.1 及以上、GDB 8.3 及以上、GCC 和 Binutils。

如果环境配置遇到问题，请到答疑时间咨询，或在 Piazza 发帖，我们很乐意提供帮助！

### 在 Windows 上安装

首先确保已安装 [Windows Subsystem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install-win10)。然后从 [Microsoft Store 安装 Ubuntu 20.04](https://www.microsoft.com/en-us/p/ubuntu/9nblggh4msv6)。之后应能启动 Ubuntu 并与机器交互。运行以下命令安装本课程所需的全部软件：

```text
$ sudo apt-get update && sudo apt-get upgrade
$ sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

在 Windows 中，可以通过 `\\wsl$\` 目录访问全部 WSL 文件。例如，Ubuntu 20.04 的主目录应为 `\\wsl$\Ubuntu-20.04\home\<username>\`。

### 在 macOS 上安装

先安装开发者工具：

```text
$ xcode-select --install
```

接着安装 macOS 包管理器 [Homebrew](https://brew.sh/)：

```text
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

然后安装 [RISC-V 编译工具链](https://github.com/riscv/homebrew-riscv)：

```text
$ brew tap riscv/riscv
$ brew install riscv-tools
```

brew 配方可能不会把工具链接到 `/usr/local`。需要更新 shell 的 rc 文件，例如 [~/.bashrc](https://www.gnu.org/software/bash/manual/html_node/Bash-Startup-Files.html)，将相应目录加入 [$PATH](http://www.linfo.org/path_env_var.html)。

```text
PATH=$PATH:/usr/local/opt/riscv-gnu-toolchain/bin
```

最后安装 QEMU：

```text
brew install qemu
```

### Debian 或 Ubuntu

```text
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

### Arch Linux

```text
sudo pacman -S riscv64-linux-gnu-binutils riscv64-linux-gnu-gcc riscv64-linux-gnu-gdb qemu-arch-extra
```

### 运行 Linux 虚拟机

如果其他方法不可用，也可以尝试运行上述操作系统之一的虚拟机。通过平台虚拟化，Linux 可以与你通常使用的计算环境并存。安装 Linux 虚拟机分两步，首先下载虚拟化平台：

- [**VirtualBox**](https://www.virtualbox.org/)：Mac、Linux、Windows 均免费。[下载页](https://www.virtualbox.org/wiki/Downloads)。
- [VMware Player](http://www.vmware.com/products/player/)：Linux 和 Windows 上免费，需要注册。
- [VMware Fusion](http://www.vmware.com/products/fusion/)：可从 IS&T 免费下载。

VirtualBox 稍慢、灵活性稍差，但免费！

安装虚拟化平台后，下载所选 Linux 发行版的启动磁盘镜像。

- [Ubuntu Desktop](http://www.ubuntu.com/download/desktop)是一个选择。

下载的文件名类似 `ubuntu-20.04.3-desktop-amd64.iso`。启动虚拟化平台，创建新的 64 位虚拟机，将下载的 Ubuntu 镜像用作启动盘。不同虚拟机软件的步骤不同，但应该不难。

### Athena

虽然可以使用 `athena.dialup.mit.edu` 完成实验，但由于可能不太方便，我们不推荐。如果使用运行 Linux 的 MIT Athena 机器，所有工具都位于 `6.828` 课程软件目录中。

通过 SSH 登录某台 Athena 拨入机器，添加工具：

```text
$ ssh {your kerberos}@athena.dialup.mit.edu
$ add -f 6.828
```

## 测试安装

安装正确时，应能够编译并运行 xv6。退出 QEMU 时输入 `Ctrl-a x`：

```text
# in the xv6 directory
$ make qemu
# ... lots of output ...
init: starting sh
$
```

如果无法运行，可以逐项检查组件，例如 QEMU：

```text
$ qemu-system-riscv64 --version
QEMU emulator version 5.1.0
```

以及至少一种 RISC-V 版本的 GCC：

```text
$ riscv64-linux-gnu-gcc --version
riscv64-linux-gnu-gcc (Debian 10.3.0-8) 10.3.0
...
```

```text
$ riscv64-unknown-elf-gcc --version
riscv64-unknown-elf-gcc (GCC) 10.1.0
...
```

```text
$ riscv64-unknown-linux-gnu-gcc --version
riscv64-unknown-linux-gnu-gcc (GCC) 10.1.0
...
```

---

对 S6.081 有疑问或意见？请发送邮件给课程团队：[*6S081@lists.csail.mit.edu*](mailto:6s081@lists.csail.mit.edu)。

[![知识共享许可](assets/5b0302e13e7f.png)](https://creativecommons.org/licenses/by/3.0/us/) **[顶部](#top)** // **[S6.081 首页](https://pdos.csail.mit.edu/6.828/2021/index.html)** // *最后更新于美国东部夏令时间 2025 年 9 月 4 日星期四 23:15:04*

---

**译者说明（不属于原文）**：本页是官方历史安装说明的译文，保留旧系统版本、安装命令和软件授权描述。当前 Mac 的实验环境已经配置为 Lima + Ubuntu ARM64；无需重新执行本页安装命令。

