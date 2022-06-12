/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         torrent.h
 *\author       xt
 *\version      1.0.0
 *\date         2022.02.08
 *\brief        bencode编码的种子文件解析定义,UTF-8(No BOM)
 */
#ifndef _TORRENT_H_
#define _TORRENT_H_

/// 种子中文件信息
typedef struct _file_info
{
    unsigned int        name_len;               ///< 文件名长度,utf8

    char                name[512];              ///< 文件名,utf8

    short               name_unicode[512];      ///< 文件名,unicode

    char                name_tmp[512];          ///< 文件名,临时

    unsigned __int64    len;                    ///< 文件长

}file_info, *p_file_info;                       ///< 种子内包含的文件信息

/// 种子信息
typedef struct _torrent_info
{
    int                 count;                  ///< 种子内包含的文件数量
    file_info           file[512];              ///< 种子内包含的文件信息

    int                 announce_len;           ///< 数据长
    int                 announce_count;         ///< 服务器列表数量
    short               announce[10240];        ///< 服务器列表

    int                 last;                   ///< 上一个字符串是什么
    char               *name_tail;              ///< 指向当是文件名结尾
    short              *announce_tail;          ///< 指向服务器列表结尾

}torrent, *p_torrent;                           ///< 种子文件信息


/**
 *\brief        解析种子文件
 *\param[in]    filename    种子文件名
 *\param[out]   info        种子信息
 *\return       0           成功
 */
int get_torrent_info(const char *filename, p_torrent info);

#endif