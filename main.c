/**
 * Copyright:   2022, XT Tech. Co., Ltd.
 * File name:   main.c
 * Description: 主模块实现
 * Author:      张海涛
 * Version:     0.0.0.1
 * Code:        UTF-8(无BOM)
 * Date:        2022-02-08
 * History:     2022-02-08 创建此文件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

#ifndef i8
#define i8           __int8
#define i16          __int16
#define i32          __int32
#define i64          __int64
#define u8  unsigned __int8
#define u16 unsigned __int16
#define u32 unsigned __int32
#define u64 unsigned __int64
#endif

#define SIZEOF(x)               sizeof(x)/sizeof(x[0])
#define SP(...)                 _stprintf_s(info, SIZEOF(info), __VA_ARGS__)

typedef struct _arg_head {
    DWORD   len;
    char    data[1];
}arg_head,*p_arg_head;

typedef struct _data_head {
    DWORD       len;
    DWORD       func_id;
    union {
    arg_head    arg1;
    DWORD       data[1];
    };

}data_head,*p_data_head;


NOTIFYICONDATA g_nid            = {0};  // 任务栏图标数据结构
UINT   WM_MY_NOTIFY             = 0;    // 注册系统消息

TCHAR *g_title                  = _T("XLDownloadSDKServerStart");
HFONT  g_font                   = NULL;
HMENU  g_menu                   = NULL;
HWND   g_edit                   = NULL;
HWND   g_butt                   = NULL;
HWND   g_list                   = NULL;

char   g_pth[512]               = "";
char   g_arg[128]               = "BDAF7A63-568C-43ab-9406-D145CF03B08C:";
char  *g_tail                   = g_arg + 37;

UCHAR *g_recv                   = NULL; // 共享内存1M,对方接收的数据,我方发送的
UCHAR *g_send                   = NULL;

UCHAR *g_recv_tmp               = NULL;
UCHAR *g_send_tmp               = NULL;


HANDLE g_proxyAliveMutex        = NULL;
HANDLE g_serverStartUpEvent     = NULL;

HANDLE g_recvShareMemory        = NULL; // 共享内存1M,我方发送的数据,需要对方处理
HANDLE g_recvBufferFullEvent    = NULL; // 信号,表示有数据,对方可以处理啦
HANDLE g_recvBufferEmptyEvent   = NULL; // 信号,表示对方处理完成,我方可以再次发送

HANDLE g_sendShareMemory        = NULL; // 共享内存1M,对方发送的数据,需要我方处理
HANDLE g_sendBufferFullEvent    = NULL; // 信号,表示有数据,我方可以处理啦
HANDLE g_sendBufferEmptyEvent   = NULL; // 信号,表示我方可以处理完成,对方可以再次发送

enum FUNCID
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

TCHAR *g_funcname[] =
{
    _T(""),
    _T("XL_Init"),
    _T("XL_UnInit"),
    _T("XL_SetDownloadSpeedLimit"),
    _T("XL_SetUploadSpeedLimit"),
    _T("XL_SetProxy"),
    _T("XL_SetUserAgent"),
    _T("XL_CreateP2spTask"),
    _T("XL_SetTaskStrategy"),
    _T("XL_StartTask"),
    _T("XL_StopTask"),
    _T("XL_DeleteTask"),
    _T("XL_QueryTaskInfo"),
    _T("XL_AddServer"),
    _T("XL_DiscardServer"),
    _T("XL_CreateEmuleTask"),
    _T("XL_CreateBTTask"),
    _T("XL_BTStartUpload"),
    _T("XL_BTStopUpload"),
    _T("XT_Func_NULL_1"),
    _T("XL_AddPeer"),
    _T("XL_DiscardPeer"),
    _T("XL_QueryTaskIndex"),
    _T("XL_QueryTaskFlow"),
    _T("XL_QueryGlobalStat"),
    _T("XL_EnableDcdn"),
    _T("XL_DisableDcdn"),
    _T("XL_SetUserInfo"),
    _T("XL_SetOriginConnectCount"),
    _T("XL_QueryBTSubFileInfo"),
    _T("XT_Func_NULL_2"),
    _T("XL_SetDownloadStrategy"),
    _T("XL_CreateMagnetTask"),
    _T("XL_SetGlobalExtInfo"),
    _T("XL_SetTaskExtInfo"),
    _T("XL_SetP2spTaskIndex"),
    _T("XL_SetEmuleTaskIndex"),
    _T("XL_SetBTSubTaskIndex"),
    _T("XL_QueryPlayInfo"),
    _T("XL_SetAccelerateCertification"),
    _T("XL_RenameP2spTaskFile"),
    _T("XL_SetTaskExtStat"),
    _T("XL_EnableFreeDcdn"),
    _T("XL_DisableFreeDcdn"),
    _T("XL_QueryFreeDcdnAccelerate"),
    _T("XL_SetCacheSize"),
    _T("XL_SetFreeDcdnDownloadSpeedLimit"),
    _T("XL_EnableDcdnWithToken"),
    _T("XL_EnableDcdnWithSession"),
    _T("XL_SetP2SPTaskIdxURL"),
    _T("XL_GetFilePlayInfo"),
    _T("XL_SetTaskPriorityLevel"),
    _T("XL_BatchAddPeer"),
    _T("XL_BatchDiscardPeer"),
    _T("XL_GetUnRecvdRangeArray"),
    _T("XL_EnableDcdnWithVipCert"),
    _T("XL_UpdateDcdnWithVipCert"),
    _T("XL_DisableDcdnWithVipCert"),
    _T("XL_GetPeerId"),
    _T("XL_BatchAddBTTracker"),
    _T("XL_SetTaskUserAgent"),
    _T("XL_GetSumOfRemotePeerBeBenefited"),
    _T("XL_IsFileSizeSetterWorking"),
    _T("XL_LaunchFileAssistant"),
    _T("XL_GetTaskProfileLog"),
    _T("XL_SetGlobalConnectionLimit"),
    _T("XL_AddHttpHeaderField"),
    _T("XL_GetSubNetUploader"),
    _T("XL_IsDownloadTaskCFGFileExit"),
    _T("XL_UpdateNetDiscVODCachePath"),
    _T("XL_SetupNetDiskFetchTaskFlag"),
    _T("XL_UpdateTaskVideoByteRatio")
};

void create_mutex()
{
    strcpy_s(g_tail, 100, "|ProxyAliveMutex");
    g_proxyAliveMutex = CreateMutexA(NULL, TRUE, g_arg);

    strcpy_s(g_tail, 100, "|ServerStartUpEvent");
    g_serverStartUpEvent = CreateEventA(NULL, TRUE, FALSE, g_arg);
}

void open_sharememory()
{
    strcpy_s(g_tail, 100, "|RecvShareMemory");
    CloseHandle(g_recvShareMemory);
    g_recvShareMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, g_arg);
    g_recv = (UCHAR*)MapViewOfFile(g_recvShareMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0x100000);

    if (NULL == g_recv)
    {
        MessageBoxA(NULL, "recv MapViewOfFile error", "info", MB_OK);
    }

    strcpy_s(g_tail, 100, "|RecvBufferFullEvent");
    CloseHandle(g_recvBufferFullEvent);
    g_recvBufferFullEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    strcpy_s(g_tail, 100, "|RecvBufferEmptyEvent");
    CloseHandle(g_recvBufferEmptyEvent);
    g_recvBufferEmptyEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    strcpy_s(g_tail, 100, "|SendShareMemory");
    CloseHandle(g_sendShareMemory);
    g_sendShareMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, g_arg);
    g_send = (UCHAR*)MapViewOfFile(g_sendShareMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0x100000);

    if (NULL == g_send)
    {
        MessageBoxA(NULL, "send MapViewOfFile error", "info", MB_OK);
    }

    strcpy_s(g_tail, 100, "|SendBufferFullEvent");
    CloseHandle(g_sendBufferFullEvent);
    g_sendBufferFullEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);

    strcpy_s(g_tail, 100, "|SendBufferEmptyEvent");
    CloseHandle(g_sendBufferEmptyEvent);
    g_sendBufferEmptyEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);
}

void init_sys()
{
    // 等待,由DownloadSDKServer.exe触发
    WaitForSingleObject(g_serverStartUpEvent, INFINITE);

    // 设置后DownloadSDKServer.exe的第二线程退出
    strcpy_s(g_tail, 100, "|AccetpReturnEvent");
    HANDLE accetpReturnEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, g_arg);
    SetEvent(accetpReturnEvent);
    CloseHandle(accetpReturnEvent);

    // 打开共享内存
    open_sharememory();

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

    // 进入临界区,防止DownloadSDKServer.exe的主循环退出
    WaitForSingleObject(clientAliveMutex, INFINITE);
    CloseHandle(clientAliveMutex);

    // 再次打开共享内存,名称不一样啦
    open_sharememory();

    // 临时数据
    g_recv_tmp = malloc(0x10000);
    g_send_tmp = malloc(0x10000);
}

void call_func()
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

    //-------------------------------------
    DWORD *head = (DWORD*)g_send_tmp;

    if (0 != head[2])
    {
        TCHAR info[128];
        _stprintf_s(info, SIZEOF(info), _T("数据长:%02x 函数:%s 结果:%02x"),
                head[0], g_funcname[head[1]], head[2]);
        MessageBox(NULL, info, g_title, MB_OK);
    }
}

void init_sdk()
{
    p_data_head p = (p_data_head)g_recv_tmp;

    // 调用函数XL_Init,格式:数据总长(不包含本身4字节),函数ID,参数1长度,参数
    p->func_id = XL_Init;
    p->arg1.len = sprintf_s(p->arg1.data, 100, "xzcGMudGh1bmRlclg7MA^^SDK=="
                                               "edee53fd0b15e8d65dbfe7824f5f^a23");

    p_arg_head arg2 = (p_arg_head)&(p->arg1.data[p->arg1.len]);
    arg2->len = 0x28;
    memset(arg2->data, 0, arg2->len);
    strcpy_s(arg2->data, 20, "\xff\xff\xff\xff  11.3.5.1864");
    arg2->data[4] = 0x00;   // 字符串中的空格应该是\x00
    arg2->data[5] = 0x00;

    p->len = 8 + p->arg1.len + 4 + arg2->len;

    call_func();  // 调用函数

    p->func_id = XL_GetPeerId;
    p->len = 0x04;
    call_func();

    //char peerid[128];
    //strncpy_s(peerid, sizeof(peerid), g_send + 16, *(DWORD*)(g_send_tmp + 12));;
    //MessageBoxA(NULL, peerid, "peerid", MB_OK);

    p->func_id = XL_SetUserInfo;
    p->data[0] = 0x00;
    p->data[1] = 0x00;
    p->len = 0x0c;
    call_func();

    p->func_id = XL_SetGlobalExtInfo;
    p->arg1.len = sprintf_s(p->arg1.data, 100, "isvip=0,viptype=,viplevel=0,userchannel=100001");
    p->len = 8 + p->arg1.len;
    call_func();

    p->func_id = XL_SetDownloadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    call_func();

    p->func_id = XL_SetUploadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    call_func();

    p->func_id = XL_SetGlobalConnectionLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    call_func();

    p->func_id = XL_QueryGlobalStat;
    p->len = 0x04;
    call_func();
}

int create_process()
{
    g_tail += sprintf_s(g_tail, 10, "%d", GetCurrentProcessId());

    char *arg = GetCommandLineA();      // 参数前后带有""
    char *tail = strrchr(arg, '\\');

    strncpy_s(g_pth, sizeof(g_pth), arg, tail - arg + 1);
    strcat_s(g_pth, sizeof(g_pth), "DownloadSDKServer.exe\" ");
    strcat_s(g_pth, sizeof(g_pth), g_arg);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    BOOL ret = CreateProcessA(NULL, g_pth, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (!ret)
    {
        sprintf_s(g_arg, sizeof(g_arg), "open DownloadSDKServer.exe error:%d", GetLastError());
        MessageBoxA(NULL, g_arg, "error", MB_OK);
        return -1;
    }

    g_tail += sprintf_s(g_tail, 10, ":%d", pi.dwProcessId);
    return 0;
}

void start()
{
    if (0 != create_process())   // 创建子进程
    {
        return;
    }

    create_mutex();
    init_sys();
    init_sdk();
}

/**
 * \brief   查询任务
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void btn_download(HWND wnd)
{
    p_data_head p = (p_data_head)g_recv_tmp;
    GetDlgItemText(wnd, IDC_EDIT, (TCHAR*)p->arg1.data, 1024*100); // 得到UNICODE编码的URL

    p->func_id = XL_CreateMagnetTask;
    p->arg1.len = lstrlen((TCHAR*)p->arg1.data);

    p_arg_head arg2 = (p_arg_head)&(p->arg1.data[p->arg1.len * 2]);
    arg2->len = _stprintf_s((TCHAR*)arg2->data, 100, _T("d:\\5.downloads\\%d.torrent"),
                            GetTickCount());

    p->len = 8 + p->arg1.len * 2 + 4 + arg2->len * 2;
    call_func();

    DWORD taskid = *(DWORD*)(g_send + 12);

    TCHAR info[128];
    _stprintf_s(info, SIZEOF(info), _T("%d"), taskid);

    LVITEM item;
    item.mask = LVIF_TEXT;
    item.pszText = info;
    item.iItem = ListView_GetItemCount(g_list);
    item.iSubItem = 0;
    ListView_InsertItem(g_list, &item);

    item.pszText = (TCHAR*)arg2->data;
    item.iSubItem = 1;
    ListView_SetItem(g_list, &item);

    item.pszText = _T("0.00%");
    item.iSubItem = 2;
    ListView_SetItem(g_list, &item);

    p->func_id = XL_StartTask;
    p->data[0] = taskid;
    p->len = 0x08;
    call_func();
    return;
/*
    lstrcpy(info, _T("d:\\5.downloads\\28878171.torrent"));

    FILE *fp = NULL;
    _tfopen_s(&fp, info, _T("rb"));

    if (NULL == fp)
    {
        SP(_T("open file error %d"), GetLastError());
        MessageBox(NULL, info, g_title, MB_ICONEXCLAMATION);
    }

    fseek(fp, 0, SEEK_END);
    u32 size = ftell(fp);
    i8 *buff = malloc(size);
    fseek(fp, 0, SEEK_SET);
    fread(buff, 1, size, fp);
    fclose(fp);

    SP(_T("bencode start"));
    MessageBox(NULL, info, g_title, MB_ICONEXCLAMATION);

    i32 ret = bencode(buff, size);

    SP(_T("bencode:%d"), ret);
    MessageBox(NULL, info, g_title, MB_ICONEXCLAMATION);
*/
}

