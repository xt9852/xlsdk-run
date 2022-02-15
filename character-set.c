/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   character-set.c
 * Description: 字符集转码实现
 * Author:      xt
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-08
 * History:     2022-02-08 创建此文件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>

/**
 * \brief   将utf8转成unicode
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int utf8_unicode(const char *src, int src_len, short *dst, int *dst_len)
{
    if (NULL == src || src_len <= 0 || NULL == dst || NULL == dst_len)
    {
        return -1;
    }

    // 转成unicode后的长度
    int len = MultiByteToWideChar(CP_UTF8, 0, src, src_len, NULL, 0);

    if (len >= *dst_len)
    {
        return -2;
    }

    // 转成unicode
	MultiByteToWideChar(CP_UTF8, 0, src, src_len, dst, *dst_len);

    dst[len] = L'\0';

    *dst_len = len;
	return 0;
}

/**
 * \brief   将unicode转成ansi(gbk)
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int unicode_ansi(const short *src, int src_len, char *dst, int *dst_len)
{
    if (NULL == src || src_len <= 0 || NULL == dst || NULL == dst_len)
    {
        return -1;
    }

    int len = WideCharToMultiByte(CP_ACP, 0, src, src_len, 0, 0, 0, 0);

    if (len >= *dst_len)
    {
        return -2;
    }

    WideCharToMultiByte(CP_ACP, 0, src, src_len, dst, *dst_len, 0, 0);

    dst[len] = '\0';

    *dst_len = len;
    return 0;
}
