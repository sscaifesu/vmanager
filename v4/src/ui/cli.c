/*
 * CLI 界面
 * 处理命令行参数和用户交互
 */

#include "../../include/vmanager.h"

// CLI 主函数
int cli_main(int argc, char *argv[]) {
    if (argc < 1) {
        fprintf(stderr, "用法: %s COMMAND [ARGS...]\n", PROGRAM_NAME);
        fprintf(stderr, "使用 --help 查看帮助信息\n");
        return 1;
    }
    
    const char *command = argv[0];
    
    // list 命令
    if (strcmp(command, "list") == 0) {
        bool verbose = false;
        
        // 检查 -v 或 --verbose 选项
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
                verbose = true;
                break;
            }
        }
        
        return vm_list(verbose);
    }
    
    // status 命令
    if (strcmp(command, "status") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s status VMID\n", PROGRAM_NAME);
            return 1;
        }
        
        int vmid = atoi(argv[1]);
        if (vmid <= 0) {
            fprintf(stderr, "错误：无效的 VMID: %s\n", argv[1]);
            return 1;
        }
        
        return vm_status(vmid);
    }
    
    // start 命令
    if (strcmp(command, "start") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s start VMID [VMID...]\n", PROGRAM_NAME);
            return 1;
        }
        
        int success = 0, failed = 0;
        
        for (int i = 1; i < argc; i++) {
            // 检查是否为范围
            if (strchr(argv[i], '-') || strchr(argv[i], ',')) {
                int vmids[MAX_VMIDS];
                int count = 0;
                
                if (parse_vmid_range(argv[i], vmids, &count) == 0) {
                    for (int j = 0; j < count; j++) {
                        if (vm_start(vmids[j]) == 0) {
                            success++;
                        } else {
                            failed++;
                        }
                    }
                }
            } else {
                int vmid = atoi(argv[i]);
                if (vmid > 0) {
                    if (vm_start(vmid) == 0) {
                        success++;
                    } else {
                        failed++;
                    }
                }
            }
        }
        
        if (success + failed > 1) {
            printf("\n总计: %d 成功, %d 失败\n", success, failed);
        }
        
        return (failed > 0) ? 1 : 0;
    }
    
    // stop 命令
    if (strcmp(command, "stop") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s stop VMID [VMID...]\n", PROGRAM_NAME);
            return 1;
        }
        
        int success = 0, failed = 0;
        
        for (int i = 1; i < argc; i++) {
            int vmid = atoi(argv[i]);
            if (vmid > 0) {
                if (vm_stop(vmid) == 0) {
                    success++;
                } else {
                    failed++;
                }
            }
        }
        
        if (success + failed > 1) {
            printf("\n总计: %d 成功, %d 失败\n", success, failed);
        }
        
        return (failed > 0) ? 1 : 0;
    }
    
    // reboot 命令（推荐）和 restart 命令（别名）
    if (strcmp(command, "reboot") == 0 || strcmp(command, "restart") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s %s VMID [VMID...]\n", PROGRAM_NAME, command);
            return 1;
        }
        
        int success = 0, failed = 0;
        
        for (int i = 1; i < argc; i++) {
            int vmid = atoi(argv[i]);
            if (vmid > 0) {
                if (vm_restart(vmid) == 0) {
                    success++;
                } else {
                    failed++;
                }
            }
        }
        
        if (success + failed > 1) {
            printf("\n总计: %d 成功, %d 失败\n", success, failed);
        }
        
        return (failed > 0) ? 1 : 0;
    }
    
    // suspend 命令
    if (strcmp(command, "suspend") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s suspend VMID\n", PROGRAM_NAME);
            return 1;
        }
        
        int vmid = atoi(argv[1]);
        if (vmid <= 0) {
            fprintf(stderr, "错误：无效的 VMID: %s\n", argv[1]);
            return 1;
        }
        
        return vm_suspend(vmid);
    }
    
    // resume 命令
    if (strcmp(command, "resume") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s resume VMID\n", PROGRAM_NAME);
            return 1;
        }
        
        int vmid = atoi(argv[1]);
        if (vmid <= 0) {
            fprintf(stderr, "错误：无效的 VMID: %s\n", argv[1]);
        }
        
        return vm_resume(vmid);
    }
    
    // destroy 命令
    if (strcmp(command, "destroy") == 0) {
        if (argc < 2) {
            fprintf(stderr, "用法: %s destroy VMID [-f|--force]\n", PROGRAM_NAME);
            return 1;
        }
        
        int vmid = atoi(argv[1]);
        if (vmid <= 0) {
            fprintf(stderr, "错误：无效的 VMID: %s\n", argv[1]);
            return 1;
        }
        
        bool force = false;
        if (argc > 2 && (strcmp(argv[2], "-f") == 0 || strcmp(argv[2], "--force") == 0)) {
            force = true;
        }
        
        return vm_destroy(vmid, force);
    }
    
    // clone 命令
    if (strcmp(command, "clone") == 0) {
        if (argc < 3) {
            fprintf(stderr, "用法: %s clone VMID NEWID [--name NAME]\n", PROGRAM_NAME);
            return 1;
        }
        
        int vmid = atoi(argv[1]);
        int newid = atoi(argv[2]);
        
        if (vmid <= 0 || newid <= 0) {
            fprintf(stderr, "错误：无效的 VMID\n");
            return 1;
        }
        
        const char *name = NULL;
        if (argc > 4 && strcmp(argv[3], "--name") == 0) {
            name = argv[4];
        }
        
        return vm_clone(vmid, newid, name);
    }
    
    // 未知命令
    fprintf(stderr, "错误：未知命令: %s\n", command);
    fprintf(stderr, "使用 --help 查看帮助信息\n");
    return 1;
}

