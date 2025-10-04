# vmanager v2.0 - Proxmox 虚拟机管理工具

## 简介

vmanager 是一个专为 Proxmox VE 设计的高性能虚拟机管理命令行工具。采用 C 语言开发，提供快速、高效、标准化的 VM 操作体验。

## 特性

- **标准命令行接口** - 支持 `--help`, `--version` 等标准选项
- **子命令系统** - `start`, `stop`, `restart`, `suspend`, `resume`, `destroy`, `status`, `list`
- **批量操作** - 支持多个 VMID 和范围操作
- **管道友好** - 支持 `-q` 静默模式，便于脚本集成
- **强制模式** - `-f` 选项跳过确认
- **交互式模式** - 无参数时进入交互模式
- **完整的帮助系统** - 详细的使用说明和示例

---

## 安装

### 方法 1: 使用 Makefile

```bash
cd vmanager/v2
make
sudo make install
```

### 方法 2: 手动编译

```bash
gcc -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c
sudo install -m 755 vmanager /usr/local/bin/vmanager
```

---

## 使用方法

### 基本语法

```
vmanager [OPTIONS] COMMAND [VMID...]
```

### 命令列表

| 命令 | 说明 |
|------|------|
| `start` | 启动虚拟机 |
| `stop` | 停止虚拟机 |
| `restart` | 重启虚拟机 |
| `suspend` | 暂停虚拟机 |
| `resume` | 恢复虚拟机 |
| `destroy` | 销毁虚拟机（需要确认） |
| `status` | 查看虚拟机状态 |
| `list` | 列出所有虚拟机 |

### 选项

| 选项 | 说明 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `-V, --version` | 显示版本信息 |
| `-f, --force` | 强制执行（跳过确认） |
| `-q, --quiet` | 静默模式（减少输出） |

---

## 使用示例

### 1. 查看帮助和版本

```bash
# 查看帮助
vmanager --help
vmanager -h

# 查看版本
vmanager --version
vmanager -V
```

### 2. 启动/停止 VM

```bash
# 启动单个 VM
sudo vmanager start 100

# 停止单个 VM
sudo vmanager stop 100

# 重启 VM
sudo vmanager restart 100
```

### 3. 批量操作

```bash
# 启动多个 VM
sudo vmanager start 100 101 102

# 停止范围内的 VM
sudo vmanager stop 100-110

# 混合使用
sudo vmanager start 100 105-110 115
```

### 4. 暂停和恢复

```bash
# 暂停 VM
sudo vmanager suspend 100

# 恢复 VM
sudo vmanager resume 100
```

### 5. 销毁 VM

```bash
# 销毁 VM（需要确认）
sudo vmanager destroy 100

# 强制销毁（跳过确认）
sudo vmanager destroy -f 100

# 批量销毁
sudo vmanager destroy -f 100-110
```

### 6. 查看状态

```bash
# 查看单个 VM 状态
sudo vmanager status 100

# 列出所有 VM
sudo vmanager list
```

### 7. 静默模式（用于脚本）

```bash
# 静默启动
sudo vmanager -q start 100

# 检查返回值
if sudo vmanager -q start 100; then
    echo "启动成功"
else
    echo "启动失败"
fi
```

### 8. 交互式模式

```bash
# 无参数进入交互模式
sudo vmanager

# 然后输入命令
> start 100
> stop 101
> list
> exit
```

---

## 高级用法

### 1. 在脚本中使用

```bash
#!/bin/bash
# 批量启动 VM

VMS="100 101 102 103 104"

for vm in $VMS; do
    if sudo vmanager -q start $vm; then
        echo "VM $vm 启动成功"
    else
        echo "VM $vm 启动失败"
    fi
done
```

### 2. 配合其他工具

```bash
# 获取所有运行中的 VM
sudo vmanager list | grep running

# 停止所有运行中的 VM
sudo vmanager list | grep running | awk '{print $1}' | \
    xargs -I {} sudo vmanager stop {}
```

### 3. 定时任务

```bash
# 添加到 crontab
# 每天凌晨 2 点停止 VM 100-110
0 2 * * * /usr/local/bin/vmanager -q stop 100-110

# 每天早上 8 点启动
0 8 * * * /usr/local/bin/vmanager -q start 100-110
```

---

## 性能特点

| 指标 | 数值 |
|------|------|
| 启动时间 | ~1ms |
| 内存占用 | ~0.5MB |
| 文件大小 | ~26KB |
| 编译时间 | <1s |

---

## 设计理念

### 符合 Linux 标准

1. **命令行优先** - 默认使用命令行参数
2. **标准选项** - `-h`, `-V`, `-f`, `-q` 等
3. **返回值** - 成功返回 0，失败返回非 0
4. **静默模式** - 便于脚本集成
5. **帮助系统** - 详细的使用说明

### 用户体验

1. **彩色输出** - 更好的视觉反馈
2. **清晰的错误信息** - 明确的错误提示
3. **操作统计** - 批量操作时显示统计
4. **确认机制** - 危险操作需要确认
5. **交互模式** - 保留传统交互方式

---

## 故障排查

### 问题 1: 权限错误

```bash
# 错误: 此程序需要 root 权限运行
# 解决:
sudo vmanager start 100
```

### 问题 2: 命令未找到

```bash
# 错误: vmanager: command not found
# 解决: 安装到系统
sudo make install

# 或使用完整路径
sudo /usr/local/bin/vmanager start 100
```

### 问题 3: VM 不存在

```bash
# 错误: VM 操作失败
# 检查 VM 是否存在
sudo vmanager list
sudo vmanager status 100
```

---

## 开发文档

### 编译选项

```bash
# 标准编译
gcc -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c

# 调试版本
gcc -g -O0 -std=c11 -o vmanager_debug vmanager.c

# 静态链接
gcc -Wall -Wextra -O2 -std=c11 -static -o vmanager vmanager.c
```

### 代码结构

```c
main()                      // 主函数，解析命令行参数
├── parse options           // 解析选项 (-h, -V, -f, -q)
├── parse command           // 解析命令 (start, stop, etc.)
├── parse VMIDs             // 解析 VMID（单个、多个、范围）
├── execute commands        // 执行命令
└── print statistics        // 显示统计

Helper functions:
├── print_version()         // 显示版本信息
├── print_help()            // 显示帮助信息
├── execute_qm_command()    // 执行 qm 命令
├── parse_vmid_range()      // 解析 VMID 范围
└── run_interactive_mode()  // 交互式模式
```

---

## 未来计划

### v2.1 计划

- [ ] 配置文件支持 (`~/.vmanager.conf`)
- [ ] 日志记录功能
- [ ] 更多的 VM 操作（clone, migrate 等）
- [ ] JSON 输出格式
- [ ] Bash 补全脚本

### v3.0 计划

- [ ] TUI 界面（使用 ncurses）
- [ ] 并行操作支持
- [ ] 远程 Proxmox 节点支持
- [ ] 配置模板系统

---

## 许可证

本项目采用 [GNU General Public License v3.0](../LICENSE) 许可证。

---

## 作者

**YXWA Infosec Lab**
- Crystal
- evalEvil

---

## 链接

- 项目主页: https://github.com/sscaifesu/vmanager
- 问题反馈: https://github.com/sscaifesu/vmanager/issues

---

**版本**: 2.0.0  
**发布日期**: 2025-10-04  
**状态**: 稳定版本
