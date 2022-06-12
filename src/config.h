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

/**
 *\brief        初始化配置
 *\param[in]    filename    配置文件名
 *\return       0           成功
 */
int config_init(const char *filename);

/**
 *\brief        得到下载路径
 *\return       下载路径
 */
const char * config_get_download_path();

#endif