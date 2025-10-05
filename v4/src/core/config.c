/*
 * 配置管理模块
 * 遵循 GNU 标准，支持 INI 格式配置文件
 */

#define _POSIX_C_SOURCE 200809L
#include "../../include/vmanager.h"
#include <errno.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 1024

// 去除字符串首尾空白
static char* trim(char *str) {
    if (!str) return NULL;
    
    // 去除开头空白
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }
    
    // 去除结尾空白
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return str;
}

// 检查是否在 PVE 服务器上
bool is_on_pve_server(void) {
    struct stat st;
    // 检查 /usr/bin/qm 是否存在
    if (stat("/usr/bin/qm", &st) == 0) {
        return true;
    }
    // 检查 /etc/pve 目录是否存在
    if (stat("/etc/pve", &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}

// 加载配置文件
int config_load(Config *config, const char *file) {
    if (!config || !file) {
        return -1;
    }
    
    FILE *fp = fopen(file, "r");
    if (!fp) {
        if (g_debug) {
            fprintf(stderr, "无法打开配置文件: %s (%s)\n", file, strerror(errno));
        }
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    char section[64] = "";
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char *trimmed = trim(line);
        
        // 跳过空行和注释
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }
        
        // 解析节 [section]
        if (trimmed[0] == '[') {
            char *end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }
        
        // 解析键值对 key = value
        char *eq = strchr(trimmed, '=');
        if (!eq) {
            if (g_debug) {
                fprintf(stderr, "配置文件语法错误 (行 %d): %s\n", line_num, trimmed);
            }
            continue;
        }
        
        *eq = '\0';
        char *key = trim(trimmed);
        char *value = trim(eq + 1);
        
        // 根据节和键设置配置
        if (strcmp(section, "server") == 0 || section[0] == '\0') {
            if (strcmp(key, "host") == 0) {
                strncpy(config->host, value, sizeof(config->host) - 1);
            } else if (strcmp(key, "port") == 0) {
                config->port = atoi(value);
            } else if (strcmp(key, "node") == 0) {
                strncpy(config->node, value, sizeof(config->node) - 1);
            } else if (strcmp(key, "verify_ssl") == 0) {
                config->verify_ssl = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        } else if (strcmp(section, "auth") == 0) {
            if (strcmp(key, "token_id") == 0) {
                strncpy(config->token_id, value, sizeof(config->token_id) - 1);
            } else if (strcmp(key, "token_secret") == 0) {
                strncpy(config->token_secret, value, sizeof(config->token_secret) - 1);
            }
        }
    }
    
    fclose(fp);
    
    // 验证必需字段
    if (config->host[0] == '\0' || config->port == 0 || 
        config->node[0] == '\0' || config->token_id[0] == '\0' || 
        config->token_secret[0] == '\0') {
        if (g_debug) {
            fprintf(stderr, "配置文件缺少必需字段\n");
        }
        return -1;
    }
    
    strncpy(config->config_file, file, sizeof(config->config_file) - 1);
    return 0;
}

// 保存配置文件
int config_save(Config *config, const char *file) {
    if (!config || !file) {
        return -1;
    }
    
    FILE *fp = fopen(file, "w");
    if (!fp) {
        fprintf(stderr, "无法创建配置文件: %s (%s)\n", file, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "# vmanager v%s 配置文件\n", VERSION);
    fprintf(fp, "# 自动生成于 %s\n\n", __DATE__);
    
    fprintf(fp, "[server]\n");
    fprintf(fp, "host = %s\n", config->host);
    fprintf(fp, "port = %d\n", config->port);
    fprintf(fp, "node = %s\n", config->node);
    fprintf(fp, "verify_ssl = %s\n\n", config->verify_ssl ? "true" : "false");
    
    fprintf(fp, "[auth]\n");
    fprintf(fp, "token_id = %s\n", config->token_id);
    fprintf(fp, "token_secret = %s\n\n", config->token_secret);
    
    fprintf(fp, "# UI 配置 (可选)\n");
    fprintf(fp, "#[ui]\n");
    fprintf(fp, "#default_mode = tui\n");
    fprintf(fp, "#refresh_interval = 5\n");
    fprintf(fp, "#color_scheme = default\n\n");
    
    fprintf(fp, "# 日志配置 (可选)\n");
    fprintf(fp, "#[logging]\n");
    fprintf(fp, "#enabled = true\n");
    fprintf(fp, "#level = info\n");
    fprintf(fp, "#file = ~/.vmanager.log\n");
    
    fclose(fp);
    
    // 设置文件权限为 600 (仅所有者可读写)
    chmod(file, S_IRUSR | S_IWUSR);
    
    printf("配置已保存到: %s\n", file);
    return 0;
}

// 配置向导
int config_wizard(Config *config) {
    if (!config) {
        return -1;
    }
    
    printf("\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("vmanager v%s 配置向导\n", VERSION);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    char input[512];
    
    // Proxmox 服务器地址
    printf("Proxmox 服务器地址 (IP 或域名): ");
    if (fgets(input, sizeof(input), stdin)) {
        char *trimmed = trim(input);
        if (trimmed[0] != '\0') {
            strncpy(config->host, trimmed, sizeof(config->host) - 1);
        }
    }
    
    // 端口
    printf("端口 [8006]: ");
    if (fgets(input, sizeof(input), stdin)) {
        char *trimmed = trim(input);
        if (trimmed[0] != '\0') {
            config->port = atoi(trimmed);
        } else {
            config->port = 8006;
        }
    }
    
    // 节点名称
    printf("节点名称 (例如: pve): ");
    if (fgets(input, sizeof(input), stdin)) {
        char *trimmed = trim(input);
        if (trimmed[0] != '\0') {
            strncpy(config->node, trimmed, sizeof(config->node) - 1);
        }
    }
    
    // 认证方式
    printf("\n认证方式:\n");
    printf("  1. API Token (推荐)\n");
    printf("  2. 用户名密码 (暂未实现)\n");
    printf("选择 [1]: ");
    
    int auth_method = 1;
    if (fgets(input, sizeof(input), stdin)) {
        char *trimmed = trim(input);
        if (trimmed[0] != '\0') {
            auth_method = atoi(trimmed);
        }
    }
    
    if (auth_method == 1) {
        printf("\nToken ID (例如: root@pam!vmanager): ");
        if (fgets(input, sizeof(input), stdin)) {
            char *trimmed = trim(input);
            if (trimmed[0] != '\0') {
                strncpy(config->token_id, trimmed, sizeof(config->token_id) - 1);
            }
        }
        
        printf("Token Secret: ");
        if (fgets(input, sizeof(input), stdin)) {
            char *trimmed = trim(input);
            if (trimmed[0] != '\0') {
                strncpy(config->token_secret, trimmed, sizeof(config->token_secret) - 1);
            }
        }
    } else {
        fprintf(stderr, "\n错误：用户名密码认证暂未实现\n");
        return -1;
    }
    
    // SSL 验证
    printf("\n验证 SSL 证书? (y/n) [n]: ");
    if (fgets(input, sizeof(input), stdin)) {
        char *trimmed = trim(input);
        config->verify_ssl = (trimmed[0] == 'y' || trimmed[0] == 'Y');
    }
    
    // 验证配置
    if (config->host[0] == '\0' || config->port == 0 || 
        config->node[0] == '\0' || config->token_id[0] == '\0' || 
        config->token_secret[0] == '\0') {
        fprintf(stderr, "\n错误：配置信息不完整\n");
        return -1;
    }
    
    // 保存配置
    char config_file[512];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(config_file, sizeof(config_file), "%s/.vmanager.conf", home);
    } else {
        snprintf(config_file, sizeof(config_file), ".vmanager.conf");
    }
    
    if (config_save(config, config_file) != 0) {
        fprintf(stderr, "\n错误：无法保存配置文件\n");
        return -1;
    }
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("配置完成！\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    return 0;
}
