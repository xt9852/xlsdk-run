/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   sdk.c
 * Description: 主模块实现
 * Author:      xt
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-08
 * History:     2022-02-08 创建此文件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
//#include <CommCtrl.h>
#include "resource.h"

#define SIZEOF(x)               sizeof(x)/sizeof(x[0])
#define SP(...)                 _stprintf_s(info, SIZEOF(info), __VA_ARGS__)

typedef struct _arg_head
{
    DWORD               len;                // 参数长度

    char                data[1];            // 参数

}arg_head, *p_arg_head;                     // 参数头节点

typedef struct _data_head
{
    DWORD               len;                // 数据长度(不包含本身4字节)

    DWORD               func_id;            // 函数ID

union {
    arg_head            arg1;               // 参数1头节点

    DWORD               data[1];            // 参数1整数
    };

}data_head, *p_data_head;                   // 数据包头节点

typedef struct _task_info
{
    unsigned __int64    down;               // 已经下载的数量

    unsigned int        time;               // 用时秒数

}task_info, *p_task_info;

char   g_pth[512]               = "";
char   g_arg[128]               = "BDAF7A63-568C-43ab-9406-D145CF03B08C:";
char  *g_tail                   = g_arg + 37;

BOOL   g_init                   = FALSE;    // 是否完成初始化

UCHAR *g_recv                   = NULL;     // 共享内存1M,对方接收的数据,我方发送的
UCHAR *g_send                   = NULL;     // 共享内存1M,我方发送的数据,需要对方处理

UCHAR *g_recv_tmp               = NULL;
UCHAR *g_send_tmp               = NULL;

HANDLE g_proxyAliveMutex        = NULL;
HANDLE g_serverStartUpEvent     = NULL;

HANDLE g_recvShareMemory        = NULL;     // 共享内存1M,我方发送的数据,需要对方处理
HANDLE g_recvBufferFullEvent    = NULL;     // 信号,表示有数据,对方可以处理啦
HANDLE g_recvBufferEmptyEvent   = NULL;     // 信号,表示对方处理完成,我方可以再次发送

HANDLE g_sendShareMemory        = NULL;     // 共享内存1M,对方发送的数据,需要我方处理
HANDLE g_sendBufferFullEvent    = NULL;     // 信号,表示有数据,我方可以处理啦
HANDLE g_sendBufferEmptyEvent   = NULL;     // 信号,表示我方可以处理完成,对方可以再次发送



enum    // SDK函数ID
{
    /* 1*/ XL_Init = 1,
    /* 2*/ XL_UnInit,
    /* 3*/ XL_SetDownloadSpeedLimit,
    /* 4*/ XL_SetUploadSpeedLimit,
    /* 5*/ XL_SetProxy,
    /* 6*/ XL_SetUserAgent,
    /* 7*/ XL_CreateP2spTask,
    /* 8*/ XL_SetTaskStrategy,
    /* 9*/ XL_StartTask,
    /* A*/ XL_StopTask,
    /* B*/ XL_DeleteTask,
    /* C*/ XL_QueryTaskInfo,
    /* D*/ XL_AddServer,
    /* E*/ XL_DiscardServer,
    /* F*/ XL_CreateEmuleTask,
    /*10*/ XL_CreateBTTask,
    /*11*/ XL_BTStartUpload,
    /*12*/ XL_BTStopUpload,
    /*13*/ XT_Func_NULL_1,
    /*14*/ XL_AddPeer,
    /*15*/ XL_DiscardPeer,
    /*16*/ XL_QueryTaskIndex,
    /*17*/ XL_QueryTaskFlow,
    /*18*/ XL_QueryGlobalStat,
    /*19*/ XL_EnableDcdn,
    /*1A*/ XL_DisableDcdn,
    /*1B*/ XL_SetUserInfo,
    /*1C*/ XL_SetOriginConnectCount,
    /*1D*/ XL_QueryBTSubFileInfo,
    /*1E*/ XT_Func_NULL_2,
    /*1F*/ XL_SetDownloadStrategy,
    /*20*/ XL_CreateMagnetTask,
    /*21*/ XL_SetGlobalExtInfo,
    /*22*/ XL_SetTaskExtInfo,
    /*23*/ XL_SetP2spTaskIndex,
    /*24*/ XL_SetEmuleTaskIndex,
    /*25*/ XL_SetBTSubTaskIndex,
    /*26*/ XL_QueryPlayInfo,
    /*27*/ XL_SetAccelerateCertification,
    /*28*/ XL_RenameP2spTaskFile,
    /*29*/ XL_SetTaskExtStat,
    /*2A*/ XL_EnableFreeDcdn,
    /*2B*/ XL_DisableFreeDcdn,
    /*2C*/ XL_QueryFreeDcdnAccelerate,
    /*2D*/ XL_SetCacheSize,
    /*2E*/ XL_SetFreeDcdnDownloadSpeedLimit,
    /*2F*/ XL_EnableDcdnWithToken,
    /*30*/ XL_EnableDcdnWithSession,
    /*31*/ XL_SetP2SPTaskIdxURL,
    /*32*/ XL_GetFilePlayInfo,
    /*33*/ XL_SetTaskPriorityLevel,
    /*34*/ XL_BatchAddPeer,
    /*35*/ XL_BatchDiscardPeer,
    /*36*/ XL_GetUnRecvdRangeArray,
    /*37*/ XL_EnableDcdnWithVipCert,
    /*38*/ XL_UpdateDcdnWithVipCert,
    /*39*/ XL_DisableDcdnWithVipCert,
    /*3A*/ XL_GetPeerId,
    /*3B*/ XL_BatchAddBTTracker,
    /*3C*/ XL_SetTaskUserAgent,
    /*3D*/ XL_GetSumOfRemotePeerBeBenefited,
    /*3E*/ XL_IsFileSizeSetterWorking,
    /*3F*/ XL_LaunchFileAssistant,
    /*40*/ XL_GetTaskProfileLog,
    /*41*/ XL_SetGlobalConnectionLimit,
    /*42*/ XL_AddHttpHeaderField,
    /*43*/ XL_GetSubNetUploader,
    /*44*/ XL_IsDownloadTaskCFGFileExit,
    /*45*/ XL_UpdateNetDiscVODCachePath,
    /*46*/ XL_SetupNetDiskFetchTaskFlag,
    /*47*/ XL_UpdateTaskVideoByteRatio
};

