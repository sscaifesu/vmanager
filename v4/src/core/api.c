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
    
    cJSON_Delete(response);
    return 0;
}

int api_vm_action(int vmid, const char *action) {
    if (!action) return -1;
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d/status/%s",
             api_config->node, vmid, action);
    
    cJSON *response = api_post(endpoint, NULL);
    if (!response) return -1;
    
    cJSON_Delete(response);
    return 0;
}

void api_cleanup(void) {
    // 清理工作（当前使用 curl 命令，无需清理）
}
