# vmanager v1.0 - 交互式版本

## 简介

这是 vmanager 的 v1.0 版本，采用交互式界面，适合手动操作。

**注意**: 推荐使用 [v2.0](../v2/README.md) 版本，提供更好的命令行体验。

## 特性

- 交互式菜单
- 单个/范围 VMID 操作
- 停止/销毁 VM
- 彩色输出
- 批量操作支持

## 编译

```bash
cd vmanager/v1
gcc -Wall -Wextra -O2 -std=c11 -o vm_manager vm_manager.c
```

## 使用

```bash
sudo ./vm_manager
```

然后按照提示操作：
1. 选择操作类型（停止/销毁）
2. 选择输入方式（单个/范围）
3. 输入 VMID
4. 确认操作

## 示例

```
╔════════════════════════════════════════════════════╗
║                  VM Manager v1.0                   ║
║                                                    ║
║             Design by YXWA Infosec Lab             ║
╚════════════════════════════════════════════════════╝

请选择操作：
  1. 仅停止 VM (qm stop)
  2. 停止并销毁 VM (qm stop && qm destroy --purge)
输入选项 (1 或 2): 1

请选择输入方式：
  1. 单个 VMID (例如: 112)
  2. 范围 VMID (例如: 111-113)
输入选项 (1 或 2): 1
请输入单个 VMID: 100
正在停止 VM 100...
VM 100 已停止成功。

操作完成！
```

## 迁移到 v2.0

推荐迁移到 v2.0 版本以获得更好的体验：

```bash
cd ../v2
gcc -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c
sudo ./vmanager start 100
```

查看 [v2.0 文档](../v2/README.md) 了解更多。

## 许可证

GNU General Public License v3.0

## 作者

YXWA Infosec Lab (Crystal & evalEvil)

## 链接

- 项目主页: https://github.com/sscaifesu/vmanager
- v2.0 文档: [../v2/README.md](../v2/README.md)