/**
 * \brief   调用SDK的函数
 * \return  0-成功，其它失败
 */
int call_sdk_func()
{
    WaitForSingleObject(g_recvBufferEmptyEvent, INFINITE);

    p_data_head p = (p_data_head)g_recv_tmp;
    memcpy(g_recv, g_recv_tmp, p->len + 4);

    ResetEvent(g_recvBufferEmptyEvent);
    SetEvent(g_recvBufferFullEvent);

    WaitForSingleObject(g_sendBufferFullEvent, INFINITE);

    p = (p_data_head)g_send;
    memcpy(g_send_tmp, g_send, p->len + 4);

    ResetEvent(g_sendBufferFullEvent);
    SetEvent(g_sendBufferEmptyEvent);

    DWORD *head = (DWORD*)g_send_tmp;

    if (0 != head[2])
    {
        return 1;
    }

    return 0;
}

/**
 * \brief   调用SDK的进程
 * \return  0-成功，其它失败
 */
int create_process()
{
    g_tail += sprintf_s(g_tail, 10, "%d", GetCurrentProcessId());

    char *arg = GetCommandLineA();      // 参数前后带有""
    char *tail = strrchr(arg, '\\');

    memcpy(g_pth, arg, tail - arg + 1);
    strcat_s(g_pth, sizeof(g_pth), "DownloadSDKServer.exe\" ");
    strcat_s(g_pth, sizeof(g_pth), g_arg);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    BOOL ret = CreateProcessA(NULL, g_pth, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (!ret)
    {
        return -1;
    }

    g_tail += sprintf_s(g_tail, 10, ":%d", pi.dwProcessId);
    return 0;
}

int create_mutex()
{
    strcpy_s(g_tail, 100, "|ProxyAliveMutex");
    g_proxyAliveMutex = CreateMutexA(NULL, TRUE, g_arg);

    if (NULL == g_proxyAliveMutex)
    {
        return -1;
    }

    strcpy_s(g_tail, 100, "|ServerStartUpEvent");
    g_serverStartUpEvent = CreateEventA(NULL, TRUE, FALSE, g_arg);

    if (NULL == g_serverStartUpEvent)
    {
        return -2;
    }

    return 0;
}

int open_sharememory()
{
    strcpy_s(g_tail, 100, "|RecvShareMemory");
    CloseHandle(g_recvShareMemory);
    g_recvShareMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, g_arg);

    if (NULL == g_recvShareMemory)
    {
        return -1;
    }

    g_recv = (UCHAR*)MapViewOfFile(g_recvShareMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0x100000);

    if (NULL == g_recv)
    {
        return -2;
    }

    strcpy_s(g_tail, 100, "|RecvBufferFullEvent");
    CloseHandle(g_recvBufferFullEvent);
    g_recvBufferFullEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    if (NULL == g_recvBufferFullEvent)
    {
        return -3;
    }

    strcpy_s(g_tail, 100, "|RecvBufferEmptyEvent");
    CloseHandle(g_recvBufferEmptyEvent);
    g_recvBufferEmptyEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    if (NULL == g_recvBufferEmptyEvent)
    {
        return -4;
    }

    strcpy_s(g_tail, 100, "|SendShareMemory");
    CloseHandle(g_sendShareMemory);
    g_sendShareMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, g_arg);

    if (NULL == g_sendShareMemory)
    {
        return -5;
    }

    g_send = (UCHAR*)MapViewOfFile(g_sendShareMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0x100000);

    if (NULL == g_send)
    {
        return -6;
    }

    strcpy_s(g_tail, 100, "|SendBufferFullEvent");
    CloseHandle(g_sendBufferFullEvent);
    g_sendBufferFullEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    if (NULL == g_sendBufferFullEvent)
    {
        return -7;
    }

    strcpy_s(g_tail, 100, "|SendBufferEmptyEvent");
    CloseHandle(g_sendBufferEmptyEvent);
    g_sendBufferEmptyEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    if (NULL == g_sendBufferEmptyEvent)
    {
        return -8;
    }

    return 0;
}

