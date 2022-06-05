/*************************************************
 * Copyright:   XT Tech. Co., Ltd.
 * File name:   xt_sdk.h
 * Author:      xt
 * Version:     1.0.0
 * Date:        2022.06.04
 * Code:        UTF-8(No BOM)
 * Description: 迅雷接口定义
*************************************************/

#ifndef _XL_SDK_H_
#define _XL_SDK_H_

enum    // 任务类型
{
    TASK_MAGNET,
    TASK_BT,
    TASK_URL
};

/**
 * \brief   初始化SDK
 * \return  0-成功
 */
int xl_sdk_init();

/**
 * \brief   添加服务器
 * \param   [in]   int      *taski      任务ID
 * \param   [in]   int      count       服务器数量
 * \param   [in]   void     *data       服务器数据
 * \param   [in]   int      data_len    数据长度
 * \return  0-成功
 */
int xl_sdk_add_bt_tracker(int taskid, int count, void *data, int data_len);

/**
 * \brief   创建下载URL文件任务
 * \param   [in]    short   *url        URL地址
 * \param   [in]    short   *path       本地下载目录
 * \param   [out]   int     *taskid     任务ID
 * \param   [out]   short   *task_name  任务名称
 * \return  0-成功
 */
int xl_sdk_create_url_task(short *url, short *path, int *taskid, short *task_name);

/**
 * \brief   创建下载种子文件任务
 * \param   [in]    short   *magnet     magnet磁力URL,UNICODE
 * \param   [in]    short   *path       本地存储路径,UNICODE
 * \param   [out]   int     *taskid     任务ID
 * \param   [out]   short   *task_name  任务名称
 * \return  0-成功
 */
int xl_sdk_create_magnet_task(short *magnet, short *path, int *taskid, short *task_name);

/**
 * \brief   创建下载BT文件任务
 * \param   [in]    short   *torrent        种子文件全名
 * \param   [in]    short   *path           本地下载目录
 * \param   [in]    char    *list           文件下载列表,例:"001",0-不下载,1-下载,文件按拼音顺序排列
 * \param   [in]    int      announce_count tracker服务器数量
 * \param   [in]    short   *announce       tracker服务器数据
 * \param   [in]    int      announce_len   tracker服务器数据长度
 * \param   [out]   int     *taskid         任务ID
 * \param   [out]   short   *task_name      任务名称
 * \return  0-成功
 */
int xl_sdk_create_bt_task(short *torrent, short *path, char *list,
                          int announce_count, short *announce, int announce_len,
                          int *taskid);

/**
 * \brief   开始下载文件
 * \param   [in]   int     *taski       任务ID
 * \param   [in]   int      task_type   任务类型
 * \return  0-成功
 */
int xl_sdk_start_download_file(int taskid, int task_type);

/**
 * \brief   停止下载文件
 * \param   [in]   int     *taski       任务ID
 * \return  0-成功
 */
int xl_sdk_stop_download_file(int taskid);

/**
 * \brief   得到下载任务信息
 * \param   [in]    int                 *taskid     任务ID
 * \param   [out]   unsigned __int64    *size       下载的数据总大小
 * \param   [out]   unsigned __int64    *down       已经下载的数据总大小
 * \param   [out]   unsigned __int64    *time       本次下载任务用时单位秒
 * \return  0-成功
 */
int xl_sdk_get_task_info(int taskid, unsigned __int64 *size, unsigned __int64 *down, unsigned int *time);

#endif