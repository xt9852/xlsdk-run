/*************************************************
 * Copyright:   XT Tech. Co., Ltd.
 * File name:   config.h
 * Author:      xt
 * Version:     1.0.0
 * Date:        2022.06.04
 * Code:        UTF-8(No BOM)
 * Description: 配置模块接口定义
*************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_


/**
 *\brief      初始化配置
 *\param[in]  const char *filename 配置文件名
 *\return     0-成功
 */
int config_init(const char *filename);

/**
 *\brief      得到配置的路径
 *\return     配置的路径
 */
const char * config_get_path();

#endif