int init_sys()
{
    // 等待,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_serverStartUpEvent, INFINITE);

    // 设置后DownloadSDKServer.exe的第二线程退出
    strcpy_s(g_tail, 100, "|AccetpReturnEvent");
    HANDLE accetpReturnEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);
    SetEvent(accetpReturnEvent);
    CloseHandle(accetpReturnEvent);

    // 打开共享内存
    int ret = open_sharememory();

    if (0 != ret)
    {
        return -1;
    }

    // 等待对方接收数据,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_recvBufferEmptyEvent, INFINITE);

    // 写入数据1固定的
    g_recv[0] = 1;

    ResetEvent(g_recvBufferEmptyEvent);
    SetEvent(g_recvBufferFullEvent);

    // 等待对方发送数据,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_sendBufferFullEvent, INFINITE);

    // 接收数据
    DWORD data = *(DWORD*)&g_send[1];
    g_tail += sprintf_s(g_tail, 100, "@%d", data);

    ResetEvent(g_sendBufferFullEvent);
    SetEvent(g_sendBufferEmptyEvent);

    // 等待对方接收数据,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_recvBufferEmptyEvent, INFINITE);

    // 写入数据3固定的
    g_recv[0] = 3;

    ResetEvent(g_recvBufferEmptyEvent);
    SetEvent(g_recvBufferFullEvent);

    // ClientAliveMutex
    strcpy_s(g_tail, 100, "|ClientAliveMutex");
    HANDLE clientAliveMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, g_arg);

    if (NULL == clientAliveMutex)
    {
        return -2;
    }

    // 进入临界区,防止DownloadSDKServer.exe的主循环退出
    WaitForSingleObject(clientAliveMutex, INFINITE);
    CloseHandle(clientAliveMutex);

    // 再次打开共享内存,名称不一样啦
    ret = open_sharememory();

    if (0 != ret)
    {
        return -3;
    }

    // 临时数据
    g_recv_tmp = malloc(0x10000);
    g_send_tmp = malloc(0x10000);

    return 0;
}

