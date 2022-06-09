/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         torrent.c
 *\author       xt
 *\version      1.0.0
 *\date         2022-02-08
 *\brief        bencode编码的种子文件解析实现,UTF-8(No BOM)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include "torrent.h"
#include "xt_log.h"
#include "xt_pinyin.h"
#include "xt_character_set.h"

/// 种子文件字段
enum    
{
    ED2K = 1,
    FILEHASH,
    NAME,
    PATH,
    LENGTH,
    PIECES,
    ANNOUNCE,
    PATH_LIST,              ///< path的list
    ANNOUNCE_LIST,          ///< announce的list
    ANNOUNCE_LIST_LIST,     ///< announce的list的list
};

int bencode_dict(const char *s, unsigned int len, p_torrent info);

/**
 *\brief        解析bencode编码的字符串,格式:<字符串长度>字符串
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   info        信息
 *\return       文本长度,小于0错误
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
                *((unsigned int*)info->announce_tail) = str_len;    // 字符串长度
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
            ERR("string char error");
            return -100;
        }
    }

    ERR("string error");
    return -101;
}

/**
 *\brief        解析bencode编码的整数,格式:i<整数>e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   info        信息
 *\return       文本长度,小于0错误
 */
int bencode_int(const char *s, unsigned int len, p_torrent info)
{
    if (s[0] != 'i')
    {
        ERR("int flag error");
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
            if (info->last == LENGTH) // 上一个字符串是"length","name"
            {
                info->file[info->count].len  = num;
                info->file[info->count].name_len = 0;
                info->name_tail = info->file[info->count].name;
            }

            return i + 1;
        }
        else
        {
            ERR("int num error");
            return -201;
        }
    }

    ERR("int error");
    return -202;
}

/**
 *\brief        解析bencode编码的列表,格式:l<bencoding编码类型>e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   info        信息
 *\return       文本长度,小于0错误
 */
int bencode_list(const char *s, unsigned int len, p_torrent info)
{
    if (s[0] != 'l')
    {
        ERR("list flag error");
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
            ERR("list sub item len error");
            return -301;
        }

        i += ret;
    }

    ERR("list error");
    return -302;
}

/**
 *\brief        解析bencode编码的字典,格式:d<bencoding字符串><bencoding编码类型>e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   info        信息
 *\return       文本长度,小于0错误
 */
int bencode_dict(const char *s, unsigned int len, p_torrent info)
{
    if (s[0] != 'd')
    {
        ERR("dict flag error");
        return -400;
    }

    int ret = bencode_str(&s[1], len, info);

    if (ret <= 0)
    {
        ERR("dict key len error");
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
 *\brief        打开文件取得文件数据
 *\param[in]    filename    文件名
 *\param[out]   data        数据
 *\param[out]   len         数据长
 *\return       0           成功
 */
int load_file_data(const char *filename, char **data, unsigned int *len)
{
    FILE *fp = NULL;
    fopen_s(&fp, filename, "rb");

    if (NULL == fp)
    {
        printf("open %s fail %d", filename, errno);
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
 *\brief        解析种子文件
 *\param[in]    filename    种子文件名
 *\param[out]   info        种子信息
 *\return       0           成功
 */
int get_torrent_info(const char *filename, p_torrent info)
{
    if (NULL == filename || NULL == info)
    {
        ERR("filename,info is null");
        return -1;
    }

    unsigned int size = strlen(filename);

    if (0 != strcmp(filename + size - 8, ".torrent"))
    {
        ERR("isn't torrent %s", filename);
        return -2;
    }

    char *buff;

    if (0 != load_file_data(filename, &buff, &size)) // malloc buff
    {
        ERR("load file data fail %s", filename);
        return -3;
    }

    memset(info, 0, sizeof(info));
    info->announce_tail = info->announce;

    if (size != bencode_dict(buff, size, info))
    {
        ERR("dict fail %s", filename);
        free(buff);
        return -4;
    }

    info->announce_len = (info->announce_tail - info->announce) * 2;

    free(buff);

    // 删除旧版本信息
    for (int i = 0; i < info->count;)
    {
        if (0 == strncmp(info->file[i].name, "_____padding_file_", 18))
        {
            for (int j = i; j < info->count - 1; j++)
            {
                info->file[j] = info->file[j + 1];
            }
    
            info->count--;
        }
        else
        {
            i++;
        }
    }

    unsigned int len;

    for (int i = 0; i < info->count; i++)
    {
        len = 512;

        // 列表控件要使用
        if (0 != utf8_unicode(info->file[i].name, info->file[i].name_len, info->file[i].name_unicode, &len))
        {
            ERR("utf8 to unicode fail");
            return -5;
        }
    }

    return 0;
}
