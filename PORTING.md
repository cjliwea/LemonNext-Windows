# LemonNext Windows 移植蓝图

本文档是 **LemonNext 1.0.0（macOS）→ LemonNext-Windows** 的移植依据与任务清单。

规格来源有三个，按可信度排序：

| 来源 | 内容 | 可信度 |
|---|---|---|
| A. 发行包内文档 | `macOS发行说明.md`、`SPJ-README.md`、`选手与账号导入说明.md`、`注册码使用说明.txt` | 行为规格，作者自述，最高 |
| B. 二进制符号表 | 类名、模块名、翻译单元名（Mach-O 未剥离） | 结构规格，机器还原 |
| C. 二进制内嵌资源 | `qrc_server_assets` 中的学生端 HTML/CSS/JS | 可直接复用 |

> LemonNext 与 Project LemonLime Online 同源代码系（二进制内含 `SPDX-FileCopyrightText: 2026 Project LemonLime Online`），
> 因此本项目以 Project LemonLime Online 为底座做增量移植，而非重写。

---

## 一、底座现状（Project LemonLime Online）

- 题型枚举 `Task::TaskType` = `Traditional / AnswersOnly / Interaction / Communication / CommunicationExec`
- 比较方式 `ComparisonMode` 已含 `TestlibSpecialJudgeMode`（SPJ 作为**比较方式**，不是独立题型）
- 已有：本地评测、内置在线提交服务（`SubmissionServer` / `OnlineServerDialog`）、账号（`UserStore`）、
  会话（`SessionManager`）、统计（`StatisticsBrowser`）、导出（`ExportUtil`）、评测调度（`JudgingController`）

## 二、模块差异（B 来源，符号表还原）

### 双方都有
`SubmissionServer`、`OnlineServerDialog`、`UserStore`、`SessionManager`、`SpecialJudge`、`AddTaskDialog`、`ExportUtil`、`JudgingController`

### LemonNext 独有 —— 本项目待移植

| 模块 | 作用 | 依据 |
|---|---|---|
| `ChoiceJudge` / `ChoiceEditorDialog` / `core/choicejudge.cpp` | 选择题题型与判分 | A+C |
| `AddTaskBasePage` / `AddTaskTraditionalPage` / `AddTaskAnswersOnlyPage` / `AddTaskDropArea` | 新版「加题」向导，按 4 题型分页 | A+B |
| `StatementEditorDialog` / `MarkdownHighlighter` / `MarkdownImageEditor` / `MarkdownEditorUtil` | 题面 Markdown 编辑器（含图片） | B |
| `Roster` / `RosterWidget` | 准考证号名单：列映射、随机分配、可编辑预览、待更新状态 | A+B |
| `PresenceSocket` / `PresenceMonitor` | 连接心跳与在线状态（支撑「在线/离线/待批准」「断线锁定」） | A+B |
| `LauncherDialog` | 首次启动向导（编译器检测 / 稍后配置 / 体验示例比赛） | A+B |
| `TaskScanner` | 题目扫描 | B |
| 学生端 GESP 页面 | 客观题作答、提交状态、`/api/choice/*` 路由 | A+C |

### 明确不做

- **`Licensing`（联网激活 / 设备绑定 / 名额）**：第三方作者的商业授权机制，不复刻、不绕过。
- 交互题与通信题：LemonNext **移除**了这两种题型；本移植保留 `Interaction` / `Communication` 代码以兼容旧比赛数据，
  但新建入口不提供（与 LemonNext 行为一致）。

## 三、题型模型改造

LemonNext 的新建入口顺序（依据 A）：

1. 编程题 —— 逐行 / 忽略空格 / 实数 / 外部 diff 比较
2. 编程题（仅提交答案）—— 只收答案文件，不编译源码
3. 编程题（Special Judge）—— 独立题型，存储编号 **6**，使用 testlib / Lemon 判题程序
4. 选择题 —— 存储编号 **5**

底座改造点：
- `Task::TaskType` 增加 `Choice`；SPJ 从「比较方式」提升为独立题型（编号 6）
- 旧比赛的 2/3/4（交互/通信）仅用于读取与保留历史数据，不可编辑或重测
- 含已移除题型的比赛不能启动在线提交，需先复制比赛再删除

## 四、分阶段计划

| 阶段 | 内容 | 验收 |
|---|---|---|
| P0 | 仓库与 CI（已完成） | Windows zip 可下载 |
| P1 | 题型模型：枚举、序列化、向导入口、旧比赛兼容读取 | 新建 4 种题型 + 旧比赛可打开 |
| P2 | SPJ 独立题型 + testlib 判定与计分（`quitp` / `_pc(n)`） | 内置 testlib 编译并跑通 |
| P3 | 选择题：编辑、判分、`/api/choice/*`、GESP 学生端页面 | 学生端可答题、成绩回写 |
| P4 | 题面 Markdown 编辑器（含图片、高亮） | 题面可编辑与预览 |
| P5 | Roster 名单（列映射 / 随机分配 / 预览） | 导入名单生成账号并导出 |
| P6 | Presence 在线状态与断线锁定 | 账号页状态筛选与设备绑定生效 |

## 五、来源与合规

- 底座 Project LemonLime Online 与 LemonNext 均为 **GPL-3.0-or-later**。
- 本项目不复刻第三方授权/激活机制，不含任何注册码校验、设备绑定或许可证发放逻辑。
- LemonNext 的原始作者信息保留在 `AUTHORS` 与版权声明中。
