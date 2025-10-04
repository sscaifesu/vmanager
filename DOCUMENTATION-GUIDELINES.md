# 文档命名规范

**版本**: 1.0.0  
**日期**: 2025-10-05

## 文档分类

本项目的文档分为两类：公开文档和内部文档。

### 公开文档（推送到 GitHub）

公开文档使用标准命名，会被推送到 GitHub 供所有用户查看。

**命名规则**:
- `README.md` - 项目主文档
- `CONTRIBUTING.md` - 贡献指南
- `CHANGELOG.md` - 变更日志
- `LICENSE` - 许可证文件
- `RELEASE-NOTES.md` - 发布说明

**特点**:
- 面向最终用户
- 内容经过审核
- 保持专业严谨
- 无内部信息

### 内部文档（仅本地保存）

内部文档用于团队内部沟通和记录，不会推送到 GitHub。

**命名规则**:

1. **前缀式命名**:
   - `INTERNAL-*.md` - 内部文档
   - `INTERNAL-*.txt` - 内部文本文件
   - 示例: `INTERNAL-cleanup-report.md`

2. **后缀式命名**:
   - `*-INTERNAL.md` - 内部文档
   - `*-INTERNAL.txt` - 内部文本文件
   - 示例: `development-INTERNAL.md`

3. **临时文件**:
   - `TODO.md` - 待办事项
   - `NOTES.md` - 开发笔记
   - `DRAFT-*.md` - 草稿文档
   - `WIP-*.md` - 进行中的文档

**特点**:
- 团队内部使用
- 包含开发细节
- 可以包含敏感信息
- 不对外公开

## .gitignore 配置

内部文档已在 `.gitignore` 中配置，自动忽略：

```gitignore
# 内部文档（不推送到 GitHub）
# 命名规则: INTERNAL-*.md
INTERNAL-*.md
INTERNAL-*.txt
*-INTERNAL.md
*-INTERNAL.txt

# 临时文件和笔记
TODO.md
NOTES.md
DRAFT-*.md
WIP-*.md
```

## 使用示例

### 创建内部文档

```bash
# 清理报告（内部）
INTERNAL-cleanup-report.md

# 开发笔记（内部）
INTERNAL-development-notes.md

# 测试计划（内部）
testing-plan-INTERNAL.md

# 待办事项（临时）
TODO.md
```

### 创建公开文档

```bash
# 项目主文档
README.md

# 版本发布说明
v3/RELEASE-NOTES.md

# 贡献指南
CONTRIBUTING.md

# 变更日志
CHANGELOG.md
```

## 文档审核流程

### 公开文档

1. 撰写文档
2. 团队审核
3. 移除敏感信息
4. 检查专业性
5. 推送到 GitHub

### 内部文档

1. 使用规范命名
2. 自动被 Git 忽略
3. 仅本地保存
4. 团队内部共享（如需要）

## 注意事项

1. **严格遵守命名规则** - 确保内部文档不会被意外推送
2. **定期清理** - 删除过时的内部文档
3. **敏感信息** - 内部文档可以包含敏感信息，但要注意安全
4. **备份** - 重要的内部文档应该备份

## 检查清单

推送到 GitHub 前检查：

- [ ] 所有公开文档已审核
- [ ] 无敏感信息泄露
- [ ] 内部文档已正确命名
- [ ] .gitignore 配置正确
- [ ] Git 状态检查通过

```bash
# 检查是否有内部文档被意外添加
git status | grep -i "internal"
```

## 文档模板

### 内部文档模板

```markdown
# [文档标题]

**类型**: 内部文档  
**日期**: YYYY-MM-DD  
**作者**: [作者名]

## 目的

[说明文档目的]

## 内容

[文档内容]

---

**注意**: 本文档仅供内部使用，不要推送到 GitHub
```

### 公开文档模板

```markdown
# [文档标题]

**版本**: X.Y.Z  
**日期**: YYYY-MM-DD

## 概述

[文档概述]

## 内容

[文档内容]

---

**版本**: X.Y.Z  
**状态**: 发布
```

## 总结

遵循本规范可以确保：
- 内部文档不会被意外公开
- 公开文档保持专业水准
- 项目文档结构清晰
- 团队协作更加高效

---

**版本**: 1.0.0  
**最后更新**: 2025-10-05  
**状态**: 生效
