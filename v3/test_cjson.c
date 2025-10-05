#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

int main() {
    const char *json_str = "{\"data\":[{\"vmid\":111,\"name\":\"srv-web-25c201\",\"status\":\"running\",\"maxmem\":4294967296}]}";
    
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        printf("JSON 解析失败\n");
        return 1;
    }
    
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (cJSON_IsArray(data)) {
        cJSON *vm = NULL;
        cJSON_ArrayForEach(vm, data) {
            int vmid = cJSON_GetObjectItem(vm, "vmid")->valueint;
            const char *name = cJSON_GetObjectItem(vm, "name")->valuestring;
            const char *status = cJSON_GetObjectItem(vm, "status")->valuestring;
            long long maxmem = cJSON_GetObjectItem(vm, "maxmem")->valuedouble;
            
            printf("VMID: %d, Name: %s, Status: %s, Mem: %lld MB\n", 
                   vmid, name, status, maxmem / 1024 / 1024);
        }
    }
    
    cJSON_Delete(root);
    printf("✓ cJSON 测试成功！\n");
    return 0;
}
