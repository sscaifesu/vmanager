/*
 * vmanager v4.0.0 - 主头文件
 */

#ifndef VMANAGER_H
#define VMANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../cJSON.h"

#define VERSION "4.0.0"
#define PROGRAM_NAME "vmanager"
#define MAX_VMIDS 100

// 配置结构
typedef struct {
    char host[256];
    int port;
    char node[64];
    char token_id[256];
    char token_secret[256];
    bool verify_ssl;
    char config_file[512];
} Config;

// VM 信息结构
typedef struct {
    int vmid;
    char name[128];
    char status[32];
    int cpus;
    uint64_t maxmem;      // bytes
    uint64_t mem;         // bytes
    uint64_t maxdisk;     // bytes
    uint64_t disk;        // bytes
    double cpu_percent;
    char ip_address[64];
    char bridge[32];
    char bootdisk[64];
    int uptime;           // seconds
} VMInfo;

// 执行模式
typedef enum {
    MODE_AUTO,
    MODE_LOCAL,
    MODE_REMOTE
} ExecutionMode;

// UI 模式
typedef enum {
    UI_CLI,
    UI_TUI
} UIMode;

// 命令类型
typedef enum {
    CMD_NONE,
    CMD_LIST,
    CMD_STATUS,
    CMD_START,
    CMD_STOP,
    CMD_RESTART,
    CMD_SUSPEND,
    CMD_RESUME,
    CMD_DESTROY,
    CMD_MONITOR,
    CMD_CLONE,
    CMD_SNAPSHOT
} Command;

// 全局配置
extern Config g_config;
extern ExecutionMode g_exec_mode;
extern UIMode g_ui_mode;
extern bool g_debug;

// core/api.c
int api_init(Config *config);
cJSON* api_get(const char *endpoint);
cJSON* api_post(const char *endpoint, cJSON *data);
int api_get_vm_list(VMInfo **vms, int *count);
int api_get_vm_status(int vmid, VMInfo *vm);
int api_vm_action(int vmid, const char *action);
void api_cleanup(void);

// core/config.c
int config_load(Config *config, const char *file);
int config_save(Config *config, const char *file);
int config_wizard(Config *config);
bool is_on_pve_server(void);

// core/vm.c
int vm_list(bool verbose);
int vm_status(int vmid);
int vm_start(int vmid);
int vm_stop(int vmid);
int vm_restart(int vmid);
int vm_suspend(int vmid);
int vm_resume(int vmid);
int vm_destroy(int vmid, bool force);

// ui/cli.c
int cli_main(int argc, char *argv[]);
void cli_print_vm_list(VMInfo *vms, int count, bool verbose);
void cli_print_vm_status(VMInfo *vm);

// ui/tui.c
int tui_main(void);
void tui_init(void);
void tui_cleanup(void);
void tui_refresh(void);

// utils/json.c
const char* json_get_string(cJSON *json, const char *key, const char *default_val);
int json_get_int(cJSON *json, const char *key, int default_val);
double json_get_double(cJSON *json, const char *key, double default_val);
bool json_get_bool(cJSON *json, const char *key, bool default_val);

// utils/logger.c
void log_init(const char *file);
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_cleanup(void);

// utils/common.c
bool is_number(const char *str);
int parse_vmid_range(const char *range, int *vmids, int *count);
char* format_bytes(uint64_t bytes);
char* format_uptime(int seconds);

#endif // VMANAGER_H
