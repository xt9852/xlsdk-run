/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         xl_sdk.c
 *\author       xt
 *\version      1.0.0
 *\date         2022-02-08
 *\brief        迅雷接口实现,UTF-8(No BOM)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "xl_sdk.h"
#include "xt_log.h"

#define SIZEOF(x)               sizeof(x)/sizeof(x[0])
#define FLAG                    "BDAF7A63-568C-43ab-9406-D145CF03B08C"

/// 参数头
typedef struct _arg_head
{
    DWORD               len;                ///< 参数长度

    char                data[1];            ///< 参数

}arg_head, *p_arg_head;                     ///< 参数头节点

/// 数据头
typedef struct _data_head
{
    DWORD               len;                ///< 数据长度(不包含本身4字节)

    DWORD               func_id;            ///< 函数ID

    union {
    arg_head            arg1;               ///< 参数1头节点

    DWORD               data[1];            ///< 参数1整数
    };

}data_head, *p_data_head;                   ///< 数据包头节点

typedef struct _task_info
{
    unsigned __int64    down;               ///< 已经下载的数量

    unsigned int        time;               ///< 用时秒数

}task_info, *p_task_info;

int    g_cur_process_id         = 0;        ///< 本进程ID
int    g_sdk_process_id         = 0;        ///< SDK进程ID
int    g_share_memory_id        = 0;        ///< 共享内存ID

BOOL   g_init                   = FALSE;    ///< 是否完成初始化

UCHAR *g_recv                   = NULL;     ///< 共享内存1M,对方接收的数据,我方发送的
UCHAR *g_send                   = NULL;     ///< 共享内存1M,我方发送的数据,需对方处理

UCHAR *g_recv_tmp               = NULL;     ///< 临时缓存区
UCHAR *g_send_tmp               = NULL;     ///< 临时缓存区

HANDLE g_proxyAliveMutex        = NULL;     ///< 互斥
HANDLE g_serverStartUpEvent     = NULL;     ///< 事件

HANDLE g_recvShareMemory        = NULL;     ///< 共享内存1M,我方发送的数据,需要对方处理
HANDLE g_recvBufferFullEvent    = NULL;     ///< 信号,表示有数据,对方可以处理啦
HANDLE g_recvBufferEmptyEvent   = NULL;     ///< 信号,表示对方处理完成,我方可以再次发送

HANDLE g_sendShareMemory        = NULL;     ///< 共享内存1M,对方发送的数据,需要我方处理
HANDLE g_sendBufferFullEvent    = NULL;     ///< 信号,表示有数据,我方可以处理啦
HANDLE g_sendBufferEmptyEvent   = NULL;     ///< 信号,表示我方可以处理完成,对方可以再次发送

int    g_track_len              = 0;        ///< track数据长度

int    g_track_count            = 0;        ///< track数据数量

short *g_track_data[10240]      = {0};      ///< track缓冲区

