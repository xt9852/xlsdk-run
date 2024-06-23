/**
 *\file     torrent.c
 *\note     UTF-8
 *\author   xt
 *\version  1.0.0
 *\date     2022.02.08
 *\brief    bencode编码的种子文件解析实现
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

int bencode_dict(const char *s, unsigned int len, p_bt_torrent torrent);

/**
 *\brief                    解析bencode编码的字符串,格式:{字符串长度}字符串,utf8
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return                   文本长度,小于0错误
 */
int bencode_str(const char *s, unsigned int len, p_bt_torrent torrent)
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

            if (torrent->key == ED2K || torrent->key == FILEHASH || torrent->key == PIECES) // 不处理这三种数据
            {
                torrent->key = 0;
                return i + str_len;
            }

            if (torrent->key == NAME) // 单文件
            {
                torrent->key = 0;
                torrent->count = 1;
                torrent->file[0].name_len = str_len;
                memcpy(torrent->file[0].name, s + i, str_len);
            }
            else if (torrent->key == PATH_LIST) // 多文件
            {
                memcpy(torrent->name, s + i, str_len);
                torrent->name[str_len] = '\\';
                torrent->name += str_len + 1;
            }
            else if (torrent->key == ANNOUNCE_LIST_LIST)
            {
                dst_len = TORRENT_ANNOUNCE_SIZE - torrent->tracker.len / 2;

                if (0 != utf8_unicode(s + i, str_len, torrent->ptr + 2, &dst_len))
                {
                    return 100;
                }

                torrent->tracker.count++;
                torrent->tracker.len += 4 + str_len * 2;    // 字节长度
                *((unsigned int*)torrent->ptr) = str_len;   // 字符串长度
                torrent->ptr += 2 + str_len;
            }
            else if (0 == strncmp(s + i, "ed2k", str_len))
            {
                torrent->key = ED2K;
            }
            else if (0 == strncmp(s + i, "filehash", str_len))
            {
                torrent->key = FILEHASH;
            }
            else if (0 == strncmp(s + i, "pieces", str_len))
            {
                torrent->key = PIECES;
            }
            else if (0 == strncmp(s + i, "name", str_len))
            {
                if (torrent->key == LENGTH) torrent->key = NAME;    // 有只有name没有length的情况
            }
            else if (0 == strncmp(s + i, "path", str_len))
            {
                torrent->key = PATH;
            }
            else if (0 == strncmp(s + i, "length", str_len))
            {
                torrent->key = LENGTH;
            }
            else if (0 == strncmp(s + i, "announce-list", str_len))
            {
                torrent->key = ANNOUNCE;
            }

            return i + str_len;
        }
        else
        {
            E("string char error");
            return -101;
        }
    }

    E("string error");
    return -102;
}

/**
 *\brief                    解析bencode编码的整数,格式:i{整数}e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return                   文本长度,小于0错误
 */
int bencode_int(const char *s, unsigned int len, p_bt_torrent torrent)
{
    if (s[0] != 'i')
    {
        E("int flag error");
        return -200;
    }

    int flage = 1;
    __int64 num = 0;
    unsigned int i = 1;

    for (; i < len; i++)
    {
        if (s[i] == '-')    // 负号
        {
            flage = -1;
        }
        else if (s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] - '0';
        }
        else if (s[i] == 'e')
        {
            if (torrent->key == LENGTH) torrent->file[torrent->count].size = num * flage;
            return i + 1;
        }
        else
        {
            E("int num error %s", s);
            return -201;
        }
    }

    E("int error");
    return -202;
}

/**
 *\brief                    解析bencode编码的列表,格式:l{bencoding编码类型}e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return                   文本长度,小于0错误
 */
int bencode_list(const char *s, unsigned int len, p_bt_torrent torrent)
{
    if (s[0] != 'l')
    {
        E("list flag error");
        return -300;
    }

    if (torrent->key == PATH)
    {
        torrent->key = PATH_LIST;
    }
    else if (torrent->key == ANNOUNCE)
    {
        torrent->key = ANNOUNCE_LIST;
    }
    else if (torrent->key == ANNOUNCE_LIST)
    {
        torrent->key = ANNOUNCE_LIST_LIST;
    }

    int ret;

    for (unsigned int i = 1; i < len; )
    {
        if (s[i] == 'e')
        {
            if (torrent->key == PATH_LIST)
            {
                p_bt_torrent_file file = &(torrent->file[torrent->count]);
                file->name_len = torrent->name - file->name - 1;
                torrent->count++;
                torrent->key = 0;
                torrent->name[-1] = '\0'; // 结尾多了个'\\'
                torrent->name = torrent->file[torrent->count].name;
            }
            else if (torrent->key == ANNOUNCE_LIST)
            {
                torrent->key = 0;
            }
            else if (torrent->key == ANNOUNCE_LIST_LIST)
            {
                torrent->key = ANNOUNCE_LIST;
            }

            return i + 1;
        }
        else if (s[i] == 'd')
        {
            ret = bencode_dict(&s[i], len, torrent);
        }
        else if (s[i] == 'l')
        {
            ret = bencode_list(&s[i], len, torrent);
        }
        else if (s[i] == 'i')
        {
            ret = bencode_int(&s[i], len, torrent);
        }
        else
        {
            ret = bencode_str(&s[i], len, torrent);
        }

        if (ret <= 0)
        {
            E("list sub item len error");
            return -301;
        }

        i += ret;
    }

    E("list error");
    return -302;
}

/**
 *\brief                    解析bencode编码的字典,格式:d{bencoding字符串}{bencoding编码类型}e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return                   文本长度,小于0错误
 */
int bencode_dict(const char *s, unsigned int len, p_bt_torrent torrent)
{
    if (s[0] != 'd')
    {
        E("dict flag error");
        return -400;
    }

    int ret = bencode_str(&s[1], len, torrent);

    if (ret <= 0)
    {
        E("dict key len error");
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
            ret = bencode_dict(&s[i], len, torrent);
        }
        else if (s[i] == 'l')
        {
            ret = bencode_list(&s[i], len, torrent);
        }
        else if (s[i] == 'i')
        {
            ret = bencode_int(&s[i], len, torrent);
        }
        else
        {
            ret = bencode_str(&s[i], len, torrent);
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
 *\brief                    打开文件取得文件数据
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
    *data = malloc(*len);

    fseek(fp, 0, SEEK_SET);
    fread(*data, 1, *len, fp);

    fclose(fp);
    return 0;
}

/**
 *\brief                    解析种子文件
 *\param[in]    filename    种子文件名
 *\param[out]   torrent     种子数据
 *\return       0           成功
 */
int get_torrent_info(const char *filename, p_bt_torrent torrent)
{
    if (NULL == filename || NULL == torrent)
    {
        E("filename,torrent is null");
        return -1;
    }

    unsigned int size = strlen(filename);

    if (0 != strcmp(filename + size - 8, ".torrent"))
    {
        E("isn't torrent %s", filename);
        return -2;
    }

    char *buf;

    if (0 != load_file_data(filename, &buf, &size)) // malloc buf
    {
        E("load file data fail %s", filename);
        return -3;
    }

    memset(torrent, 0, sizeof(bt_torrent));
    strcpy_s(torrent->filename, TORRENT_FILENAME_SIZE, filename);
    torrent->name = torrent->file[0].name;
    torrent->ptr  = torrent->tracker.data;

    if (size != bencode_dict(buf, size, torrent))
    {
        E("dict fail %s", filename);
        free(buf);
        return -4;
    }

    free(buf);

    D("tracker count:%d len:%d", torrent->tracker.count, torrent->tracker.len);
    return 0;
}
