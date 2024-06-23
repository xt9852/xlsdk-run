/**
 *\file     config.c
 *\note     UTF-8
 *\author   xt
 *\version  1.0.0
 *\date     2022.02.08
 *\brief    配置模块实现
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "config.h"
#include "cJSON.h"
#include "xt_log.h"

/**
 *\brief        得到文件大小
 *\param[in]    filename    文件名
 *\return       文件大小,小于0时失败
 */
int config_get_size(const char *filename)
{
    struct _stat buf;

    return (_stat(filename, &buf) == 0) ? buf.st_size : -1;
}

/**
 *\brief        加载配置文件数据
 *\param[in]    filename    文件名
 *\param[in]    buf         数据指针
 *\param[in]    len         数据长度
 *\return       0           成功
 */
int config_get_data(const char *filename, char *buf, int len)
{
    if (NULL == filename || NULL == buf)
    {
        printf("%s|filename, buf is null", __FUNCTION__);
        return -1;
    }

    FILE *fp = NULL;

    if (0 != fopen_s(&fp, filename, "rb"))
    {
        printf("%s|open %s fail", __FUNCTION__, filename);
        return -2;
    }

    if (len != fread(buf, 1, len, fp))
    {
        printf("%s|read %s fail", __FUNCTION__, filename);
        fclose(fp);
        return -3;
    }

    fclose(fp);
    return 0;
}

/**
 *\brief        得到JSON数据指针
 *\param[in]    filename    文件名
 *\param[out]   root        JSON数据指针
 *\return       0           成功
 */
int config_get_json(const char *filename, cJSON **root)
{
    if (NULL == filename)
    {
        return -1;
    }

    int size = config_get_size(filename);

    if (size <= 0)
    {
        printf("%s|get file %s size error\n", __FUNCTION__, filename);
        return -2;
    }

    char *buff = (char*)malloc(size + 16);

    if (NULL == buff)
    {
        printf("%s|malloc %d fail\n", __FUNCTION__, size + 16);
        return -3;
    }

    int ret = config_get_data(filename, buff, size);

    *root = (0 == ret) ? cJSON_Parse(buff) : NULL;

    free(buff);

    if (NULL == *root)
    {
        printf("%s|parse json fail\n", __FUNCTION__);
        return -5;
    }

    return 0;
}

/**
 *\brief        解析log节点数据
 *\param[in]    root        JSON根节点
 *\param[out]   log         日志数据
 *\return       0           成功
 */
int config_log(cJSON *root, p_xt_log log)
{
    if (NULL == root || NULL == log)
    {
        return -1;
    }

    cJSON *item = cJSON_GetObjectItem(root, "log");

    if (NULL == item)
    {
        printf("%s|config json no log node\n", __FUNCTION__);
        return -2;
    }

    cJSON *name = cJSON_GetObjectItem(item, "name");

    if (NULL == name)
    {
        printf("%s|config json no log.name node\n", __FUNCTION__);
        return -3;
    }

    strncpy_s(log->filename, sizeof(log->filename), name->valuestring, sizeof(log->filename) - 1);

    cJSON *level = cJSON_GetObjectItem(item, "level");

    if (NULL == level)
    {
        printf("%s|config json no log.level node\n", __FUNCTION__);
        return -4;
    }

    if (0 == strcmp(level->valuestring, "debug"))
    {
        log->level = LOG_LEVEL_DEBUG;
    }
    else if (0 == strcmp(level->valuestring, "info"))
    {
        log->level = LOG_LEVEL_INFO;
    }
    else if (0 == strcmp(level->valuestring, "warn"))
    {
        log->level = LOG_LEVEL_WARN;
    }
    else if (0 == strcmp(level->valuestring, "error"))
    {
        log->level = LOG_LEVEL_ERROR;
    }
    else
    {
        printf("%s|config json no log.level value error\n", __FUNCTION__);
        return -5;
    }

    cJSON *cycle = cJSON_GetObjectItem(item, "cycle");

    if (NULL == cycle)
    {
        printf("%s|config json no log.cycle node\n", __FUNCTION__);
        return -6;
    }

    if (0 == strcmp(cycle->valuestring, "minute"))
    {
        log->cycle = LOG_CYCLE_MINUTE;
    }
    else if (0 == strcmp(cycle->valuestring, "hour"))
    {
        log->cycle = LOG_CYCLE_HOUR;
    }
    else if (0 == strcmp(cycle->valuestring, "day"))
    {
        log->cycle = LOG_CYCLE_DAY;
    }
    else if (0 == strcmp(cycle->valuestring, "week"))
    {
        log->cycle = LOG_CYCLE_WEEK;
    }
    else
    {
        printf("%s|config no log.cycle value error\n", __FUNCTION__);
        return -7;
    }

    cJSON *backup = cJSON_GetObjectItem(item, "backup");

    if (NULL == backup)
    {
        printf("%s|config no log.backup value error\n", __FUNCTION__);
        return -8;
    }

    log->backup = backup->valueint;

    cJSON *clean_log = cJSON_GetObjectItem(item, "clean_log");

    if (NULL == clean_log)
    {
        printf("%s|config no log.clean_log value error\n", __FUNCTION__);
        return -9;
    }

    log->clean_log = clean_log->valueint;


    cJSON *clean_file = cJSON_GetObjectItem(item, "clean_file");

    if (NULL == clean_file)
    {
        printf("%s|config no log.clean_file value error\n", __FUNCTION__);
        return -9;
    }

    log->clean_file = clean_file->valueint;

    return 0;
}

