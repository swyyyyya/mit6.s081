<!-- 本文件为项目 README 的中文译文。正文按原文顺序翻译，人名、链接及命令保留原样。 -->

xv6 是对 Dennis Ritchie 和 Ken Thompson 的 Unix Version 6（v6）的重新实现。xv6 大体沿用了 v6 的结构和风格，但使用 ANSI C 实现，面向现代 RISC-V 多处理器系统。

## 致谢

xv6 的灵感来自 John Lions 的《UNIX 第六版评注》（Commentary on UNIX 6th Edition；Peer to Peer Communications 出版；ISBN：1-57398-013-7；第 1 版，2000 年 6 月 14 日）。另请参阅 https://pdos.csail.mit.edu/6.828/，其中提供了指向 v6 在线资源的链接。

以下人员作出了贡献：Russ Cox（上下文切换、锁）、Cliff Frey（多处理器，MP）、Xiao Yu（多处理器，MP）、Nickolai Zeldovich 和 Austin Clements。

我们也感谢以下人员贡献的错误报告和补丁：
Takahiro Aoyagi、Silas Boyd-Wickizer、Anton Burtsev、Ian Chen、Dan
Cross、Cody Cutler、Mike CAT、Tej Chajed、Asami Doi、eyalz800、Nelson
Elhage、Saar Ettinger、Alice Ferrazzi、Nathaniel Filardo、flespark、
Peter Froehlich、Yakir Goaron、Shivam Handa、Matt Harvey、Bryan Henry、
jaichenhengjie、Jim Huang、Matúš Jókay、Alexander Kapshuk、Anders
Kaseorg、kehao95、Wolfgang Keller、Jungwoo Kim、Jonathan Kimmitt、
Eddie Kohler、Vadim Kolontsov、Austin Liew、l0stman、Pavan
Maddamsetti、Imbar Marinescu、Yandong Mao、Matan Shabtay、Hitoshi
Mitake、Carmi Merimovich、Mark Morrissey、mtasm、Joel Nider、
OptimisticSide、Greg Price、Jude Rich、Ayan Shafqat、Eldar Sehayek、
Yongming Shen、Fumiya Shigemitsu、Cam Tenny、tyfkda、Warren Toomey、
Stephen Tu、Rafael Ubal、Amane Uehara、Pablo Ventura、Xi Wang、Keiichi
Watanabe、Nicolas Wolovick、wxdao、Grant Wu、Jindong Zhang、Icenowy
Zheng、ZhUyU1997 和 Zou Chang Wei。

构成 xv6 的各文件中的代码，其版权归属如下：
Copyright 2006-2020 Frans Kaashoek、Robert Morris 和 Russ Cox。

## 错误报告

请将错误和建议发送给 Frans Kaashoek 和 Robert Morris（kaashoek,rtm@mit.edu）。xv6 的主要用途是作为 MIT 6.S081 课程的教学操作系统，因此，相比新增功能，我们更关注简化实现和澄清说明。

## 构建和运行 xv6

你需要从 https://github.com/riscv/riscv-gnu-toolchain 获取 RISC-V “newlib” 工具链，以及针对 `riscv64-softmmu` 编译的 QEMU。安装完成并将这些工具加入 shell 的搜索路径后，即可运行 `make qemu`。