int init_sdk()
{
    p_data_head p = (p_data_head)g_recv_tmp;

    // 调用函数XL_Init,格式:数据总长(不包含本身4字节),函数ID,参数1长度,参数1,...
    strcpy_s(p->arg1.data, 100, "xzcGMudGh1bmRlclg7MA^^SDK==edee53fd0b15e8d65dbfe7824f5f^a23");
    p->arg1.len = 59;

    p_arg_head arg2 = (p_arg_head)(p->arg1.data + p->arg1.len);
    arg2->len = 0x28;
    arg2->data[0] = 0xff;
    arg2->data[1] = 0xff;
    arg2->data[2] = 0xff;
    arg2->data[3] = 0xff;
    arg2->data[4] = 0x00;
    arg2->data[5] = 0x00;
    strcpy_s(arg2->data + 6, 20, "11.3.5.1864");

    p->func_id = XL_Init;
    p->len = 8 + p->arg1.len + 4 + arg2->len;

    int ret = call_sdk_func();  // 调用函数

    if (0 != ret)
    {
        return -1;
    }

    p->func_id = XL_GetPeerId;
    p->len = 0x04;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    //char peerid[128];
    //strncpy_s(peerid, sizeof(peerid), g_send + 16, *(DWORD*)(g_send_tmp + 12));;
    //MessageBoxA(NULL, peerid, "peerid", MB_OK);

    p->func_id = XL_SetUserInfo;
    p->data[0] = 0x00;
    p->data[1] = 0x00;
    p->len = 0x0c;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -3;
    }

    p->func_id = XL_SetGlobalExtInfo;
    p->arg1.len = sprintf_s(p->arg1.data, 100, "isvip=0,viptype=,viplevel=0,userchannel=100001");
    p->len = 8 + p->arg1.len;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -4;
    }

    p->func_id = XL_SetDownloadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -5;
    }

    p->func_id = XL_SetUploadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -6;
    }

    p->func_id = XL_SetGlobalConnectionLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -7;
    }

    p->func_id = XL_QueryGlobalStat;
    p->len = 0x04;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -8;
    }

    return 0;
}

/**
 * \brief   初始化SDK
 * \return  0-成功，其它失败
 */
int init()
{
    int ret = create_process(); // 创建子进程,启动DownloadSDKServer.exe

    if (0 != ret)
    {
        return -1;
    }

    ret = create_mutex();

    if (0 != ret)
    {
        return -2;
    }

    ret = init_sys();

    if (0 != ret)
    {
        return -3;
    }

    ret = init_sdk();

    if (0 != ret)
    {
        return -4;
    }

    g_init = TRUE;

    return 0;
}

/**
 * \brief   创建下载种子文件任务
 * \param   [in]    short   *magnet     magnet磁力URL,UNICODE
 * \param   [in]    short   *path       本地存储路径,UNICODE
 * \param   [out]   int     *taskid     任务ID
 * \param   [out]   short   *fullname   种子文件全名,UNICODE
 * \return  0-成功，其它失败
 */
int create_magnet_task(short *magnet, short *path, int *taskid, short *fullname)
{
    if (NULL == magnet || NULL == path || NULL == taskid || NULL == fullname)
    {
        return -1;
    }

    if (!g_init)
    {
        return -2;
    }

    if (0 != wcsncmp(magnet, L"magnet:?", 8))   // 不是磁力连接URL
    {
        return -3;
    }

    wchar_t id[128];
    wcsncpy_s(id, sizeof(id), magnet + 20, 40); // 取得magnet中的id

    // 参数1,磁力连接URL
    p_data_head p = (p_data_head)g_recv_tmp;
    short *url = (short*)p->arg1.data;
    int len = wcslen(magnet);
    p->arg1.len = len;
    memcpy(url, magnet, len * 2);

    // 参数2,本地文件名
    p_arg_head arg2 = (p_arg_head)(p->arg1.data + p->arg1.len * 2);
    short *file = (short*)arg2->data;
    arg2->len = swprintf_s(file, MAX_PATH, L"%s\\%s.torrent", path, id);

    // 创建下载种子文件任务
    p->func_id = XL_CreateMagnetTask;
    p->len = 8 + p->arg1.len * 2 + 4 + arg2->len * 2;

    int ret = call_sdk_func();

    if (0 != ret)
    {
        return -4;
    }

    *taskid = *(int*)(g_send_tmp + 12);

    wcscpy_s(fullname, MAX_PATH, file);

    return 0;
}

/**
 * \brief   创建下载BT文件任务
 * \param   [in]    short   *torrent    种子文件全名
 * \param   [in]    short   *path       本地下载目录
 * \param   [in]    char    *list       文件下载列表,例:"001",0-不下载,1-下载,文件按拼音顺序排列
 * \param   [out]   int     *taskid     任务ID
 * \return  0-成功，其它失败
 */
