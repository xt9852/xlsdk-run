/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   torrent.c
 * Description: bencode编码的种子文件解析实现
 * Author:      xt
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-15
 * History:     2022-02-15 创建此文件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "torrent.h"
#include "pinyin.h"
#include "character-set.h"

enum    // 种子文件字段
{
    ED2K = 1,
    FILEHASH,
    NAME,
    PATH,
    LENGTH,
    PIECES,
    ANNOUNCE,
    PATH_LIST,              // path的list
    ANNOUNCE_LIST,          // announce的list
    ANNOUNCE_LIST_LIST,     // announce的list的list
};

/**
 * \brief   解析bencode编码的字符串，格式：<字符串长度>字符串
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_torrent      info    信息
 * \return  0-成功，其它失败
 */
int bencode_str(const char *s, unsigned int len, p_torrent info)
{
    unsigned int str_len = 0;

    for (unsigned int i = 0; i < len; i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            str_len = str_len * 10 + s[i] - '0';
        }
        else if (s[i] == ':')
        {
            i++; // 绕过冒号:

            // 不处理这三种数据
            if ((info->last == ED2K) ||
                (info->last == FILEHASH) ||
                (info->last == PIECES))
            {
                info->last = 0;
                return i + str_len;
            }

            if (info->last == NAME) // 单文件
            {
                info->last = 0;
                info->count = 1;
                memcpy(info->file[0].name, s + i, str_len);
            }
            else if (info->last == PATH_LIST) // 多文件
            {
                memcpy(info->name_tail, s + i, str_len);

                info->name_tail += str_len;
                *(info->name_tail++) = '\\';

                info->file[info->count].name_len += str_len;
            }
            else if (info->last == ANNOUNCE_LIST_LIST)
            {
                ((DWORD*)info->announce_tail)[0] = str_len;    // 字符串长度
                memcpy(info->announce_tail + 4, s + i, str_len);
                info->announce_tail += 4 + str_len;
            }
            else if (0 == strncmp(s + i, "ed2k", str_len))
            {
                info->last = ED2K;
            }
            else if (0 == strncmp(s + i, "filehash", str_len))
            {
                info->last = FILEHASH;
            }
            else if (0 == strncmp(s + i, "pieces", str_len))
            {
                info->last = PIECES;
            }
            else if (0 == strncmp(s + i, "name", str_len))
            {
                if (info->last == LENGTH) info->last = NAME;    // 有只有name没有length的情况
            }
            else if (0 == strncmp(s + i, "path", str_len))
            {
                info->last = PATH;
            }
            else if (0 == strncmp(s + i, "length", str_len))
            {
                info->last = LENGTH;
            }
            else if (0 == strncmp(s + i, "announce-list", str_len))
            {
                info->last = ANNOUNCE;
            }

            return i + str_len;
        }
        else
        {
            printf("string char error\n");
            return -100;
        }
    }

    printf("string error\n");
    return -101;
}

/**
 * \brief   解析bencode编码的整数，格式：i<整数>e
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_torrent      info    信息
 * \return  0-成功，其它失败
 */
int bencode_int(const char *s, unsigned int len, p_torrent info)
{
    if (s[0] != 'i')
    {
        printf("int flage error\n");
        return -200;
    }

    unsigned int     i   = 1;
    unsigned __int64 num = 0;

    for (; i < len; i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] - '0';
        }
        else if (s[i] == 'e')
        {
            if (info->last == LENGTH) // 上一个字符中是"length","name"
            {
                info->file[info->count].len  = num;
                info->file[info->count].name_len = 0;
                info->name_tail = info->file[info->count].name;
            }

            return i + 1;
        }
        else
        {
            printf("int num error\n");
            return -201;
        }
    }

    printf("int error\n");
    return -202;
}

/**
 * \brief   解析bencode编码的列表，格式：l<bencoding编码类型>e
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_torrent      info    信息
 * \return  0-成功，其它失败
 */
