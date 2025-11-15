# Release v2

发布版本：v2

发行日期：2025-11-15（UTC）

提交说明：`docs(readme): 使用 Mermaid 重绘架构图并补充说明` (commit: `944e678`)

---

## 概述

v2 主要内容是改善 README 的可读性与架构展示：将原本的 ASCII 架构图替换为 Mermaid 流程图，并在文档中补充渲染提示。该改动属于文档优化类更新，不影响二进制或 API 行为。

此版本旨在为贡献者和阅读者提供更清晰的架构视图及渲染说明，方便理解项目的多线程与解耦设计。

---

## 主要变更点（概要）

- 文档：将 README 中的 ASCII 架构图替换为 Mermaid 流程图以提升可读性与可视化表达。
  - 保留中文 + 英文注释，方便国际化阅读。
  - 为 Mermaid 图添加样式优化（节点颜色、边框）以提高对比度。
- 文档：在 Mermaid 图上方补充说明提示：如果某些平台不支持 Mermaid 渲染，可启用插件或使用静态图片备选方案。

---

## 详细变更（改动列表）

- README.md
  - `### 核心架构` 页面：用 Mermaid 流程图替换了原 ASCII 艺术图。
  - 新增一行说明，提醒用户在 GitHub 或其它平台启用 Mermaid 渲染器。
  - 添加了样式和注释，保持结构清晰。

---

## 发行影响

- 向后兼容性：仅修改文档，不影响任何运行时代码或 API。
- 升级步骤：无，需要额外操作。

---

## 已知问题与限制

- GitHub 自带渲染对 Mermaid 的支持可能受限（取决于平台/设置），如果你在 GitHub 上看不到图形，请启用支持或使用替代方案（如直接在 `README.md` 中嵌入 PNG）。

---

## 致谢

感谢所有贡献者和测试者。特别鸣谢：@huminglong（仓库维护者）提交此次文档优化。

---

## 附：如何把本文作为 Release 描述发布到 GitHub

如果你希望把 `RELEASE_NOTES_v2.md` 的内容发布到 v2 的 GitHub Release 描述中，请按下面任一方式操作：

1) 使用 `gh` CLI（推荐）:

```powershell
# 安装 gh (win): winget install --id GitHub.cli
# 登录一次: gh auth login
# 更新 release 描述为文件文本
gh release edit v2 --notes-file RELEASE_NOTES_v2.md --title "v2" --target main
```

2) 使用 GitHub API（需要一个 `GITHUB_TOKEN`）:

```powershell
$env:GITHUB_TOKEN = 'YOUR_GITHUB_TOKEN_HERE'
$body = @{
  tag_name = 'v2'
  name = 'v2'
  body = Get-Content ./RELEASE_NOTES_v2.md -Raw
  draft = $false
  prerelease = $false
} | ConvertTo-Json -Depth 5
curl -X PATCH -H "Authorization: token $env:GITHUB_TOKEN" -H "Accept: application/vnd.github.v3+json" `
  https://api.github.com/repos/huminglong/09_LiveStreamPullPlayer/releases/tags/v2 -d $body
```

3) 在 GitHub 网站上编辑发布页面：仓库 -> Releases -> 点击 v2 -> Edit -> 粘贴 `RELEASE_NOTES_v2.md` 内容 -> Publish

---

如果你需要我执行创建/更新 release 的 API 请求，请提供一个有限权限的 `GITHUB_TOKEN`（只需 `repo` 权限），或者在本机运行 `gh auth login` 并让我使用 gh（需要你的同意）。