/**
 * \brief   定时任务
 * \param   无
 * \return  无
 */
void on_timer()
{
    TCHAR info[128];

    for (int i = 0; i < ListView_GetItemCount(g_list); i++)
    {
        ListView_GetItemText(g_list, i, 2, info, SIZEOF(info));

        if (0 == lstrcmp(info, _T("100%")))
        {
            continue;
        }

        ListView_GetItemText(g_list, i, 0, info, SIZEOF(info));

        p_data_head p = (p_data_head)g_recv_tmp;
        p->func_id = XL_QueryTaskInfo;
        p->data[0] = _ttoi(info);
        p->len = 0x08;
        call_func();

        u64 size = *(u64*)(g_send_tmp + 0x18);
        u64 down = *(u64*)(g_send_tmp + 0x20);
        u32 time = *(u32*)(g_send_tmp + 0x28);

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 2;
        item.pszText = info;

        if (0 == size) // 下载完成
        {
            //ListView_DeleteItem(g_list, i);

            p->func_id = XL_DeleteTask;
            p->data[0] = _ttoi(info);
            p->len = 0x08;
            call_func();

            lstrcpy(info, _T("100%"));
        }
        else
        {
            SP(_T("%.2f"), down / (double)size);
        }

        ListView_SetItem(g_list, &item);
    }
}

