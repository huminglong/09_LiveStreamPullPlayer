# Release v2 (Detailed Release Notes)

发布版本：v2

发布日期：2025-11-15（UTC）

对应提交：`944e678772becfcd4375e050551dd45447f64a37`（`docs(readme): 使用 Mermaid 重绘架构图并补充说明`）

---

## 概述

v2 主要是对项目文档进行可视化与可读性优化：原先 README 中使用 ASCII 艺术图来描述系统架构。这次将其替换为 Mermaid 流程图并补充渲染提示，提升文档的表达能力与可维护性。

该改动属于文档更新，不会影响运行时功能或 API 行为。

---

## 版本目的与动机

1. 提升架构图可读性与层次性：Mermaid 流程图支持 HTML 兼容换行、样式着色与子图（subgraph）等，使架构层次和流程更加清晰。
2. 保持 README 可维护性：Mermaid 文本更易编辑、扩展（如增加更多组件、连接）并能生成静态图片作为备选。
3. 提供渲染兼容建议：部分平台不支持 Mermaid 渲染，因此在 README 中补充了提示和替代方案，降低跨平台阅读差异。

---

## 主要变更点（详细）

- README.md
  - 将 `### 核心架构` 下的 ASCII 艺术图替换为 Mermaid 流程图。
  - 在 Mermaid 图上方增加一行注释，提示读者：如果无法渲染 Mermaid，可启用渲染器或使用静态图片备选方案。
  - Mermaid 图的节点采用中文与英文标签双语展示，便于跨语言读者理解（例如：`解封装线程 (Demux)`, `数据包队列 (Jitter Buffer)` 等）。
  - 添加了 node 样式（节点背景与边框颜色）以提升可视对比度。

---

## 技术细节（对想了解背景的读者）

- 为什么选择 Mermaid：Mermaid 是 markdown 原生或扩展插件常见支持的可视化语法，易于维护，文本即图，便于代码审阅与版本控制。
- 兼容策略：为确保跨平台可读性，我们遵循如下策略：
  1) 在 README 中保留 Mermaid 源文件（文本）。
  2) 在需要兼容性更好的平台时，可以导出静态 PNG 并在 README 中作为 fallback 图像嵌入（可在未来 PR 中加入）。

---

## 兼容性与影响

- 升级策略：无，因为这只是文档层面更新。
- 运行时影响：无。仓库代码、生成二进制、API 都未修改。
- 回退策略：如需回退，仅回滚 README 的修改或修改 tag 对应描述；不会影响标签本身指向的 commit。

---

## 已知问题与注意事项

- GitHub（或部分 Markdown 渲染器）对 Mermaid 的支持可能需要仓库设置或者插件。若你在查看 README 时无法看到图形，请使用任一方式：
  - 在本地开启支持（例如 VS Code Mermaid 插件）；
  - 将 Mermaid 导出为 PNG 并在 README 中嵌入（未来可作为 PR）；
  - 在 GitHub Release 页面将 Mermaid 内容作为普通文本或导出的图片上传。

---

## 更新 GitHub Release（建议）

1) 本地安装 `gh` 并登录后，直接用命令将 `RELEASE_NOTES_v2.md` 的内容作为 release 描述：

```powershell
gh auth login
gh release create v2 --title "v2" --notes-file RELEASE_NOTES_v2.md --target main
```

2) 如果你希望 PATCH 已有 release（假设你之前已经创建了一个 release 与 tag 对应），使用：

```powershell
gh release edit v2 --notes-file RELEASE_NOTES_v2.md --title "v2" --target main
```

3) 使用 GitHub API（需要 `GITHUB_TOKEN`）：

```powershell
$env:GITHUB_TOKEN = 'YOUR_GITHUB_TOKEN_HERE'
$body = @{
    tag_name = 'v2'
    name = 'v2'
    body = Get-Content ./RELEASE_NOTES_v2.md -Raw
    draft = $false
    prerelease = $false
} | ConvertTo-Json -Depth 5

curl -X POST -H "Authorization: token $env:GITHUB_TOKEN" -H "Accept: application/vnd.github.v3+json" `
  https://api.github.com/repos/huminglong/09_LiveStreamPullPlayer/releases -d $body
```

4) 手动操作：在 GitHub 页面上确认 `v2` tag 是否存在（应已存在），然后仓库 -> Releases -> Draft a new release 或 Edit release -> 将 `RELEASE_NOTES_v2.md` 内容粘贴到描述框并发布。

---

## 致谢

感谢所有贡献者和审阅者。若你希望我代为发布 release 描述，请在会话中提供一个临时的 `GITHUB_TOKEN`（有 `repo` 权限）或运行 `gh auth login` 并允许我使用 `gh` 进行发布。