/*
 * VM 操作层
 * 提供高层次的 VM 管理接口
 */

#define _POSIX_C_SOURCE 200809L
#include "../../include/vmanager.h"
#include <strings.h>

// 列出所有 VM
int vm_list(bool verbose) {
    VMInfo *vms = NULL;
    int count = 0;
    
    int ret = api_get_vm_list(&vms, &count);
    if (ret != 0) {
        fprintf(stderr, "错误：无法获取 VM 列表\n");
        return -1;
    }
    
    if (count == 0) {
        printf("没有找到虚拟机\n");
        free(vms);
        return 0;
    }
    
    // 详细模式下获取额外信息
    if (verbose) {
        for (int i = 0; i < count; i++) {
            api_get_vm_config_details(vms[i].vmid, &vms[i]);
            api_get_vm_ip(vms[i].vmid, &vms[i]);
        }
    }
    
    // 打印表头
    printf("\033[1m");  // 粗体
    if (verbose) {
        printf("%-6s %-20s %-10s %-6s %-10s %-12s %-15s %-12s\n",
               "VMID", "NAME", "STATUS", "CPU%", "MEM", "BRIDGE", "IP", "STORAGE");
        printf("────────────────────────────────────────────────────────────────────────────────────────────────────────\n");
    } else {
        printf("%-6s %-20s %-10s %-6s %-10s\n",
               "VMID", "NAME", "STATUS", "CPU%", "MEM");
        printf("────────────────────────────────────────────────────────────\n");
    }
    printf("\033[0m");  // 重置
    
    // 打印每个 VM
    for (int i = 0; i < count; i++) {
        VMInfo *vm = &vms[i];
        
        if (verbose) {
            printf("%-6d %-20s %-10s %5.1f%% %-10s %-12s %-15s %-12s\n",
                   vm->vmid,
                   vm->name,
                   vm->status,
                   vm->cpu_percent,
                   format_bytes(vm->mem),
                   vm->bridge,
                   vm->ip_address,
                   vm->storage);
        } else {
            printf("%-6d %-20s %-10s %5.1f%% %-10s\n",
                   vm->vmid,
                   vm->name,
                   vm->status,
                   vm->cpu_percent,
                   format_bytes(vm->mem));
        }
    }
    
    printf("\n共 %d 个虚拟机\n", count);
    
    free(vms);
    return 0;
}

