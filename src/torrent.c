/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         torrent.c
 *\author       xt
 *\version      1.0.0
 *\date         2022.02.08
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

int bencode_dict(const char *s, unsigned int len, p_bt_torrent torrent);

/**
 *\brief        解析bencode编码的字符串,格式:{字符串长度}字符串,utf8
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return       文本长度,小于0错误
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

            // 不处理这三种数据
            if ((torrent->last == ED2K) ||
                (torrent->last == FILEHASH) ||
                (torrent->last == PIECES))
            {
                torrent->last = 0;
                return i + str_len;
            }

            if (torrent->last == NAME) // 单文件
            {
                torrent->last = 0;
                torrent->file_count = 1;
                torrent->file[0].name_len = str_len;
                memcpy(torrent->file[0].name, s + i, str_len);
            }
            else if (torrent->last == PATH_LIST) // 多文件
            {
                memcpy(torrent->name_tail, s + i, str_len);

                torrent->name_tail += str_len;
                *(torrent->name_tail++) = '\\';
            }
            else if (torrent->last == ANNOUNCE_LIST_LIST)
            {
                dst_len = MAX_PATH;
                torrent->announce_count++;
                *((unsigned int*)torrent->announce_tail) = str_len;    // 字符串长度
                utf8_unicode(s + i, str_len, torrent->announce_tail + 2, &dst_len);
                torrent->announce_tail += 2 + str_len;
            }
            else if (0 == strncmp(s + i, "ed2k", str_len))
            {
                torrent->last = ED2K;
            }
            else if (0 == strncmp(s + i, "filehash", str_len))
            {
                torrent->last = FILEHASH;
            }
            else if (0 == strncmp(s + i, "pieces", str_len))
            {
                torrent->last = PIECES;
            }
            else if (0 == strncmp(s + i, "name", str_len))
            {
                if (torrent->last == LENGTH) torrent->last = NAME;    // 有只有name没有length的情况
            }
            else if (0 == strncmp(s + i, "path", str_len))
            {
                torrent->last = PATH;
            }
            else if (0 == strncmp(s + i, "length", str_len))
            {
                torrent->last = LENGTH;
            }
            else if (0 == strncmp(s + i, "announce-list", str_len))
            {
                torrent->last = ANNOUNCE;
            }

            return i + str_len;
        }
        else
        {
            E("string char Eor");
            return -100;
        }
    }

    E("string Eor");
    return -101;
}

/**
 *\brief        解析bencode编码的整数,格式:i{整数}e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return       文本长度,小于0错误
 */
int bencode_int(const char *s, unsigned int len, p_bt_torrent torrent)
{
    if (s[0] != 'i')
    {
        E("int flag Eor");
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
            if (torrent->last == LENGTH) // 上一个字符串是"length","name"
            {
                torrent->file[torrent->file_count].size = num;
                torrent->file[torrent->file_count].name_len = 0;
                torrent->name_tail = torrent->file[torrent->file_count].name;
            }

            return i + 1;
        }
        else
        {
            E("int num Eor");
            return -201;
        }
    }

    E("int Eor");
    return -202;
}

/**
 *\brief        解析bencode编码的列表,格式:l{bencoding编码类型}e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return       文本长度,小于0错误
 */
int bencode_list(const char *s, unsigned int len, p_bt_torrent torrent)
{
    if (s[0] != 'l')
    {
        E("list flag Eor");
        return -300;
    }

    if (torrent->last == PATH)
    {
        torrent->last = PATH_LIST;
    }
    else if (torrent->last == ANNOUNCE)
    {
        torrent->last = ANNOUNCE_LIST;
    }
    else if (torrent->last == ANNOUNCE_LIST)
    {
        torrent->last = ANNOUNCE_LIST_LIST;
    }

    int ret;

    for (unsigned int i = 1; i < len; )
    {
        if (s[i] == 'e')
        {
            if (torrent->last == PATH_LIST)
            {
                torrent->file[torrent->file_count].name_len = torrent->name_tail - torrent->file[torrent->file_count].name -1;
                torrent->file_count++;
                torrent->last = 0;
                torrent->name_tail[-1] = '\0'; // 结尾多了个'\\'
                torrent->name_tail = NULL;
            }
            else if (torrent->last == ANNOUNCE_LIST)
            {
                torrent->last = 0;
            }
            else if (torrent->last == ANNOUNCE_LIST_LIST)
            {
                torrent->last = ANNOUNCE_LIST;
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
            E("list sub item len Eor");
            return -301;
        }

        i += ret;
    }

    E("list Eor");
    return -302;
}

/**
 *\brief        解析bencode编码的字典,格式:d{bencoding字符串}{bencoding编码类型}e
 *\param[in]    s           数据
 *\param[in]    len         数据长
 *\param[out]   torrent     种子数据
 *\return       文本长度,小于0错误
 */
int bencode_dict(const char *s, unsigned int len, p_bt_torrent torrent)
{
    if (s[0] != 'd')
    {
        E("dict flag Eor");
        return -400;
    }

    int ret = bencode_str(&s[1], len, torrent);

    if (ret <= 0)
    {
        E("dict key len Eor");
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
            printf("dict item len Eor\n");
            return -402;
        }

        i += ret;
    }

    printf("dict Eor\n");
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

    char *buff;

    if (0 != load_file_data(filename, &buff, &size)) // malloc buff
    {
        E("load file data fail %s", filename);
        return -3;
    }

    memset(torrent, 0, sizeof(bt_torrent));
    torrent->announce_tail = torrent->announce;

    if (size != bencode_dict(buff, size, torrent))
    {
        E("dict fail %s", filename);
        free(buff);
        return -4;
    }

    torrent->announce_len = (torrent->announce_tail - torrent->announce) * 2;

    free(buff);

    // 删除旧版本信息
    for (int i = 0; i < torrent->file_count;)
    {
        if (0 == strncmp(torrent->file[i].name, "_____padding_file_", 18))
        {
            for (int j = i; j < torrent->file_count - 1; j++)
            {
                torrent->file[j] = torrent->file[j + 1];
            }

            torrent->file_count--;
        }
        else
        {
            i++;
        }
    }

    return 0;
}
