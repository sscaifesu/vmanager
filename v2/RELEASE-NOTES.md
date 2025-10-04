# vmanager v2.0.0 Release Notes

## 发布日期

2025-10-04

## 简介

vmanager v2.0.0 是一个专为 Proxmox VE 设计的高性能虚拟机管理命令行工具，采用 C 语言开发，提供快速、高效、标准化的 VM 操作体验。

## 主要特性

### 核心功能
- **标准命令行工具** - 符合 Linux 命令行工具标准，支持 `--help`, `--version` 等选项
- **丰富的子命令** - 支持 start, stop, restart, suspend, resume, destroy, status, list
- **批量操作** - 支持单个、多个和范围 VMID 操作 (例如: 100-110)
- **高级选项** - 静默模式 (-q)、强制模式 (-f)
- **交互式模式** - 无参数时自动进入交互模式
- **完整的帮助系统** - 详细的使用说明和示例

### 性能特点
- **启动时间**: ~1ms
- **内存占用**: ~0.5MB
- **文件大小**: 26KB (Linux AMD64)
- **编译时间**: <1s

## 支持平台

本次 Release 提供以下平台的预编译二进制文件：

| 平台 | 文件名 | 大小 | 说明 |
|------|--------|------|------|
| Linux x86_64 | vmanager-linux-amd64 | 26KB | Intel/AMD 64位处理器 |
| Linux ARM64 | vmanager-linux-arm64 | 764KB | ARMv8 64位处理器 (静态链接) |
| Linux ARM | vmanager-linux-arm | 485KB | ARMv7 32位处理器 (静态链接) |

**注意**: macOS 版本需要在 macOS 系统上本地编译。编译命令：
```bash
clang -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c
```

## 安装方法

### 方法 1: 下载预编译二进制文件

1. 从 [Releases](https://github.com/sscaifesu/vmanager/releases) 页面下载对应平台的文件
2. 添加执行权限：
   ```bash
   chmod +x vmanager-linux-amd64
   ```
3. 移动到系统路径：
   ```bash
   sudo mv vmanager-linux-amd64 /usr/local/bin/vmanager
   ```
4. 验证安装：
   ```bash
   vmanager --version
   ```

### 方法 2: 从源代码编译

```bash
# 克隆仓库
git clone https://github.com/sscaifesu/vmanager.git
cd vmanager/v2

# 编译
gcc -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c

# 安装
sudo install -m 755 vmanager /usr/local/bin/vmanager
```

## 使用示例

### 基本操作

```bash
# 查看帮助
vmanager --help

# 查看版本
vmanager --version

# 启动 VM
sudo vmanager start 100

# 停止 VM
sudo vmanager stop 100

# 重启 VM
sudo vmanager restart 100
```

### 批量操作

```bash
# 启动多个 VM
sudo vmanager start 100 101 102

# 停止范围内的 VM
sudo vmanager stop 100-110

# 混合使用
sudo vmanager start 100 105-110 115
```

### 高级功能

```bash
# 暂停 VM
sudo vmanager suspend 100

# 恢复 VM
sudo vmanager resume 100

# 销毁 VM (需要确认)
sudo vmanager destroy 100

# 强制销毁 (跳过确认)
sudo vmanager destroy -f 100

# 查看 VM 状态
sudo vmanager status 100

# 列出所有 VM
sudo vmanager list
```

### 脚本集成

```bash
# 静默模式 (用于脚本)
sudo vmanager -q start 100

# 检查返回值
if sudo vmanager -q start 100; then
    echo "启动成功"
else
    echo "启动失败"
fi
```

## 文件校验

下载文件后，建议验证文件完整性：

```bash
# 下载 SHA256SUMS 文件
# 验证校验和
sha256sum -c SHA256SUMS
```

## 版本对比

### v2.0 vs v1.0

| 特性 | v1.0 | v2.0 |
|------|------|------|
| 交互式模式 | ✓ | ✓ |
| 命令行模式 | ✗ | ✓ |
| 标准选项 | ✗ | ✓ |
| 批量操作 | ✓ | ✓ |
| 范围支持 | ✓ | ✓ |
| 暂停/恢复 | ✗ | ✓ |
| 静默模式 | ✗ | ✓ |
| 强制模式 | ✗ | ✓ |
| 脚本集成 | ⚠️ | ✓ |

## 已知问题

- macOS 版本需要本地编译（暂无预编译二进制）
- 需要 root 权限运行（Proxmox qm 命令要求）

## 未来计划

### v2.1
- [ ] 配置文件支持 (`~/.vmanager.conf`)
- [ ] 日志记录功能
- [ ] 更多的 VM 操作（clone, migrate 等）
- [ ] JSON 输出格式
- [ ] Bash 补全脚本

### v3.0
- [ ] TUI 界面（使用 ncurses）
- [ ] 并行操作支持
- [ ] 远程 Proxmox 节点支持
- [ ] 配置模板系统

## 贡献

欢迎提交 Issue 和 Pull Request！

- 项目主页: https://github.com/sscaifesu/vmanager
- 问题反馈: https://github.com/sscaifesu/vmanager/issues

## 许可证

本项目采用 [GNU General Public License v3.0](https://github.com/sscaifesu/vmanager/blob/main/LICENSE) 许可证。

## 作者

**YXWA Infosec Lab**
- Crystal
- evalEvil

## 致谢

感谢所有为本项目做出贡献的开发者和用户。

---

**完整文档**: https://github.com/sscaifesu/vmanager/blob/main/v2/README.md  
**下载地址**: https://github.com/sscaifesu/vmanager/releases/tag/v2.0.0
