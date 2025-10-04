/*
 * vmanager-remote - Proxmox 虚拟机远程管理工具
 * 版本：2.1.0
 * 作者：YXWA Infosec Lab (Crystal & evalEvil)
 * 许可：GNU General Public License v3.0
 * 项目：https://github.com/sscaifesu/vmanager
 * 
 * 支持通过 Proxmox API 远程管理 VM
 * 可在 macOS、Linux、Windows 上运行
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#ifdef _WIN32
    #include <windows.h>
    #define HOME_ENV "USERPROFILE"
    #define PATH_SEP "\\"
#else
    #include <unistd.h>
    #define HOME_ENV "HOME"
    #define PATH_SEP "/"
#endif

#define VERSION "2.1.0"
#define PROGRAM_NAME "vmanager"
#define CONFIG_FILE ".vmanager.conf"
#define MAX_COMMAND 2048
#define MAX_VMIDS 100
#define MAX_LINE 512

// 颜色定义（跨平台）
#ifdef _WIN32
    #define COLOR_RESET   ""
    #define COLOR_RED     ""
    #define COLOR_GREEN   ""
    #define COLOR_YELLOW  ""
    #define COLOR_BLUE    ""
    #define COLOR_CYAN    ""
    #define COLOR_BOLD    ""
#else
    #define COLOR_RESET   "\033[0m"
    #define COLOR_RED     "\033[31m"
    #define COLOR_GREEN   "\033[32m"
    #define COLOR_YELLOW  "\033[33m"
    #define COLOR_BLUE    "\033[34m"
    #define COLOR_CYAN    "\033[36m"
    #define COLOR_BOLD    "\033[1m"
#endif

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

// 函数声明
void print_version(void);
void print_help(void);
void print_usage(void);
int load_config(Config *config);
int save_config(Config *config);
void config_wizard(Config *config);
int execute_api_command(Config *config, const char *cmd, int vmid);
int execute_api_list(Config *config);
int is_number(const char *str);
int parse_vmid_range(const char *range, int *vmids, int *count);
char* get_config_path(void);

int main(int argc, char *argv[]) {
    Command cmd = CMD_NONE;
    Config config = {0};
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
        {"config",  no_argument,       0, 'c'},
        {"host",    required_argument, 0, 'H'},
        {"user",    required_argument, 0, 'u'},
        {"node",    required_argument, 0, 'n'},
        {0, 0, 0, 0}
    };
    
    // 解析选项
    while ((opt = getopt_long(argc, argv, "hVfqcH:u:n:", long_options, NULL)) != -1) {
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
            case 'c':
                cmd = CMD_CONFIG;
                break;
            case 'H':
                strncpy(config.host, optarg, sizeof(config.host) - 1);
                break;
            case 'u':
                strncpy(config.user, optarg, sizeof(config.user) - 1);
                break;
            case 'n':
                strncpy(config.node, optarg, sizeof(config.node) - 1);
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
    
    // 加载配置
    if (load_config(&config) != 0) {
        fprintf(stderr, COLOR_RED "错误：未找到配置文件\n" COLOR_RESET);
        fprintf(stderr, "请先运行配置向导：%s --config\n", PROGRAM_NAME);
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
        return execute_api_list(&config);
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
        
        if (!quiet) {
            printf(COLOR_CYAN "执行: %s VM %d...\n" COLOR_RESET, cmd_str, vmid);
        }
        
        result = execute_api_command(&config, cmd_str, vmid);
        
        if (result == 0) {
            if (!quiet) {
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

void print_version(void) {
    printf(COLOR_BOLD "%s" COLOR_RESET " version " COLOR_CYAN "%s\n" COLOR_RESET, 
           PROGRAM_NAME, VERSION);
    printf("\n");
    printf("vmanager-remote - Proxmox 虚拟机远程管理工具\n");
    printf("作者：YXWA Infosec Lab (Crystal & evalEvil)\n");
    printf("许可：GNU General Public License v3.0\n");
    printf("项目：https://github.com/sscaifesu/vmanager\n");
    printf("\n");
    printf("功能：\n");
    printf("  • 通过 Proxmox API 远程管理 VM\n");
    printf("  • 支持 macOS、Linux、Windows\n");
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
    printf("  " COLOR_CYAN "-c, --config" COLOR_RESET "   运行配置向导\n");
    printf("  " COLOR_CYAN "-f, --force" COLOR_RESET "    强制执行（跳过确认）\n");
    printf("  " COLOR_CYAN "-q, --quiet" COLOR_RESET "    静默模式\n");
    printf("  " COLOR_CYAN "-H, --host" COLOR_RESET "     Proxmox 主机地址\n");
    printf("  " COLOR_CYAN "-u, --user" COLOR_RESET "     用户名\n");
    printf("  " COLOR_CYAN "-n, --node" COLOR_RESET "     节点名称\n");
    printf("\n");
    
    printf(COLOR_BOLD "首次使用：\n" COLOR_RESET);
    printf("  1. 运行配置向导\n");
    printf("     $ %s --config\n\n", PROGRAM_NAME);
    printf("  2. 输入 Proxmox 服务器信息\n");
    printf("  3. 开始使用\n");
    printf("     $ %s start 100\n\n", PROGRAM_NAME);
    
    printf(COLOR_BOLD "示例：\n" COLOR_RESET);
    printf("  # 配置连接\n");
    printf("  $ %s --config\n\n", PROGRAM_NAME);
    
    printf("  # 启动 VM\n");
    printf("  $ %s start 100\n\n", PROGRAM_NAME);
    
    printf("  # 批量停止\n");
    printf("  $ %s stop 100-110\n\n", PROGRAM_NAME);
    
    printf("  # 列出所有 VM\n");
    printf("  $ %s list\n\n", PROGRAM_NAME);
    
    printf("配置文件位置: ~/%s\n", CONFIG_FILE);
    printf("更多信息请访问：https://github.com/sscaifesu/vmanager\n");
}

void print_usage(void) {
    fprintf(stderr, "用法：%s [OPTIONS] COMMAND [VMID...]\n", PROGRAM_NAME);
    fprintf(stderr, "尝试 '%s --help' 获取更多信息。\n", PROGRAM_NAME);
}

char* get_config_path(void) {
    static char path[512];
    const char *home = getenv(HOME_ENV);
    if (home) {
        snprintf(path, sizeof(path), "%s%s%s", home, PATH_SEP, CONFIG_FILE);
    } else {
        snprintf(path, sizeof(path), "%s", CONFIG_FILE);
    }
    return path;
}

int load_config(Config *config) {
    FILE *fp = fopen(get_config_path(), "r");
    if (!fp) {
        return -1;
    }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        // 去除注释和空行
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char key[64], value[256];
        if (sscanf(line, "%63[^=]=%255[^\n]", key, value) == 2) {
            // 去除前后空格
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
    
    // 设置默认值
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
    
    printf(COLOR_GREEN "✓ 配置已保存到: %s\n" COLOR_RESET, get_config_path());
    return 0;
}

void config_wizard(Config *config) {
    printf(COLOR_BOLD "vmanager 配置向导\n" COLOR_RESET);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    char input[256];
    
    // Proxmox 主机
    printf("Proxmox 主机地址 (例如: 192.168.1.100): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        strncpy(config->host, input, sizeof(config->host) - 1);
        config->host[sizeof(config->host) - 1] = '\0';
    }
    
    // 端口
    printf("端口 [8006]: ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        config->port = strlen(input) > 0 ? atoi(input) : 8006;
    }
    
    // 节点名称
    printf("节点名称 (例如: pve): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        strncpy(config->node, input, sizeof(config->node) - 1);
        config->node[sizeof(config->node) - 1] = '\0';
    }
    
    // 用户名
    printf("用户名 (例如: root@pam): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        strncpy(config->user, input, sizeof(config->user) - 1);
        config->user[sizeof(config->user) - 1] = '\0';
    }
    
    // 认证方式
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
            printf("Token ID (例如: user@pam!tokenname): ");
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
    
    // SSL 验证
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

int execute_api_command(Config *config, const char *cmd, int vmid) {
    char command[MAX_COMMAND];
    
    // 构建 curl 命令
    // 注意：这里需要 curl 支持，实际使用时建议使用 libcurl
    if (strlen(config->token_id) > 0) {
        // 使用 API Token
        snprintf(command, sizeof(command),
                "curl -s -k -X POST 'https://%s:%d/api2/json/nodes/%s/qemu/%d/status/%s' "
                "-H 'Authorization: PVEAPIToken=%s=%s'",
                config->host, config->port, config->node, vmid, cmd,
                config->token_id, config->token_secret);
    } else {
        // 使用用户名密码（需要先获取 ticket）
        fprintf(stderr, COLOR_YELLOW "警告：密码认证暂未实现，请使用 API Token\n" COLOR_RESET);
        return -1;
    }
    
    return system(command);
}

int execute_api_list(Config *config) {
    char command[MAX_COMMAND];
    
    if (strlen(config->token_id) > 0) {
        snprintf(command, sizeof(command),
                "curl -s -k 'https://%s:%d/api2/json/nodes/%s/qemu' "
                "-H 'Authorization: PVEAPIToken=%s=%s' | "
                "python3 -m json.tool 2>/dev/null || cat",
                config->host, config->port, config->node,
                config->token_id, config->token_secret);
    } else {
        fprintf(stderr, COLOR_YELLOW "警告：密码认证暂未实现，请使用 API Token\n" COLOR_RESET);
        return -1;
    }
    
    return system(command);
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
