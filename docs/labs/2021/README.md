# MIT 6.S081 2021 实验题目（中英对照）

本目录包含 2021 年课程表列出的全部 10 个 lab，以及工具安装和实验指导的完整中文译文；英文原文与原始网页并列保留。

## 实验顺序

| 顺序 | 实验 | 中文译文 | 英文原文 |
|---|---|---|---|
| 1 | util：Unix 实用工具 | [Unix 实用工具](util.zh-CN.md) | [英文](util.md) |
| 2 | syscall：系统调用 | [系统调用](syscall.zh-CN.md) | [英文](syscall.md) |
| 3 | pgtbl：页表 | [页表](pgtbl.zh-CN.md) | [英文](pgtbl.md) |
| 4 | traps：陷阱 | [陷阱](traps.zh-CN.md) | [英文](traps.md) |
| 5 | cow：写时复制 fork | [写时复制 fork](cow.zh-CN.md) | [英文](cow.md) |
| 6 | thread：多线程 | [多线程](thread.zh-CN.md) | [英文](thread.md) |
| 7 | net：网络驱动 | [网络驱动](net.zh-CN.md) | [英文](net.md) |
| 8 | lock：并行与锁 | [并行与锁](lock.zh-CN.md) | [英文](lock.md) |
| 9 | fs：文件系统 | [文件系统](fs.zh-CN.md) | [英文](fs.md) |
| 10 | mmap：内存映射 | [内存映射](mmap.zh-CN.md) | [英文](mmap.md) |

## 配套指导

- [工具安装（中文）](tools.zh-CN.md) · [英文](tools.md)
- [实验指导与调试建议（中文）](guidance.zh-CN.md) · [英文](guidance.md)

## 阅读说明

- 在 VS Code 打开 Markdown 文件，按 `Command+Shift+V` 查看预览。
- 中文正文按原文翻译，保留要求、提示、思考题、可选挑战、致谢和提交说明，未添加实验解答。
- 命令、代码示例、终端输出和文件路径保留原样；代码块中的英文注释也随原代码保留，以便复制和对照。
- 原文的历史环境、版本、日期和课程提交网站保留原样；当前环境已配置，不需重装。
- 原文个别路径笔误、分支说明差异和调试指示问题，在相应译文末尾用“译者说明”单独标出。
- `original/` 保存下载时的官方原始 HTML；`html/` 为修正本地链接和样式的英文离线阅读版。
- `assets/` 保存页面图片和样式。教材、论文、视频、参考手册等外部链接仍需联网。
- `manifest.json` 记录原文来源、下载日期及校验值；`translation-checks.json` 记录译文文件、标题数量及代码块校验结果。
- 当前实验代码在 `util` 分支。做其他 lab 时按题目切换分支，先保存已有代码和文档改动。

来源：[MIT 2021 课程表](https://pdos.csail.mit.edu/6.828/2021/schedule.html)。
