/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         xl_sdk.h
 *\author       xt
 *\version      1.0.0
 *\date         2022.02.08
 *\brief        迅雷SDK定义,UTF-8(No BOM)
 */
#ifndef _XL_SDK_H_
#define _XL_SDK_H_

/// 任务类型
enum
{
    TASK_MAGNET,                            ///< 磁力
    TASK_BT,                                ///< BT下载
    TASK_URL                                ///< 普通文件
};

typedef struct _xl_task
{
    unsigned int        id;                 ///< 任务ID

    unsigned __int64    size;               ///< 已经下载的数量

    unsigned __int64    down;               ///< 已经下载的数量

    unsigned int        time;               ///< 用时秒数

    int                 type;               ///< 任务类型

    char                filename[512];      ///< 文件名

}xl_task, *p_xl_task;

/**
 *\brief   初始化SDK
 *\return  0-成功
 */
int xl_sdk_init();

/**
 *\brief        添加服务器
 *\param[in]    taskid          任务ID
 *\param[in]    count           服务器数量
 *\param[in]    data            服务器数据
 *\param[in]    data_len        数据长度
 *\return       0               成功
 */
int xl_sdk_add_bt_tracker(int taskid, int count, const short *data, int data_len);

/**
 *\brief        创建下载BT文件任务
 *\param[in]    torrent         种子文件全名
 *\param[in]    path            本地下载目录
 *\param[in]    list            文件下载列表,例:"001",0-不下载,1-下载,文件按拼音顺序排列
 *\param[out]   taskid          任务ID
 *\param[out]   task_name       任务名称
 *\param[in]    task_name_size  任务名称缓冲区大小
 *\return       0               成功
 */
int xl_sdk_create_bt_task(const char *torrent, const char *path, const char *list, int *taskid, char *task_name, int task_name_size);

/**
 *\brief        创建下载URL文件任务
 *\param[in]    url             URL地址
 *\param[in]    path            本地下载目录
 *\param[out]   taskid          任务ID
 *\param[out]   task_name       任务名称
 *\param[in]    task_name_size  任务名称缓冲区大小
 *\return       0               成功
 */
int xl_sdk_create_url_task(const char *url, const char *path, int *taskid, char *task_name, int task_name_size);

/**
 *\brief        创建下载磁力文件任务
 *\param[in]    magnet          磁力URL
 *\param[in]    path            本地存储路径
 *\param[out]   taskid          任务ID
 *\param[out]   task_name       任务名称
 *\param[in]    task_name_size  任务名称缓冲区大小
 *\return       0               成功
 */
int xl_sdk_create_magnet_task(const char *magnet, const char *path, int *taskid, char *task_name, int task_name_size);

/**
 *\brief        开始下载文件
 *\param[in]    taski       任务ID
 *\param[in]    task_type   任务类型
 *\return       0           成功
 */
int xl_sdk_start_download_file(int taskid, int task_type);

/**
 *\brief        停止下载文件
 *\param[in]    taski       任务ID
 *\return       0           成功
 */
int xl_sdk_stop_download_file(int taskid);

/**
 *\brief        得到下载任务信息
 *\param[in]    taskid      任务ID
 *\param[out]   size        下载的数据总大小
 *\param[out]   down        已经下载的数据总大小
 *\param[out]   time        本次下载任务用时单位秒
 *\return       0           成功
 */
int xl_sdk_get_task_info(int taskid, unsigned __int64 *size, unsigned __int64 *down, unsigned int *time);

#endif