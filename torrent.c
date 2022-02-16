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
    unsigned int dst_len = 0;

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
                info->file[0].name_len = str_len;
                memcpy(info->file[0].name, s + i, str_len);
            }
            else if (info->last == PATH_LIST) // 多文件
            {
                memcpy(info->name_tail, s + i, str_len);

                info->name_tail += str_len;
                *(info->name_tail++) = '\\';
            }
            else if (info->last == ANNOUNCE_LIST_LIST)
            {
                dst_len = MAX_PATH;
                info->announce_count++;
                *((DWORD*)info->announce_tail) = str_len;    // 字符串长度
                utf8_unicode(s + i, str_len, info->announce_tail + 2, &dst_len);
                info->announce_tail += 2 + str_len;
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
                info->file[info->count].name_len = info->name_tail - info->file[info->count].name;
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

int sort(p_file_info info, int count)
{
    if (NULL == info || count < 0)
    {
        return -1;
    }

    file_info tmp;

    // 冒泡排序
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (strcmp(info[i].name_pinyin, info[j].name_pinyin) > 0)
            {
                tmp = info[i];
                info[i] = info[j];
                info[j] = tmp;
            }
        }
    }

    return 0;
}

int sort_group(p_file_info info, int count)
{
    if (NULL == info || count < 0)
    {
        return -1;
    }

    // 分组,1-目录,2-符号,3-数字,4-英文,5-中文拼音

    int             dir_id              = 0;   // 目录
    int             dir_cnt             = 0;

    int             flage_id            = 0;   // 符号
    int             flage_cnt           = 0;

    int             number_id           = 0;   // 数字
    int             number_cnt          = 0;

    int             english_id          = 0;   // 英文，不区分大小写
    int             english_cnt         = 0;

    int             pinyin_id           = 0;   // 拼音
    int             pinyin_cnt          = 0;

    int             next_group_begin    = 0;   // 下一组开始
    unsigned char   c;
    unsigned char   s;
    file_info       tmp;

    // 目录
    for (int i = 0; i < count; i++)
    {
        if (strstr(info[i].name_tmp, "\\") != NULL)
        {
            tmp = info[i];
            info[i] = info[dir_id + dir_cnt];
            info[dir_id + dir_cnt] = tmp;
            dir_cnt++;

            next_group_begin = dir_id + dir_cnt;
        }
    }

    flage_id = next_group_begin;

    // 符号
    for (int i = flage_id; i < count; i++)
    {
        c = info[i].name_tmp[0];
        s = info[i].name_tmp[1];

        // 符号
        if ((c >= 0x21 && c <= 0x2F) ||
            (c >= 0x3A && c <= 0x40) ||
            (c >= 0x5B && c <= 0x60) ||
            (c >= 0x7B && c <= 0x7E) ||
            (c >= 0xA1 && c <= 0xA9 && s >= 0xA1 && s <= 0xFE) ||   // gb2312符号
            (c >= 0xA8 && c <= 0xA9 && s >= 0x40 && s <= 0xA0))     // gbk符号

        {
            tmp = info[i];
            info[i] = info[flage_id + flage_cnt];
            info[flage_id + flage_cnt] = tmp;
            flage_cnt++;

            next_group_begin = flage_id + flage_cnt;
        }
    }

    number_id = next_group_begin;

    // 数字
    for (int i = number_id; i < count; i++)
    {
        c = info[i].name_tmp[0];

        if (c >= '0' && c <= '9')
        {
            tmp = info[i];
            info[i] = info[number_id + number_cnt];
            info[number_id + number_cnt] = tmp;
            number_cnt++;

            next_group_begin = number_id + number_cnt;
        }
    }

    english_id = next_group_begin;

    // 英文
    for (int i = english_id; i < count; i++)
    {
        c = info[i].name_tmp[0];

        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            tmp = info[i];
            info[i] = info[english_id + english_cnt];
            info[english_id + english_cnt] = tmp;
            english_cnt++;

            next_group_begin = english_id + english_cnt;
        }
    }

    // 中文
    if (next_group_begin < count)
    {
        pinyin_id  = next_group_begin;
        pinyin_cnt = count - next_group_begin;
    }

    if (dir_cnt > 0)
    {
        int   begin = dir_id;
        int   count = 1;
        int   dir_name_len;
        char *ptr;
        char *head;
        char  dir_name[MAX_PATH];
        char  tmp_name[MAX_PATH];

        head = info[dir_id].name_tmp;

        ptr = strstr(head, "\\") + 1;

        dir_name_len = ptr - head;

        memcpy(dir_name, head, dir_name_len);       // 得到目录名

        dir_name[dir_name_len] = '\0';

        strcpy_s(head, MAX_PATH, ptr);              // 去除目录

        for (int i = dir_id + 1; i < dir_id + dir_cnt; i++)
        {
            head = info[i].name_tmp;

            ptr = strstr(head, "\\") + 1;

            dir_name_len = ptr - head;

            memcpy(tmp_name, head, dir_name_len);   // 得到目录名

            tmp_name[dir_name_len] = L'\0';

            strcpy_s(head, MAX_PATH, ptr);          // 去除目录

            if (strcmp(dir_name, tmp_name) == 0)    // 同一目录
            {
                count++;
            }
            else // 新目录
            {
                if (0 != sort_group(&info[begin], count))
                {
                    return -2;
                }

                memcpy(dir_name, tmp_name, dir_name_len * 2);

                dir_name[dir_name_len] = L'\0';

                begin = i;
                count = 1;
            }
        }

        if (count > 0)
        {
            if (0 != sort_group(&info[begin], count))
            {
                return -2;
            }
        }
    }

    if (flage_cnt > 0)
    {
        if (0 != sort(&info[flage_id], flage_cnt))
        {
            return -2;
        }
    }

    if (number_cnt > 0)
    {
        if (0 != sort(&info[number_id], number_cnt))
        {
            return -3;
        }
    }

    if (english_cnt > 0)
    {
        if (0 != sort(&info[english_id], english_cnt))
        {
            return -4;
        }
    }

    if (pinyin_cnt > 0)
    {
        if (0 != sort(&info[pinyin_id], pinyin_cnt))
        {
            return -5;
        }
    }

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
        free(buff);
        return -4;
    }

    info->announce_len = (info->announce_tail - info->announce) * 2;

    free(buff);

    // 删除旧版本信息
    //for (int i = 0; i < info->count;)
    //{
    //    if (0 == strncmp(info->file[i].name, "_____padding_file_", 18))
    //    {
    //        for (int j = i; j < info->count - 1; j++)
    //        {
    //            info->file[j] = info->file[j + 1];
    //        }
    //
    //        info->count--;
    //    }
    //    else
    //    {
    //        i++;
    //    }
    //}

    DWORD len1;
    DWORD len2;
    DWORD len3;
    char  c;

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
                              info->file[i].name_ansi, &len2))
        {
            return -6;
        }

        memcpy(info->file[i].name_tmp, info->file[i].name_ansi, len2);

        len3 = MAX_PATH;

        // 将全部英文转成大写
        for (unsigned int j = 0; j < strlen(info->file[i].name_ansi); j++)
        {
            c = info->file[i].name_ansi[j];

            if (c >= 'a' && c <= 'z')
            {
                info->file[i].name_ansi[j] = c - ('a' - 'A');
            }
        }

        // 转拼音
        if (0 != gbk_pinyin(info->file[i].name_ansi, len2, info->file[i].name_pinyin, &len3))
        {
            return -7;
        }

        //MessageBox(NULL,  info->file[i].name_unicode, _T("unicode"),  MB_OK);
        //MessageBoxA(NULL, info->file[i].name_pinyin,  "pinyin",       MB_OK);
    }

    //if (0 != sort_group(info->file, info->count))
    //{
    //    return -8;
    //}

    return 0;
}