const short *g_track[] = {
    L"http://1337.abcvg.info:80/announce",
    L"http://filetracker.xyz:11451/announce",
    L"http://nyaa.tracker.wf:7777/announce",
    L"http://opentracker.xyz:80/announce",
    L"http://rt.tace.ru:80/announce",
    L"http://share.camoe.cn:8080/announce",
    L"http://t.nyaatracker.com:80/announce",
    L"http://torrentsmd.com:8080/announce",
    L"http://tr.cili001.com:8070/announce",
    L"http://tracker-cdn.moeking.me:2095/announce",
    L"http://tracker.anirena.com:80/announce",
    L"http://tracker.anirena.com:80/b16a15d9a238d1f59178d3614b857290/announce",
    L"http://tracker.bt4g.com:2095/announce",
    L"http://tracker.darmowy-torrent.pl:80/announce",
    L"http://tracker.files.fm:6969/announce",
    L"http://tracker.gbitt.info:80/announce",
    L"http://tracker.ipv6tracker.ru:80/announce",
    L"http://tracker.tfile.co:80/announce",
    L"http://tracker.trackerfix.com:80/announce",
    L"http://trk.publictracker.xyz:6969/announce",
    L"http://vps02.net.orel.ru:80/announce",
    L"https://1337.abcvg.info:443/announce",
    L"https://bt.nfshost.com:443/announce",
    L"https://tp.m-team.cc:443/announce.php",
    L"https://tracker.coalition.space:443/announce",
    L"https://tracker.foreverpirates.co:443/announce",
    L"https://tracker.gbitt.info:443/announce",
    L"https://tracker.hama3.net:443/announce",
    L"https://tracker.imgoingto.icu:443/announce",
    L"https://tracker.iriseden.eu:443/announce",
    L"https://tracker.iriseden.fr:443/announce",
    L"https://tracker.lilithraws.cf:443/announce",
    L"https://tracker.nanoha.org:443/announce",
    L"https://tracker.nitrix.me:443/announce",
    L"https://tracker.tamersunion.org:443/announce",
    L"https://w.wwwww.wtf:443/announce",
    L"udp://6rt.tace.ru:80/announce",
    L"udp://9.rarbg.me:2710/announce",
    L"udp://9.rarbg.to:2710/announce",
    L"udp://bubu.mapfactor.com:6969/announce",
    L"udp://code2chicken.nl:6969/announce",
    L"udp://discord.heihachi.pw:6969/announce",
    L"udp:/edu.uifr.ru:6969/announce",
    L"udp://engplus.ru:6969/announce",
    L"udp://exodus.desync.com:6969/announce",
    L"udp://explodie.org:6969/announce",
    L"udp://fe.dealclub.de:6969/announce",
    L"udp://free.publictracker.xyz:6969/announce",
    L"udp://ipv6.tracker.zerobytes.xyz:16661/announce",
    L"udp://mail.realliferpg.de:6969/announce",
    L"udp://movies.zsw.ca:6969/announce",
    L"udp://mts.tvbit.co:6969/announce",
    L"udp://newtoncity.org:6969/announce",
    L"udp://open.demonii.com:1337/announce",
    L"udp://open.stealth.si:80/announce",
    L"udp://opentor.org:2710/announce",
    L"udp://opentracker.i2p.rocks:6969/announce",
    L"udp://p4p.arenabg.com:1337/announce",
    L"udp://retracker.lanta-net.ru:2710/announce",
    L"udp://retracker.netbynet.ru:2710/announce",
    L"udp://t1.leech.ie:1337/announce",
    L"udp:/t2.leech.ie:1337/announce",
    L"udp://thetracker.org:80/announce",
    L"udp://torrentclub.online:54123/announce",
    L"udp://tracker.0x.tf:6969/announce",
    L"udp://tracker.altrosky.nl:6969/announce",
    L"udp://tracker.army:6969/announce",
    L"udp://tracker.beeimg.com:6969/announce",
    L"udp://tracker.birkenwald.de:6969/announce",
    L"udp://tracker.ccp.ovh:6969/announce",
    L"udp://tracker.dler.org:6969/announce",
    L"udp://tracker.moeking.me:6969/announce",
    L"udp://tracker.monitorit4.me:6969/announce",
    L"udp://tracker.nrx.me:6969/announce",
    L"udp://tracker.openbittorrent.com:6969/announce",
    L"udp://tracker.opentrackr.org:1337/announce",
    L"udp://tracker.shkinev.me:6969/announce",
    L"udp://tracker.theoks.net:6969/announce",
    L"udp://tracker.tiny-vps.com:6969/announce",
    L"udp://tracker.torrent.eu.org:451/announce",
    L"udp://tracker.uw0.xyz:6969/announce",
    L"udp://tracker.v6speed.org:6969/announce",
    L"udp://tracker.zemoj.com:6969/announce",
    L"udp://tracker.zerobytes.xyz:1337/announce",
    L"udp://tracker0.ufibox.com:6969/announce",
    L"udp://tracker2.dler.org:80/announce",
    L"udp://tracker4.itzmx.com:2710/announce",
    L"udp://u.wwwww.wtf:1/announce",
    L"udp://udp-tracker.shittyurl.org:6969/announce",
    L"udp://valakas.rollo.dnsabr.com:2710/announce",
    L"udp://vibe.community:6969/announce",
    L"udp://vibe.sleepyinternetfun.xyz:1738/announce",
    L"udp://wassermann.online:6969/announce",
    L"udp://www.torrent.eu.org:451/announce",
    L"udp://z.mercax.com:53/announce",
    L"udp://zephir.monocul.us:6969/announce",
    L"udp://tracker4.itzmx.com:2710/announce",
    L"http://tracker4.itzmx.com:2710/announce",
    L"http://tracker3.itzmx.com:6961/announce",
    L"http://tracker2.itzmx.com:6961/announce",
    L"http://tracker1.itzmx.com:8080/announce"
};

/// SDK函数ID
enum    
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

