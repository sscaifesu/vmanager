# vmanager - Proxmox 虚拟机管理工具

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/version-2.0.0-green.svg)](https://github.com/sscaifesu/vmanager)

一个强大的 Proxmox VE 虚拟机管理命令行工具，采用 C 语言开发，提供快速、高效的 VM 操作体验。

## 特性

- **高性能** - C 语言实现，启动速度快，资源占用低
- **标准化** - 符合 Linux 命令行工具标准，支持 `--help`, `--version` 等选项
- **批量操作** - 支持单个、多个和范围 VMID 操作
- **丰富功能** - 启动、停止、重启、暂停、恢复、销毁、状态查询
- **友好界面** - 彩色输出，清晰的操作反馈
- **安全可靠** - 危险操作需要确认，支持强制模式
- **脚本友好** - 静默模式，标准返回值，便于集成

## 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/sscaifesu/vmanager.git
cd vmanager/v2

# 编译
make

# 安装到系统
sudo make install
```

### 基本使用

```bash
# 查看帮助
vmanager --help

# 启动 VM
sudo vmanager start 100

# 停止多个 VM
sudo vmanager stop 100 101 102

# 批量重启
sudo vmanager restart 100-110

# 查看状态
sudo vmanager status 100

# 列出所有 VM
sudo vmanager list
```

## 文档

- [v2 完整文档](v2/README.md) - 最新版本使用指南
- [v1 文档](v1/README.md) - 旧版本参考

## 项目结构

```
vmanager/
├── README.md           # 项目主文档
├── LICENSE             # GPL v3 许可证
├── v1/                 # v1.0 版本（交互式）
│   ├── vm_manager.c
│   ├── Makefile
│   └── README.md
└── v2/                 # v2.0 版本（命令行）
    ├── vmanager.c
    ├── Makefile
    ├── README.md
    └── test.sh
```

## 版本说明

### v2.0 (推荐)
- 标准 Linux 命令行工具
- 支持子命令和选项
- 批量操作和范围支持
- 脚本集成友好

### v1.0
- 交互式界面
- 简单易用
- 适合手动操作

## 许可证

本项目采用 [GNU General Public License v3.0](LICENSE) 许可证。

## 作者

**YXWA Infosec Lab**
- Crystal
- evalEvil

## 链接

- 项目主页: https://github.com/sscaifesu/vmanager
- 问题反馈: https://github.com/sscaifesu/vmanager/issues
- 贡献指南: [CONTRIBUTING.md](CONTRIBUTING.md)

## 致谢

感谢所有为本项目做出贡献的开发者和用户。

---

**版本**: 2.0.0  
**发布日期**: 2025-10-04  
**状态**: 稳定版本