/**
 * \brief   拖拽文件
 * \param   [in]  WPARAM w 拖拽句柄
 * \return  无
 */
void on_dropfiles(WPARAM w)
{
    HDROP drop = (HDROP)w;
    TCHAR name[512];
    int count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0); // 拖拽文件个数

    for (int i = 0; i < count; i++)
    {
        DragQueryFile(drop, i, name, MAX_PATH);
        MessageBox(NULL, name, g_title, MB_OK);
    }

    DragFinish(drop);
}

/**
 * \brief   系统托盘消息处理函数
 * \param   [in]  HWND   wnd 窗体句柄
 * \param   [in]  LPARAM l   操作
 * \return  无
 */
void on_sys_notify(HWND wnd, LPARAM l)
{
    if (LOWORD(l) == WM_LBUTTONDOWN || LOWORD(l) == WM_RBUTTONDOWN)
    {
        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(g_menu, 0, pt.x, pt.y, 0, wnd, 0);
    }
}

/**
 * \brief   命令消息处理函数,菜单,按钮都会发此消息
 * \param   [in]  HWND   wnd 窗体句柄
 * \param   [in]  WPARAM w   消息参数
 * \return  无
 */
void on_command(HWND wnd, WPARAM w)
{
    int obj = LOWORD(w);
    int cmd = HIWORD(w);

    switch (obj)
    {
        case IDM_EXIT:          PostMessage(wnd, WM_CLOSE, 0, 0);                       break;
        case IDM_SHOW:          ShowWindow(wnd, IsWindowVisible(wnd)?SW_HIDE:SW_SHOW);  break;
        case IDC_BTN_DOWNLOAD:  btn_download(wnd);                                      break;
    }
}

