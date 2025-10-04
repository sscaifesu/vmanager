/*
 * Proxmox VM 管理工具
 * 作者：YXWA Infosec Lab (Crystal & evalEvil)
 * 编译：gcc -o vm_manager vm_manager.c
 * 用法：sudo ./vm_manager
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_INPUT 256
#define MAX_COMMAND 512

// 颜色定义
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"

// 函数声明
void print_header(void);
int get_action_choice(void);
int get_input_type(void);
int get_single_vmid(void);
int get_vmid_range(int *start, int *end);
int confirm_destroy(void);
int execute_qm_stop(int vmid);
int execute_qm_destroy(int vmid);
int check_vm_exists(int vmid);
void trim_string(char *str);
int is_number(const char *str);

int main(void) {
    int action, input_type;
    int vmid, start, end;
    int success_count = 0, fail_count = 0;
    
    // 检查是否以 root 权限运行
    if (geteuid() != 0) {
        printf(COLOR_RED "错误：此程序需要 root 权限运行！\n" COLOR_RESET);
        printf("请使用：sudo %s\n", "vm_manager");
        return 1;
    }
    
    print_header();
    
    // 获取操作类型
    action = get_action_choice();
    if (action == -1) {
        printf(COLOR_RED "无效选项！退出。\n" COLOR_RESET);
        return 1;
    }
    
    // 获取输入方式
    input_type = get_input_type();
    if (input_type == -1) {
        printf(COLOR_RED "无效输入方式！退出。\n" COLOR_RESET);
        return 1;
    }
    
    // 根据输入方式处理
    if (input_type == 1) {
        // 单个 VMID
        vmid = get_single_vmid();
        if (vmid == -1) {
            printf(COLOR_RED "VMID 必须是数字！退出。\n" COLOR_RESET);
            return 1;
        }
        
        // 如果是销毁操作，需要确认
        if (action == 2) {
            if (!confirm_destroy()) {
                printf(COLOR_YELLOW "操作取消。\n" COLOR_RESET);
                return 0;
            }
        }
        
        // 执行操作
        if (action == 1) {
            // 仅停止
            printf(COLOR_CYAN "正在停止 VM %d...\n" COLOR_RESET, vmid);
            if (execute_qm_stop(vmid) == 0) {
                printf(COLOR_GREEN "VM %d 已停止成功。\n" COLOR_RESET, vmid);
            } else {
                printf(COLOR_YELLOW "VM %d 停止失败（可能已停止或不存在）。\n" COLOR_RESET, vmid);
                check_vm_exists(vmid);
            }
        } else {
            // 停止并销毁
            printf(COLOR_CYAN "正在停止并销毁 VM %d...\n" COLOR_RESET, vmid);
            int stop_result = execute_qm_stop(vmid);
            int destroy_result = execute_qm_destroy(vmid);
            
            if (stop_result == 0 && destroy_result == 0) {
                printf(COLOR_GREEN "VM %d 已停止并销毁成功。\n" COLOR_RESET, vmid);
            } else {
                printf(COLOR_YELLOW "VM %d 停止并销毁失败（检查是否已停止）。\n" COLOR_RESET, vmid);
                if (stop_result != 0) {
                    printf(COLOR_RED "  - stop 步骤失败。\n" COLOR_RESET);
                }
                if (destroy_result != 0) {
                    printf(COLOR_RED "  - destroy 步骤失败。\n" COLOR_RESET);
                }
                check_vm_exists(vmid);
            }
        }
    } else {
        // 范围 VMID
        if (get_vmid_range(&start, &end) == -1) {
            return 1;
        }
        
        // 如果是销毁操作，需要确认
        if (action == 2) {
            if (!confirm_destroy()) {
                printf(COLOR_YELLOW "操作取消。\n" COLOR_RESET);
                return 0;
            }
        }
        
        // 批量执行操作
        for (int i = start; i <= end; i++) {
            if (action == 1) {
                // 仅停止
                printf(COLOR_CYAN "正在停止 VM %d...\n" COLOR_RESET, i);
                if (execute_qm_stop(i) == 0) {
                    printf(COLOR_GREEN "VM %d 已停止成功。\n" COLOR_RESET, i);
                    success_count++;
                } else {
                    printf(COLOR_YELLOW "VM %d 停止失败（可能已停止或不存在）。\n" COLOR_RESET, i);
                    check_vm_exists(i);
                    fail_count++;
                }
            } else {
                // 停止并销毁
                printf(COLOR_CYAN "正在停止并销毁 VM %d...\n" COLOR_RESET, i);
                int stop_result = execute_qm_stop(i);
                int destroy_result = execute_qm_destroy(i);
                
                if (stop_result == 0 && destroy_result == 0) {
                    printf(COLOR_GREEN "VM %d 已停止并销毁成功。\n" COLOR_RESET, i);
                    success_count++;
                } else {
                    printf(COLOR_YELLOW "VM %d 停止并销毁失败（检查是否已停止）。\n" COLOR_RESET, i);
                    if (stop_result != 0) {
                        printf(COLOR_RED "  - stop 步骤失败。\n" COLOR_RESET);
                    }
                    if (destroy_result != 0) {
                        printf(COLOR_RED "  - destroy 步骤失败。\n" COLOR_RESET);
                    }
                    check_vm_exists(i);
                    fail_count++;
                }
            }
        }
        
        // 显示统计
        printf("\n" COLOR_BLUE "=== 操作统计 ===" COLOR_RESET "\n");
        printf(COLOR_GREEN "成功: %d\n" COLOR_RESET, success_count);
        if (fail_count > 0) {
            printf(COLOR_RED "失败: %d\n" COLOR_RESET, fail_count);
        }
    }
    
    printf(COLOR_GREEN "\n操作完成！\n" COLOR_RESET);
    return 0;
}

void print_header(void) {
    printf(COLOR_BLUE "╔════════════════════════════════════════════════════╗\n" COLOR_RESET);
    printf(COLOR_BLUE "║                  " COLOR_CYAN "VM Manager v1.0" COLOR_BLUE "                   ║\n" COLOR_RESET);
    printf(COLOR_BLUE "║                                                    ║\n" COLOR_RESET);
    printf(COLOR_BLUE "║             " COLOR_RESET "Design by YXWA Infosec Lab" COLOR_BLUE "             ║\n" COLOR_RESET);
    printf(COLOR_BLUE "╚════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
}

int get_action_choice(void) {
    char input[MAX_INPUT];
    
    printf("请选择操作：\n");
    printf("  1. 仅停止 VM (qm stop)\n");
    printf("  2. 停止并销毁 VM (qm stop && qm destroy --purge)\n");
    printf("输入选项 (1 或 2): ");
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1;
    }
    
    trim_string(input);
    
    if (strcmp(input, "1") == 0) {
        return 1;
    } else if (strcmp(input, "2") == 0) {
        return 2;
    }
    
    return -1;
}

int get_input_type(void) {
    char input[MAX_INPUT];
    
    printf("\n请选择输入方式：\n");
    printf("  1. 单个 VMID (例如: 112)\n");
    printf("  2. 范围 VMID (例如: 111-113)\n");
    printf("输入选项 (1 或 2): ");
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1;
    }
    
    trim_string(input);
    
    if (strcmp(input, "1") == 0) {
        return 1;
    } else if (strcmp(input, "2") == 0) {
        return 2;
    }
    
    return -1;
}

int get_single_vmid(void) {
    char input[MAX_INPUT];
    
    printf("请输入单个 VMID: ");
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1;
    }
    
    trim_string(input);
    
    if (!is_number(input)) {
        return -1;
    }
    
    return atoi(input);
}

int get_vmid_range(int *start, int *end) {
    char input[MAX_INPUT];
    char *dash_pos;
    char start_str[MAX_INPUT];
    char end_str[MAX_INPUT];
    
    printf("请输入范围 VMID (格式: start-end, 例如 111-113): ");
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf(COLOR_RED "读取输入失败！\n" COLOR_RESET);
        return -1;
    }
    
    trim_string(input);
    
    // 查找连字符
    dash_pos = strchr(input, '-');
    if (dash_pos == NULL) {
        printf(COLOR_RED "范围格式无效！退出。\n" COLOR_RESET);
        return -1;
    }
    
    // 分割起始和结束
    size_t start_len = dash_pos - input;
    strncpy(start_str, input, start_len);
    start_str[start_len] = '\0';
    strcpy(end_str, dash_pos + 1);
    
    trim_string(start_str);
    trim_string(end_str);
    
    // 验证是否为数字
    if (!is_number(start_str) || !is_number(end_str)) {
        printf(COLOR_RED "范围格式无效！退出。\n" COLOR_RESET);
        return -1;
    }
    
    *start = atoi(start_str);
    *end = atoi(end_str);
    
    // 验证范围
    if (*start > *end) {
        printf(COLOR_RED "起始 ID 必须小于结束 ID！退出。\n" COLOR_RESET);
        return -1;
    }
    
    return 0;
}

int confirm_destroy(void) {
    char input[MAX_INPUT];
    
    printf(COLOR_YELLOW "\n警告：销毁操作不可逆！确认继续？(y/N): " COLOR_RESET);
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }
    
    trim_string(input);
    
    if (strlen(input) > 0 && (input[0] == 'y' || input[0] == 'Y')) {
        return 1;
    }
    
    return 0;
}

int execute_qm_stop(int vmid) {
    char command[MAX_COMMAND];
    
    snprintf(command, sizeof(command), "qm stop %d >/dev/null 2>&1", vmid);
    return system(command);
}

int execute_qm_destroy(int vmid) {
    char command[MAX_COMMAND];
    
    snprintf(command, sizeof(command), "qm destroy %d --purge >/dev/null 2>&1", vmid);
    return system(command);
}

int check_vm_exists(int vmid) {
    char command[MAX_COMMAND];
    int result;
    
    snprintf(command, sizeof(command), "qm list | grep -q '^ *%d '", vmid);
    result = system(command);
    
    if (result != 0) {
        printf(COLOR_RED "  VM %d 不存在。\n" COLOR_RESET, vmid);
        return 0;
    }
    
    return 1;
}

void trim_string(char *str) {
    char *start = str;
    char *end;
    
    // 去除前导空格
    while (isspace((unsigned char)*start)) {
        start++;
    }
    
    // 如果全是空格
    if (*start == 0) {
        *str = 0;
        return;
    }
    
    // 去除尾随空格和换行符
    end = start + strlen(start) - 1;
    while (end > start && (isspace((unsigned char)*end) || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    // 写入新的结尾
    *(end + 1) = 0;
    
    // 移动字符串到开头
    if (start != str) {
        memmove(str, start, end - start + 2);
    }
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
