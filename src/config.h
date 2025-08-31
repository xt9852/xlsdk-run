/**
 *\file     config.h
 *\note     UTF-8
 *\author   xt
 *\version  1.0.0
 *\date     2022.02.08
 *\brief    配置模块定义
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "xt_log.h"


typedef struct _config                  ///  配置数据,配置文件应为utf8
{
    p_xt_log  log;                      ///< 日志

    char    http_ip[MAX_PATH];          ///< HTTP地址
    int     http_port;                  ///< HTTP端口

    char    path_tmp[MAX_PATH];         ///< 临时路径
    char    path_download[MAX_PATH];    ///< 下载路径

} config, *p_config;

/**
 *\brief                    初始化配置
 *\param[in]    filename    配置文件名
 *\param[out]   config      配置数据
 *\return       0           成功
 */
int config_init(const char *filename, p_config config);

#endif