/// 函数名称
const char * SDK_FUNC_NAME[] =
{
    /* 1*/ "XL_Init",
    /* 2*/ "XL_UnInit",
    /* 3*/ "XL_SetDownloadSpeedLimit",
    /* 4*/ "XL_SetUploadSpeedLimit",
    /* 5*/ "XL_SetProxy",
    /* 6*/ "XL_SetUserAgent",
    /* 7*/ "XL_CreateP2spTask",
    /* 8*/ "XL_SetTaskStrategy",
    /* 9*/ "XL_StartTask",
    /* A*/ "XL_StopTask",
    /* B*/ "XL_DeleteTask",
    /* C*/ "XL_QueryTaskInfo",
    /* D*/ "XL_AddServer",
    /* E*/ "XL_DiscardServer",
    /* F*/ "XL_CreateEmuleTask",
    /*10*/ "XL_CreateBTTask",
    /*11*/ "XL_BTStartUpload",
    /*12*/ "XL_BTStopUpload",
    /*13*/ "XT_Func_NULL_1",
    /*14*/ "XL_AddPeer",
    /*15*/ "XL_DiscardPeer",
    /*16*/ "XL_QueryTaskIndex",
    /*17*/ "XL_QueryTaskFlow",
    /*18*/ "XL_QueryGlobalStat",
    /*19*/ "XL_EnableDcdn",
    /*1A*/ "XL_DisableDcdn",
    /*1B*/ "XL_SetUserInfo",
    /*1C*/ "XL_SetOriginConnectCount",
    /*1D*/ "XL_QueryBTSubFileInfo",
    /*1E*/ "XT_Func_NULL_2",
    /*1F*/ "XL_SetDownloadStrategy",
    /*20*/ "XL_CreateMagnetTask",
    /*21*/ "XL_SetGlobalExtInfo",
    /*22*/ "XL_SetTaskExtInfo",
    /*23*/ "XL_SetP2spTaskIndex",
    /*24*/ "XL_SetEmuleTaskIndex",
    /*25*/ "XL_SetBTSubTaskIndex",
    /*26*/ "XL_QueryPlayInfo",
    /*27*/ "XL_SetAccelerateCertification",
    /*28*/ "XL_RenameP2spTaskFile",
    /*29*/ "XL_SetTaskExtStat",
    /*2A*/ "XL_EnableFreeDcdn",
    /*2B*/ "XL_DisableFreeDcdn",
    /*2C*/ "XL_QueryFreeDcdnAccelerate",
    /*2D*/ "XL_SetCacheSize",
    /*2E*/ "XL_SetFreeDcdnDownloadSpeedLimit",
    /*2F*/ "XL_EnableDcdnWithToken",
    /*30*/ "XL_EnableDcdnWithSession",
    /*31*/ "XL_SetP2SPTaskIdxURL",
    /*32*/ "XL_GetFilePlayInfo",
    /*33*/ "XL_SetTaskPriorityLevel",
    /*34*/ "XL_BatchAddPeer",
    /*35*/ "XL_BatchDiscardPeer",
    /*36*/ "XL_GetUnRecvdRangeArray",
    /*37*/ "XL_EnableDcdnWithVipCert",
    /*38*/ "XL_UpdateDcdnWithVipCert",
    /*39*/ "XL_DisableDcdnWithVipCert",
    /*3A*/ "XL_GetPeerId",
    /*3B*/ "XL_BatchAddBTTracker",
    /*3C*/ "XL_SetTaskUserAgent",
    /*3D*/ "XL_GetSumOfRemotePeerBeBenefited",
    /*3E*/ "XL_IsFileSizeSetterWorking",
    /*3F*/ "XL_LaunchFileAssistant",
    /*40*/ "XL_GetTaskProfileLog",
    /*41*/ "XL_SetGlobalConnectionLimit",
    /*42*/ "XL_AddHttpHeaderField",
    /*43*/ "XL_GetSubNetUploader",
    /*44*/ "XL_IsDownloadTaskCFGFileExit",
    /*45*/ "XL_UpdateNetDiscVODCachePath",
    /*46*/ "XL_SetupNetDiskFetchTaskFlag",
    /*47*/ "XL_UpdateTaskVideoByteRatio"
};

/**
 *\brief    调用SDK的函数
 *\return   0   成功
 */
