# LemonNext for Windows

为 OI 比赛而生的轻量评测系统，内置「在线提交服务」：选手通过浏览器提交代码，教师端一键收卷评测。

A tiny judging environment for OI contests, with a built-in online submission server.

本仓库是 **LemonNext 1.0.0（macOS）的 Windows 移植**，以 Project LemonLime Online 为底座做增量实现。
移植依据、模块差异清单与分阶段计划见 **[PORTING.md](PORTING.md)**。

需要 Qt 6.8 或更高版本。支持 Windows、Linux 与 macOS。

## 在线提交服务（LemonLime Online）

教师端在比赛目录上一键启动 HTTP 服务，选手用浏览器访问即可完成整场比赛的提交：

- **账号管理**：批量生成选手账号、CSV 导入/导出、明文名单留存
- **比赛时间窗**：设置起止时间，未开始/已结束时禁止提交，页面实时倒计时
- **题面及样例下发**：支持 PDF、压缩包等任意单个文件，浏览器内预览或下载
- **多种提交方式**：
  - 在线编辑器粘贴/编写代码，Ctrl+Enter 快捷提交
  - 单题上传源码文件（自动按题目要求的文件名保存）
  - 整包上传以准考证号命名的选手文件夹，原样写入 `source/<准考证号>/`
- **自动评测**：可开关；开启时提交完成即整包评测，成绩回写选手列表
- **安全**：目录名校验防路径穿越、单层文件名过滤、上传体量限制、登录态会话管理

学生端界面：浅色清爽风格，大字号比赛倒计时、题目卡片、单行/卡片式布局、移动端自适应。

## 本地评测

- 支持 4 种题目类型：传统题、提交答案题、交互题、通信题（交互/通信暂时只确保 C++）
- 选手×题目粒度任意多选重测，一键测试未测试/未找到源文件/编译错误
- 子任务依赖、增强测试点调整器、分数统计分析、整理文件
- 实数比较同时判断绝对/相对误差及 `nan`/`inf`
- 可自定义最大重新评测次数、多线程评测（实验）、高 DPI、成绩颜色主题（IOI/JOI 风格，可自定义）
- 导出带颜色与跳转的 HTML 成绩单

## 安装

### Windows

从 [Releases](https://github.com/cjliwea/LemonNext-Windows/releases) 下载 `lemon-win-qt6-x64-Release.zip`，解压即用（绿色便携版）。

### macOS / Linux

从 Releases 或 GitHub Actions 取用预构建包，或参考构建指南自行编译。

## 用户手册

软件内置离线用户手册。

## 构建

请看 [LemonLime 构建指南](BUILD.md)。

## Credit

```
Copyright (c) 2019-2022 Project LemonLime.
Copyright (c) 2026 Project LemonLime Online.

Libraries and other files that have been used in LemonLime are listed below:

Copyright (c) 2020 Itay Grudev (@itay-grudev): SingleApplication (MIT)

Copyright (c) 2020 Qv2ray Development Group (@Qv2ray): Design of Translator/Log, Project Structure and CI files (GPLv3)
```
