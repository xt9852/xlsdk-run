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

/// 配置数据,配置文件应为utf8
typedef struct _config
{
    char    log_filename[512];  ///< 日志文件名
    int     log_level;          ///< 日志级别(调试,信息,警告,错误)
    int     log_cycle;          ///< 日志文件保留周期(时,天,周)
    int     log_backup;         ///< 日志文件保留数量
    int     log_clean_log;      ///< 首次打开日志文件时是否清空文件内容
    int     log_clean_file;     ///< 首次打开日志文件时是否删除过期文件

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