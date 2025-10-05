# vmanager API 权限配置指南

## 📋 快速开始

在使用 vmanager 之前，您需要在 Proxmox VE 上配置 API Token 和相应的权限。

### 最简单的方法（推荐）

在 PVE 服务器上执行以下命令：

```bash
# 1. 创建角色并配置所有权限
pveum role add VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"

# 2. 创建 API Token（不使用权限分离）
pveum user token add root@pam vmanager --privsep 0

# 3. 分配权限（如果使用权限分离）
pveum acl modify / -token 'root@pam!vmanager' -role VMManager
```

**重要**：保存显示的 Token Secret，这是唯一一次显示！

---

## 📊 权限说明

### 完整权限列表

| 权限 | 必需程度 | 用途 | 相关命令 |
|------|---------|------|----------|
| `VM.Audit` | ✅ **必需** | 查看 VM 信息 | `list`, `status` |
| `VM.PowerMgmt` | ✅ **必需** | 电源管理 | `start`, `stop`, `reboot`, `suspend`, `resume` |
| `VM.Allocate` | ⚠️ 可选 | 删除 VM | `destroy` |
| `VM.Clone` | ⚠️ 可选 | 克隆 VM | `clone` |
| `Datastore.AllocateSpace` | ⚠️ 可选 | 分配存储空间 | `clone` (创建新磁盘) |
| `SDN.Use` | ⚠️ 可选 | 使用 SDN 网络 | `clone` (如果使用 SDN) |

### 权限组合建议

#### 基础权限（只读 + 电源管理）
```bash
VM.Audit,VM.PowerMgmt
```
适用于：查看 VM、启动/停止 VM

#### 标准权限（推荐）
```bash
VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use
```
适用于：所有 vmanager 功能

---

## 🔧 详细配置步骤

### 方法一：Web 界面配置（图形化）

#### 步骤 1：创建角色

1. 登录 Proxmox VE Web 界面
2. 导航到：**Datacenter** → **Permissions** → **Roles**
3. 点击 **Create** 按钮
4. 填写信息：
   - **Name**: `VMManager`
   - **Privileges**: 勾选以下权限
     - ☑ `VM.Audit`
     - ☑ `VM.PowerMgmt`
     - ☑ `VM.Allocate`
     - ☑ `VM.Clone`
     - ☑ `Datastore.AllocateSpace`
     - ☑ `SDN.Use`
5. 点击 **Create**

#### 步骤 2：创建 API Token

1. 导航到：**Datacenter** → **Permissions** → **API Tokens**
2. 点击 **Add** 按钮
3. 填写信息：
   - **User**: `root@pam`
   - **Token ID**: `vmanager`
   - **Privilege Separation**: ☐ **取消勾选**（推荐）
4. 点击 **Add**
5. **重要**：复制并保存显示的 **Secret**（只显示一次！）

#### 步骤 3：分配权限（仅在使用权限分离时需要）

如果您在步骤 2 中勾选了 "Privilege Separation"：

1. 导航到：**Datacenter** → **Permissions**
2. 点击 **Add** → **API Token Permission**
3. 填写信息：
   - **Path**: `/`
   - **API Token**: `root@pam!vmanager`
   - **Role**: `VMManager`
   - **Propagate**: ☑ **勾选**
4. 点击 **Add**

---

### 方法二：命令行配置（快速）

在 PVE 服务器的 SSH 终端执行：

#### 完整配置（一键执行）

```bash
# 创建角色
pveum role add VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"

# 创建 API Token（不使用权限分离）
pveum user token add root@pam vmanager --privsep 0

# 如果使用权限分离，执行以下命令
pveum acl modify / -token 'root@pam!vmanager' -role VMManager
```

#### 基础配置（最小权限）

如果只需要查看和电源管理：

```bash
# 创建基础角色
pveum role add VMManagerBasic -privs "VM.Audit,VM.PowerMgmt"

# 创建 API Token
pveum user token add root@pam vmanager --privsep 0

# 分配权限
pveum acl modify / -token 'root@pam!vmanager' -role VMManagerBasic
```