int xl_sdk_call_sdk_func()
{
    WaitForSingleObject(g_recvBufferEmptyEvent, INFINITE);  // 等待对方准备好接收数据信号

    p_data_head p = (p_data_head)g_recv_tmp;
    memcpy(g_recv, g_recv_tmp, p->len + 4);                 // 复制数据到对方缓冲区

    ResetEvent(g_recvBufferEmptyEvent);                     // 清空对方准备好接收数据信号
    SetEvent(g_recvBufferFullEvent);                        // 设置对方接收数据信号

    DBG("send %s len:%d", SDK_FUNC_NAME[p->func_id], p->len);

    WaitForSingleObject(g_sendBufferFullEvent, INFINITE);   // 等待对方已经发送数据信号

    p = (p_data_head)g_send;
    memcpy(g_send_tmp, g_send, p->len + 4);                 // 将对方数据复制到自己的缓冲区

    ResetEvent(g_sendBufferFullEvent);                      // 清空对方已经发送数据信号
    SetEvent(g_sendBufferEmptyEvent);                       // 设置对方已经发送数据完成信号

    DWORD *head = (DWORD*)g_send_tmp;

    if (0 != head[2])                                       // 返回值:0-成功
    {
        ERR("recv %s len:%d ret:%d", SDK_FUNC_NAME[p->func_id], head[2]);
        return head[2];
    }

    DBG("recv %s len:%d ret:0", SDK_FUNC_NAME[p->func_id], p->len);
    return 0;
}

/**
 *\brief    打开共享内存和内存操作信号,RecvShareMemory,SendShareMemory
 *\return   0   成功
 */
int xl_sdk_open_share_memory_event(bool reopen)
{
    char mem[16] = "";
    char name[512];

    if (reopen)
    {
        sprintf_s(mem, sizeof(mem), "@%d", g_share_memory_id);
    }

    if (NULL != g_recvShareMemory)
    {
        CloseHandle(g_recvShareMemory);
    }

    if (NULL != g_recvBufferFullEvent)
    {
        CloseHandle(g_recvBufferFullEvent);
    }

    if (NULL != g_recvBufferEmptyEvent)
    {
        CloseHandle(g_recvBufferEmptyEvent);
    }

    if (NULL != g_sendShareMemory)
    {
        CloseHandle(g_sendShareMemory);
    }

    if (NULL != g_sendBufferFullEvent)
    {
        CloseHandle(g_sendBufferFullEvent);
    }

    if (NULL != g_sendBufferEmptyEvent)
    {
        CloseHandle(g_sendBufferEmptyEvent);
    }

    // 对方接收的数据
    sprintf_s(name, sizeof(name), "%s:%d:%d%s|RecvShareMemory", FLAG, g_cur_process_id, g_sdk_process_id, mem);

    g_recvShareMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);

    if (NULL == g_recvShareMemory)
    {
        ERR("open fail %s", name);
        return -11;
    }

    DBG(name);

    g_recv = (UCHAR*)MapViewOfFile(g_recvShareMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0x100000);

    if (NULL == g_recv)
    {
        ERR("map fail %s", name);
        return -12;
    }

    DBG(name);

    sprintf_s(name, sizeof(name), "%s:%d:%d%s|RecvBufferFullEvent", FLAG, g_cur_process_id, g_sdk_process_id, mem);

    g_recvBufferFullEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, name);

    if (NULL == g_recvBufferFullEvent)
    {
        ERR("open fail %s", name);
        return -13;
    }

    DBG(name);

    sprintf_s(name, sizeof(name), "%s:%d:%d%s|RecvBufferEmptyEvent", FLAG, g_cur_process_id, g_sdk_process_id, mem);

    g_recvBufferEmptyEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, name);

    if (NULL == g_recvBufferEmptyEvent)
    {
        ERR("open fail %s", name);
        return -14;
    }

    DBG(name);

    // 对方发送的数据
    sprintf_s(name, sizeof(name), "%s:%d:%d%s|SendShareMemory", FLAG, g_cur_process_id, g_sdk_process_id, mem);

    g_sendShareMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);

    if (NULL == g_sendShareMemory)
    {
        ERR("open fail %s", name);
        return -21;
    }

    DBG(name);

    g_send = (UCHAR*)MapViewOfFile(g_sendShareMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0x100000);

    if (NULL == g_send)
    {
        ERR("map fail %s", name);
        return -22;
    }

    DBG(name);

    sprintf_s(name, sizeof(name), "%s:%d:%d%s|SendBufferFullEvent", FLAG, g_cur_process_id, g_sdk_process_id, mem);

    g_sendBufferFullEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, name);

    if (NULL == g_sendBufferFullEvent)
    {
        ERR("open fail %s", name);
        return -23;
    }

    DBG(name);

    sprintf_s(name, sizeof(name), "%s:%d:%d%s|SendBufferEmptyEvent", FLAG, g_cur_process_id, g_sdk_process_id, mem);

    g_sendBufferEmptyEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, name);

    if (NULL == g_sendBufferEmptyEvent)
    {
        ERR("open fail %s", name);
        return -24;
    }

    DBG(name);
    return 0;
}

