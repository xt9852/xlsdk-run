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

/// 种子中文件数量
#define TORRENT_FILE_SIZE                               512

/// 种子中文件名长度
#define TORRENT_FILENAME_SIZE                           512

/// 种子中的文件信息
typedef struct _bt_torrent_file
{
    char                name[TORRENT_FILENAME_SIZE];    ///< 文件名,utf8

    unsigned __int64    size;                           ///< 文件大小

    unsigned int        name_len;                       ///< 文件名长度,utf8

} bt_torrent_file, *p_bt_torrent_file;                  ///< 种子内包含的文件信息

/// 种子信息
typedef struct _bt_torrent
{
    bt_torrent_file     file[TORRENT_FILE_SIZE];        ///< 种子内包含的文件信息
    int                 file_count;                     ///< 种子内包含的文件数量

    int                 announce_len;                   ///< 数据长
    int                 announce_count;                 ///< 服务器列表数量
    short               announce[10240];                ///< 服务器列表

    int                 last;                           ///< 上一个字符串是什么
    char               *name_tail;                      ///< 指向当是文件名结尾
    short              *announce_tail;                  ///< 指向服务器列表结尾

} bt_torrent, *p_bt_torrent;                            ///< 种子文件信息


/**
 *\brief        解析种子文件
 *\param[in]    filename    种子文件名
 *\param[out]   torrent     种子数据
 *\return       0           成功
 */
int get_torrent_info(const char *filename, p_bt_torrent torrent);

#endif