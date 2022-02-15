/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   pinyin.c
 * Description: 将gb2312转成拼音实现
 * Author:      xt
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-15
 * History:     2022-02-15 创建此文件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>


unsigned char   *g_pinyin = NULL;     // 拼音数据

typedef struct _peyin_unit
{
    char        *m;
    int         len;

}peyin_unit, *p_peyin_unit;

const peyin_unit g_pinyin_sm[] = {  // 拼音声母
    { "",   1},
    { "ch", 2},
    { "sh", 2},
    { "zh", 2},
    { "b",  1},
    { "c",  1},
    { "d",  1},
    { "f",  1},
    { "g",  1},
    { "h",  1},
    { "j",  1},
    { "k",  1},
    { "l",  1},
    { "m",  1},
    { "n",  1},
    { "p",  1},
    { "q",  1},
    { "r",  1},
    { "s",  1},
    { "t",  1},
    { "w",  1},
    { "x",  1},
    { "y",  1},
    { "z",  1}
};

const peyin_unit g_pinyin_ym[] = {  // 拼音韵母
    { "",     1},
    { "iang", 4},
    { "iong", 4},
    { "uang", 4},
    { "ueng", 4},
    { "ang",  3},
    { "eng",  3},
    { "ian",  3},
    { "iao",  3},
    { "ing",  3},
    { "ong",  3},
    { "uai",  3},
    { "uan",  3},
    { "uei",  3},
    { "uen",  3},
    { "ai",   2},
    { "an",   2},
    { "ao",   2},
    { "ei",   2},
    { "en",   2},
    { "ia",   2},
    { "ie",   2},
    { "in",   2},
    { "iu",   2},
    { "ou",   2},
    { "ua",   2},
    { "uo",   2},
    { "ue",   2},   // üe
    { "un",   2},   // ün
    { "a",    1},
    { "e",    1},
    { "i",    1},
    { "o",    1},
    { "u",    1},
};

/**
 * \brief   从资源中加载拼音数据
 * \param   [in]    int             res_id      资源ID
 * \param   [in]    TCHAR          *res_name    资源类名,_T("PINYIN")
 * \param   [out]   unsigned char **data        拼音数据
 * \param   [out]   unsigned int   *len         数据长
 * \return  0-成功，其它失败
 */
int load_pinyin_res(int res_id, TCHAR *res_name, unsigned char **data, unsigned int *len)
{
    if (NULL == res_name || NULL == data)
    {
        return -1;
    }

    HRSRC res = FindResource(NULL, MAKEINTRESOURCE(res_id), res_name);

    if (NULL == res)
	{
        return -2;
    }

    // 加载资源到内存
    HGLOBAL res_global = LoadResource(NULL, res);

    if (NULL == res_global)
	{
        return -3;
    }

    // 锁定资源内存
    *data = LockResource(res_global);

    if (NULL == *data)
	{
        return -4;
    }

    // 获取资源的大小
    *len = SizeofResource(NULL, res);

    if (0 == *len)
	{
        return -5;
    }

    return 0;
}

/**
 * \brief   打开文件取得文件数据
 * \param   [in]  const char    *filename   文件名
 * \param   [out] char          **data      数据
 * \param   [out] unsigned int  *len        数据长
 * \return  0-成功，其它失败
 */
int load_pinyin_data(const char *filename, char **data, unsigned int *len)
{
    if (NULL == filename || NULL == data)
    {
        printf("%s arg error\n", __FUNCTION__);
        return -1;
    }

    FILE *fp = NULL;
    fopen_s(&fp, filename, "rb");

    if (NULL == fp)
    {
        printf("open %s error %d", filename, GetLastError());
        return -1;
    }

    fseek(fp, 0, SEEK_END);

    *len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *data = malloc(*len);
    fread(*data, 1, *len, fp);

    fclose(fp);
    return 0;
}

/**
 * \brief   将gb2312转成拼音
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int gb2312_pinyin(const unsigned char *src, int src_len, char *dst, int *dst_len)
{
    if (NULL == src || NULL == dst)
    {
        return -1;
    }

    if (NULL == g_pinyin)
    {
        return -2;
    }

    unsigned char *end  = dst + *dst_len;
    unsigned char *tail = dst;
    unsigned char  sm_value;
    unsigned char  ym_value;
    int            buff_pos;

    for (int i = 0; i < src_len; )
    {
        if (src[i]     >= 0xb0 && src[i]     <= 0xfe &&
            src[i + 1] >= 0xa1 && src[i + 1] <= 0xfe) // gb2312的汉字
        {
            buff_pos = ((src[i] - 0xb0) * 94 + (src[i + 1] - 0xa1)) * 2;

            sm_value = g_pinyin[buff_pos];
            ym_value = g_pinyin[buff_pos + 1];
/*
            char info[MAX_PATH];
            sprintf_s(info, MAX_PATH, "%c%c=0x%02x%02x pos:%5d sm=%02d ym=%02d pinyin:%s%s\n",
                src[i], src[i + 1], src[i], src[i + 1], buff_pos,
                sm_value, ym_value, g_pinyin_sm[sm_value].m, g_pinyin_ym[ym_value].m);
            MessageBoxA(NULL, info, "pinyin",  MB_OK);
*/
            if ((tail + g_pinyin_sm[sm_value].len) >= end)
            {
                return -3;
            }

            strcpy_s(tail, end - tail, g_pinyin_sm[sm_value].m);

            tail += g_pinyin_sm[sm_value].len;

            if ((tail + g_pinyin_ym[ym_value].len) >= end)
            {
                return -4;
            }

            strcpy_s(tail, end - tail, g_pinyin_ym[ym_value].m);

            tail += g_pinyin_ym[ym_value].len;

            i += 2;
        }
        else if (src[i] >= 0x80)
        {
            if ((tail + 2) >= end)
            {
                return -5;
            }
/*
            char info[MAX_PATH];
            sprintf_s(info, MAX_PATH, "%c%c=0x%02x%02x i:%d",
            src[i], src[i + 1], src[i], src[i + 1], i);
            MessageBoxA(NULL, info, "pinyin 2B",  MB_OK);
*/
            *tail++ = src[i];
            *tail++ = src[i + 1];

            i += 2;
        }
        else
        {
            if ((tail + 1) >= end)
            {
                return -6;
            }
/*
            char info[MAX_PATH];
            sprintf_s(info, MAX_PATH, "%c=0x%02x i:%d", src[i], src[i],i);
            MessageBoxA(NULL, info, "pinyin 1B",  MB_OK);
*/
            *tail++ = src[i];

            i++;
        }
    }

    *dst_len = tail - dst;

    return 0;
}