int bencode_list(const char *s, unsigned int len, p_torrent info)
{
    if (s[0] != 'l')
    {
        printf("list flage error\n");
        return -300;
    }

    if (info->last == PATH)
    {
        info->last = PATH_LIST;
    }
    else if (info->last == ANNOUNCE)
    {
        info->last = ANNOUNCE_LIST;
    }
    else if (info->last == ANNOUNCE_LIST)
    {
        info->last = ANNOUNCE_LIST_LIST;
    }

    int ret;

    for (unsigned int i = 1; i < len; )
    {
        if (s[i] == 'e')
        {
            if (info->last == PATH_LIST)
            {
                info->count++;
                info->last = 0;
                info->name_tail[-1] = '\0'; // 结尾多了个'\\'
                info->name_tail = NULL;
            }
            else if (info->last == ANNOUNCE_LIST)
            {
                info->last = 0;
            }
            else if (info->last == ANNOUNCE_LIST_LIST)
            {
                info->last = ANNOUNCE_LIST;
            }

            return i + 1;
        }
        else if (s[i] == 'd')
        {
            ret = bencode_dict(&s[i], len, info);
        }
        else if (s[i] == 'l')
        {
            ret = bencode_list(&s[i], len, info);
        }
        else if (s[i] == 'i')
        {
            ret = bencode_int(&s[i], len, info);
        }
        else
        {
            ret = bencode_str(&s[i], len, info);
        }

        if (ret <= 0)
        {
            printf("list sub item len error\n");
            return -301;
        }

        i += ret;
    }

    printf("list error\n");
    return -302;
}

/**
 * \brief   解析bencode编码的字典，格式：d<bencoding字符串><bencoding编码类型>e
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_torrent      info    信息
 * \return  0-成功，其它失败
 */
int bencode_dict(const char *s, unsigned int len, p_torrent info)
{
    if (s[0] != 'd')
    {
        printf("dict flage error\n");
        return -400;
    }

    int ret = bencode_str(&s[1], len, info);

    if (ret <= 0)
    {
        printf("dict key len error\n");
        return -401;
    }

    for (unsigned int i = 1 + ret; i < len; )
    {
        if (s[i] == 'e')
        {
            return i + 1;
        }
        else if (s[i] == 'd')
        {
            ret = bencode_dict(&s[i], len, info);
        }
        else if (s[i] == 'l')
        {
            ret = bencode_list(&s[i], len, info);
        }
        else if (s[i] == 'i')
        {
            ret = bencode_int(&s[i], len, info);
        }
        else
        {
            ret = bencode_str(&s[i], len, info);
        }

        if (ret <= 0)
        {
            printf("dict item len error\n");
            return -402;
        }

        i += ret;
    }

    printf("dict error\n");
    return -403;
}

/**
 * \brief   打开文件取得文件数据
 * \param   [in]  const char    *filename   文件名
 * \param   [out] char          **data      数据
 * \param   [out] unsigned int  *len        数据长
 * \return  0-成功，其它失败
 */
int load_file_data(const char *filename, char **data, unsigned int *len)
{
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
 * \brief   解析种子文件
 * \param   [in]    const char *filename    种子文件名
 * \param   [out]   p_torrent   info        种子信息
 * \return  0-成功，其它失败
 */
int get_torrent_info(const char *filename, p_torrent info)
{
    if (NULL == filename || NULL == info)
    {
        return -1;
    }

    unsigned int size = strlen(filename);

    if (0 != strcmp(filename + size - 8, ".torrent"))
    {
        return -2;
    }

    char *buff;

    if (0 != load_file_data(filename, &buff, &size))
    {
        return -3;
    }

    memset(info, 0, sizeof(torrent));
    info->announce_tail = info->announce;

    if (size != bencode_dict(buff, size, info))
    {
        return -4;
    }

    free(buff);

    //-----------------------------------------------------
    // 排序,1-目录,2-符号,3-英文,4-中文拼音

    DWORD len1;
    DWORD len2;
    DWORD len3;

    for (int i = 0; i < info->count; i++)
    {
        len1 = MAX_PATH;

        // 列表控件要使用
        if (0 != utf8_unicode(info->file[i].name, info->file[i].name_len,
                              info->file[i].name_unicode, &len1))
        {
            return -5;
        }

        len2 = MAX_PATH;

        // 转拼音要使用
        if (0 != unicode_ansi(info->file[i].name_unicode, len1,
                              info->file[i].name_asni, &len2))
        {
            return -6;
        }

        len3 = MAX_PATH;

        // 转拼音
        if (0 != gb2312_pinyin(info->file[i].name_asni, len2,
                               info->file[i].name_pinyin, &len3))
        {
            return -7;
        }

        //MessageBox(NULL, info.file[i].name_unicode, _T("unicode"),  MB_OK);
        //MessageBoxA(NULL, info.file[i].name_pinyin, "pinyin",  MB_OK);
    }

    // 冒泡排序
    file_info tmp;

    for (int i = 0; i < info->count - 1; i++)
    {
        for (int j = i + 1; j < info->count; j++)
        {
            if (strcmp(info->file[i].name_pinyin, info->file[j].name_pinyin) > 0)
            {
                tmp = info->file[i];
                info->file[i] = info->file[j];
                info->file[j] = tmp;
            }
        }
    }

    return 0;
}
