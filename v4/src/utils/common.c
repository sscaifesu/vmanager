/*
 * 通用工具函数
 * 提供格式化、解析等辅助功能
 */

#define _POSIX_C_SOURCE 200809L
#include "../../include/vmanager.h"

// 检查字符串是否为数字
bool is_number(const char *str) {
    if (!str || *str == '\0') {
        return false;
    }
    
    while (*str) {
        if (*str < '0' || *str > '9') {
            return false;
        }
        str++;
    }
    
    return true;
}

// 解析 VMID 范围 (例如: "100-105" 或 "100,102,104" 或 "100-105,110")
int parse_vmid_range(const char *range, int *vmids, int *count) {
    if (!range || !vmids || !count) {
        return -1;
    }
    
    *count = 0;
    char *range_copy = strdup(range);
    if (!range_copy) {
        return -1;
    }
    
    char *token = strtok(range_copy, ",");
    while (token && *count < MAX_VMIDS) {
        // 去除空白
        while (*token == ' ' || *token == '\t') token++;
        
        // 检查是否为范围 (例如: 100-105)
        char *dash = strchr(token, '-');
        if (dash) {
            *dash = '\0';
            int start = atoi(token);
            int end = atoi(dash + 1);
            
            if (start > 0 && end >= start) {
                for (int i = start; i <= end && *count < MAX_VMIDS; i++) {
                    vmids[(*count)++] = i;
                }
            }
        } else {
            // 单个 VMID
            int vmid = atoi(token);
            if (vmid > 0) {
                vmids[(*count)++] = vmid;
            }
        }
        
        token = strtok(NULL, ",");
    }
    
    free(range_copy);
    return 0;
}

// 格式化字节数 (返回静态缓冲区，非线程安全)
char* format_bytes(uint64_t bytes) {
    static char buffer[32];
    
    if (bytes == 0) {
        snprintf(buffer, sizeof(buffer), "0 B");
    } else if (bytes < 1024) {
        snprintf(buffer, sizeof(buffer), "%lu B", (unsigned long)bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes < 1024ULL * 1024 * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f TB", bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0));
    }
    
    return buffer;
}

// 格式化运行时间 (返回静态缓冲区，非线程安全)
char* format_uptime(int seconds) {
    static char buffer[64];
    
    if (seconds < 0) {
        snprintf(buffer, sizeof(buffer), "N/A");
        return buffer;
    }
    
    int days = seconds / 86400;
    int hours = (seconds % 86400) / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (days > 0) {
        snprintf(buffer, sizeof(buffer), "%dd %dh %dm", days, hours, minutes);
    } else if (hours > 0) {
        snprintf(buffer, sizeof(buffer), "%dh %dm", hours, minutes);
    } else if (minutes > 0) {
        snprintf(buffer, sizeof(buffer), "%dm %ds", minutes, secs);
    } else {
        snprintf(buffer, sizeof(buffer), "%ds", secs);
    }
    
    return buffer;
}
