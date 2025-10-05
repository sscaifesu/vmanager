/*
 * JSON 辅助函数
 */

#include "../../include/vmanager.h"

const char* json_get_string(cJSON *json, const char *key, const char *default_val) {
    if (!json || !key) return default_val;
    
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsString(item)) {
        return item->valuestring;
    }
    return default_val;
}

int json_get_int(cJSON *json, const char *key, int default_val) {
    if (!json || !key) return default_val;
    
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return default_val;
}

double json_get_double(cJSON *json, const char *key, double default_val) {
    if (!json || !key) return default_val;
    
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsNumber(item)) {
        return item->valuedouble;
    }
    return default_val;
}

bool json_get_bool(cJSON *json, const char *key, bool default_val) {
    if (!json || !key) return default_val;
    
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return default_val;
}