#### 修改现有角色权限

如果角色已存在，需要添加权限：

```bash
# 修改角色权限
pveum role modify VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"
```

---

## 🧪 验证配置

### 1. 检查角色权限

```bash
pveum role list
```

应该看到 `VMManager` 角色。

### 2. 查看角色详情

```bash
pveum role list VMManager
```

应该显示所有配置的权限。

### 3. 检查 API Token

```bash
pveum user token list root@pam
```

应该看到 `vmanager` token。

### 4. 测试 vmanager

```bash
# 配置 vmanager
vmanager list

# 如果配置正确，应该能看到 VM 列表
```

---

## ❌ 常见错误及解决方案

### 错误 1：Permission check failed

**错误信息**：
```
错误：Permission check failed (/storage/vmdata-1, Datastore.AllocateSpace)
```

**原因**：缺少 `Datastore.AllocateSpace` 权限

**解决方案**：
```bash
pveum role modify VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"
```

---

### 错误 2：SDN.Use 权限

**错误信息**：
```
错误：Permission check failed (/sdn/zones/localnetwork/vmbr0, SDN.Use)
```

**原因**：使用了 SDN 网络但缺少权限

**解决方案**：
```bash
pveum role modify VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"
```

---

### 错误 3：Token already exists

**错误信息**：
```
400 Parameter verification failed.
tokenid: Token already exists.
```

**原因**：Token 已经创建

**解决方案**：
1. 删除旧 Token：
   ```bash
   pveum user token remove root@pam vmanager
   ```
2. 重新创建：
   ```bash
   pveum user token add root@pam vmanager --privsep 0
   ```

或者直接使用现有 Token，只需修改权限即可。

---

### 错误 4：403 Forbidden

**错误信息**：
```
错误：403 Forbidden
```

**可能原因**：
1. Token Secret 不正确
2. Token 已过期
3. 权限配置错误

**解决方案**：
1. 检查 `~/.vmanager.conf` 中的配置
2. 重新生成 Token
3. 确认权限配置正确

---

## 🔐 安全建议

### 1. 使用专用 Token

为 vmanager 创建专用的 API Token，不要与其他应用共享。

### 2. 最小权限原则

只授予需要的权限：
- 只需查看？使用 `VM.Audit`
- 需要管理？添加 `VM.PowerMgmt`
- 需要克隆？添加 `VM.Clone` + `Datastore.AllocateSpace` + `SDN.Use`

### 3. 保护配置文件

```bash
chmod 600 ~/.vmanager.conf
```

### 4. 定期轮换 Token

建议每 3-6 个月更换一次 Token Secret。

### 5. 审计日志

定期检查 PVE 的审计日志：
```bash
journalctl -u pveproxy | grep vmanager
```

---

## 📝 配置文件示例

创建 `~/.vmanager.conf`：

```ini
[proxmox]
host = 10.10.10.4
port = 8006
node = pve
token_id = root@pam!vmanager
token_secret = xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
verify_ssl = false
```

---

## 🆘 获取帮助

### 查看 vmanager 帮助

```bash
vmanager --help
```

### 查看 PVE 权限帮助

```bash
pveum --help
pveum role --help
pveum user token --help
```

### 在线文档

- Proxmox VE API: https://pve.proxmox.com/pve-docs/api-viewer/
- vmanager GitHub: https://github.com/sscaifesu/vmanager

---

## ✅ 配置检查清单

使用此清单确保配置正确：

- [ ] 已创建 `VMManager` 角色
- [ ] 角色包含所需的所有权限
- [ ] 已创建 API Token `root@pam!vmanager`
- [ ] 已保存 Token Secret
- [ ] 已配置 `~/.vmanager.conf`
- [ ] 配置文件权限为 600
- [ ] 测试 `vmanager list` 成功
- [ ] 测试 `vmanager status <VMID>` 成功
- [ ] 测试 `vmanager start <VMID>` 成功（如需要）
- [ ] 测试 `vmanager clone <VMID> <NEWID>` 成功（如需要）

---

**配置完成后，您就可以开始使用 vmanager 了！** 🎉
