/*
 * vmanager - Proxmox 虚拟机管理工具
 * 版本：2.0.0
 * 作者：YXWA Infosec Lab (Crystal & evalEvil)
 * 许可：GNU General Public License v3.0
 * 项目：https://github.com/sscaifesu/vmanager
 * 编译：gcc -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c
 * 用法：vmanager [OPTIONS] COMMAND [VMID...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#define VERSION "2.0.0"
#define PROGRAM_NAME "vmanager"
#define MAX_COMMAND 512
#define MAX_VMIDS 100

// 颜色定义
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// 命令类型
typedef enum {
    CMD_NONE,
    CMD_START,
    CMD_STOP,
    CMD_RESTART,
    CMD_SUSPEND,
    CMD_RESUME,
    CMD_DESTROY,
    CMD_STATUS,
    CMD_LIST,
    CMD_INTERACTIVE
} Command;

// 函数声明
void print_version(void);
void print_help(void);
void print_usage(void);
int execute_qm_command(const char *cmd, int vmid);
int execute_qm_command_silent(const char *cmd, int vmid);
int check_vm_exists(int vmid);
int is_number(const char *str);
int parse_vmid_range(const char *range, int *vmids, int *count);
void run_interactive_mode(void);

int main(int argc, char *argv[]) {
    Command cmd = CMD_NONE;
    int vmids[MAX_VMIDS];
    int vmid_count = 0;
    int force = 0;
    int quiet = 0;
    int opt;
    
    // 长选项定义
    static struct option long_options[] = {
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'V'},
        {"force",   no_argument,       0, 'f'},
        {"quiet",   no_argument,       0, 'q'},
        {0, 0, 0, 0}
    };
    
    // 如果没有参数，进入交互模式
    if (argc == 1) {
        run_interactive_mode();
        return 0;
    }
    
    // 解析选项
    while ((opt = getopt_long(argc, argv, "hVfq", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'V':
                print_version();
                return 0;
            case 'f':
                force = 1;
                break;
            case 'q':
                quiet = 1;
                break;
            default:
                print_usage();
                return 1;
        }
    }
    
    // 检查是否以 root 权限运行
    if (geteuid() != 0) {
        fprintf(stderr, COLOR_RED "错误：此程序需要 root 权限运行！\n" COLOR_RESET);
        fprintf(stderr, "请使用：sudo %s\n", PROGRAM_NAME);
        return 1;
    }
    
    // 解析命令
    if (optind < argc) {
        const char *command = argv[optind];
        
        if (strcmp(command, "start") == 0) {
            cmd = CMD_START;
        } else if (strcmp(command, "stop") == 0) {
            cmd = CMD_STOP;
        } else if (strcmp(command, "restart") == 0) {
            cmd = CMD_RESTART;
        } else if (strcmp(command, "suspend") == 0) {
            cmd = CMD_SUSPEND;
        } else if (strcmp(command, "resume") == 0) {
            cmd = CMD_RESUME;
        } else if (strcmp(command, "destroy") == 0) {
            cmd = CMD_DESTROY;
        } else if (strcmp(command, "status") == 0) {
            cmd = CMD_STATUS;
        } else if (strcmp(command, "list") == 0) {
            cmd = CMD_LIST;
        } else {
            fprintf(stderr, COLOR_RED "错误：未知命令 '%s'\n" COLOR_RESET, command);
            print_usage();
            return 1;
        }
        
        optind++;
    } else {
        fprintf(stderr, COLOR_RED "错误：缺少命令\n" COLOR_RESET);
        print_usage();
        return 1;
    }
    
    // 特殊命令：list 不需要 VMID
    if (cmd == CMD_LIST) {
        system("qm list");
        return 0;
    }
    
    // 解析 VMID
    while (optind < argc && vmid_count < MAX_VMIDS) {
        const char *arg = argv[optind];
        
        // 检查是否为范围格式 (如 100-110)
        if (strchr(arg, '-') != NULL) {
            int count = 0;
            if (parse_vmid_range(arg, &vmids[vmid_count], &count) == 0) {
                vmid_count += count;
            } else {
                fprintf(stderr, COLOR_RED "错误：无效的 VMID 范围 '%s'\n" COLOR_RESET, arg);
                return 1;
            }
        } else if (is_number(arg)) {
            vmids[vmid_count++] = atoi(arg);
        } else {
            fprintf(stderr, COLOR_RED "错误：无效的 VMID '%s'\n" COLOR_RESET, arg);
            return 1;
        }
        
        optind++;
    }
    
    // 检查是否有 VMID
    if (vmid_count == 0) {
        fprintf(stderr, COLOR_RED "错误：缺少 VMID\n" COLOR_RESET);
        print_usage();
        return 1;
    }
    
    // 销毁命令需要确认
    if (cmd == CMD_DESTROY && !force) {
        printf(COLOR_YELLOW "警告：销毁操作不可逆！\n" COLOR_RESET);
        printf("将销毁 %d 个 VM: ", vmid_count);
        for (int i = 0; i < vmid_count; i++) {
            printf("%d ", vmids[i]);
        }
        printf("\n");
        printf("确认继续？(yes/no): ");
        
        char confirm[10];
        if (fgets(confirm, sizeof(confirm), stdin) == NULL ||
            strcmp(confirm, "yes\n") != 0) {
            printf(COLOR_YELLOW "操作已取消。\n" COLOR_RESET);
            return 0;
        }
    }
    
    // 执行命令
    int success_count = 0;
    int fail_count = 0;
    
    for (int i = 0; i < vmid_count; i++) {
        int vmid = vmids[i];
        int result = 0;
        
        if (!quiet) {
            switch (cmd) {
                case CMD_START:
                    printf(COLOR_CYAN "启动 VM %d...\n" COLOR_RESET, vmid);
                    break;
                case CMD_STOP:
                    printf(COLOR_CYAN "停止 VM %d...\n" COLOR_RESET, vmid);
                    break;
                case CMD_RESTART:
                    printf(COLOR_CYAN "重启 VM %d...\n" COLOR_RESET, vmid);
                    break;
                case CMD_SUSPEND:
                    printf(COLOR_CYAN "暂停 VM %d...\n" COLOR_RESET, vmid);
                    break;
                case CMD_RESUME:
                    printf(COLOR_CYAN "恢复 VM %d...\n" COLOR_RESET, vmid);
                    break;
                case CMD_DESTROY:
                    printf(COLOR_CYAN "销毁 VM %d...\n" COLOR_RESET, vmid);
                    break;
                case CMD_STATUS:
                    printf(COLOR_CYAN "VM %d 状态:\n" COLOR_RESET, vmid);
                    break;
                default:
                    break;
            }
        }
        
        switch (cmd) {
            case CMD_START:
                result = execute_qm_command_silent("start", vmid);
                break;
            case CMD_STOP:
                result = execute_qm_command_silent("stop", vmid);
                break;
            case CMD_RESTART:
                result = execute_qm_command_silent("stop", vmid);
                if (result == 0) {
                    sleep(2);
                    result = execute_qm_command_silent("start", vmid);
                }
                break;
            case CMD_SUSPEND:
                result = execute_qm_command_silent("suspend", vmid);
                break;
            case CMD_RESUME:
                result = execute_qm_command_silent("resume", vmid);
                break;
            case CMD_DESTROY:
                result = execute_qm_command_silent("stop", vmid);
                if (result == 0 || force) {
                    result = execute_qm_command_silent("destroy", vmid);
                }
                break;
            case CMD_STATUS:
                result = execute_qm_command("status", vmid);
                break;
            default:
                break;
        }
        
        if (result == 0) {
            if (!quiet && cmd != CMD_STATUS) {
                printf(COLOR_GREEN "✓ VM %d 操作成功\n" COLOR_RESET, vmid);
            }
            success_count++;
        } else {
            if (!quiet) {
                printf(COLOR_RED "✗ VM %d 操作失败\n" COLOR_RESET, vmid);
                check_vm_exists(vmid);
            }
            fail_count++;
        }
    }
    
    // 显示统计
    if (!quiet && vmid_count > 1) {
        printf("\n" COLOR_BOLD "操作统计:\n" COLOR_RESET);
        printf(COLOR_GREEN "成功: %d\n" COLOR_RESET, success_count);
        if (fail_count > 0) {
            printf(COLOR_RED "失败: %d\n" COLOR_RESET, fail_count);
        }
    }
    
    return (fail_count > 0) ? 1 : 0;
}

void print_version(void) {
    printf(COLOR_BOLD "%s" COLOR_RESET " version " COLOR_CYAN "%s\n" COLOR_RESET, 
           PROGRAM_NAME, VERSION);
    printf("\n");
    printf("vmanager - Proxmox 虚拟机管理工具\n");
    printf("作者：YXWA Infosec Lab (Crystal & evalEvil)\n");
    printf("许可：GNU General Public License v3.0\n");
    printf("项目：https://github.com/sscaifesu/vmanager\n");
    printf("\n");
    printf("功能：\n");
    printf("  • 启动/停止/重启虚拟机\n");
    printf("  • 暂停/恢复虚拟机\n");
    printf("  • 销毁虚拟机\n");
    printf("  • 查看虚拟机状态\n");
    printf("  • 批量操作支持\n");
    printf("  • 交互式模式\n");
    printf("\n");
    printf("报告 bug: https://github.com/sscaifesu/vmanager/issues\n");
}

void print_help(void) {
    printf(COLOR_BOLD "用法：" COLOR_RESET " %s [OPTIONS] COMMAND [VMID...]\n\n", PROGRAM_NAME);
    
    printf(COLOR_BOLD "命令：\n" COLOR_RESET);
    printf("  " COLOR_CYAN "start" COLOR_RESET "      启动虚拟机\n");
    printf("  " COLOR_CYAN "stop" COLOR_RESET "       停止虚拟机\n");
    printf("  " COLOR_CYAN "restart" COLOR_RESET "    重启虚拟机\n");
    printf("  " COLOR_CYAN "suspend" COLOR_RESET "    暂停虚拟机\n");
    printf("  " COLOR_CYAN "resume" COLOR_RESET "     恢复虚拟机\n");
    printf("  " COLOR_CYAN "destroy" COLOR_RESET "    销毁虚拟机（需要确认）\n");
    printf("  " COLOR_CYAN "status" COLOR_RESET "     查看虚拟机状态\n");
    printf("  " COLOR_CYAN "list" COLOR_RESET "       列出所有虚拟机\n");
    printf("\n");
    
    printf(COLOR_BOLD "选项：\n" COLOR_RESET);
    printf("  " COLOR_CYAN "-h, --help" COLOR_RESET "     显示此帮助信息\n");
    printf("  " COLOR_CYAN "-V, --version" COLOR_RESET "  显示版本信息\n");
    printf("  " COLOR_CYAN "-f, --force" COLOR_RESET "    强制执行（跳过确认）\n");
    printf("  " COLOR_CYAN "-q, --quiet" COLOR_RESET "    静默模式（减少输出）\n");
    printf("\n");
    
    printf(COLOR_BOLD "VMID 格式：\n" COLOR_RESET);
    printf("  单个:     100\n");
    printf("  多个:     100 101 102\n");
    printf("  范围:     100-110\n");
    printf("  混合:     100 105-110 115\n");
    printf("\n");
    
    printf(COLOR_BOLD "示例：\n" COLOR_RESET);
    printf("  # 启动单个 VM\n");
    printf("  $ sudo %s start 100\n\n", PROGRAM_NAME);
    
    printf("  # 停止多个 VM\n");
    printf("  $ sudo %s stop 100 101 102\n\n", PROGRAM_NAME);
    
    printf("  # 重启范围内的 VM\n");
    printf("  $ sudo %s restart 100-110\n\n", PROGRAM_NAME);
    
    printf("  # 销毁 VM（需要确认）\n");
    printf("  $ sudo %s destroy 100\n\n", PROGRAM_NAME);
    
    printf("  # 强制销毁 VM（跳过确认）\n");
    printf("  $ sudo %s destroy -f 100-110\n\n", PROGRAM_NAME);
    
    printf("  # 查看 VM 状态\n");
    printf("  $ sudo %s status 100\n\n", PROGRAM_NAME);
    
    printf("  # 列出所有 VM\n");
    printf("  $ sudo %s list\n\n", PROGRAM_NAME);
    
    printf("  # 交互式模式（无参数）\n");
    printf("  $ sudo %s\n\n", PROGRAM_NAME);
    
    printf("更多信息请访问：https://github.com/sscaifesu/vmanager\n");
}

void print_usage(void) {
    fprintf(stderr, "用法：%s [OPTIONS] COMMAND [VMID...]\n", PROGRAM_NAME);
    fprintf(stderr, "尝试 '%s --help' 获取更多信息。\n", PROGRAM_NAME);
}

int execute_qm_command(const char *cmd, int vmid) {
    char command[MAX_COMMAND];
    snprintf(command, sizeof(command), "qm %s %d", cmd, vmid);
    return system(command);
}

int execute_qm_command_silent(const char *cmd, int vmid) {
    char command[MAX_COMMAND];
    snprintf(command, sizeof(command), "qm %s %d >/dev/null 2>&1", cmd, vmid);
    return system(command);
}

int check_vm_exists(int vmid) {
    char command[MAX_COMMAND];
    snprintf(command, sizeof(command), "qm list | grep -q '^ *%d '", vmid);
    
    if (system(command) != 0) {
        printf(COLOR_YELLOW "  提示：VM %d 可能不存在\n" COLOR_RESET, vmid);
        return 0;
    }
    return 1;
}

int is_number(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    
    return 1;
}

int parse_vmid_range(const char *range, int *vmids, int *count) {
    char *dash = strchr(range, '-');
    if (dash == NULL) {
        return -1;
    }
    
    char start_str[32], end_str[32];
    size_t start_len = dash - range;
    
    if (start_len >= sizeof(start_str)) {
        return -1;
    }
    
    strncpy(start_str, range, start_len);
    start_str[start_len] = '\0';
    strcpy(end_str, dash + 1);
    
    if (!is_number(start_str) || !is_number(end_str)) {
        return -1;
    }
    
    int start = atoi(start_str);
    int end = atoi(end_str);
    
    if (start > end || end - start >= MAX_VMIDS) {
        return -1;
    }
    
    *count = 0;
    for (int i = start; i <= end; i++) {
        vmids[(*count)++] = i;
    }
    
    return 0;
}

void run_interactive_mode(void) {
    // 这里保留原来的交互式模式代码
    printf(COLOR_BLUE "╔════════════════════════════════════════════════════╗\n" COLOR_RESET);
    printf(COLOR_BLUE "║                  " COLOR_CYAN "VM Manager v%s" COLOR_BLUE "                 ║\n" COLOR_RESET, VERSION);
    printf(COLOR_BLUE "║                                                    ║\n" COLOR_RESET);
    printf(COLOR_BLUE "║             " COLOR_RESET "Design by YXWA Infosec Lab" COLOR_BLUE "             ║\n" COLOR_RESET);
    printf(COLOR_BLUE "╚════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    
    printf(COLOR_YELLOW "提示：交互式模式已启动\n" COLOR_RESET);
    printf("使用命令行模式可获得更好的体验，例如：\n");
    printf("  sudo %s start 100\n", PROGRAM_NAME);
    printf("  sudo %s --help\n\n", PROGRAM_NAME);
    
    printf("按 Ctrl+C 退出，或输入命令：\n");
    printf("1. start <VMID>   - 启动 VM\n");
    printf("2. stop <VMID>    - 停止 VM\n");
    printf("3. list           - 列出所有 VM\n");
    printf("4. exit           - 退出\n");
    printf("\n");
    
    char input[256];
    while (1) {
        printf(COLOR_CYAN "> " COLOR_RESET);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 去除换行符
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        
        if (strcmp(input, "list") == 0) {
            system("qm list");
            continue;
        }
        
        // 简单的命令解析
        char *cmd = strtok(input, " ");
        char *vmid_str = strtok(NULL, " ");
        
        if (cmd == NULL) {
            continue;
        }
        
        if (vmid_str == NULL) {
            printf(COLOR_RED "错误：缺少 VMID\n" COLOR_RESET);
            continue;
        }
        
        if (!is_number(vmid_str)) {
            printf(COLOR_RED "错误：无效的 VMID\n" COLOR_RESET);
            continue;
        }
        
        int vmid = atoi(vmid_str);
        
        if (strcmp(cmd, "start") == 0) {
            printf("启动 VM %d...\n", vmid);
            execute_qm_command("start", vmid);
        } else if (strcmp(cmd, "stop") == 0) {
            printf("停止 VM %d...\n", vmid);
            execute_qm_command("stop", vmid);
        } else {
            printf(COLOR_RED "错误：未知命令 '%s'\n" COLOR_RESET, cmd);
        }
    }
    
    printf("\n再见！\n");
}
