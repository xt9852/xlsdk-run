/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         config.h
 *\author       xt
 *\version      1.0.0
 *\date         2022.02.08
 *\brief        配置模块定义,UTF-8(No BOM)
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "xt_log.h"

/// 配置数据,配置文件应为utf8
typedef struct _config
{
    p_xt_log  log;              ///< 日志

    char    http_ip[512];       ///< HTTP地址
    int     http_port;          ///< HTTP端口

    char    path_download[512]; ///< 下载路径
    char    path_move[512];     ///< 完成路径

} config, *p_config;            ///< 配置数据指针

/**
 *\brief        初始化配置
 *\param[in]    filename    配置文件名
 *\param[out]   config      配置数据
 *\return       0           成功
 */
int config_init(const char *filename, p_config config);

#endif