// 查看 VM 状态
int vm_status(int vmid) {
    VMInfo vm = {0};
    
    int ret = api_get_vm_status(vmid, &vm);
    if (ret != 0) {
        fprintf(stderr, "错误：无法获取 VM %d 的状态\n", vmid);
        return -1;
    }
    
    printf("\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("VM %d 详细信息\n", vmid);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("\033[36m基本信息:\033[0m\n");
    printf("  VMID:       %d\n", vm.vmid);
    printf("  名称:       %s\n", vm.name);
    printf("  状态:       %s\n", vm.status);
    printf("  运行时间:   %s\n", format_uptime(vm.uptime));
    
    printf("\n\033[36m网络信息:\033[0m\n");
    printf("  网桥:       %s\n", vm.bridge);
    printf("  IP 地址:    %s\n", vm.ip_address);
    
    printf("\n\033[36m资源配置:\033[0m\n");
    printf("  CPU:        %d 核\n", vm.cpus);
    printf("  CPU 使用:   %.2f%%\n", vm.cpu_percent);
    printf("  内存:       %s / %s (%.1f%%)\n",
           format_bytes(vm.mem),
           format_bytes(vm.maxmem),
           vm.maxmem > 0 ? (vm.mem * 100.0 / vm.maxmem) : 0);
    printf("  磁盘:       %s / %s\n",
           format_bytes(vm.disk),
           format_bytes(vm.maxdisk));
    
    printf("\n\033[36m存储信息:\033[0m\n");
    printf("  存储位置:   %s\n", vm.storage);
    printf("  配置文件:   %s\n", vm.config_file[0] ? vm.config_file : "N/A");
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    return 0;
}

// 启动 VM
int vm_start(int vmid) {
    printf("正在启动 VM %d...\n", vmid);
    
    int ret = api_vm_action(vmid, "start");
    if (ret != 0) {
        fprintf(stderr, "错误：无法启动 VM %d\n", vmid);
        return -1;
    }
    
    printf("\033[32m✓\033[0m VM %d 启动成功\n", vmid);
    return 0;
}

// 停止 VM
int vm_stop(int vmid) {
    printf("正在停止 VM %d...\n", vmid);
    
    int ret = api_vm_action(vmid, "stop");
    if (ret != 0) {
        fprintf(stderr, "错误：无法停止 VM %d\n", vmid);
        return -1;
    }
    
    printf("\033[32m✓\033[0m VM %d 停止成功\n", vmid);
    return 0;
}

// 重启 VM
int vm_restart(int vmid) {
    printf("正在重启 VM %d...\n", vmid);
    
    // Proxmox API 使用 "reboot" 而不是 "restart"
    int ret = api_vm_action(vmid, "reboot");
    if (ret != 0) {
        fprintf(stderr, "错误：无法重启 VM %d\n", vmid);
        return -1;
    }
    
    printf("\033[32m✓\033[0m VM %d 重启成功\n", vmid);
    return 0;
}

// 暂停 VM
int vm_suspend(int vmid) {
    printf("正在暂停 VM %d...\n", vmid);
    
    int ret = api_vm_action(vmid, "suspend");
    if (ret != 0) {
        fprintf(stderr, "错误：无法暂停 VM %d\n", vmid);
        return -1;
    }
    
    printf("\033[32m✓\033[0m VM %d 暂停成功\n", vmid);
    return 0;
}

// 恢复 VM
int vm_resume(int vmid) {
    printf("正在恢复 VM %d...\n", vmid);
    
    int ret = api_vm_action(vmid, "resume");
    if (ret != 0) {
        fprintf(stderr, "错误：无法恢复 VM %d\n", vmid);
        return -1;
    }
    
    printf("\033[32m✓\033[0m VM %d 恢复成功\n", vmid);
    return 0;
}

// 销毁 VM
int vm_destroy(int vmid, bool force) {
    if (!force) {
        printf("\033[33m警告：此操作将永久删除 VM %d！\033[0m\n", vmid);
        printf("确认继续？(y/n): ");
        
        char confirm[10];
        if (fgets(confirm, sizeof(confirm), stdin)) {
            // 去除换行符
            confirm[strcspn(confirm, "\n")] = '\0';
            
            if (strcasecmp(confirm, "y") != 0 && strcasecmp(confirm, "yes") != 0) {
                printf("操作已取消\n");
                return 0;
            }
        }
    }
    
    printf("正在销毁 VM %d...\n", vmid);
    
    int ret = api_vm_action(vmid, "destroy");
    if (ret != 0) {
        fprintf(stderr, "错误：无法销毁 VM %d\n", vmid);
        return -1;
    }
    
    printf("\033[32m✓\033[0m VM %d 已销毁\n", vmid);
    return 0;
}

// 克隆 VM
int vm_clone(int vmid, int newid, const char *name) {
    printf("正在克隆 VM %d 到 %d...\n", vmid, newid);
    
    // 构建 API 端点
    char endpoint[512];
    if (name) {
        snprintf(endpoint, sizeof(endpoint), 
                "/api2/json/nodes/%s/qemu/%d/clone?newid=%d&name=%s",
                "pve", vmid, newid, name);
    } else {
        snprintf(endpoint, sizeof(endpoint), 
                "/api2/json/nodes/%s/qemu/%d/clone?newid=%d",
                "pve", vmid, newid);
    }
    
    // 调用 API（clone 使用 POST）
    extern cJSON* api_post(const char *endpoint, cJSON *data);
    cJSON *response = api_post(endpoint, NULL);
    
    if (!response) {
        fprintf(stderr, "错误：无法克隆 VM %d\n", vmid);
        return -1;
    }
    
    cJSON_Delete(response);
    printf("\033[32m✓\033[0m VM %d 已克隆到 %d\n", vmid, newid);
    
    return 0;
}
