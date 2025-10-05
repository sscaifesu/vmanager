/*
 * vmanager v4.0.0 - 主程序
 */

#include "../include/vmanager.h"
#include <getopt.h>

// 全局变量
Config g_config = {0};
ExecutionMode g_exec_mode = MODE_AUTO;
UIMode g_ui_mode = UI_CLI;
bool g_verbose = false;
bool g_debug = false;
bool g_tui_mode = false;

static void print_version(void) {
    printf("%s version %s\n\n", PROGRAM_NAME, VERSION);
    printf("Proxmox 虚拟机管理工具\n\n");
    printf("Copyright (C) 2025 YXWA Infosec Lab\n");
    printf("作者：Crystal & evalEvil\n\n");
    printf("许可：GNU General Public License v3.0\n");
    printf("项目：https://github.com/sscaifesu/vmanager\n");
}

static void print_help(void) {
    printf("用法： %s [OPTIONS] COMMAND [ARGS...]\n\n", PROGRAM_NAME);
    printf("选项：\n");
    printf("  --cli              使用 CLI 模式 (默认)\n");
    printf("  --tui              使用 TUI 模式 (交互式界面)\n");
    printf("  --config FILE      指定配置文件\n");
    printf("  --mode MODE        强制模式 (local/remote)\n");
    printf("  -v, --verbose      详细输出\n");
    printf("  -d, --debug        调试模式\n");
    printf("  -h, --help         显示帮助信息\n");
    printf("  --version          显示版本信息\n\n");
    printf("命令：\n");
    printf("  list                    列出所有 VM\n");
    printf("  status VMID             查看 VM 状态\n");
    printf("  start VMID...           启动 VM (支持批量和范围)\n");
    printf("  stop VMID...            停止 VM (支持批量和范围)\n");
    printf("  reboot VMID...          重启 VM (支持批量和范围)\n");
    printf("  suspend VMID...         暂停 VM (支持批量和范围)\n");
    printf("  resume VMID...          恢复 VM (支持批量和范围)\n");
    printf("  destroy VMID [-f]       删除 VM\n");
    printf("  clone VMID NEWID        克隆 VM\n\n");
    printf("批量操作格式：\n");
    printf("  单个:   111\n");
    printf("  多个:   111 112 113\n");
    printf("  范围:   111-115\n");
    printf("  逗号:   111,112,113\n");
    printf("  混合:   111 112 113-115 120,121,122\n\n");
    printf("示例：\n");
    printf("  %s list\n", PROGRAM_NAME);
    printf("  %s status 111\n", PROGRAM_NAME);
    printf("  %s start 111 112 113\n", PROGRAM_NAME);
    printf("  %s start 111-115\n", PROGRAM_NAME);
    printf("  %s start 111 112 113-115 120,121,122\n", PROGRAM_NAME);
    printf("  %s reboot 111-120\n", PROGRAM_NAME);
    printf("  %s stop 111,112,113\n", PROGRAM_NAME);
    printf("  %s destroy 111 -f\n", PROGRAM_NAME);
    printf("  %s clone 111 112 --name new-vm\n", PROGRAM_NAME);
    printf("  %s --tui\n", PROGRAM_NAME);
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"cli",     no_argument,       0, 'c'},
        {"tui",     no_argument,       0, 't'},
        {"config",  required_argument, 0, 'C'},
        {"mode",    required_argument, 0, 'm'},
        {"verbose", no_argument,       0, 'v'},
        {"debug",   no_argument,       0, 'd'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    char config_file[512] = {0};
    
    // 使用 + 前缀让 getopt 在遇到第一个非选项参数时停止
    while ((opt = getopt_long(argc, argv, "+ctC:m:vdhV", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                g_ui_mode = UI_CLI;
                break;
            case 't':
                g_ui_mode = UI_TUI;
                break;
            case 'C':
                strncpy(config_file, optarg, sizeof(config_file) - 1);
                break;
            case 'm':
                if (strcmp(optarg, "local") == 0) {
                    g_exec_mode = MODE_LOCAL;
                } else if (strcmp(optarg, "remote") == 0) {
                    g_exec_mode = MODE_REMOTE;
                }
                break;
            case 'v':
                // verbose mode
                break;
            case 'd':
                g_debug = true;
                break;
            case 'h':
                print_help();
                return 0;
            case 'V':
                print_version();
                return 0;
            default:
                print_help();
                return 1;
        }
    }
    
    // 加载配置
    if (config_file[0] == '\0') {
        snprintf(config_file, sizeof(config_file), "%s/.vmanager.conf", getenv("HOME"));
    }
    
    if (config_load(&g_config, config_file) != 0) {
        fprintf(stderr, "警告：无法加载配置文件，使用配置向导\n");
        if (config_wizard(&g_config) != 0) {
            fprintf(stderr, "错误：配置失败\n");
            return 1;
        }
    }
    
    // 初始化 API
    if (api_init(&g_config) != 0) {
        fprintf(stderr, "错误：API 初始化失败\n");
        return 1;
    }
    
    int ret = 0;
    
    // 选择 UI 模式
    if (g_ui_mode == UI_TUI) {
        g_tui_mode = true;  // 设置 TUI 模式标志
        ret = tui_main();
    } else {
        ret = cli_main(argc - optind, argv + optind);
    }
    
    api_cleanup();
    return ret;
}
