> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab net: Network driver](https://pdos.csail.mit.edu/6.828/2021/labs/net.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](net.md) · [官方原始 HTML](original/net.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：网络

本实验中，你将为网络接口卡（NIC）编写 xv6 设备驱动。

获取实验源码，并检出 `net` 分支：

```text
  $ git fetch
  $ git checkout net
  $ make clean
```

## 背景

编码前，复习 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)第 5 章“中断与设备驱动”可能有所帮助。

你将使用名为 E1000 的网络设备处理网络通信。对于 xv6 及你编写的驱动，E1000 看起来就像连接在真实以太网局域网（LAN）上的真实硬件。实际上，驱动交互的 E1000 由 QEMU 模拟，连接的局域网同样由 QEMU 模拟。在模拟局域网中，xv6（客户机）的 IP 地址为 `10.0.2.15`。QEMU 还会让运行它的计算机以 `10.0.2.2` 的 IP 地址出现在该局域网中。xv6 通过 E1000 向 `10.0.2.2` 发送数据包时，QEMU 会把包交给运行 QEMU 的真实计算机（宿主机）上的相应应用。

你将使用 QEMU 的“用户模式网络栈”。相关说明见 [QEMU 文档](https://wiki.qemu.org/Documentation/Networking#User_Networking_.28SLIRP.29)。我们已经更新 Makefile，启用了用户模式网络栈和 E1000 网卡。

Makefile 配置 QEMU，将所有收发数据包记录到实验目录中的 `packets.pcap`。查看记录有助于确认 xv6 是否按预期收发数据包。显示记录的数据包：

```text
tcpdump -XXnr packets.pcap
```

本实验为 xv6 仓库添加了一些文件。`kernel/e1000.c` 包含 E1000 初始化代码，以及等待你补全的收发数据包空函数。`kernel/e1000_dev.h` 定义了 E1000 的寄存器和标志位，说明见 Intel E1000 [软件开发者手册](https://pdos.csail.mit.edu/6.828/2021/readings/8254x_GBe_SDM.pdf)。`kernel/net.c` 和 `kernel/net.h` 包含一个简单网络栈，实现了 [IP](https://en.wikipedia.org/wiki/Internet_Protocol)、[UDP](https://en.wikipedia.org/wiki/User_Datagram_Protocol) 和 [ARP](https://en.wikipedia.org/wiki/Address_Resolution_Protocol) 协议，也包含用于保存数据包的灵活数据结构 `mbuf`。最后，`kernel/pci.c` 包含 xv6 启动时在 PCI 总线上查找 E1000 网卡的代码。

## 你的任务

补全 `kernel/e1000.c` 中的 `e1000_transmit()` 和 `e1000_recv()`，使驱动能够收发数据包。`make grade` 报告全部测试通过时，任务就完成了。

编写代码时，需要查阅 E1000 [软件开发者手册](https://pdos.csail.mit.edu/6.828/2021/readings/8254x_GBe_SDM.pdf)。以下章节可能尤其有用：

- 第 2 节必读，它概述了整个设备。
- 第 3.2 节概述数据包接收。
- 第 3.3 节与第 3.4 节一起介绍数据包发送。
- 第 13 节概述 E1000 使用的寄存器。
- 第 14 节有助于理解我们提供的初始化代码。

浏览 E1000 [软件开发者手册](https://pdos.csail.mit.edu/6.828/2021/readings/8254x_GBe_SDM.pdf)。它涵盖多种关系密切的以太网控制器，QEMU 模拟的是 82540EM。现在先略读第 2 章，了解设备。编写驱动需要熟悉第 3、14 章和 4.1 节，不必阅读 4.1 的子节。还需将第 13 章作为参考。其余章节大多涉及驱动无需交互的 E1000 部件。一开始不必纠结细节，先了解文档结构，以便之后查找。E1000 有很多高级功能，大部分可忽略；完成实验只需少量基本功能。

`e1000.c` 中提供的 `e1000_init()` 将 E1000 配置为：从 RAM 读取待发送的数据包，并将收到的数据包写入 RAM。这种技术称为 DMA（直接内存访问），因为 E1000 硬件直接在 RAM 中读写数据包。

突发到达的数据包可能比驱动处理得更快，因此 `e1000_init()` 为 E1000 提供多个可写入数据包的缓冲区。E1000 要求用 RAM 中的“描述符”数组描述这些缓冲区；每个描述符包含一个 RAM 地址，E1000 可在该处写入收到的数据包。`struct rx_desc` 描述了描述符格式。描述符数组称为接收环（receive ring）或接收队列。所谓环，是指网卡或驱动到达数组末尾后会回到开头。`e1000_init()` 使用 `mbufalloc()` 分配 `mbuf` 数据包缓冲区，供 E1000 通过 DMA 写入。另有一个发送环，驱动将希望 E1000 发送的数据包放入其中。`e1000_init()` 将两个环的大小分别配置为 `RX_RING_SIZE` 和 `TX_RING_SIZE`。

`net.c` 中的网络栈需要发送数据包时，会调用 `e1000_transmit()`，传入保存待发送数据包的 mbuf。你的发送代码必须将指向数据包内容的指针放进 TX（发送）环的描述符。描述符格式见 `struct tx_desc`。必须确保每个 mbuf 最终被释放，但只能在 E1000 完成发送之后释放；E1000 通过设置描述符中的 `E1000_TXD_STAT_DD` 位表示完成。

E1000 每次从以太网收到数据包时，会先通过 DMA 将其写入 RX（接收）环下一个描述符所指向的 mbuf，再产生中断。你的 `e1000_recv()` 必须扫描 RX 环，并调用 `net_rx()`，将每个新数据包的 mbuf 交给 `net.c` 中的网络栈。随后需分配新的 mbuf 并放入描述符，使 E1000 再次到达该位置时，有新的缓冲区可用于 DMA 写入数据包。

除了读写 RAM 中的描述符环，驱动还需通过内存映射控制寄存器与 E1000 交互，检测是否收到数据包，并告知 E1000 驱动已将待发送数据包填入 TX 描述符。全局变量 `regs` 指向 E1000 的第一个控制寄存器；可像访问数组一样按索引访问其他寄存器。尤其需要使用 `E1000_RDT` 和 `E1000_TDT`。

测试驱动时，在一个窗口运行 `make server`，在另一个窗口运行 `make qemu`，然后在 xv6 中运行 `nettests`。`nettests` 的第一个测试尝试向宿主操作系统发送 UDP 包，目标是 `make server` 启动的程序。如果尚未完成实验，E1000 驱动并不会真正发送数据包，因此基本不会发生什么。

完成后，驱动会发包，QEMU 将其交给宿主机，`make server` 收到后会发送响应，E1000 驱动继而 `nettests` 将收到响应。不过，宿主机在响应前会先向 xv6 发送 ARP 请求，询问其 48 位以太网地址，并等待 ARP 应答。完成 E1000 驱动后，`kernel/net.c` 会负责这部分。如果一切正常，`nettests` 打印 `testing ping: OK`，`make server` 打印 `a message from xv6!`。

`tcpdump -XXnr packets.pcap` 的输出开头应类似：

```text
reading from file packets.pcap, link-type EN10MB (Ethernet)
15:27:40.861988 IP 10.0.2.15.2000 > 10.0.2.2.25603: UDP, length 19
        0x0000:  ffff ffff ffff 5254 0012 3456 0800 4500  ......RT..4V..E.
        0x0010:  002f 0000 0000 6411 3eae 0a00 020f 0a00  ./....d.>.......
        0x0020:  0202 07d0 6403 001b 0000 6120 6d65 7373  ....d.....a.mess
        0x0030:  6167 6520 6672 6f6d 2078 7636 21         age.from.xv6!
15:27:40.862370 ARP, Request who-has 10.0.2.15 tell 10.0.2.2, length 28
        0x0000:  ffff ffff ffff 5255 0a00 0202 0806 0001  ......RU........
        0x0010:  0800 0604 0001 5255 0a00 0202 0a00 0202  ......RU........
        0x0020:  0000 0000 0000 0a00 020f                 ..........
15:27:40.862844 ARP, Reply 10.0.2.15 is-at 52:54:00:12:34:56, length 28
        0x0000:  ffff ffff ffff 5254 0012 3456 0806 0001  ......RT..4V....
        0x0010:  0800 0604 0002 5254 0012 3456 0a00 020f  ......RT..4V....
        0x0020:  5255 0a00 0202 0a00 0202                 RU........
15:27:40.863036 IP 10.0.2.2.25603 > 10.0.2.15.2000: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 0000 0000 4011 62b0 0a00 0202 0a00  .-....@.b.......
        0x0020:  020f 6403 07d0 0019 3406 7468 6973 2069  ..d.....4.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
```

实际输出会略有不同，但应包含 `"ARP, Request"`、`"ARP, Reply"`、`"UDP"`、`"a.message.from.xv6"` 和 `"this.is.the.host"`。

`nettests` 还会执行其他测试，最后通过真实互联网向 Google 的一个域名服务器发送 DNS 请求。确保代码通过所有这些测试，之后应看到：

```text
$ nettests
nettests running on port 25603
testing ping: OK
testing single-process pings: OK
testing multi-process pings: OK
testing DNS
DNS arecord for pdos.csail.mit.edu. is 128.52.129.126
DNS OK
all tests passed.
```

还应确保 `make grade` 也确认你的实现通过测试。

## 提示

先在 `e1000_transmit()` 和 `e1000_recv()` 中加入打印语句，再运行 `make server` 和 xv6 中的 `nettests`。应能从打印看到 `nettests` 触发了对 `e1000_transmit` 的调用。

实现 `e1000_transmit` 的一些提示：

- 先读取 `E1000_TDT` 控制寄存器，获取 E1000 期待下一个数据包所在的 TX 环索引。
- 检查环是否已满。如果 `E1000_TDT` 索引的描述符中未设置 `E1000_TXD_STAT_DD`，表明 E1000 尚未完成此前对应的发送请求，应返回错误。
- 否则，使用 `mbuffree()` 释放此前由该描述符发送的 mbuf，如果存在。
- 填写描述符。`m->head` 指向内存中的包内容，`m->len` 为包长。设置必要的 cmd 标志，参阅手册 3.3 节，并保存 mbuf 指针，以便之后释放。
- 最后，将 `E1000_TDT` 加 1 后对 `TX_RING_SIZE` 取模，更新环位置。
- 如果成功将 mbuf 加入环，`e1000_transmit()` 返回 0；失败时，例如没有可用描述符，返回 -1，让调用者知道需要释放 mbuf。

实现 `e1000_recv` 的一些提示：

- 先读取 `E1000_RDT`，加 1 后对 `RX_RING_SIZE` 取模，获取下一个等待处理的已收数据包所在的环索引，如果存在。
- 检查描述符 `status` 中的 `E1000_RXD_STAT_DD` 位，判断有无新数据包；没有则停止。
- 否则，将 mbuf 的 `m->len` 更新为描述符报告的长度，再用 `net_rx()` 将 mbuf 交给网络栈。
- 使用 `mbufalloc()` 分配新 mbuf，替换刚交给 `net_rx()` 的缓冲区。将其数据指针 `m->head` 写入描述符，并将描述符状态位清零。
- 最后，将 `E1000_RDT` 更新为最后一个已处理环描述符的索引。
- `e1000_init()` 用 mbuf 初始化 RX 环，可参考甚至复用相应代码。
- 累计收到的数据包总数最终会超过环大小 16，确保代码能处理这种情况。

你需要使用锁，处理 xv6 多个进程可能同时使用 E1000，或者内核线程使用 E1000 时发生中断的情况。

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

以下部分挑战的收益，只有在真实的高性能硬件上，也就是基于 x86 的计算机上，才能测量或测试。

- 本实验的网络栈通过中断处理入站数据包，但不使用中断处理出站数据包。更复杂的策略是先在软件中排队保存出站包，每次只向 NIC 提供有限数量，再依靠 TX 中断补充发送环。这样就可以为不同类型的出站流量设置优先级。
- 提供的网络代码只部分支持 ARP。实现完整的 [ARP 缓存](https://tools.ietf.org/html/rfc826)，并将其接入 `net_tx_eth()`。
- E1000 支持多个 RX、TX 环。配置 E1000，为每个核心提供一对收发环，并修改网络栈以支持多个环。这有望提高吞吐量并减少锁竞争，但难以测试和测量。
- `sockrecvudp()` 使用单向链表查找目标 socket，效率较低。尝试改用哈希表和 RCU 提高性能，不过完整实现很难测试和测量。
- [ICMP](https://tools.ietf.org/html/rfc792) 可以通知网络通信失败。检测这些通知，并通过 socket 系统调用接口将其作为错误传递。
- E1000 支持多种无状态硬件卸载，包括校验和计算、RSC 和 GRO。利用其中一种或多种提高网络栈吞吐量，不过难以测试和测量。
- 本实验网络栈容易出现接收活锁。结合课堂内容和指定阅读，设计并实现解决方案，不过难以测试。
- 为 xv6 实现 UDP 服务器。
- 实现最小化 TCP 协议栈，并下载一个网页。

如果你选择完成挑战题，无论是否与网络有关，都请告知课程团队！
