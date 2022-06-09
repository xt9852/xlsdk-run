/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         config.c
 *\author       xt
 *\version      1.0.0
 *\date         2022-02-08
 *\brief        配置模块实现,UTF-8(No BOM)
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "config.h"
#include "xt_log.h"
#include "cJSON.h"

static char g_config_download_path[512];    ///< 下载路径

/**
 *\brief        得到文件大小
 *\param[in]    filename    文件名
 *\return       文件大小,小于0时失败
 */
int get_file_size(const char *filename)
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
int get_config_data(const char *filename, char *buf, int len)
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
 *\brief        初始化配置
 *\param[in]    filename    配置文件名
 *\return       0           成功
 */
int config_init(const char *filename)
{
    if (NULL == filename)
    {
        printf("%s|filename is null\n", __FUNCTION__);
        return -1;
    }

    int size = get_file_size(filename);

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

    int ret = get_config_data(filename, buff, size);

    if (ret != 0)
    {
        printf("%s|get config %s data fail\n", __FUNCTION__, filename);
        free(buff);
        return -4;
    }

    cJSON *root = cJSON_Parse(buff);

    if (NULL == root)
    {
        printf("%s|parse json fail\n", __FUNCTION__);
        free(buff);
        return -5;
    }

    cJSON *log_root = cJSON_GetObjectItem(root, "log");

    if (NULL == log_root)
    {
        printf("%s|config json no log.file node\n", __FUNCTION__);
        return -6;
    }

    ret = xt_log_parse_config(".\\", log_root);

    if (ret != 0)
    {
        printf("%s|parse config fail\n", __FUNCTION__);
        return -7;
    }

    ret = xt_log_init();

    if (ret != 0)
    {
        printf("%s|log init fail\n", __FUNCTION__);
        return -8;
    }


    cJSON *path = cJSON_GetObjectItem(root, "path");

    if (NULL == path)
    {
        DBG("config json no path node\n");
        return -3;
    }

    strcpy_s(g_config_download_path, sizeof(g_config_download_path), path->valuestring);

    DBG("ok");
    return 0;
}

/**
 *\brief      得到下载路径
 *\return     下载路径
 */
const char * config_get_download_path()
{
    return g_config_download_path;
}