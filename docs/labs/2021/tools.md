> MIT 6.S081 2021 官方原文的本地 Markdown 转换版。未翻译、未添加答案。
> 来源：[Tools Used in 6.S081](https://pdos.csail.mit.edu/6.828/2021/tools.html)；下载日期：2026-10-25。
> 如格式有歧义，请对照 [本地 HTML](html/tools.html) 或 [原始页面存档](original/tools.html)。

Toggle navigation

[6.S081: Operating System Engineering](https://pdos.csail.mit.edu/6.828/2021/index.html)

- [Schedule](https://pdos.csail.mit.edu/6.828/2021/schedule.html)
- [Class](#)
  - [Overview](https://pdos.csail.mit.edu/6.828/2021/overview.html)
  - [Administrivia](https://pdos.csail.mit.edu/6.828/2021/general.html)
  - [Handin website](https://6828.scripts.mit.edu/2021/handin.py/)
  - [6.S081 2020](http://pdos.csail.mit.edu/6.828/2020)
- [Labs](#)
  - [Tools](tools.md)
  - [Guidance](guidance.md)
  - [Lab Utilities](util.md)
  - [Lab System calls](syscall.md)
  - [Lab Page tables](pgtbl.md)
  - [Lab Traps](traps.md)
  - [Lab Copy on-write](cow.md)
  - [Lab Multithreading](thread.md)
  - [Lab network driver](net.md)
  - [Lab Lock](lock.md)
  - [Lab File system](fs.md)
  - [Lab mmap](mmap.md)
- [xv6](#)
  - [xv6](https://pdos.csail.mit.edu/6.828/2021/xv6.html)
  - [xv6 book](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)
- [References](https://pdos.csail.mit.edu/6.828/2021/reference.html)
- [Piazza](https://piazza.com/mit/fall2021/6s081)

2021

# Tools Used in 6.S081

For this class you'll need the RISC-V versions of a couple different tools:
QEMU 5.1+, GDB 8.3+, GCC, and Binutils.

If you are having trouble getting things set up, please come by to office
hours or post on Piazza. We're happy to help!

### Installing on Windows

First make sure you have the [Windows Subsystem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install-win10) installed. Then add [Ubuntu 20.04 from the Microsoft Store](https://www.microsoft.com/en-us/p/ubuntu/9nblggh4msv6). Afterwards you should be able to launch Ubuntu and interact with the machine. To install all the software you need for this class, run:

```text
$ sudo apt-get update && sudo apt-get upgrade
$ sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

From Windows, you can access all of your WSL files under the *"\\wsl$\"*
directory. For instance, the home directory for an Ubuntu 20.04 installation should
be at *"\\wsl$\Ubuntu-20.04\home\<username>\"*.

### Installing on macOS

First, install developer tools:

```text
$ xcode-select --install
```

Next, install [Homebrew](https://brew.sh/), a package manager for macOS:

```text
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Next, install the [RISC-V compiler toolchain](https://github.com/riscv/homebrew-riscv):

```text
$ brew tap riscv/riscv
$ brew install riscv-tools
```

The brew formula may not link into `/usr/local`. You will need to
update your shell's rc file (e.g. [~/.bashrc](https://www.gnu.org/software/bash/manual/html_node/Bash-Startup-Files.html))
to add the appropriate directory to [$PATH](http://www.linfo.org/path_env_var.html).

```text
PATH=$PATH:/usr/local/opt/riscv-gnu-toolchain/bin
```

Finally, install QEMU:

```text
brew install qemu
```

### Debian or Ubuntu

```text
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

### Arch Linux

```text
sudo pacman -S riscv64-linux-gnu-binutils riscv64-linux-gnu-gcc riscv64-linux-gnu-gdb qemu-arch-extra
```

### Running a Linux VM

If the other options listed don't work, you can also try running a virtual
machine with one of the other operating systems listed above. With platform
virtualization, Linux can run alongside your normal computing environment.
Installing a Linux virtual machine is a two step process. First, you download
the virtualization platform.

- [**VirtualBox**](https://www.virtualbox.org/)
  (free for Mac, Linux, Windows) — [Download
  page](https://www.virtualbox.org/wiki/Downloads)
- [VMware
  Player](http://www.vmware.com/products/player/) (free for Linux and Windows, registration required)
- [VMware
  Fusion](http://www.vmware.com/products/fusion/) (Downloadable from IS&T for free).

VirtualBox is a little slower and less flexible, but free!

Once the virtualization platform is installed, download a boot
disk image for the Linux distribution of your choice.

- [Ubuntu
  Desktop](http://www.ubuntu.com/download/desktop) is one option.

This will download a file named something like
`ubuntu-20.04.3-desktop-amd64.iso`. Start up your
virtualization platform and create a new (64-bit) virtual machine.
Use the downloaded Ubuntu image as a boot disk; the
procedure differs among VMs but shouldn't be too difficult.

### Athena

We don't recommend it since it can be somewhat inconvenient, but you can use
athena.dialup.mit.edu to work on the labs. If you use the MIT Athena machines
that run Linux, then all of these tools are located in the 6.828 locker.

ssh into one of the Athena dialup machines and add the tools:

```text
$ ssh {your kerberos}@athena.dialup.mit.edu
$ add -f 6.828
```

## Testing your Installation

To test your installation, you should be able to compile and run xv6 (to quit qemu type Ctrl-a x):

```text
# in the xv6 directory
$ make qemu
# ... lots of output ...
init: starting sh
$
```

If that doesn't work, you can double check individual components. Which include QEMU:

```text
$ qemu-system-riscv64 --version
QEMU emulator version 5.1.0
```

And at least one RISC-V version of GCC:

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



Questions or comments regarding S6.081? Send e-mail to the course staff at
[*6S081@lists.csail.mit.edu*](mailto:6s081@lists.csail.mit.edu).

[![Creative Commons License](assets/5b0302e13e7f.png)](https://creativecommons.org/licenses/by/3.0/us/) **[Top](#top)** //
**[S6.081 home](https://pdos.csail.mit.edu/6.828/2021/index.html)** //
*Last updated Thursday, 04-Sep-2025 23:15:04 EDT*
