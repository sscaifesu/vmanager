/*
 * Proxmox API 封装
 * 使用 curl 命令行工具（兼容性更好）
 * 未来可以升级到 libcurl
 */

#define _POSIX_C_SOURCE 200809L
#include "../../include/vmanager.h"

static Config *api_config = NULL;

int api_init(Config *config) {
    if (!config) return -1;
    api_config = config;
    return 0;
}

// 执行 curl 命令并返回 JSON
cJSON* api_get(const char *endpoint) {
    if (!api_config || !endpoint) return NULL;
    
    char command[2048];
    snprintf(command, sizeof(command),
            "curl -s -k 'https://%s:%d%s' "
            "-H 'Authorization: PVEAPIToken=%s=%s'",
            api_config->host, api_config->port, endpoint,
            api_config->token_id, api_config->token_secret);
    
    if (g_debug) {
        fprintf(stderr, "API GET: %s\n", endpoint);
    }
    
    FILE *fp = popen(command, "r");
    if (!fp) return NULL;
    
    char *response = malloc(65536);
    if (!response) {
        pclose(fp);
        return NULL;
    }
    
    size_t total = 0;
    char line[4096];
    response[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) && total < 65535) {
        size_t len = strlen(line);
        if (total + len < 65535) {
            strcat(response, line);
            total += len;
        }
    }
    pclose(fp);
    
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    return json;
}

cJSON* api_post(const char *endpoint, cJSON *data) {
    (void)data;  // 未使用的参数，保留用于未来扩展
    if (!api_config || !endpoint) return NULL;
    
    char command[2048];
    snprintf(command, sizeof(command),
            "curl -s -k -X POST 'https://%s:%d%s' "
            "-H 'Authorization: PVEAPIToken=%s=%s'",
            api_config->host, api_config->port, endpoint,
            api_config->token_id, api_config->token_secret);
    
    if (g_debug) {
        fprintf(stderr, "API POST: %s\n", endpoint);
    }
    
    FILE *fp = popen(command, "r");
    if (!fp) return NULL;
    
    char *response = malloc(65536);
    if (!response) {
        pclose(fp);
        return NULL;
    }
    
    size_t total = 0;
    char line[4096];
    response[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) && total < 65535) {
        size_t len = strlen(line);
        if (total + len < 65535) {
            strcat(response, line);
            total += len;
        }
    }
    pclose(fp);
    
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    return json;
}

// 获取 VM 配置信息（网络、存储等）
int api_get_vm_config_details(int vmid, VMInfo *vm) {
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d/config",
             api_config->node, vmid);
    
    cJSON *response = api_get(endpoint);
    if (!response) return -1;
    
    cJSON *data = cJSON_GetObjectItem(response, "data");
    if (!data) {
        cJSON_Delete(response);
        return -1;
    }
    
    // 获取网桥信息
    const char *net0 = json_get_string(data, "net0", NULL);
    if (net0) {
        const char *bridge_start = strstr(net0, "bridge=");
        if (bridge_start) {
            sscanf(bridge_start, "bridge=%31[^,]", vm->bridge);
        }
    }
    
    // 获取存储信息（从 bootdisk 或第一个磁盘）
    const char *bootdisk = json_get_string(data, "bootdisk", NULL);
    if (bootdisk) {
        const char *disk_value = json_get_string(data, bootdisk, NULL);
        if (disk_value) {
            // 提取存储名称（例如：vmdata-1:vm-111-disk-0）
            char *colon = strchr(disk_value, ':');
            if (colon) {
                size_t len = colon - disk_value;
                if (len < sizeof(vm->storage)) {
                    strncpy(vm->storage, disk_value, len);
                    vm->storage[len] = '\0';
                }
            }
        }
    }
    
    // 设置配置文件路径
    snprintf(vm->config_file, sizeof(vm->config_file), 
             "/etc/pve/nodes/%s/qemu-server/%d.conf", api_config->node, vmid);
    
    cJSON_Delete(response);
    return 0;
}

// 获取 VM IP 地址（通过 qemu-guest-agent）
int api_get_vm_ip(int vmid, VMInfo *vm) {
    if (strcmp(vm->status, "running") != 0) {
        return 0; // 只有运行中的 VM 才能获取 IP
    }
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), 
             "/api2/json/nodes/%s/qemu/%d/agent/network-get-interfaces",
             api_config->node, vmid);
    
    cJSON *response = api_get(endpoint);
    if (!response) return -1;
    
    cJSON *data = cJSON_GetObjectItem(response, "data");
    if (!data) {
        cJSON_Delete(response);
        return -1;
    }
    
    // data 可能包含 result 数组
    cJSON *result = cJSON_GetObjectItem(data, "result");
    cJSON *interfaces = result ? result : data;
    
    if (!cJSON_IsArray(interfaces)) {
        cJSON_Delete(response);
        return -1;
    }
    
    // 遍历网络接口
    cJSON *iface = NULL;
    cJSON_ArrayForEach(iface, interfaces) {
        cJSON *ip_addresses = cJSON_GetObjectItem(iface, "ip-addresses");
        if (!cJSON_IsArray(ip_addresses)) continue;
        
        cJSON *ip_addr = NULL;
        cJSON_ArrayForEach(ip_addr, ip_addresses) {
            const char *ip = json_get_string(ip_addr, "ip-address", NULL);
            const char *ip_type = json_get_string(ip_addr, "ip-address-type", NULL);
            
            if (ip && ip_type && strcmp(ip_type, "ipv4") == 0) {
                // 跳过回环地址
                if (strncmp(ip, "127.", 4) != 0) {
                    strncpy(vm->ip_address, ip, sizeof(vm->ip_address) - 1);
                    cJSON_Delete(response);
                    return 0;
                }
            }
        }
    }
    
    cJSON_Delete(response);
    return 0;
}