/**
 *\brief    创建SDK的进程,"C:\***\DownloadSDKServer.exe" BDAF7A63-568C-43ab-9406-D145CF03B08C:2824(父进程ID)
 *\return   0   成功
 */
int xl_sdk_create_download_process()
{
    g_cur_process_id = GetCurrentProcessId();

    char name[512];
    sprintf_s(name, sizeof(name), "\"DownloadSDKServer.exe\" %s:%d", FLAG, g_cur_process_id);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    BOOL ret = CreateProcessA(NULL, name, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (!ret)
    {
        ERR("create fail %s", name);
        return -1;
    }

    g_sdk_process_id = pi.dwProcessId;

    DBG("%s sdk:%d", name, g_sdk_process_id);
    return 0;
}

/**
 *\brief    创建互斥和信号,ProxyAliveMutex,ServerStartUpEvent
 *\return   0   成功
 */
int xl_sdk_create_ServerStartUpEvent()
{
    char name[512];

    sprintf_s(name, sizeof(name), "%s:%d:%d|ProxyAliveMutex", FLAG, g_cur_process_id, g_sdk_process_id);

    g_proxyAliveMutex = CreateMutexA(NULL, TRUE, name);

    if (NULL == g_proxyAliveMutex)
    {
        ERR("create fail %s", name);
        return -1;
    }

    DBG(name);

    sprintf_s(name, sizeof(name), "%s:%d:%d|ServerStartUpEvent", FLAG, g_cur_process_id, g_sdk_process_id);

    g_serverStartUpEvent = CreateEventA(NULL, TRUE, FALSE, name);

    if (NULL == g_serverStartUpEvent)
    {
        ERR("create fail %s", name);
        return -2;
    }

    DBG(name);
    return 0;
}

/**
 *\brief    创建信号,发送数据1,接收数据2,发送数据3
 *\return   0   成功
 */
int xl_sdk_open_AccetpReturnEvent()
{
    WaitForSingleObject(g_serverStartUpEvent, INFINITE);    // 等待,由DownloadSDKServer.exe触发

    char name[512];

    sprintf_s(name, sizeof(name), "%s:%d:%d|AccetpReturnEvent", FLAG, g_cur_process_id, g_sdk_process_id);

    HANDLE accetpReturnEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, name);

    if (NULL == accetpReturnEvent)
    {
        ERR("open fail %s", name);
        return -1;
    }

    SetEvent(accetpReturnEvent);    // 设置后DownloadSDKServer.exe的第二线程退出
    CloseHandle(accetpReturnEvent);

    DBG(name);
    return 0;
}

/**
 *\brief    打开互斥,ClientAliveMutex
 *\return   0   成功
 */
int xl_sdk_open_ClientAliveMutex()
{
    char name[512];

    sprintf_s(name, sizeof(name), "%s:%d:%d@%d|ClientAliveMutex", FLAG, g_cur_process_id, g_sdk_process_id, g_share_memory_id);

    HANDLE clientAliveMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, name);

    if (NULL == clientAliveMutex)
    {
        ERR("open fail %s", name);
        return -1;
    }

    WaitForSingleObject(clientAliveMutex, INFINITE);    // 进入临界区,防止DownloadSDKServer.exe的主循环退出
    CloseHandle(clientAliveMutex);

    DBG(name);
    return 0;
}

/**
 *\brief    得到共享内存ID,发送数据1,接收数据2,发送数据3
 *\return   0   成功
 */
int xl_sdk_call_get_share_memory_id()
{
    // 等待对方接收数据,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_recvBufferEmptyEvent, INFINITE);

    // 写入固定数据1
    g_recv[0] = 1;

    ResetEvent(g_recvBufferEmptyEvent);
    SetEvent(g_recvBufferFullEvent);

    DBG("write 1");

    // 等待对方发送数据,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_sendBufferFullEvent, INFINITE);

    // 固定数据2
    if (g_send[0] != 2)
    {
        ERR("get share memory id fail");
        return -1;
    }

    // 共享内存ID,4字节大小
    g_share_memory_id = *(DWORD*)&g_send[1];
    DBG("recei 2 id:%d", g_share_memory_id);

    ResetEvent(g_sendBufferFullEvent);
    SetEvent(g_sendBufferEmptyEvent);

    // 等待对方接收数据,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_recvBufferEmptyEvent, INFINITE);

    // 写入固定数据3
    g_recv[0] = 3;

    ResetEvent(g_recvBufferEmptyEvent);
    SetEvent(g_recvBufferFullEvent);

    DBG("write 3");

    return 0;
}

