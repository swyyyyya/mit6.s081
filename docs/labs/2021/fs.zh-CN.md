> MIT 6.S081 2021 官方实验说明中文译文。按原文翻译，未添加解答；命令、代码及终端输出保留原样。
> 官方来源：[Lab fs: File system](https://pdos.csail.mit.edu/6.828/2021/labs/fs.html)；原文下载日期：2026-10-25。
> 对照：[英文 Markdown](fs.md) · [官方原始 HTML](original/fs.html)。
> 原文中的历史环境要求、截止日期和提交流程均保留，不表示当前自学环境需要重新配置。

# 实验：文件系统

本实验将为 xv6 文件系统添加大文件和符号链接支持。

编码前，请阅读 [xv6 教材](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)第 8 章“文件系统”，并研究对应代码。

获取实验源码，并检出 `util` 分支：

```text
  $ git fetch
  $ git checkout fs
  $ make clean
```

## 大文件

本练习将提高 xv6 文件的最大大小。目前每个文件最多包含 268 个块，即 `268*BSIZE` 字节，xv6 中 `BSIZE` 为 1024。这一限制源于 xv6 inode 包含 12 个“直接”块号和一个“一级间接”块号。后者指向的块最多能保存 256 个块号，因此总共为 `12+256=268` 个块。

`bigfile` 命令会创建尽可能长的文件，并报告大小：

```text
$ bigfile
..
wrote 268 blocks
bigfile: file is too small
$
```

测试失败是因为 `bigfile` 期待创建包含 65803 个块的文件，但未修改的 xv6 将文件限制为 268 个块。

你将修改文件系统代码，使每个 inode 支持一个“二级间接”块，其中包含 256 个一级间接块地址，而每个一级间接块又可包含最多 256 个数据块地址。这样，文件最多可包含 65803 个块，即 `256*256+256+11` 个块。这里是 11 而不是 12，因为需要牺牲一个直接块号的位置来保存二级间接块号。

### 准备工作

`mkfs` 创建 xv6 文件系统磁盘镜像，决定文件系统的总块数，大小由 `kernel/param.h` 中的 `FSSIZE` 控制。本实验仓库将 `FSSIZE` 设为 200,000 个块。在 make 输出中，应看到 `mkfs/mkfs` 的以下信息：

```text
nmeta 70 (boot, super, log blocks 30 inode blocks 13, bitmap blocks 25) blocks 199930 total 200000
```

这一行描述构建出的文件系统：70 个元数据块，即描述文件系统的块，以及 199,930 个数据块，共 200,000 个块。

实验过程中，如果需要从头重建文件系统，可运行 `make clean`，强制 make 重新构建 `fs.img`。

### 需要阅读的内容

磁盘 inode 的格式由 `fs.h` 中的 `struct dinode` 定义。重点关注 `NDIRECT`、`NINDIRECT`、`MAXFILE` 和 `struct dinode` 的 `addrs[]`。教材图 8.3 给出了标准 xv6 inode 的示意图。

根据文件信息在磁盘上查找数据的代码是 `fs.c` 中的 `bmap()`。阅读并确保理解其工作方式。读写文件都会调用 `bmap()`。写文件时，它会按需分配新块存放文件内容，必要时还会分配间接块保存块地址。

`bmap()` 处理两种块号。参数 `bn` 是“逻辑块号”，即文件内部相对于文件起始处的块号。`ip->addrs[]` 中的块号，以及传给 `bread()` 的块号，则是磁盘块号。可以将 `bmap()` 理解为把文件的逻辑块号映射为磁盘块号。

### 你的任务

修改 `bmap()`，使其除了直接块和一级间接块之外，还支持二级间接块。为腾出位置，直接块数量必须从 12 减为 11；不允许改变磁盘 inode 的大小。`ip->addrs[]` 前 11 项应为直接块，第 12 项是与当前相同的一级间接块，第 13 项是新增的二级间接块。当 `bigfile` 能写入 65803 个块，且 `usertests` 成功运行时，本练习即完成：

```text
$ bigfile
..................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................
wrote 65803 blocks
done; ok
$ usertests
...
ALL TESTS PASSED
$
```

`bigfile` 至少需要运行一分半钟。

提示：

- 确保理解 `bmap()`。画图表示 `ip->addrs[]`、间接块、二级间接块及其指向的一级间接块、数据块之间的关系。理解为什么增加二级间接块可使最大文件大小增加 `256*256` 个块，准确说还要减 1，因为直接块数量减少了一个。
- 思考如何使用逻辑块号索引二级间接块及其指向的间接块。
- 修改 `NDIRECT` 后，可能还需要修改 `file.h` 的 `struct inode` 中 `addrs[]` 的声明。确保 `struct inode` 与 `struct dinode` 的 `addrs[]` 元素数量相同。
- 修改 `NDIRECT` 后，务必创建新的 `fs.img`，因为 `mkfs` 使用 `NDIRECT` 构建文件系统。
- 如果文件系统因崩溃等原因进入损坏状态，请从 Unix 而不是 xv6 中删除 `fs.img`。make 会重新构建干净的文件系统镜像。
- 每个通过 `bread()` 获取的块，都不要忘记调用 `brelse()`。
- 与原始 `bmap()` 一样，仅在需要时分配一级、二级间接块。
- 确保 `itrunc` 释放文件的所有块，包括二级间接块。

## 符号链接

本练习将为 xv6 添加符号链接。符号链接，也称软链接，使用路径名引用目标文件；打开符号链接时，内核会沿链接找到目标。符号链接类似硬链接，但硬链接只能指向同一磁盘上的文件，而符号链接可跨磁盘设备。虽然 xv6 不支持多个设备，实现该系统调用仍是理解路径名查找方式的好练习。

### 你的任务

实现 `symlink(char *target, char *path)` 系统调用，在 `path` 创建指向 `target` 所指定文件的新符号链接。更多信息见 `symlink` 手册页。将 `symlinktest` 加入 Makefile 并运行。测试产生以下输出且 `usertests` 成功时，实现即完成：

```text
$ symlinktest
Start: test symlinks
test symlinks: ok
Start: test concurrent symlinks
test concurrent symlinks: ok
$ usertests
...
ALL TESTS PASSED
$
```

提示：

- 先为 `symlink` 创建新的系统调用编号，在 `user/usys.pl` 和 `user/user.h` 中添加条目，并在 `kernel/sysfile.c` 中实现空的 `sys_symlink`。
- 在 `kernel/stat.h` 添加新文件类型 `T_SYMLINK`，表示符号链接。
- 在 `kernel/fcntl.h` 中添加 `open` 可使用的新标志 `O_NOFOLLOW`。传给 `open` 的标志通过按位或组合，因此新标志不应与已有标志重叠。将 `user/symlinktest.c` 加入 Makefile 后，这些修改可使它编译通过。
- 实现 `symlink(target, path)`，在 `path` 创建指向 `target` 的新符号链接。即使 `target` 不存在，调用也应能成功。需要选择保存目标路径的位置，例如 inode 的数据块。与 `link`、`unlink` 类似，`symlink` 返回整数，0 表示成功，-1 表示失败。
- 修改 `open`，处理路径指向符号链接的情况。目标文件不存在时，`open` 必须失败。如果进程指定 `O_NOFOLLOW`，则打开符号链接本身，不沿链接访问目标。
- 如果目标仍是符号链接，必须递归跟随，直到到达非链接文件。如果链接形成环，必须返回错误。可以通过链接深度达到某个阈值，例如 10 时返回错误，近似检测环。
- 其他系统调用，如 `link` 和 `unlink`，不得跟随符号链接，而应操作符号链接本身。
- 本实验不要求处理指向目录的符号链接。

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

支持三级间接块。

### 致谢

感谢华盛顿大学 CSEP551（2019 年秋季）课程团队提供符号链接练习。

---

**译者说明（不属于原文）**：原文开头的说明文字写作 `util` 分支，但紧接着的命令使用 `git checkout fs`。译文保留该差异；本实验代码位于 `fs` 分支。

