#pragma once

/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   pinyin.c
 * Description: 将gb2312转成拼音定义
 * Author:      xt
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-15
 * History:     2022-02-15 创建此文件。
 */


/**
 * \brief   从资源中加载拼音数据
 * \param   [in]    int             res_id      资源ID   
 * \param   [in]    TCHAR          *res_name    资源类名,_T("PINYIN")
 * \param   [out]   unsigned char **data        拼音数据
 * \param   [out]   unsigned int   *len         数据长
 * \return  0-成功，其它失败
 */
int load_pinyin_res(int res_id, TCHAR *res_name, unsigned char **data, unsigned int *len);

/**
 * \brief   打开文件取得文件数据
 * \param   [in]  const char    *filename   文件名
 * \param   [out] char          **data      数据
 * \param   [out] unsigned int  *len        数据长
 * \return  0-成功，其它失败
 */
int load_pinyin_data(const char *filename, char **data, unsigned int *len);

/**
 * \brief   将gb2312转成拼音
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int gb2312_pinyin(const unsigned char *src, int src_len, char *dst, int *dst_len);