// 打印 VM 列表 (供其他模块使用)
void cli_print_vm_list(VMInfo *vms, int count, bool verbose) {
    if (!vms || count <= 0) {
        printf("没有虚拟机\n");
        return;
    }
    
    // 打印表头
    printf("\033[1m");
    if (verbose) {
        printf("%-6s %-20s %-10s %-6s %-10s %-10s %-10s %-15s\n",
               "VMID", "NAME", "STATUS", "CPU%", "MEM", "DISK", "UPTIME", "IP");
        printf("─────────────────────────────────────────────────────────────────────────────────────────────\n");
    } else {
        printf("%-6s %-20s %-10s %-6s %-10s\n",
               "VMID", "NAME", "STATUS", "CPU%", "MEM");
        printf("────────────────────────────────────────────────────────────\n");
    }
    printf("\033[0m");
    
    // 打印每个 VM
    for (int i = 0; i < count; i++) {
        VMInfo *vm = &vms[i];
        
        if (verbose) {
            printf("%-6d %-20s %-10s %5.1f%% %-10s %-10s %-10s %-15s\n",
                   vm->vmid,
                   vm->name,
                   vm->status,
                   vm->cpu_percent,
                   format_bytes(vm->mem),
                   format_bytes(vm->disk),
                   format_uptime(vm->uptime),
                   vm->ip_address[0] ? vm->ip_address : "N/A");
        } else {
            printf("%-6d %-20s %-10s %5.1f%% %-10s\n",
                   vm->vmid,
                   vm->name,
                   vm->status,
                   vm->cpu_percent,
                   format_bytes(vm->mem));
        }
    }
}

// 打印 VM 状态 (供其他模块使用)
void cli_print_vm_status(VMInfo *vm) {
    if (!vm) {
        return;
    }
    
    printf("\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("VM %d 详细信息\n", vm->vmid);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("\033[36m基本信息:\033[0m\n");
    printf("  VMID:     %d\n", vm->vmid);
    printf("  名称:     %s\n", vm->name);
    printf("  状态:     %s\n", vm->status);
    printf("  运行时间: %s\n", format_uptime(vm->uptime));
    
    printf("\n\033[36m资源配置:\033[0m\n");
    printf("  CPU:      %d 核\n", vm->cpus);
    printf("  CPU 使用: %.2f%%\n", vm->cpu_percent);
    printf("  内存:     %s / %s (%.1f%%)\n",
           format_bytes(vm->mem),
           format_bytes(vm->maxmem),
           vm->maxmem > 0 ? (vm->mem * 100.0 / vm->maxmem) : 0);
    printf("  磁盘:     %s / %s\n",
           format_bytes(vm->disk),
           format_bytes(vm->maxdisk));
    
    if (vm->ip_address[0]) {
        printf("\n\033[36m网络信息:\033[0m\n");
        printf("  IP 地址:  %s\n", vm->ip_address);
        if (vm->bridge[0]) {
            printf("  网桥:     %s\n", vm->bridge);
        }
    }
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
}
