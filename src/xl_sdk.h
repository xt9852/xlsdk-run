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

/// 任务数量
#define TASK_SIZE                                       128

/// 任务名长度
#define TASK_NAME_SIZE                                  512

/// 任务类型
enum
{
    TASK_NULL,                                          ///< 无任务
    TASK_MAGNET,                                        ///< 磁力
    TASK_BT,                                            ///< BT下载
    TASK_URL                                            ///< 普通文件
};

/// 任务信息
typedef struct _xl_task
{
    unsigned int        id;                             ///< 任务ID

    unsigned int        type;                           ///< 任务类型

    unsigned __int64    size;                           ///< 已经下载的数量

    unsigned __int64    down;                           ///< 已经下载的数量

    unsigned int        time;                           ///< 用时秒数
    
    char                filename[TASK_NAME_SIZE];       ///< 文件名

    char                name[TASK_NAME_SIZE];           ///< 任务名

    unsigned int        name_len;                       ///< 任务名长度

    unsigned __int64    last_down;                      ///< 上次计算时已经下载的数量

    unsigned int        last_time;                      ///< 上次计算时用时秒数

    unsigned __int64    speed;                          ///< 下载速度

    double              prog;                           ///< 下载进度

} xl_task, *p_xl_task;                                   ///< 任务信息指针

/**
 *\brief   初始化SDK
 *\return  0-成功
 */
int xl_sdk_init();

/**
 *\brief        开始下载文件
 *\param[in]    taskid      任务ID
 *\param[in]    task_type   任务类型
 *\return       0           成功
 */
int xl_sdk_start_download_file(int taskid, int task_type);

/**
 *\brief        停止下载文件
 *\param[in]    taskid      任务ID
 *\return       0           成功
 */
int xl_sdk_stop_download_file(int taskid);

/**
 *\brief        下载文件
 *\param[in]    path            本地地址
 *\param[in]    filename        文件地址
 *\param[in]    list            下载BT文件时选中的要下载的文件,如:"10100",1-选中,0-末选
 *\return       0               成功
 */
int xl_sdk_download(const char *path, const char *filename, const char *list);

#endif