int create_file_task(short *torrent, short *path, char *list, int *taskid)
{
    if (NULL == torrent || NULL == path || NULL == list || NULL == taskid)
    {
        return -1;
    }

    if (!g_init)
    {
        return -2;
    }

    // 参数1,本地种子文件全名
    p_data_head p = (p_data_head)g_recv_tmp;
    short *file = (short*)p->arg1.data;
    int len = wcslen(torrent);
    p->arg1.len = len;
    memcpy(file, torrent, len * 2);

    // 参数2,本地下载目录
    p_arg_head arg2 = (p_arg_head)(p->arg1.data + p->arg1.len * 2);
    len = wcslen(path);
    memcpy(arg2->data, path, len * 2);

    // 参数3,下载列表,1-下载,0-不下载
    p_arg_head arg3 = (p_arg_head)(arg2->data + arg2->len * 2);
    len = strlen(list);
    memcpy(arg3->data, list, len);

    // 创建下载BT文件任务
    p->func_id = XL_CreateBTTask;
    p->len = 8 + p->arg1.len * 2 + 4 + arg2->len * 2 + 4 + arg3->len;

    int ret = call_sdk_func();

    if (0 != ret)
    {
        return -3;
    }

    *taskid = *(int*)(g_send_tmp + 12);

    return 0;
}

/**
 * \brief   开始下载文件
 * \param   [in]   int     *taski       任务ID
 * \param   [in]   BOOL     magnet      磁力任务
 * \return  0-成功，其它失败
 */
int start_download_file(int taskid, BOOL magnet)
{
    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_SetTaskStrategy;
    p->data[0] = taskid;
    p->data[1] = 7;
    p->len = 0x0c;
    int ret = call_sdk_func();

    if (0 != ret)
    {
        return -1;
    }

    p->func_id = XL_SetTaskExtInfo;
    p->data[0] = taskid;
    p->data[1] = sprintf_s((char*)&(p->data[2]), 100,
                           "parentid=109183876,taskorigin=%s",
                           magnet ? "Magnet" : "newwindow_url");
    p->len = 0x0c + p->data[1];
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    p->func_id = XL_StartTask;
    p->data[0] = taskid;
    p->len = 0x08;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -3;
    }
/*
    // 发送tracker服务器列表
    if (!bt)// || 0 == g_data.announce_len
    {
        return 0;
    }

    //TCHAR info[MAX_PATH];

    short *head = g_data.announce;

    for (int i = 0; head < g_data.announce_tail; i++)
    {
        int len = ((DWORD*)head)[0];

        SP(_T("announce_list[%d] len:%d"), i, len);
        MessageBox(NULL, info, _T("announce_list"), MB_OK);

        memcpy(info, head + 2, len * 2);

        info[len] = L'\0';

        MessageBox(NULL, info, _T("announce_list"), MB_OK);

        head += 2 + len;
    }

    MessageBox(NULL, _T("XL_BatchAddBTTracker begin"), _T("XL_BatchAddBTTracker"), MB_OK);

    int len = ((DWORD*)head)[0];

    p->func_id = XL_BatchAddBTTracker;
    p->data[0] = taskid;
    p->data[1] = 0x65;
    memcpy(p->data + 2, head, 4 + len * 2);
    p->len = 0x0c + 4 + len * 2;
    ret = call_sdk_func();

    MessageBox(NULL, _T("XL_BatchAddBTTracker end"), _T("XL_BatchAddBTTracker"), MB_OK);

    if (0 != ret)
    {
        return -4;
    }
*/
    return 0;
}

/**
 * \brief   停止下载文件
 * \param   [in]   int     *taski       任务ID
 * \return  0-成功，其它失败
 */
int stop_download_file(int taskid)
{
    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_StopTask;
    p->data[0] = taskid;
    p->len = 0x08;
    int ret = call_sdk_func();

    if (0 != ret)
    {
        return -1;
    }

    p->func_id = XL_DeleteTask;
    p->data[0] = taskid;
    p->len = 0x08;
    ret = call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    return 0;
}

/**
 * \brief   得到下载任务信息
 * \param   [in]    int                 *taskid     任务ID
 * \param   [out]   unsigned __int64    *size       下载的数据总大小
 * \param   [out]   unsigned __int64    *down       已经下载的数据总大小
 * \param   [out]   unsigned __int64    *time       本次下载任务用时单位秒
 * \return  0-成功，其它失败
 */
int get_task_info(int taskid, unsigned __int64 *size, unsigned __int64 *down, unsigned int *time)
{
    if (NULL == size || NULL == down || NULL == time)
    {
        return -1;
    }

    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_QueryTaskInfo;
    p->data[0] = taskid;
    p->len = 0x08;

    int ret = call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    *size = *(unsigned __int64*)(g_send_tmp + 0x18);
    *down = *(unsigned __int64*)(g_send_tmp + 0x20);
    *time = *(unsigned int*)(g_send_tmp + 0x30);

    return 0;
}