int api_get_vm_list(VMInfo **vms, int *count) {
    if (!vms || !count) return -1;
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu", api_config->node);
    
    cJSON *response = api_get(endpoint);
    if (!response) return -1;
    
    cJSON *data = cJSON_GetObjectItem(response, "data");
    if (!cJSON_IsArray(data)) {
        cJSON_Delete(response);
        return -1;
    }
    
    int vm_count = cJSON_GetArraySize(data);
    *vms = calloc(vm_count, sizeof(VMInfo));
    *count = vm_count;
    
    int i = 0;
    cJSON *vm_json = NULL;
    cJSON_ArrayForEach(vm_json, data) {
        VMInfo *vm = &(*vms)[i++];
        vm->vmid = json_get_int(vm_json, "vmid", 0);
        strncpy(vm->name, json_get_string(vm_json, "name", "N/A"), sizeof(vm->name) - 1);
        strncpy(vm->status, json_get_string(vm_json, "status", "N/A"), sizeof(vm->status) - 1);
        vm->cpus = json_get_int(vm_json, "cpus", 0);
        vm->maxmem = (uint64_t)json_get_double(vm_json, "maxmem", 0);
        vm->mem = (uint64_t)json_get_double(vm_json, "mem", 0);
        vm->maxdisk = (uint64_t)json_get_double(vm_json, "maxdisk", 0);
        vm->disk = (uint64_t)json_get_double(vm_json, "disk", 0);
        vm->cpu_percent = json_get_double(vm_json, "cpu", 0) * 100;
        vm->uptime = json_get_int(vm_json, "uptime", 0);
        
        // 初始化新字段
        strcpy(vm->ip_address, "N/A");
        strcpy(vm->bridge, "N/A");
        strcpy(vm->storage, "N/A");
        vm->config_file[0] = '\0';
    }
    
    cJSON_Delete(response);
    return 0;
}

int api_get_vm_status(int vmid, VMInfo *vm) {
    if (!vm) return -1;
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d/status/current",
             api_config->node, vmid);
    
    cJSON *response = api_get(endpoint);
    if (!response) return -1;
    
    cJSON *data = cJSON_GetObjectItem(response, "data");
    if (!data) {
        cJSON_Delete(response);
        return -1;
    }
    
    vm->vmid = vmid;
    strncpy(vm->name, json_get_string(data, "name", "N/A"), sizeof(vm->name) - 1);
    strncpy(vm->status, json_get_string(data, "status", "N/A"), sizeof(vm->status) - 1);
    vm->cpus = json_get_int(data, "cpus", 0);
    vm->maxmem = (uint64_t)json_get_double(data, "maxmem", 0);
    vm->mem = (uint64_t)json_get_double(data, "mem", 0);
    vm->maxdisk = (uint64_t)json_get_double(data, "maxdisk", 0);
    vm->disk = (uint64_t)json_get_double(data, "disk", 0);
    vm->cpu_percent = json_get_double(data, "cpu", 0) * 100;
    vm->uptime = json_get_int(data, "uptime", 0);
    
    // 初始化新字段
    strcpy(vm->ip_address, "N/A");
    strcpy(vm->bridge, "N/A");
    strcpy(vm->storage, "N/A");
    vm->config_file[0] = '\0';
    
    cJSON_Delete(response);
    
    // 获取配置详情（网络、存储等）
    api_get_vm_config_details(vmid, vm);
    
    // 获取 IP 地址（如果 VM 正在运行）
    api_get_vm_ip(vmid, vm);
    
    return 0;
}

int api_vm_action(int vmid, const char *action) {
    if (!action) return -1;
    
    char command[2048];
    char endpoint[256];
    
    // destroy 操作需要使用 DELETE 方法
    if (strcmp(action, "destroy") == 0) {
        snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d",
                 api_config->node, vmid);
        snprintf(command, sizeof(command),
                "curl -s -k -X DELETE 'https://%s:%d%s' "
                "-H 'Authorization: PVEAPIToken=%s=%s'",
                api_config->host, api_config->port, endpoint,
                api_config->token_id, api_config->token_secret);
    } else {
        // 其他操作使用 POST 到 status/<action>
        snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d/status/%s",
                 api_config->node, vmid, action);
        snprintf(command, sizeof(command),
                "curl -s -k -X POST 'https://%s:%d%s' "
                "-H 'Authorization: PVEAPIToken=%s=%s'",
                api_config->host, api_config->port, endpoint,
                api_config->token_id, api_config->token_secret);
    }
    
    if (g_debug) {
        fprintf(stderr, "API %s: %s\n", 
                strcmp(action, "destroy") == 0 ? "DELETE" : "POST", endpoint);
    }
    
    FILE *fp = popen(command, "r");
    if (!fp) return -1;
    
    char *response = malloc(65536);
    if (!response) {
        pclose(fp);
        return -1;
    }
    
    size_t total = 0;
    char line[4096];
    response[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) && total < 65535) {
        size_t len = strlen(line);
        if (total + len < 65535) {
            strcat(response, line);
            total += len;
        }
    }
    pclose(fp);
    
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    if (!json) return -1;
    
    cJSON_Delete(json);
    return 0;
}

void api_cleanup(void) {
    // 清理工作（当前使用 curl 命令，无需清理）
}
