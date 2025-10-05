// 新的 list_vms_remote 函数 - 使用 cJSON
int list_vms_remote_new(Config *config, int verbose) {
    char command[MAX_COMMAND];
    FILE *fp;
    char line[4096];
    int debug_mode = (getenv("VMANAGER_DEBUG") != NULL);
    
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
    
    // 使用 cJSON 解析
    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        fprintf(stderr, COLOR_RED "错误：JSON 解析失败\n" COLOR_RESET);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "解析错误位置: %s\n", error_ptr);
        }
        return -1;
    }
    
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsArray(data)) {
        fprintf(stderr, COLOR_RED "错误：API 响应格式错误\n" COLOR_RESET);
        cJSON_Delete(root);
        return -1;
    }
    
    int vm_count = cJSON_GetArraySize(data);
    if (vm_count == 0) {
        printf(COLOR_YELLOW "没有找到虚拟机\n" COLOR_RESET);
        cJSON_Delete(root);
        return 0;
    }
    
    // 打印表头
    printf(COLOR_BOLD "%-10s %-20s %-12s %-10s %-15s %-12s %-20s\n" COLOR_RESET,
           "VMID", "NAME", "STATUS", "MEM(MB)", "BOOTDISK(GB)", "BRIDGE", "IP ADDRESS");
    printf("─────────────────────────────────────────────────────────────────────────────────────────────────\n");
    
    // 遍历每个 VM
    cJSON *vm = NULL;
    cJSON_ArrayForEach(vm, data) {
        int vmid = cjson_get_int(vm, "vmid", 0);
        const char *name = cjson_get_string(vm, "name", "N/A");
        const char *status = cjson_get_string(vm, "status", "N/A");
        long long maxmem = (long long)cjson_get_double(vm, "maxmem", 0);
        int mem_mb = (int)(maxmem / 1024 / 1024);
        
        char bootdisk[32] = "N/A";
        char bridge[32] = "N/A";
        char ipaddr[64] = "N/A";
        
        // 详细模式：获取额外信息
        if (verbose && vmid > 0) {
            // 获取 VM 配置
            char config_cmd[MAX_COMMAND];
            snprintf(config_cmd, sizeof(config_cmd),
                    "curl -s -k 'https://%s:%d/api2/json/nodes/%s/qemu/%d/config' "
                    "-H 'Authorization: PVEAPIToken=%s=%s'",
                    config->host, config->port, config->node, vmid,
                    config->token_id, config->token_secret);
            
            FILE *config_fp = popen(config_cmd, "r");
            if (config_fp) {
                char config_response[32768] = {0};
                size_t config_total = 0;
                while (fgets(line, sizeof(line), config_fp) != NULL && config_total < sizeof(config_response) - 1) {
                    size_t len = strlen(line);
                    if (config_total + len < sizeof(config_response)) {
                        strcat(config_response, line);
                        config_total += len;
                    }
                }
                pclose(config_fp);
                
                // 解析配置 JSON
                cJSON *config_root = cJSON_Parse(config_response);
                if (config_root) {
                    cJSON *config_data = cJSON_GetObjectItem(config_root, "data");
                    if (config_data) {
                        // 提取 bootdisk
                        const char *bootdisk_key = cjson_get_string(config_data, "bootdisk", NULL);
                        if (bootdisk_key) {
                            const char *disk_value = cjson_get_string(config_data, bootdisk_key, NULL);
                            if (disk_value) {
                                // 解析 size=2252M
                                const char *size_ptr = strstr(disk_value, "size=");
                                if (size_ptr) {
                                    float size_val = 0;
                                    char size_unit[8] = "";
                                    if (sscanf(size_ptr, "size=%f%7s", &size_val, size_unit) >= 1) {
                                        // 转换为 GB
                                        if (strcasecmp(size_unit, "G") == 0 || strcasecmp(size_unit, "GB") == 0) {
                                            snprintf(bootdisk, sizeof(bootdisk), "%.2f", size_val);
                                        } else if (strcasecmp(size_unit, "M") == 0 || strcasecmp(size_unit, "MB") == 0) {
                                            snprintf(bootdisk, sizeof(bootdisk), "%.2f", size_val / 1024.0);
                                        } else if (strcasecmp(size_unit, "T") == 0 || strcasecmp(size_unit, "TB") == 0) {
                                            snprintf(bootdisk, sizeof(bootdisk), "%.2f", size_val * 1024.0);
                                        }
                                    }
                                }
                            }
                        }
                        
                        // 提取 bridge
                        const char *net0 = cjson_get_string(config_data, "net0", NULL);
                        if (net0) {
                            const char *bridge_ptr = strstr(net0, "bridge=");
                            if (bridge_ptr) {
                                sscanf(bridge_ptr, "bridge=%31[^,\"]", bridge);
                            }
                        }
                    }
                    cJSON_Delete(config_root);
                }
            }
            
            // 获取 IP 地址（仅当 running 时）
            if (strcmp(status, "running") == 0) {
                char ip_cmd[MAX_COMMAND];
                snprintf(ip_cmd, sizeof(ip_cmd),
                        "curl -s -k 'https://%s:%d/api2/json/nodes/%s/qemu/%d/agent/network-get-interfaces' "
                        "-H 'Authorization: PVEAPIToken=%s=%s' 2>/dev/null",
                        config->host, config->port, config->node, vmid,
                        config->token_id, config->token_secret);
                
                FILE *ip_fp = popen(ip_cmd, "r");
                if (ip_fp) {
                    char ip_response[16384] = {0};
                    size_t ip_total = 0;
                    while (fgets(line, sizeof(line), ip_fp) != NULL && ip_total < sizeof(ip_response) - 1) {
                        size_t len = strlen(line);
                        if (ip_total + len < sizeof(ip_response)) {
                            strcat(ip_response, line);
                            ip_total += len;
                        }
                    }
                    pclose(ip_fp);
                    
                    // 解析 IP JSON
                    cJSON *ip_root = cJSON_Parse(ip_response);
                    if (ip_root) {
                        cJSON *ip_data = cJSON_GetObjectItem(ip_root, "data");
                        if (cJSON_IsArray(ip_data)) {
                            cJSON *iface = NULL;
                            cJSON_ArrayForEach(iface, ip_data) {
                                cJSON *ip_addresses = cJSON_GetObjectItem(iface, "ip-addresses");
                                if (cJSON_IsArray(ip_addresses)) {
                                    cJSON *ip_addr = NULL;
                                    cJSON_ArrayForEach(ip_addr, ip_addresses) {
                                        const char *ip = cjson_get_string(ip_addr, "ip-address", NULL);
                                        const char *ip_type = cjson_get_string(ip_addr, "ip-address-type", NULL);
                                        
                                        if (ip && ip_type && strcmp(ip_type, "ipv4") == 0) {
                                            if (strncmp(ip, "127.", 4) != 0) {
                                                strncpy(ipaddr, ip, sizeof(ipaddr) - 1);
                                                ipaddr[sizeof(ipaddr) - 1] = '\0';
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (strcmp(ipaddr, "N/A") != 0) break;
                            }
                        }
                        cJSON_Delete(ip_root);
                    }
                }
            }
        }
        
        // 打印 VM 信息
        printf("%-10d %-20s %-12s %-10d %-15s %-12s %-20s\n",
               vmid, name, status, mem_mb, bootdisk, bridge, ipaddr);
    }
    
    cJSON_Delete(root);
    return 0;
}