/**
 *\brief        解析http节点数据
 *\param[in]    root        JSON根节点
 *\param[out]   config      配置数据
 *\return       0           成功
 */
int config_http(cJSON *root, p_config config)
{
    if (NULL == root || NULL == config)
    {
        return -1;
    }

    cJSON *http = cJSON_GetObjectItem(root, "http");

    if (NULL == http)
    {
        printf("%s|config json no http node\n", __FUNCTION__);
        return -2;
    }

    cJSON *ip = cJSON_GetObjectItem(http, "ip");

    if (NULL == ip)
    {
        printf("%s|config json no http.ip node\n", __FUNCTION__);
        return -3;
    }

    strncpy_s(config->http_ip, sizeof(config->http_ip), ip->valuestring, sizeof(config->http_ip) - 1);

    cJSON *port = cJSON_GetObjectItem(http, "port");

    if (NULL == port)
    {
        printf("%s|config json no http.port node\n", __FUNCTION__);
        return -4;
    }

    config->http_port = port->valueint;

    return 0;
}

/**
 *\brief        解析download节点数据
 *\param[in]    root        JSON根节点
 *\param[out]   config      配置数据
 *\return       0           成功
 */
int config_path(cJSON *root, p_config config)
{
    if (NULL == root || NULL == config)
    {
        return -1;
    }

    cJSON *path = cJSON_GetObjectItem(root, "path");

    if (NULL == path)
    {
        printf("%s|config json no path node\n", __FUNCTION__);
        return -2;
    }

    cJSON *download = cJSON_GetObjectItem(path, "download");

    if (NULL == download)
    {
        printf("%s|config json no path.download node\n", __FUNCTION__);
        return -3;
    }

    strcpy_s(config->path_download, sizeof(config->path_download), download->valuestring);

    cJSON *move = cJSON_GetObjectItem(path, "move");

    if (NULL == move)
    {
        printf("%s|config json no path.move node\n", __FUNCTION__);
        return -4;
    }

    strcpy_s(config->path_move, sizeof(config->path_move), move->valuestring);

    return 0;
}

/**
 *\brief        初始化配置
 *\param[in]    filename    配置文件名
 *\param[out]   config      配置数据
 *\return       0           成功
 */
int config_init(const char *filename, p_config config)
{
    if (NULL == filename || NULL == config)
    {
        printf("%s|filename is null\n", __FUNCTION__);
        return -1;
    }

    cJSON *root;

    int ret = config_get_json(filename, &root);

    if (0 != ret)
    {
        printf("%s|get config %s data fail\n", __FUNCTION__, filename);
        return -2;
    }

    if (0 != config_log(root, config->log))
    {
        printf("%s|config json log node error\n", __FUNCTION__);
        return -3;
    }

    if (0 != config_http(root, config))
    {
        printf("%s|config json http node error\n", __FUNCTION__);
        return -4;
    }

    if (0 != config_path(root, config))
    {
        printf("%s|config json path node Eor\n", __FUNCTION__);
        return -5;
    }

    printf("%s|config ok\n", __FUNCTION__);
    return 0;
}
