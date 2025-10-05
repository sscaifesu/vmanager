# vmanager v4.0.0 (开发中)

## 当前状态

### ✅ 已完成
- 项目架构设计
- 模块化目录结构
- cJSON 集成
- 核心头文件 (`include/vmanager.h`)
- JSON 工具函数 (`src/utils/json.c`)
- API 封装 (`src/core/api.c`) - 使用 libcurl
- 主程序框架 (`src/main.c`)

### 🚧 进行中
- CLI 界面实现
- 配置管理
- VM 操作函数

### 📋 待实现
- TUI 界面 (ncurses)
- 日志系统
- 错误处理
- 实时监控
- 快照管理
- 备份功能
- 完整测试

## 架构

```
v4/
├── include/
│   └── vmanager.h          # 主头文件
├── src/
│   ├── main.c              # 主程序
│   ├── core/
│   │   ├── api.c           # API 封装 (libcurl + cJSON)
│   │   ├── config.c        # 配置管理 (待实现)
│   │   └── vm.c            # VM 操作 (待实现)
│   ├── ui/
│   │   ├── cli.c           # CLI 界面 (待实现)
│   │   └── tui.c           # TUI 界面 (待实现)
│   └── utils/
│       ├── json.c          # JSON 工具
│       ├── logger.c        # 日志系统 (待实现)
│       └── common.c        # 通用工具 (待实现)
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

### Phase 1: 核心功能 (当前)
- [x] 项目架构
- [x] cJSON 集成
- [x] API 封装
- [ ] CLI 实现
- [ ] 配置管理
- [ ] VM 操作

### Phase 2: TUI 界面
- [ ] ncurses 集成
- [ ] 交互式界面
- [ ] 键盘导航
- [ ] 实时刷新

### Phase 3: 高级功能
- [ ] 实时监控
- [ ] 快照管理
- [ ] 备份功能
- [ ] 克隆 VM
- [ ] 迁移 VM

### Phase 4: 优化和发布
- [ ] 性能优化
- [ ] 错误处理
- [ ] 完整测试
- [ ] 文档完善
- [ ] 发布 v4.0.0

## 为什么要 v4？

v3 存在的问题：
1. **JSON 解析不科学** - 使用字符串搜索，容易出错
2. **代码难以维护** - 单文件 1500+ 行
3. **功能受限** - 只有基础功能
4. **没有 TUI** - 只能命令行操作

v4 的改进：
1. **科学的 JSON 解析** - 使用 cJSON 库
2. **模块化设计** - 易于维护和扩展
3. **功能丰富** - 监控、快照、备份等
4. **双模式 UI** - CLI + TUI

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