/**
 *\brief    初始化SDK,调用XL_Init,XL_GetPeerId,XL_SetUserInfo,XL_SetGlobalExtInfo,XL_SetDownloadSpeedLimit,XL_SetUploadSpeedLimit
 *\return   0   成功
 */
int xl_sdk_call_sdk_init()
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

    int ret = xl_sdk_call_sdk_func();  // 调用函数

    if (0 != ret)
    {
        ERR("call XL_Init fail");
        return -1;
    }

    p->func_id = XL_GetPeerId;
    p->len = 0x04;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_GetPeerId fail");
        return -2;
    }

    //char peerid[128];
    //strncpy_s(peerid, sizeof(peerid), g_send + 16, *(DWORD*)(g_send_tmp + 12));;
    //MessageBoxA(NULL, peerid, "peerid", MB_OK);

    p->func_id = XL_SetUserInfo;
    p->data[0] = 0x00;
    p->data[1] = 0x00;
    p->len = 0x0c;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_SetUserInfo fail");
        return -3;
    }

    p->func_id = XL_SetGlobalExtInfo;
    p->arg1.len = sprintf_s(p->arg1.data, 100, "isvip=0,viptype=,viplevel=0,userchannel=100001");
    p->len = 8 + p->arg1.len;
    ret =xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_SetGlobalExtInfo fail");
        return -4;
    }

    p->func_id = XL_SetDownloadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_SetDownloadSpeedLimit fail");
        return -5;
    }

    p->func_id = XL_SetUploadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_SetUploadSpeedLimit fail");
        return -6;
    }

    p->func_id = XL_SetGlobalConnectionLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_SetGlobalConnectionLimit fail");
        return -7;
    }

    p->func_id = XL_QueryGlobalStat;
    p->len = 0x04;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        ERR("call XL_QueryGlobalStat fail");
        return -8;
    }

    DBG("call XL_Init ok");
    return 0;
}

/**
 *\brief    生成TRACK数据UNICODE
 *\return   0   成功
 */
int xl_sdk_proc_track()
{
    DWORD len;

    g_track_count = SIZEOF(g_track);

    for (int i = 0; i < g_track_count; i++)
    {
        len = wcslen(g_track[i]);

        *((DWORD*)g_track_data + g_track_len) = len;

        memcpy(g_track_data + g_track_len + 2, g_track[i], len * 2);

        g_track_len += 2 + len;
    }

    return 0;
}

/**
 *\brief    初始化SDK
 *\return   0   成功
 */
int xl_sdk_init()
{
    int ret = xl_sdk_create_download_process();

    if (0 != ret)
    {
        ERR("create process fail %d", ret);
        return -1;
    }

    ret = xl_sdk_create_ServerStartUpEvent();

    if (0 != ret)
    {
        ERR("create handler fail %d", ret);
        return -2;
    }

    ret = xl_sdk_open_AccetpReturnEvent();

    if (0 != ret)
    {
        ERR("open handler fail %d", ret);
        return -3;
    }

    ret = xl_sdk_open_share_memory_event(false);

    if (0 != ret)
    {
        ERR("open share memory fail %d", ret);
        return -4;
    }

    ret = xl_sdk_call_get_share_memory_id();

    if (0 != ret)
    {
        ERR("get share memory fail %d", ret);
        return -5;
    }

    ret = xl_sdk_open_ClientAliveMutex();

    if (0 != ret)
    {
        ERR("open handler fail %d", ret);
        return -6;
    }

    ret = xl_sdk_open_share_memory_event(true);   // 再次打开共享内存,名称不一样啦

    if (0 != ret)
    {
        ERR("reopen share memory fail %d", ret);
        return -7;
    }

    g_recv_tmp = malloc(0x10000);
    g_send_tmp = malloc(0x10000);

    ret = xl_sdk_call_sdk_init();

    if (0 != ret)
    {
        ERR("init sdk fail %d", ret);
        return -8;
    }

    ret = xl_sdk_proc_track();

    if (0 != ret)
    {
        ERR("create track fail %d", ret);
        return -9;
    }

    g_init = TRUE;

    DBG("ok");
    return 0;
}

