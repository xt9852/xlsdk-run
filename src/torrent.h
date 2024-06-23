/**
 *\file     torrent.h
 *\note     UTF-8
 *\author   xt
 *\version  1.0.0
 *\date     2022.02.08
 *\brief    bencode编码的种子文件解析定义
 */
#ifndef _TORRENT_H_
#define _TORRENT_H_

#define TORRENT_FILE_SIZE                               512     ///< 种子中文件数量

#define TORRENT_FILENAME_SIZE                           512     ///< 种子中文件名长度

#define TORRENT_ANNOUNCE_SIZE                           1048576 ///< 种子中服务器信息长度


typedef struct _bt_torrent_file                                 /// 种子中的文件信息
{
    char                name[TORRENT_FILENAME_SIZE];            ///< 文件名,utf8

    unsigned __int64    size;                                   ///< 文件大小

    unsigned int        name_len;                               ///< 文件名长度,utf8

} bt_torrent_file, *p_bt_torrent_file;


typedef struct _bt_torrent_tracker                              ///  服务器信息
{
    short               data[TORRENT_ANNOUNCE_SIZE];            ///< 服务器列表数据

    unsigned int        len;                                    ///< 数据长

    unsigned int        count;                                  ///< 服务器列表数量

} bt_torrent_tracker, *p_bt_torrent_tracker;


typedef struct _bt_torrent                                      ///  种子信息
{
    char                filename[TORRENT_FILENAME_SIZE];        ///< 种子文件名

    bt_torrent_file     file[TORRENT_FILE_SIZE];                ///< 种子内包含的文件信息
    int                 count;                                  ///< 种子内包含的文件数量

    bt_torrent_tracker  tracker;                                ///< 服务器信息

    int                 key;                                    ///< 上一个字符串是什么
    char               *name;                                   ///< 指向当是文件名结尾
    short              *ptr;                                    ///< 指向tracker服务器列表结尾

} bt_torrent, *p_bt_torrent;                                    ///< 种子文件信息


/**
 *\brief                    解析种子文件
 *\param[in]    filename    种子文件名
 *\param[out]   torrent     种子数据
 *\return       0           成功
 */
int get_torrent_info(const char *filename, p_bt_torrent torrent);

#endif