/*
 * vmanager - Proxmox 虚拟机管理工具
 * 版本：3.0.0
 * 作者：YXWA Infosec Lab (Crystal & evalEvil)
 * 许可：GNU General Public License v3.0
 * 项目：https://github.com/sscaifesu/vmanager
 * 
 * 特性：
 * - 智能检测运行环境（本地/远程）
 * - 本地模式：直接使用 qm 命令（PVE 服务器上）
 * - 远程模式：使用 Proxmox API（任何平台）
 * - 自动切换，无缝体验
 * 
 * 编译：gcc -Wall -Wextra -O2 -std=c11 -o vmanager vmanager.c
 * 用法：vmanager [OPTIONS] COMMAND [VMID...]
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#define VERSION "3.0.0"
#define PROGRAM_NAME "vmanager"
#define MAX_COMMAND 2048
#define MAX_VMIDS 100
#define MAX_LINE 512
#define CONFIG_FILE ".vmanager.conf"

// 颜色定义
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// 执行模式
typedef enum {
    MODE_AUTO,      // 自动检测
    MODE_LOCAL,     // 本地模式（使用 qm 命令）
    MODE_REMOTE     // 远程模式（使用 API）
} ExecutionMode;

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
    CMD_CONFIG
} Command;

// 配置结构
typedef struct {
    char host[256];
    char user[64];
    char password[256];
    char token_id[128];
    char token_secret[256];
    char node[64];
    int port;
    int verify_ssl;
} Config;

// 函数声明
void print_version(void);
void print_help(void);
void print_usage(void);

// 环境检测
ExecutionMode detect_mode(void);
int is_pve_environment(void);
char* get_config_path(void);

// 配置管理
int load_config(Config *config);
int save_config(Config *config);
void config_wizard(Config *config);

// 本地模式函数
int execute_qm_command(const char *cmd, int vmid);
int execute_qm_command_silent(const char *cmd, int vmid);
int list_vms_local(void);
int check_vm_exists(int vmid);

// 远程模式函数
int execute_api_command(Config *config, const char *cmd, int vmid);
int list_vms_remote(Config *config, int verbose);

// 通用函数
int is_number(const char *str);
int parse_vmid_range(const char *range, int *vmids, int *count);

int main(int argc, char *argv[]) {
    Command cmd = CMD_NONE;
    Config config = {0};
    ExecutionMode mode = MODE_AUTO;
    int vmids[MAX_VMIDS];
    int vmid_count = 0;
    int force = 0;
    int quiet = 0;
    int verbose = 0;
    int opt;
    
    // 长选项定义
    static struct option long_options[] = {
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'V'},
        {"force",   no_argument,       0, 'f'},
        {"quiet",   no_argument,       0, 'q'},
        {"verbose", no_argument,       0, 'v'},
        {"config",  no_argument,       0, 'c'},
        {"mode",    required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    // 解析选项
    while ((opt = getopt_long(argc, argv, "hVfqvcm:", long_options, NULL)) != -1) {
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
            case 'v':
                verbose = 1;
                break;
            case 'c':
                cmd = CMD_CONFIG;
                break;
            case 'm':
                if (strcmp(optarg, "local") == 0) {
                    mode = MODE_LOCAL;
                } else if (strcmp(optarg, "remote") == 0) {
                    mode = MODE_REMOTE;
                } else if (strcmp(optarg, "auto") == 0) {
                    mode = MODE_AUTO;
                } else {
                    fprintf(stderr, COLOR_RED "错误：无效的模式 '%s'\n" COLOR_RESET, optarg);
                    fprintf(stderr, "有效模式: auto, local, remote\n");
                    return 1;
                }
                break;
            default:
                print_usage();
                return 1;
        }
    }
    
    // 配置向导
    if (cmd == CMD_CONFIG) {
        config_wizard(&config);
        return 0;
    }
    
    // 自动检测模式
    if (mode == MODE_AUTO) {
        mode = detect_mode();
    }
    
    // 显示当前模式（调试）
    if (!quiet) {
        if (mode == MODE_LOCAL) {
            // 本地模式不显示提示
        } else if (mode == MODE_REMOTE) {
            // 远程模式加载配置
            if (load_config(&config) != 0) {
                fprintf(stderr, COLOR_RED "错误：未找到配置文件\n" COLOR_RESET);
                fprintf(stderr, "请先运行配置向导：%s --config\n", PROGRAM_NAME);
                return 1;
            }
        }
    }
    
    // 检查 root 权限（仅本地模式需要）
    if (mode == MODE_LOCAL && geteuid() != 0) {
        fprintf(stderr, COLOR_RED "错误：本地模式需要 root 权限运行！\n" COLOR_RESET);
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
        if (mode == MODE_LOCAL) {
            return list_vms_local();
        } else {
            return list_vms_remote(&config, verbose);
        }
    }
    
    // 解析 VMID
    while (optind < argc && vmid_count < MAX_VMIDS) {
        const char *arg = argv[optind];
        
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
        printf("\n确认继续？(yes/no): ");
        
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
        const char *cmd_str = NULL;
        
        switch (cmd) {
            case CMD_START:
                cmd_str = "start";
                break;
            case CMD_STOP:
                cmd_str = "stop";
                break;
            case CMD_RESTART:
                cmd_str = "restart";
                break;
            case CMD_SUSPEND:
                cmd_str = "suspend";
                break;
            case CMD_RESUME:
                cmd_str = "resume";
                break;
            case CMD_DESTROY:
                cmd_str = "destroy";
                break;
            case CMD_STATUS:
                cmd_str = "status";
                break;
            default:
                break;
        }
        
        if (!quiet && cmd != CMD_STATUS) {
            printf(COLOR_CYAN "%s VM %d...\n" COLOR_RESET, 
                   cmd == CMD_START ? "启动" :
                   cmd == CMD_STOP ? "停止" :
                   cmd == CMD_RESTART ? "重启" :
                   cmd == CMD_SUSPEND ? "暂停" :
                   cmd == CMD_RESUME ? "恢复" :
                   cmd == CMD_DESTROY ? "销毁" : "操作",
                   vmid);
        }
        
        // 根据模式执行命令
        if (mode == MODE_LOCAL) {
            if (cmd == CMD_STATUS) {
                // 增强版 status 命令，显示详细信息
                char cmd_buf[512];
                
                printf(COLOR_BOLD "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" COLOR_RESET);
                printf(COLOR_BOLD "VM %d 详细信息\n" COLOR_RESET, vmid);
                printf(COLOR_BOLD "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" COLOR_RESET);
                
                // 基本状态
                printf(COLOR_CYAN "基本信息:\n" COLOR_RESET);
                snprintf(cmd_buf, sizeof(cmd_buf), "qm status %d | grep -v '^$'", vmid);
                if (system(cmd_buf) != 0) {
                    printf("  状态: 无法获取\n");
                }
                
                // 配置信息
                printf("\n" COLOR_CYAN "配置信息:\n" COLOR_RESET);
                snprintf(cmd_buf, sizeof(cmd_buf), 
                        "qm config %d | grep -E '^(name|memory|cores|sockets|bootdisk|net0|scsi0|ide0|sata0)' | "
                        "sed 's/^/  /'", vmid);
                if (system(cmd_buf) != 0) {
                    printf("  配置: 无法获取\n");
                }
                
                // 存储信息
                printf("\n" COLOR_CYAN "存储信息:\n" COLOR_RESET);
                snprintf(cmd_buf, sizeof(cmd_buf),
                        "qm config %d | grep -E '^(scsi|ide|sata|virtio)[0-9]:' | "
                        "sed 's/^/  /' | head -5", vmid);
                if (system(cmd_buf) != 0) {
                    printf("  存储: 无法获取\n");
                }
                
                // IP 地址（仅运行中的 VM）
                snprintf(cmd_buf, sizeof(cmd_buf), "qm status %d | grep -q 'status: running'", vmid);
                if (system(cmd_buf) == 0) {
                    printf("\n" COLOR_CYAN "网络信息:\n" COLOR_RESET);
                    
                    // 获取 IPv4 地址
                    snprintf(cmd_buf, sizeof(cmd_buf),
                            "qm guest cmd %d network-get-interfaces 2>/dev/null | "
                            "grep -A 2 '\"ip-address\"' | "
                            "grep -oP '\"ip-address\"\\s*:\\s*\"\\K[^\"]+' | "
                            "grep -E '^[0-9.]+$' | grep -v '^127\\.' | "
                            "sed 's/^/  IPv4: /' | head -1",
                            vmid);
                    int ipv4_result = system(cmd_buf);
                    if (ipv4_result != 0) {
                        printf("  IPv4: N/A\n");
                    }
                    
                    // 获取 IPv6 地址
                    snprintf(cmd_buf, sizeof(cmd_buf),
                            "qm guest cmd %d network-get-interfaces 2>/dev/null | "
                            "grep -A 2 '\"ip-address\"' | "
                            "grep -oP '\"ip-address\"\\s*:\\s*\"\\K[^\"]+' | "
                            "grep ':' | grep -v '^::1' | grep -v '^fe80' | "
                            "sed 's/^/  IPv6: /' | head -1",
                            vmid);
                    int ipv6_result = system(cmd_buf);
                    if (ipv6_result != 0) {
                        printf("  IPv6: N/A\n");
                    }
                    
                    // 如果两者都没有，显示提示
                    if (ipv4_result != 0 && ipv6_result != 0) {
                        printf("  提示: 需要安装 qemu-guest-agent\n");
                    }
                    
                    // MAC 地址
                    snprintf(cmd_buf, sizeof(cmd_buf),
                            "qm config %d | grep -oP 'net0:.*,macaddr=\\K[0-9A-Fa-f:]+' | "
                            "sed 's/^/  MAC: /' || echo '  MAC: N/A'", vmid);
                    if (system(cmd_buf) != 0) {
                        printf("  MAC: N/A\n");
                    }
                }
                
                // 快照信息
                snprintf(cmd_buf, sizeof(cmd_buf), "qm listsnapshot %d 2>/dev/null | tail -n +2 | wc -l", vmid);
                FILE *snap_fp = popen(cmd_buf, "r");
                if (snap_fp) {
                    int snap_count = 0;
                    if (fscanf(snap_fp, "%d", &snap_count) == 1 && snap_count > 0) {
                        printf("\n" COLOR_CYAN "快照信息:\n" COLOR_RESET);
                        printf("  快照数量: %d\n", snap_count);
                        snprintf(cmd_buf, sizeof(cmd_buf), 
                                "qm listsnapshot %d 2>/dev/null | tail -n +2 | sed 's/^/  /' | head -5", vmid);
                        if (system(cmd_buf) != 0) {
                            printf("  快照列表: 无法获取\n");
                        }
                    }
                    pclose(snap_fp);
                }
                
                printf(COLOR_BOLD "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" COLOR_RESET);
                result = 0;
            } else if (cmd == CMD_DESTROY) {
                result = execute_qm_command_silent("stop", vmid);
                if (result == 0 || force) {
                    result = execute_qm_command_silent("destroy", vmid);
                }
            } else {
                result = execute_qm_command_silent(cmd_str, vmid);
            }
        } else {
            result = execute_api_command(&config, cmd_str, vmid);
        }
        
        if (result == 0) {
            if (!quiet && cmd != CMD_STATUS) {
                printf(COLOR_GREEN "✓ VM %d 操作成功\n" COLOR_RESET, vmid);
            }
            success_count++;
        } else {
            if (!quiet) {
                printf(COLOR_RED "✗ VM %d 操作失败\n" COLOR_RESET, vmid);
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

// 环境检测函数
ExecutionMode detect_mode(void) {
    // 检查是否在 PVE 环境中
    if (is_pve_environment()) {
        return MODE_LOCAL;
    }
    
    // 检查是否有远程配置
    char *config_path = get_config_path();
    if (access(config_path, F_OK) == 0) {
        return MODE_REMOTE;
    }
    
    // 没有配置，提示用户
    fprintf(stderr, COLOR_YELLOW "提示：未检测到本地 Proxmox 环境，且未配置远程连接\n" COLOR_RESET);
    fprintf(stderr, "\n");
    fprintf(stderr, "请选择以下操作之一:\n");
    fprintf(stderr, "  1. 在 Proxmox VE 服务器上运行此命令\n");
    fprintf(stderr, "  2. 配置远程连接: %s --config\n", PROGRAM_NAME);
    fprintf(stderr, "\n");
    exit(1);
}

int is_pve_environment(void) {
    // 检查 qm 命令是否存在
    if (access("/usr/sbin/qm", X_OK) != 0) {
        return 0;
    }
    
    // 检查 /etc/pve 目录是否存在
    if (access("/etc/pve", F_OK) != 0) {
        return 0;
    }
    
    return 1;
}

char* get_config_path(void) {
    static char path[512];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/%s", home, CONFIG_FILE);
    } else {
        snprintf(path, sizeof(path), "%s", CONFIG_FILE);
    }
    return path;
}

// 配置管理函数
int load_config(Config *config) {
    FILE *fp = fopen(get_config_path(), "r");
    if (!fp) {
        return -1;
    }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char key[64], value[256];
        if (sscanf(line, "%63[^=]=%255[^\n]", key, value) == 2) {
            char *k = key;
            while (*k == ' ') k++;
            char *v = value;
            while (*v == ' ') v++;
            
            if (strcmp(k, "host") == 0) {
                strncpy(config->host, v, sizeof(config->host) - 1);
                config->host[sizeof(config->host) - 1] = '\0';
            } else if (strcmp(k, "user") == 0) {
                strncpy(config->user, v, sizeof(config->user) - 1);
                config->user[sizeof(config->user) - 1] = '\0';
            } else if (strcmp(k, "password") == 0) {
                strncpy(config->password, v, sizeof(config->password) - 1);
                config->password[sizeof(config->password) - 1] = '\0';
            } else if (strcmp(k, "token_id") == 0) {
                strncpy(config->token_id, v, sizeof(config->token_id) - 1);
                config->token_id[sizeof(config->token_id) - 1] = '\0';
            } else if (strcmp(k, "token_secret") == 0) {
                strncpy(config->token_secret, v, sizeof(config->token_secret) - 1);
                config->token_secret[sizeof(config->token_secret) - 1] = '\0';
            } else if (strcmp(k, "node") == 0) {
                strncpy(config->node, v, sizeof(config->node) - 1);
                config->node[sizeof(config->node) - 1] = '\0';
            } else if (strcmp(k, "port") == 0) {
                config->port = atoi(v);
            } else if (strcmp(k, "verify_ssl") == 0) {
                config->verify_ssl = atoi(v);
            }
        }
    }
    
    fclose(fp);
    
    if (config->port == 0) config->port = 8006;
    
    return 0;
}

int save_config(Config *config) {
    FILE *fp = fopen(get_config_path(), "w");
    if (!fp) {
        fprintf(stderr, COLOR_RED "错误：无法创建配置文件\n" COLOR_RESET);
        return -1;
    }
    
    fprintf(fp, "# vmanager 配置文件\n");
    fprintf(fp, "# Proxmox 服务器配置\n\n");
    fprintf(fp, "host=%s\n", config->host);
    fprintf(fp, "port=%d\n", config->port);
    fprintf(fp, "user=%s\n", config->user);
    fprintf(fp, "node=%s\n", config->node);
    fprintf(fp, "verify_ssl=%d\n", config->verify_ssl);
    fprintf(fp, "\n# 认证方式 1: 用户名密码\n");
    if (strlen(config->password) > 0) {
        fprintf(fp, "password=%s\n", config->password);
    } else {
        fprintf(fp, "#password=\n");
    }
    fprintf(fp, "\n# 认证方式 2: API Token (推荐)\n");
    if (strlen(config->token_id) > 0) {
        fprintf(fp, "token_id=%s\n", config->token_id);
        fprintf(fp, "token_secret=%s\n", config->token_secret);
    } else {
        fprintf(fp, "#token_id=\n");
        fprintf(fp, "#token_secret=\n");
    }
    
    fclose(fp);
    
    // 设置文件权限为 600
    chmod(get_config_path(), S_IRUSR | S_IWUSR);
    
    printf(COLOR_GREEN "✓ 配置已保存到: %s\n" COLOR_RESET, get_config_path());
    return 0;
}

void config_wizard(Config *config) {
    printf(COLOR_BOLD "vmanager 远程连接配置向导\n" COLOR_RESET);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    char input[256];
    
    printf("Proxmox 主机地址 (例如: 192.168.1.100): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        strncpy(config->host, input, sizeof(config->host) - 1);
        config->host[sizeof(config->host) - 1] = '\0';
    }
    
    printf("端口 [8006]: ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        config->port = strlen(input) > 0 ? atoi(input) : 8006;
    }
    
    printf("节点名称 (例如: pve): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        strncpy(config->node, input, sizeof(config->node) - 1);
        config->node[sizeof(config->node) - 1] = '\0';
    }
    
    printf("用户名 (例如: root@pam): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        strncpy(config->user, input, sizeof(config->user) - 1);
        config->user[sizeof(config->user) - 1] = '\0';
    }
    
    printf("\n认证方式:\n");
    printf("  1. 用户名密码\n");
    printf("  2. API Token (推荐)\n");
    printf("选择 [2]: ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        int choice = strlen(input) > 0 ? atoi(input) : 2;
        
        if (choice == 1) {
            printf("密码: ");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                strncpy(config->password, input, sizeof(config->password) - 1);
                config->password[sizeof(config->password) - 1] = '\0';
            }
        } else {
            printf("Token ID (例如: root@pam!vmanager): ");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                strncpy(config->token_id, input, sizeof(config->token_id) - 1);
                config->token_id[sizeof(config->token_id) - 1] = '\0';
            }
            printf("Token Secret: ");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                strncpy(config->token_secret, input, sizeof(config->token_secret) - 1);
                config->token_secret[sizeof(config->token_secret) - 1] = '\0';
            }
        }
    }
    
    printf("\n验证 SSL 证书? (0=否, 1=是) [0]: ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        config->verify_ssl = strlen(input) > 0 ? atoi(input) : 0;
    }
    
    printf("\n");
    save_config(config);
    
    printf("\n" COLOR_GREEN "配置完成！\n" COLOR_RESET);
    printf("现在可以使用: %s list\n", PROGRAM_NAME);
}

// 本地模式函数（继续使用原有的实现）
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

int list_vms_local(void) {
    // 使用 v2 中增强的 list 实现
    printf(COLOR_BOLD "%-10s %-20s %-12s %-10s %-15s %-12s %-20s\n" COLOR_RESET,
           "VMID", "NAME", "STATUS", "MEM(MB)", "BOOTDISK(GB)", "BRIDGE", "IP ADDRESS");
    printf("─────────────────────────────────────────────────────────────────────────────────────────────────\n");
    
    FILE *fp = popen("qm list | tail -n +2", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            int vmid, mem, pid;
            char name[64], status[32];
            float disk;
            
            if (sscanf(line, "%d %63s %31s %d %f %d", &vmid, name, status, &mem, &disk, &pid) >= 5) {
                char ip_cmd[256];
                char ip_addr[64] = "N/A";
                char bridge[32] = "N/A";
                
                // 获取网络接口
                snprintf(ip_cmd, sizeof(ip_cmd),
                        "qm config %d 2>/dev/null | grep -oP 'net0:.*,bridge=\\K[^,]+' | head -1", vmid);
                FILE *bridge_fp = popen(ip_cmd, "r");
                if (bridge_fp) {
                    if (fgets(bridge, sizeof(bridge), bridge_fp)) {
                        bridge[strcspn(bridge, "\n")] = 0;
                        if (strlen(bridge) == 0) {
                            strcpy(bridge, "N/A");
                        }
                    }
                    pclose(bridge_fp);
                }
                
                if (strcmp(status, "running") == 0) {
                    snprintf(ip_cmd, sizeof(ip_cmd), 
                            "qm guest cmd %d network-get-interfaces 2>/dev/null | "
                            "grep -A 2 '\"ip-address\"' | "
                            "grep -oP '\"ip-address\"\\s*:\\s*\"\\K[^\"]+' | "
                            "grep -v '^127\\.' | grep -v '^::' | grep -v '^fe80' | head -1", vmid);
                    
                    FILE *ip_fp = popen(ip_cmd, "r");
                    if (ip_fp) {
                        if (fgets(ip_addr, sizeof(ip_addr), ip_fp)) {
                            ip_addr[strcspn(ip_addr, "\n")] = 0;
                            if (strlen(ip_addr) == 0) {
                                strcpy(ip_addr, "N/A");
                            }
                        } else {
                            strcpy(ip_addr, "N/A");
                        }
                        pclose(ip_fp);
                    }
                }
                
                const char *status_color = COLOR_RESET;
                if (strcmp(status, "running") == 0) {
                    status_color = COLOR_GREEN;
                } else if (strcmp(status, "stopped") == 0) {
                    status_color = COLOR_YELLOW;
                }
                
                printf("%-10d %-20s %s%-12s" COLOR_RESET " %-10d %-15.2f %-12s %-20s\n",
                       vmid, name, status_color, status, mem, disk, bridge, ip_addr);
            }
        }
        pclose(fp);
    }
    return 0;
}

int check_vm_exists(int vmid) {
    char command[MAX_COMMAND];
    snprintf(command, sizeof(command), "qm status %d >/dev/null 2>&1", vmid);
    if (system(command) != 0) {
        fprintf(stderr, COLOR_YELLOW "提示：VM %d 可能不存在\n" COLOR_RESET, vmid);
        return 0;
    }
    return 1;
}

// 远程模式函数
int execute_api_command(Config *config, const char *cmd, int vmid) {
    char command[MAX_COMMAND];
    
    if (strlen(config->token_id) == 0) {
        fprintf(stderr, COLOR_YELLOW "警告：密码认证暂未实现，请使用 API Token\n" COLOR_RESET);
        return -1;
    }
    
    // status 命令使用 GET 方法获取状态
    if (strcmp(cmd, "status") == 0) {
        snprintf(command, sizeof(command),
                "curl -s -k 'https://%s:%d/api2/json/nodes/%s/qemu/%d/status/current' "
                "-H 'Authorization: PVEAPIToken=%s=%s' | python3 -m json.tool 2>/dev/null || cat",
                config->host, config->port, config->node, vmid,
                config->token_id, config->token_secret);
    } else {
        // 其他命令使用 POST 方法
        snprintf(command, sizeof(command),
                "curl -s -k -X POST 'https://%s:%d/api2/json/nodes/%s/qemu/%d/status/%s' "
                "-H 'Authorization: PVEAPIToken=%s=%s'",
                config->host, config->port, config->node, vmid, cmd,
                config->token_id, config->token_secret);
    }
    
    return system(command);
}

int list_vms_remote(Config *config, int verbose) {
    char command[MAX_COMMAND];
    FILE *fp;
    char line[4096];
    int debug_mode = (getenv("VMANAGER_DEBUG") != NULL);
    (void)verbose; // 暂时未使用，避免警告
    
    if (strlen(config->token_id) == 0) {
        fprintf(stderr, COLOR_YELLOW "警告：密码认证暂未实现，请使用 API Token\n" COLOR_RESET);
        return -1;
    }
    
    // 获取 VM 列表
    snprintf(command, sizeof(command),
            "curl -s -k 'https://%s:%d/api2/json/nodes/%s/qemu' "
            "-H 'Authorization: PVEAPIToken=%s=%s'",
            config->host, config->port, config->node,
            config->token_id, config->token_secret);
    
    if (debug_mode) {
        fprintf(stderr, COLOR_CYAN "调试: API 请求\n" COLOR_RESET);
        fprintf(stderr, "  URL: https://%s:%d/api2/json/nodes/%s/qemu\n",
                config->host, config->port, config->node);
    }
    
    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, COLOR_RED "错误：无法执行 API 请求\n" COLOR_RESET);
        return -1;
    }
    
    // 读取完整响应
    char response[65536] = {0};
    size_t total = 0;
    while (fgets(line, sizeof(line), fp) != NULL && total < sizeof(response) - 1) {
        size_t len = strlen(line);
        if (total + len < sizeof(response)) {
            strcat(response, line);
            total += len;
        }
    }
    pclose(fp);
    
    if (debug_mode) {
        fprintf(stderr, COLOR_CYAN "调试: API 响应\n" COLOR_RESET);
        fprintf(stderr, "%s\n", response);
        fprintf(stderr, COLOR_CYAN "调试: 响应长度 = %zu 字节\n" COLOR_RESET, total);
    }
    
    // 检查是否有数据
    if (strstr(response, "\"data\"") == NULL) {
        fprintf(stderr, COLOR_RED "错误：API 响应格式错误\n" COLOR_RESET);
        if (!debug_mode) {
            fprintf(stderr, "提示: 使用 VMANAGER_DEBUG=1 查看详细信息\n");
        }
        return -1;
    }
    
    // 检查是否为空列表
    if (strstr(response, "\"data\":[]") != NULL || strstr(response, "\"data\": []") != NULL) {
        printf(COLOR_YELLOW "没有找到虚拟机\n" COLOR_RESET);
        if (debug_mode) {
            fprintf(stderr, COLOR_CYAN "调试: 节点 '%s' 上没有 VM\n" COLOR_RESET, config->node);
        }
        return 0;
    }
    
    // 打印表头
    printf(COLOR_BOLD "%-10s %-20s %-12s %-10s %-15s %-12s %-20s\n" COLOR_RESET,
           "VMID", "NAME", "STATUS", "MEM(MB)", "BOOTDISK(GB)", "BRIDGE", "IP ADDRESS");
    printf("─────────────────────────────────────────────────────────────────────────────────────────────────\n");
    
    // 简单的 JSON 解析（提取关键字段）
    char *ptr = response;
    while ((ptr = strstr(ptr, "\"vmid\"")) != NULL) {
        int vmid = 0;
        char name[64] = "N/A";
        char status[32] = "N/A";
        long long maxmem = 0;
        
        // 找到当前对象的开始位置（向前找 {）
        char *obj_start = ptr;
        while (obj_start > response && *obj_start != '{') {
            obj_start--;
        }
        
        // 找到当前对象的结束位置
        char *obj_end = strchr(ptr, '}');
        if (obj_end == NULL) {
            break;
        }
        
        // 提取 vmid
        char *vmid_ptr = strstr(obj_start, "\"vmid\"");
        if (vmid_ptr && vmid_ptr < obj_end) {
            sscanf(vmid_ptr, "\"vmid\":%d", &vmid);
        }
        
        // 提取 name
        char *name_ptr = strstr(obj_start, "\"name\"");
        if (name_ptr && name_ptr < obj_end) {
            char *colon = strchr(name_ptr, ':');
            if (colon && colon < obj_end) {
                // 跳过空格
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                if (*colon == '"') {
                    char *quote1 = colon;
                    char *quote2 = strchr(quote1 + 1, '"');
                    if (quote2 && quote2 < obj_end) {
                        size_t len = quote2 - quote1 - 1;
                        if (len < sizeof(name) && len > 0) {
                            strncpy(name, quote1 + 1, len);
                            name[len] = '\0';
                        }
                    }
                }
            }
        }
        
        // 提取 status
        char *status_ptr = strstr(obj_start, "\"status\"");
        if (status_ptr && status_ptr < obj_end) {
            char *colon = strchr(status_ptr, ':');
            if (colon && colon < obj_end) {
                // 跳过空格
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                if (*colon == '"') {
                    char *quote1 = colon;
                    char *quote2 = strchr(quote1 + 1, '"');
                    if (quote2 && quote2 < obj_end) {
                        size_t len = quote2 - quote1 - 1;
                        if (len < sizeof(status) && len > 0) {
                            strncpy(status, quote1 + 1, len);
                            status[len] = '\0';
                        }
                    }
                }
            }
        }
        
        // 提取 maxmem
        char *mem_ptr = strstr(obj_start, "\"maxmem\"");
        if (mem_ptr && mem_ptr < obj_end) {
            sscanf(mem_ptr, "\"maxmem\":%lld", &maxmem);
        }
        
        // 打印 VM 信息
        printf("%-10d %-20s %-12s %-10d %-15s %-12s %-20s\n",
               vmid, name, status, (int)(maxmem / 1024 / 1024), "N/A", "N/A", "N/A");
        
        // 移动到下一个对象
        ptr = obj_end + 1;
    }
    
    return 0;
}

// 通用函数
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

void print_version(void) {
    printf(COLOR_BOLD "%s" COLOR_RESET " version " COLOR_CYAN "%s\n" COLOR_RESET, 
           PROGRAM_NAME, VERSION);
    printf("\n");
    printf("vmanager - Proxmox 虚拟机管理工具（统一版本）\n");
    printf("作者：YXWA Infosec Lab (Crystal & evalEvil)\n");
    printf("许可：GNU General Public License v3.0\n");
    printf("项目：https://github.com/sscaifesu/vmanager\n");
    printf("\n");
    printf("特性：\n");
    printf("  • 智能检测运行环境（本地/远程）\n");
    printf("  • 本地模式：直接使用 qm 命令（PVE 服务器上）\n");
    printf("  • 远程模式：使用 Proxmox API（任何平台）\n");
    printf("  • 启动/停止/重启/暂停/恢复/销毁虚拟机\n");
    printf("  • 批量操作支持\n");
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
    printf("  " COLOR_CYAN "destroy" COLOR_RESET "    销毁虚拟机\n");
    printf("  " COLOR_CYAN "status" COLOR_RESET "     查看虚拟机状态\n");
    printf("  " COLOR_CYAN "list" COLOR_RESET "       列出所有虚拟机\n");
    printf("\n");
    
    printf(COLOR_BOLD "选项：\n" COLOR_RESET);
    printf("  " COLOR_CYAN "-h, --help" COLOR_RESET "     显示此帮助信息\n");
    printf("  " COLOR_CYAN "-V, --version" COLOR_RESET "  显示版本信息\n");
    printf("  " COLOR_CYAN "-c, --config" COLOR_RESET "   配置远程连接\n");
    printf("  " COLOR_CYAN "-f, --force" COLOR_RESET "    强制执行（跳过确认）\n");
    printf("  " COLOR_CYAN "-q, --quiet" COLOR_RESET "    静默模式\n");
    printf("  " COLOR_CYAN "-m, --mode" COLOR_RESET "     指定模式 (auto/local/remote)\n");
    printf("\n");
    
    printf(COLOR_BOLD "运行模式：\n" COLOR_RESET);
    printf("  " COLOR_GREEN "本地模式" COLOR_RESET " - 在 PVE 服务器上运行，直接使用 qm 命令\n");
    printf("  " COLOR_BLUE "远程模式" COLOR_RESET " - 在任何平台上运行，通过 API 连接 PVE\n");
    printf("  " COLOR_YELLOW "自动模式" COLOR_RESET " - 自动检测环境并选择合适的模式（默认）\n");
    printf("\n");
    
    printf(COLOR_BOLD "首次使用（远程模式）：\n" COLOR_RESET);
    printf("  1. 配置远程连接\n");
    printf("     $ %s --config\n\n", PROGRAM_NAME);
    printf("  2. 输入 Proxmox 服务器信息\n");
    printf("  3. 开始使用\n");
    printf("     $ %s list\n\n", PROGRAM_NAME);
    
    printf(COLOR_BOLD "示例：\n" COLOR_RESET);
    printf("  # 自动模式（推荐）\n");
    printf("  $ %s list\n\n", PROGRAM_NAME);
    
    printf("  # 启动 VM\n");
    printf("  $ %s start 100\n\n", PROGRAM_NAME);
    
    printf("  # 批量停止\n");
    printf("  $ %s stop 100-110\n\n", PROGRAM_NAME);
    
    printf("  # 强制指定模式\n");
    printf("  $ %s --mode=local list\n", PROGRAM_NAME);
    printf("  $ %s --mode=remote list\n\n", PROGRAM_NAME);
    
    printf("配置文件位置: ~/%s\n", CONFIG_FILE);
    printf("更多信息请访问：https://github.com/sscaifesu/vmanager\n");
}

void print_usage(void) {
    fprintf(stderr, "用法：%s [OPTIONS] COMMAND [VMID...]\n", PROGRAM_NAME);
    fprintf(stderr, "尝试 '%s --help' 获取更多信息。\n", PROGRAM_NAME);
}