/**
 *\brief        添加服务器
 *\param[in]    taski       任务ID
 *\param[in]    count       服务器数量
 *\param[in]    data        服务器数据
 *\param[in]    data_len    数据长度
 *\return       0           成功
 */
int xl_sdk_add_bt_tracker(int taskid, int count, void *data, int data_len)
{
    int cnt = SIZEOF(g_track);

    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_BatchAddBTTracker;
    p->data[0] = taskid;
    p->data[1] = count + g_track_count;
    p->len     = 0x0c + data_len + g_track_len;

    char *ptr = (char*)(p->data + 2);

    memcpy(ptr, data, data_len);

    ptr += data_len;

    memcpy(ptr + data_len, g_track_data, g_track_len);

    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -1;
    }

    return 0;
}

/**
 * \brief       创建下载种子文件任务
 * \param[in]   magnet      magnet磁力URL,UNICODE
 * \param[in]   path        本地存储路径,UNICODE
 * \param[out]  taskid      任务ID
 * \param[out]  task_name   任务名称
 * \return      0           成功
 */
int xl_sdk_create_magnet_task(short *magnet, short *path, int *taskid, short *task_name)
{
    if (NULL == magnet || NULL == path || NULL == taskid || NULL == task_name)
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

    short id[128];
    wcsncpy_s(id, sizeof(id), magnet + 20, 40); // 取得magnet中的id

    // 参数1,磁力连接URL
    int len = wcslen(magnet);
    p_data_head p = (p_data_head)g_recv_tmp;
    memcpy(p->arg1.data, magnet, len * 2);
    p->arg1.len = len;

    // 参数2,本地文件名
    p_arg_head arg2 = (p_arg_head)(p->arg1.data + p->arg1.len * 2);
    short *fullname = (short*)arg2->data;
    arg2->len = swprintf_s(fullname, MAX_PATH, L"%s\\%s.torrent", path, id);

    // 创建下载种子文件任务
    p->func_id = XL_CreateMagnetTask;
    p->len = 8 + p->arg1.len * 2 + 4 + arg2->len * 2;

    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -4;
    }

    *taskid = *(int*)(g_send_tmp + 12);

    wcscpy_s(task_name, MAX_PATH, fullname);

    return 0;
}

/**
 * \brief       创建下载BT文件任务
 * \param[in]   torrent         种子文件全名
 * \param[in]   path            本地下载目录
 * \param[in]   list            文件下载列表,例:"001",0-不下载,1-下载,文件按拼音顺序排列
 * \param[in]   announce_count  tracker服务器数量
 * \param[in]   announce        tracker服务器数据
 * \param[in]   announce_len    tracker服务器数据长度
 * \param[out]  taskid          任务ID
 * \param[out]  task_name       任务名称
 * \return      0               成功
 */
int xl_sdk_create_bt_task(short *torrent, short *path, char *list, int announce_count, short *announce, int announce_len, int *taskid)
{
    if (NULL == torrent || NULL == path || NULL == list ||
        NULL == announce || announce_count < 0 || announce_len < 0 ||
        NULL == taskid)
    {
        return -1;
    }

    if (!g_init)
    {
        return -2;
    }

    // 参数1,本地种子文件全名
    int len = wcslen(torrent);
    p_data_head p = (p_data_head)g_recv_tmp;
    memcpy(p->arg1.data, torrent, len * 2);
    p->arg1.len = len;

    // 参数2,本地下载目录
    len = wcslen(path);
    p_arg_head arg2 = (p_arg_head)(p->arg1.data + p->arg1.len * 2);
    memcpy(arg2->data, path, len * 2);
    arg2->len = len;

    // 参数3,下载列表,1-下载,0-不下载
    len = strlen(list);
    p_arg_head arg3 = (p_arg_head)(arg2->data + arg2->len * 2);
    memcpy(arg3->data, list, len);
    arg3->len = len;

    // 创建下载BT文件任务
    p->func_id = XL_CreateBTTask;
    p->len = 8 + p->arg1.len * 2 + 4 + arg2->len * 2 + 4 + arg3->len;

    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -3;
    }

    *taskid = *(int*)(g_send_tmp + 12);

    if (announce_count <=0 || announce_len <= 0)
    {
        return 0;
    }

    ret = xl_sdk_add_bt_tracker(*taskid, announce_count, announce, announce_len);

    if (0 != ret)
    {
        return -4;
    }

    return 0;
}

