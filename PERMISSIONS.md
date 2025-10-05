# vmanager 权限要求

## API Token 权限配置

vmanager 使用 Proxmox API Token 进行认证。不同的操作需要不同的权限。

### 基本权限（必需）

创建 API Token 时，需要以下基本权限：

```
VM.Audit       - 查看 VM 信息
VM.PowerMgmt   - 启动、停止、重启 VM
```

### 完整权限（推荐）

为了使用所有功能，建议配置以下权限：

| 权限 | 用途 | 命令 |
|------|------|------|
| `VM.Audit` | 查看 VM 信息 | list, status |
| `VM.PowerMgmt` | 电源管理 | start, stop, reboot, suspend, resume |
| `VM.Allocate` | 删除 VM | destroy |
| `VM.Clone` | 克隆 VM | clone |
| `Datastore.AllocateSpace` | 分配存储空间 | clone, create |
| `SDN.Use` | 使用 SDN 网络 | clone (如果使用 SDN) |

### 配置步骤

#### 1. 创建角色

在 PVE Web 界面：

1. 进入 **Datacenter** → **Permissions** → **Roles**
2. 点击 **Create**
3. 名称：`VMManager`
4. 勾选权限：
   - `VM.Audit`
   - `VM.PowerMgmt`
   - `VM.Allocate`
   - `VM.Clone`
   - `Datastore.AllocateSpace`
   - `SDN.Use` (如果使用 SDN 网络)

#### 2. 创建 API Token

1. 进入 **Datacenter** → **Permissions** → **API Tokens**
2. 点击 **Add**
3. 用户：`root@pam`
4. Token ID：`vmanager`
5. 取消勾选 **Privilege Separation**（使用用户权限）
6. 保存 Token Secret

#### 3. 分配权限

如果使用了 Privilege Separation：

1. 进入 **Datacenter** → **Permissions**
2. 点击 **Add** → **API Token Permission**
3. 路径：`/`
4. API Token：`root@pam!vmanager`
5. 角色：`VMManager`
6. 勾选 **Propagate**

### 快速配置（命令行）

```bash
# 创建角色
pveum role add VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"

# 创建 API Token（不使用 Privilege Separation）
pveum user token add root@pam vmanager --privsep 0

# 如果使用 Privilege Separation，需要分配权限
pveum acl modify / -token 'root@pam!vmanager' -role VMManager

# 如果 Token 已存在，修改角色权限
pveum role modify VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"
```

### 权限测试

测试各个功能是否有权限：

```bash
# 基本查看（需要 VM.Audit）
vmanager list
vmanager status 100

# 电源管理（需要 VM.PowerMgmt）
vmanager start 100
vmanager stop 100
vmanager reboot 100

# 删除（需要 VM.Allocate）
vmanager destroy 100

# 克隆（需要 VM.Clone + Datastore.AllocateSpace）
vmanager clone 100 101
```

### 常见错误

#### Permission check failed

```
错误：Permission check failed (/storage/vmdata-1, Datastore.AllocateSpace)
错误：Permission check failed (/sdn/zones/localnetwork/vmbr0, SDN.Use)
```

**解决方案**：
- 添加相应的权限（Datastore.AllocateSpace, SDN.Use 等）
- 或者取消 API Token 的 Privilege Separation
- 使用命令修改角色权限：
  ```bash
  pveum role modify VMManager -privs "VM.Audit,VM.PowerMgmt,VM.Allocate,VM.Clone,Datastore.AllocateSpace,SDN.Use"
  ```

#### 403 Forbidden

```
错误：403 Forbidden
```

**解决方案**：
- 检查 API Token 是否正确
- 检查权限配置
- 确保 Token 没有过期

### 最小权限原则

如果只需要基本功能，可以只配置：

```
VM.Audit       - 查看
VM.PowerMgmt   - 启动/停止
```

如果需要完整功能，建议配置所有权限或使用 `Administrator` 角色。

### 安全建议

1. **使用专用 Token** - 为 vmanager 创建专用的 API Token
2. **最小权限** - 只授予需要的权限
3. **定期轮换** - 定期更换 Token Secret
4. **安全存储** - 将配置文件权限设为 600

```bash
chmod 600 ~/.vmanager.conf
```

