/*
 * Proxmox API 封装
 * 使用 libcurl 进行 HTTP 请求
 */

#define _POSIX_C_SOURCE 200809L
#include "../../include/vmanager.h"
#include <curl/curl.h>

static Config *api_config = NULL;
static CURL *curl_handle = NULL;

// libcurl 写入回调函数
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "错误：内存分配失败\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

int api_init(Config *config) {
    if (!config) return -1;
    api_config = config;
    
    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    
    if (!curl_handle) {
        fprintf(stderr, "错误：libcurl 初始化失败\n");
        return -1;
    }
    
    return 0;
}

// 执行 HTTP GET 请求并返回 JSON
cJSON* api_get(const char *endpoint) {
    if (!api_config || !endpoint || !curl_handle) return NULL;
    
    // 构建 URL
    char url[1024];
    snprintf(url, sizeof(url), "https://%s:%d%s",
             api_config->host, api_config->port, endpoint);
    
    // 构建认证头
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header),
             "Authorization: PVEAPIToken=%s=%s",
             api_config->token_id, api_config->token_secret);
    
    if (g_debug) {
        fprintf(stderr, "API GET: %s\n", endpoint);
    }
    
    // 设置 HTTP 头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    
    // 准备接收数据
    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    // 配置 curl
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    // 执行请求
    CURLcode res = curl_easy_perform(curl_handle);
    
    cJSON *json = NULL;
    if (res != CURLE_OK) {
        if (g_debug) {
            fprintf(stderr, "curl_easy_perform() 失败: %s\n", curl_easy_strerror(res));
        }
    } else {
        json = cJSON_Parse(chunk.memory);
        if (!json && g_debug) {
            fprintf(stderr, "JSON 解析失败: %s\n", chunk.memory);
        }
    }
    
    free(chunk.memory);
    curl_slist_free_all(headers);
    
    return json;
}

cJSON* api_post(const char *endpoint, cJSON *data) {
    (void)data;  // 未使用的参数，保留用于未来扩展
    if (!api_config || !endpoint || !curl_handle) return NULL;
    
    // 构建 URL
    char url[1024];
    snprintf(url, sizeof(url), "https://%s:%d%s",
             api_config->host, api_config->port, endpoint);
    
    // 构建认证头
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header),
             "Authorization: PVEAPIToken=%s=%s",
             api_config->token_id, api_config->token_secret);
    
    if (g_debug) {
        fprintf(stderr, "API POST: %s\n", endpoint);
    }
    
    // 设置 HTTP 头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    
    // 准备接收数据
    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    // 配置 curl
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    // 执行请求
    CURLcode res = curl_easy_perform(curl_handle);
    
    cJSON *json = NULL;
    if (res != CURLE_OK) {
        if (g_debug) {
            fprintf(stderr, "curl_easy_perform() 失败: %s\n", curl_easy_strerror(res));
        }
    } else {
        json = cJSON_Parse(chunk.memory);
        if (!json && g_debug) {
            fprintf(stderr, "JSON 解析失败: %s\n", chunk.memory);
        }
    }
    
    free(chunk.memory);
    curl_slist_free_all(headers);
    
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
        
        // 获取状态，优先检查 qmpstatus（更准确）
        const char *qmpstatus = json_get_string(vm_json, "qmpstatus", NULL);
        const char *status = json_get_string(vm_json, "status", "N/A");
        
        // 如果 qmpstatus 是 paused，显示为 paused
        if (qmpstatus && strcmp(qmpstatus, "paused") == 0) {
            strncpy(vm->status, "paused", sizeof(vm->status) - 1);
        } else if (qmpstatus && strcmp(qmpstatus, "stopped") == 0) {
            strncpy(vm->status, "stopped", sizeof(vm->status) - 1);
        } else {
            strncpy(vm->status, status, sizeof(vm->status) - 1);
        }
        
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
    
    // 获取状态，优先检查 qmpstatus
    const char *qmpstatus = json_get_string(data, "qmpstatus", NULL);
    const char *status = json_get_string(data, "status", "N/A");
    
    if (qmpstatus && strcmp(qmpstatus, "paused") == 0) {
        strncpy(vm->status, "paused", sizeof(vm->status) - 1);
    } else if (qmpstatus && strcmp(qmpstatus, "stopped") == 0) {
        strncpy(vm->status, "stopped", sizeof(vm->status) - 1);
    } else {
        strncpy(vm->status, status, sizeof(vm->status) - 1);
    }
    
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
    if (!action || !curl_handle) return -1;
    
    char url[1024];
    char endpoint[256];
    bool is_destroy = (strcmp(action, "destroy") == 0);
    
    // destroy 操作使用 DELETE 方法
    if (is_destroy) {
        snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d",
                 api_config->node, vmid);
    } else {
        // 其他操作使用 POST 到 status/<action>
        snprintf(endpoint, sizeof(endpoint), "/api2/json/nodes/%s/qemu/%d/status/%s",
                 api_config->node, vmid, action);
    }
    
    snprintf(url, sizeof(url), "https://%s:%d%s",
             api_config->host, api_config->port, endpoint);
    
    // 构建认证头
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header),
             "Authorization: PVEAPIToken=%s=%s",
             api_config->token_id, api_config->token_secret);
    
    if (g_debug) {
        fprintf(stderr, "API %s: %s\n", is_destroy ? "DELETE" : "POST", endpoint);
    }
    
    // 设置 HTTP 头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    
    // 准备接收数据
    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    // 配置 curl
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    
    if (is_destroy) {
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else {
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
    }
    
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    // 执行请求
    CURLcode res = curl_easy_perform(curl_handle);
    
    int ret = -1;
    long http_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (res != CURLE_OK) {
        if (g_debug) {
            fprintf(stderr, "curl_easy_perform() 失败: %s\n", curl_easy_strerror(res));
        }
    } else {
        if (g_debug) {
            fprintf(stderr, "HTTP %s %ld: %s\n", is_destroy ? "DELETE" : "POST", http_code, endpoint);
            fprintf(stderr, "响应: %s\n", chunk.memory);
        }
        
        // 检查 HTTP 状态码
        if (http_code >= 200 && http_code < 300) {
            cJSON *json = cJSON_Parse(chunk.memory);
            if (json) {
                // 检查是否有错误信息
                cJSON *errors = cJSON_GetObjectItem(json, "errors");
                if (errors && cJSON_IsObject(errors)) {
                    if (!g_tui_mode) {
                        fprintf(stderr, "API 返回错误\n");
                    }
                    ret = -1;
                } else {
                    // 对于异步操作（如 destroy），API 返回任务 ID
                    cJSON *data = cJSON_GetObjectItem(json, "data");
                    if (data) {
                        const char *upid = cJSON_GetStringValue(data);
                        if (upid && g_debug) {
                            fprintf(stderr, "任务 ID: %s\n", upid);
                        }
                    }
                    ret = 0;
                }
                cJSON_Delete(json);
            } else {
                if (g_debug) {
                    fprintf(stderr, "JSON 解析失败: %s\n", chunk.memory);
                }
            }
        } else {
            if (!g_tui_mode) {
                fprintf(stderr, "HTTP 错误: %ld\n", http_code);
            }
        }
    }
    
    free(chunk.memory);
    curl_slist_free_all(headers);
    
    // 重置 curl 选项以供下次使用
    if (is_destroy) {
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, NULL);
    }
    curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
    
    return ret;
}

void api_cleanup(void) {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
        curl_handle = NULL;
    }
    curl_global_cleanup();
}
