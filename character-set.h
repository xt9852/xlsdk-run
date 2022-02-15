#pragma once

/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   character-set.h
 * Description: 字符集转码定义
 * Author:      xt
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-08
 * History:     2022-02-08 创建此文件。
 */

/**
 * \brief   将utf8转成unicode
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int utf8_unicode(const char *src, int src_len, short *dst, int *dst_len);

/**
 * \brief   将unicode转成ansi(gbk)
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int unicode_ansi(const short *src, int src_len, char *dst, int *dst_len);