/**
 * \brief       创建下载URL文件任务
 * \param[in]   url             URL地址
 * \param[in]   path            本地下载目录
 * \param[out]  taskid          任务ID
 * \param[out]  task_name       任务名称
 * \return      0               成功
 */
int xl_sdk_create_url_task(short *url, short *path, int *taskid, short *task_name)
{
    if (NULL == url || NULL == path || NULL == taskid || NULL == task_name)
    {
        return -1;
    }

    if (!g_init)
    {
        return -2;
    }

    short *filename = wcsrchr(url, L'/');

    if (NULL == filename)
    {
        return -3;
    }

    filename++;

    // 参数1,URL地址
    int len = wcslen(url);
    p_data_head p = (p_data_head)g_recv_tmp;
    memcpy(p->arg1.data, url, len * 2);
    p->arg1.len = len;

    // 参数2,8个00
    char *arg2 = p->arg1.data + p->arg1.len * 2;
    memset(arg2, 0, 8);

    // 参数3,本地下载目录
    len = wcslen(path);
    p_arg_head arg3 = (p_arg_head)(arg2 + 8);
    memcpy(arg3->data, path, len * 2);
    arg3->len = len;

    // 参数4,文件名称
    len = wcslen(filename);
    p_arg_head arg4 = (p_arg_head)(arg3->data + arg3->len * 2);
    memcpy(arg4->data, filename, len * 2);
    arg4->len = len;

    // 创建下载URL文件任务
    p->func_id = XL_CreateP2spTask;
    p->len = 8 + p->arg1.len * 2 + 8 + 4 + arg3->len * 2 + 4 + arg4->len * 2;

    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        swprintf_s(task_name, MAX_PATH, L"ret:%d taskname:%s\\%s", ret, path, filename);
        MessageBox(NULL, task_name, L"error", MB_OK);
        return -4;
    }

    *taskid = *(int*)(g_send_tmp + 12);

    swprintf_s(task_name, MAX_PATH, L"%s\\%s", path, filename);

    MessageBox(NULL, task_name, L"", MB_OK);

    return 0;
}

/**
 * \brief       开始下载文件
 * \param[in]   taski           任务ID
 * \param[in]   task_type       任务类型
 * \return      0               成功
 */
int xl_sdk_start_download_file(int taskid, int task_type)
{
    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_SetTaskStrategy;
    p->data[0] = taskid;
    p->data[1] = 7;
    p->len = 0x0c;
    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -1;
    }

    p->func_id = XL_SetTaskExtInfo;
    p->data[0] = taskid;
    p->data[1] = sprintf_s((char*)&(p->data[2]), 100,
                           "parentid=109183876,taskorigin=%s",
                           (TASK_MAGNET == task_type) ? "Magnet" : "newwindow_url");
    p->len = 0x0c + p->data[1];
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    p->func_id = XL_StartTask;
    p->data[0] = taskid;
    p->len = 0x08;
    ret = xl_sdk_call_sdk_func();

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
 * \brief       停止下载文件
 * \param[in]   taskid          任务ID
 * \return      0               成功
 */
int xl_sdk_stop_download_file(int taskid)
{
    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_StopTask;
    p->data[0] = taskid;
    p->len = 0x08;
    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -1;
    }

    p->func_id = XL_DeleteTask;
    p->data[0] = taskid;
    p->len = 0x08;
    ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    return 0;
}

/**
 * \brief       得到下载任务信息
 * \param[in]   taskid          任务ID
 * \param[out]  size            下载的数据总大小
 * \param[out]  down            已经下载的数据总大小
 * \param[out]  time            本次下载任务用时单位秒
 * \return      0               成功
 */
int xl_sdk_get_task_info(int taskid, unsigned __int64 *size, unsigned __int64 *down, unsigned int *time)
{
    if (NULL == size || NULL == down || NULL == time)
    {
        return -1;
    }

    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_QueryTaskInfo;
    p->data[0] = taskid;
    p->len = 0x08;

    int ret = xl_sdk_call_sdk_func();

    if (0 != ret)
    {
        return -2;
    }

    *size = *(unsigned __int64*)(g_send_tmp + 0x18);
    *down = *(unsigned __int64*)(g_send_tmp + 0x20);
    *time = *(unsigned int*)(g_send_tmp + 0x30);

    return 0;
}