/**
 * \brief   改变大小消息处理函数
 * \param   [in]  LPARAM l 窗体s宽高
 * \return  无
 */
void on_size(LPARAM l)
{
    int w = LOWORD(l);
    int h = HIWORD(l);

    MoveWindow(g_edit, 0,        1, w - 100,     25, TRUE);
    MoveWindow(g_butt, w - 100,  1, 100 - 1,     25, TRUE);
    MoveWindow(g_list, 0,       27,       w, h - 27, TRUE);

    LVCOLUMN col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = _T("名称");
    col.cx = w - 180;
    col.iSubItem = 1;
    ListView_SetColumn(g_list, col.iSubItem, &col);
}

/**
 * \brief   创建消息处理函数
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_create(HWND wnd)
{
    g_edit = CreateWindow(WC_EDIT,                                  // 控件类型
                _T(""),                                             // 名称
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, // 属性
                0, 0,                                               // 在父窗口位置
                600, 25,                                            // 大小
                wnd,                                                // 父窗口句柄
                (HMENU)IDC_EDIT,                                    // 控件ID
                NULL,                                               // 实例
                NULL);                                              // 参数

    SendMessage(g_edit, WM_SETFONT, (WPARAM)g_font, (LPARAM)TRUE);

    g_butt = CreateWindow(WC_BUTTON,
                _T("download"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                600, 00,
                100, 25,
                wnd,
                (HMENU)IDC_BTN_DOWNLOAD,
                NULL,
                NULL);

    SendMessage(g_butt, WM_SETFONT, (WPARAM)g_font, (LPARAM)TRUE);

    g_list = CreateWindow(WC_LISTVIEW,
                _T("listview"),
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
                0, 20,
                600, 500,
                wnd,
                (HMENU)IDC_LIST,
                NULL,
                NULL);

    ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES);
    
    SendMessage(g_list, WM_SETFONT, (WPARAM)g_font, (LPARAM)TRUE);

    LVCOLUMN col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = _T("任务ID");
    col.cx = 90;
    col.iSubItem = 0;
    ListView_InsertColumn(g_list, col.iSubItem, &col);

    col.pszText = _T("名称");
    col.cx = 90;
    col.iSubItem = 1;
    ListView_InsertColumn(g_list, col.iSubItem, &col);

    col.pszText = _T("进度");
    col.cx = 90;
    col.iSubItem = 2;
    ListView_InsertColumn(g_list, col.iSubItem, &col);

    DragAcceptFiles(wnd, TRUE); // 属性WS_EX_ACCEPTFILES
}

/**
 * \brief   窗体关闭处理函数
            当用户点击窗体上的关闭按钮时,
            系统发出WM_CLOSE消息,自己执行DestroyWindow关闭窗口,
            然后发送WM_DESTROY消息,自己执行PostQuitMessage关闭应用程序,
            最后发出WM_QUIT消息来关闭消息循环
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_close(HWND wnd)
{
    int ret = MessageBox(NULL, _T("确定退出?"), g_title, MB_ICONQUESTION | MB_YESNO);

    if (IDYES == ret)
    {
        DestroyWindow(wnd);
    }
}

/**
 * \brief   窗体消毁处理函数
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_destory(HWND wnd)
{
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    PostQuitMessage(0);
}

/**
 * \brief   窗体类消息处理回调函数
 * \param   [in]  HWND   wnd    窗体句柄
 * \param   [in]  UINT   msg    消息ID
 * \param   [in]  WPARAM w      消息参数
 * \param   [in]  LPARAM l      消息参数
 * \return  LRESULT 消息处理结果，它与发送的消息有关
 */
LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    if (WM_MY_NOTIFY == msg)
    {
        on_sys_notify(wnd, l);
    }

    switch(msg)
    {
        case WM_TIMER:       on_timer();            break;
        case WM_DROPFILES:   on_dropfiles(w);       break;
        case WM_COMMAND:     on_command(wnd, w);    break;
        case WM_SIZE:        on_size(l);            break;
        case WM_CREATE:      on_create(wnd);        break;
        case WM_CLOSE:       on_close(wnd);         return 0;
        case WM_DESTROY:     on_destory(wnd);       return 0;
    }

    return DefWindowProc(wnd, msg, w, l);
}

/**
 * \brief   窗体类程序主函数
 * \param   [in]  HINSTANCE hInstance       当前实例句柄
 * \param   [in]  HINSTANCE hPrevInstance   先前实例句柄
 * \param   [in]  LPSTR     lpCmdLine       命令行参数
 * \param   [in]  int       nCmdShow        显示状态(最小化,最大化,隐藏)
 * \return  int 程序返回值
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // 字体
    LOGFONT lf;
    lf.lfWeight         = 100;  // 粗细程度,0到1000,正常400，粗体700
    lf.lfHeight         = 13;   // 高度
    lf.lfWidth          = 7;    // 宽度
    lf.lfEscapement     = 0;    // 行角度900为90度
    lf.lfOrientation    = 0;    // 字符角度
    lf.lfItalic         = 0;    // 斜体
    lf.lfUnderline      = 0;    // 下划线
    lf.lfStrikeOut      = 0;    // 删除线
    lf.lfOutPrecision   = 0;    // 输出精度
    lf.lfClipPrecision  = 0;    // 剪辑精度
    lf.lfQuality        = 0;    // 输出质量
    lf.lfPitchAndFamily = 0;    // 字符间距和族
    lf.lfCharSet        = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, _T("宋体"));
    g_font = CreateFontIndirect(&lf);

    // 加载菜单,图标
    g_menu           = GetSubMenu(LoadMenu(hInstance, MAKEINTRESOURCE(IDC_MENU)), 0);
    HICON icon       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GREEN));

    // 窗体类
    WNDCLASS wc      = {0};
    wc.style         = CS_HREDRAW | CS_VREDRAW;             // 类型属性
    wc.lpfnWndProc   = window_proc;                         // 窗体消息处理函数
    wc.lpszClassName = _T("class_name");                    // 类名称
    wc.hInstance     = hInstance;                           // 实例
    wc.hIcon         = icon;                                // 图标
    wc.hCursor       = LoadCursor(NULL, IDC_CROSS);         // 鼠标指针
    wc.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));// 背景刷
    RegisterClass(&wc);

    // 窗体居中
    int cx = 800;
    int cy = 600;
    int x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;

    // 创建窗体
    HWND wnd = CreateWindow(wc.lpszClassName,       // 类名称
                            g_title,                // 窗体名称
                            WS_OVERLAPPEDWINDOW,    // 窗体属性
                            x,  y,                  // 窗体位置
                            cx, cy,                 // 窗体大小
                            NULL,                   // 父窗句柄
                            NULL,                   // 菜单句柄
                            hInstance,              // 实例句柄
                            NULL);                  // 参数,给WM_CREATE的lParam的CREATESTRUCT

    // 显示窗体
    ShowWindow(wnd, SW_SHOWNORMAL);

    // 重绘窗体
    UpdateWindow(wnd);

    // 系统托盘
    WM_MY_NOTIFY           = RegisterWindowMessage(_T("WM_MY_NOTIFYICON"));
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = wnd;                               // 指定接收托盘消息的句柄
    g_nid.hIcon            = icon;                              // 指定托盘图标
    g_nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;  // 消息,图标,文字有效
    g_nid.uCallbackMessage = WM_MY_NOTIFY;                      // 消息ID
    _tcscpy_s(g_nid.szTip, SIZEOF(g_nid.szTip), g_title);
    Shell_NotifyIcon(NIM_ADD, &g_nid);

    // 启动DownloadSDKServer.exe
    start();

    // 定时器
    SetTimer(wnd, 1, 1000, NULL);

    // 消息体
    MSG msg;

    // 消息循环,从消息队列中取得消息,只到WM_QUIT时退出
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg); // 将WM_KEYDOWN和WM_KEYUP转换为一条WM_CHAR消息
        DispatchMessage(&msg);  // 分派消息到窗口,内部调用窗体消息处理回调函数
    }

    return (int)msg.lParam;
}