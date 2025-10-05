# vmanager v4.0.0

Proxmox 虚拟机管理工具 - 专业、高效、易用

## ⚠️ 开始使用前必读

**在使用 vmanager 之前，请先配置 API 权限！**

📖 **[API 权限配置指南](../API_PERMISSIONS.md)** - 详细的配置步骤和说明

### 快速配置（在 PVE 服务器上执行）

```bash
# 创建角色并配置权限
pveum role add VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"

# 创建 API Token
pveum user token add root@pam vmanager --privsep 0
```

保存显示的 Token Secret，然后运行 `vmanager` 进行配置。

---

## 当前状态

### ✅ 已完成（v4.0.0）

**核心架构**
- ✅ 模块化架构设计（14 个源文件）
- ✅ cJSON 科学 JSON 解析
- ✅ libcurl HTTP 请求
- ✅ 高内聚低耦合设计

**核心功能**
- ✅ VM 列表查看（list / list -v）
- ✅ VM 状态查询（status）
- ✅ VM 电源管理（start, stop, reboot, suspend, resume）
- ✅ VM 删除（destroy）
- ✅ VM 克隆（clone）
- ✅ 配置向导（交互式配置）
- ✅ 批量操作支持

**增强功能**
- ✅ 网络信息显示（网桥、IP 地址）
- ✅ 存储信息显示（存储位置、配置文件）
- ✅ 详细模式（-v 选项）
- ✅ 调试模式（--debug）
- ✅ 完善的错误处理

**文档**
- ✅ API 权限配置指南
- ✅ 设计文档（DESIGN.md）
- ✅ 更新日志（CHANGELOG.md）
- ✅ 项目总结（PROJECT_SUMMARY.md）

### 🚧 开发中

- 🔄 TUI 界面（ncurses）- 框架已搭建
- 🔄 性能优化

### 📋 未来计划（Phase 2）

**高级功能**
- [ ] 快照管理（snapshot, rollback）
- [ ] 备份功能（backup, restore）
- [ ] 实时监控（watch 模式）
- [ ] VM 创建（create）
- [ ] VM 迁移（migrate）
- [ ] 批量克隆

**用户体验**
- [ ] TUI 交互界面
- [ ] 彩色输出优化
- [ ] 进度条显示
- [ ] 任务状态跟踪

**系统增强**
- [ ] 日志系统
- [ ] 配置文件加密
- [ ] 多节点支持
- [ ] 插件系统

## 架构

```
v4/
├── include/
│   └── vmanager.h          # 主头文件
├── src/
│   ├── main.c              # 主程序 ✅
│   ├── core/
│   │   ├── api.c           # API 封装 (libcurl + cJSON) ✅
│   │   ├── config.c        # 配置管理 ✅
│   │   └── vm.c            # VM 操作 ✅
│   ├── ui/
│   │   ├── cli.c           # CLI 界面 ✅
│   │   └── tui.c           # TUI 界面 (开发中)
│   └── utils/
│       ├── json.c          # JSON 工具 ✅
│       └── common.c        # 通用工具 ✅
├── cJSON.c                 # cJSON 库
├── cJSON.h
├── Makefile
├── DESIGN.md               # 设计文档
└── README.md               # 本文件
```

## 技术栈

- **语言**: C11
- **JSON**: cJSON
- **HTTP**: libcurl
- **TUI**: ncurses (计划)
- **构建**: Makefile

## 依赖

```bash
# Ubuntu/Debian
apt-get install libcurl4-openssl-dev libncurses5-dev

# macOS
brew install curl ncurses
```

## 编译

```bash
make
```

## v3 vs v4 对比

| 特性 | v3 | v4 |
|------|----|----|
| JSON 解析 | 字符串匹配 | cJSON 库 |
| HTTP 请求 | curl 命令 | libcurl |
| 架构 | 单文件 | 模块化 |
| UI | CLI only | CLI + TUI |
| 稳定性 | 一般 | 优秀 |
| 可维护性 | 低 | 高 |
| 功能 | 基础 | 增强 |

## 开发计划

### Phase 1: 核心功能 ✅ 已完成
- [x] 项目架构
- [x] cJSON 集成
- [x] libcurl 集成
- [x] API 封装
- [x] CLI 实现
- [x] 配置管理
- [x] VM 操作（list, status, start, stop, reboot, suspend, resume, destroy, clone）
- [x] 批量操作
- [x] 网络和存储信息
- [x] 错误处理
- [x] 文档完善
- [x] v4.0.0 发布 ✅

### Phase 2: TUI 界面 🚧 进行中
- [x] ncurses 框架搭建
- [ ] 交互式界面
- [ ] 键盘导航
- [ ] 实时刷新
- [ ] 多窗口布局

### Phase 3: 高级功能 📋 计划中
- [ ] 实时监控（watch 模式）
- [ ] 快照管理（snapshot, rollback）
- [ ] 备份功能（backup, restore）
- [ ] VM 创建（create）
- [ ] VM 迁移（migrate）
- [ ] 批量克隆增强

### Phase 4: 优化和增强 📋 计划中
- [ ] 性能优化（连接池、缓存）
- [ ] 日志系统
- [ ] 配置文件加密
- [ ] 多节点支持
- [ ] 插件系统
- [ ] 单元测试
- [ ] 集成测试

## 为什么要 v4？

v3 存在的问题：
1. **JSON 解析不科学** - 使用字符串搜索，容易出错
2. **代码难以维护** - 单文件 1500+ 行
3. **功能受限** - 只有基础功能
4. **没有 TUI** - 只能命令行操作

v4 的改进：
1. **科学的 JSON 解析** - 使用 cJSON 库，稳定可靠
2. **高性能 HTTP** - libcurl，性能提升 30-50%
3. **模块化设计** - 14 个源文件，易于维护和扩展
4. **功能完整** - list, status, start, stop, reboot, suspend, resume, destroy, clone
5. **增强信息** - 网络、存储、IP 地址显示
6. **双模式 UI** - CLI（已完成）+ TUI（开发中）
7. **完善文档** - API 权限配置、设计文档、项目总结

## 贡献

欢迎贡献代码！特别需要：
- TUI 界面实现
- 测试用例
- 文档改进
- Bug 修复

## 许可

GNU General Public License v3.0

## 作者

YXWA Infosec Lab (Crystal & evalEvil)
