# vmanager v4.0.0 设计文档

## 核心特性

### 1. 双模式界面
- **TUI 模式** (默认): 使用 ncurses 的交互式界面
- **CLI 模式**: 传统命令行模式（兼容 v3）

### 2. 科学的 JSON 解析
- 使用 cJSON 库
- 稳定、可靠、易维护

### 3. 增强功能
- 实时监控 VM 状态
- 批量操作支持
- 配置文件管理
- 日志记录
- 错误处理和重试机制

## 架构设计

```
vmanager v4
├── core/           # 核心功能
│   ├── api.c       # Proxmox API 封装
│   ├── config.c    # 配置管理
│   └── vm.c        # VM 操作
├── ui/             # 用户界面
│   ├── tui.c       # TUI 界面 (ncurses)
│   └── cli.c       # CLI 界面
├── utils/          # 工具函数
│   ├── json.c      # JSON 辅助函数
│   └── logger.c    # 日志系统
└── main.c          # 主程序
```

## TUI 界面设计

```
┌─────────────────────────────────────────────────────────────────┐
│ vmanager v4.0.0 - Proxmox VM Manager          [F1] Help [Q] Quit│
├─────────────────────────────────────────────────────────────────┤
│ VMID  Name              Status    CPU%  Mem(GB)  Disk(GB)  IP   │
├─────────────────────────────────────────────────────────────────┤
│ 111   srv-web-25c201    running   2.1   4.0      2.2       10..│
│ 112   srv-web-25c202    running   1.8   4.0      2.2       10..│
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│ [S]tart [T]op [R]estart [P]ause [D]estroy [Enter]Details       │
└─────────────────────────────────────────────────────────────────┘
```

## 功能列表

### 基础功能 (v3 兼容)
- ✅ list - 列出所有 VM
- ✅ status - 查看 VM 状态
- ✅ start/stop/restart - 控制 VM
- ✅ suspend/resume - 暂停/恢复
- ✅ destroy - 删除 VM

### 新增功能
- 🆕 monitor - 实时监控模式
- 🆕 clone - 克隆 VM
- 🆕 snapshot - 快照管理
- 🆕 backup - 备份管理
- 🆕 migrate - 迁移 VM
- 🆕 resize - 调整资源
- 🆕 console - 连接控制台

## 技术栈

- **语言**: C11
- **JSON**: cJSON
- **TUI**: ncurses
- **构建**: Makefile
- **平台**: Linux (macOS, PVE)

## 开发阶段

### Phase 1: 核心框架 (当前)
- cJSON 集成
- API 封装
- 基础 CLI 功能

### Phase 2: TUI 界面
- ncurses 集成
- 交互式界面
- 键盘导航

### Phase 3: 高级功能
- 实时监控
- 快照管理
- 备份功能

### Phase 4: 优化和测试
- 性能优化
- 错误处理
- 完整测试

## 配置文件格式

```ini
[server]
host = 10.10.10.4
port = 8006
node = pve
verify_ssl = false

[auth]
token_id = root@pam!vmanager
token_secret = xxx

[ui]
default_mode = tui
refresh_interval = 5
color_scheme = default

[logging]
enabled = true
level = info
file = ~/.vmanager.log
```

## 命令行参数

```bash
vmanager [OPTIONS] [COMMAND] [ARGS...]

OPTIONS:
  --tui              启动 TUI 模式 (默认)
  --cli              使用 CLI 模式
  --config FILE      指定配置文件
  --mode MODE        强制模式 (local/remote)
  -v, --verbose      详细输出
  -q, --quiet        静默模式
  -h, --help         帮助信息
  --version          版本信息

COMMANDS:
  list               列出所有 VM
  status VMID        查看 VM 状态
  start VMID         启动 VM
  stop VMID          停止 VM
  restart VMID       重启 VM
  monitor            启动监控模式
  clone VMID NEWID   克隆 VM
  snapshot           快照管理
```

## 依赖

```bash
# Ubuntu/Debian
apt-get install libncurses5-dev

# macOS
brew install ncurses

# 编译
make
make install
```
