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

HMENU  g_menu                   = NULL; // 系统托盘使用
HWND   g_edit                   = NULL; // WM_SIZE中使用
HWND   g_butt                   = NULL;
HWND   g_list                   = NULL;
HWND   g_torr                   = NULL;

char   g_pth[512]               = "";
char   g_arg[128]               = "BDAF7A63-568C-43ab-9406-D145CF03B08C:";
char  *g_tail                   = g_arg + 37;

UCHAR *g_recv                   = NULL; // 共享内存1M,对方接收的数据,我方发送的
UCHAR *g_send                   = NULL; // 共享内存1M,我方发送的数据,需要对方处理

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

BOOL   g_init                   = FALSE;

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

enum
{
    ED2K = 1,
    FILEHASH,
    NAME,
    PATH,
    LENGTH,
    PIECES,
    PATH_LIST,
};

typedef struct _file_info
{
    TCHAR            *name;

    unsigned __int64  len;

}file_info, *p_file_info;

typedef struct _info_head
{
    TCHAR *name;     // 指向名称列表

    int   last;     // 上一个字符串是什么

    int   count;

    file_info file[512];

}info_head, *p_info_head;

int bencode_dict(const char *s, unsigned int len, p_info_head info);

/**
 * \brief   打开文件取得文件数据
 * \param   [in]  const char    *filename   文件名
 * \param   [out] char          **data      数据
 * \param   [out] unsigned int  *len        数据长
 * \return  0-成功，其它失败
 */
int get_file_data(const char *filename, char **data, unsigned int *len)
{
    FILE *fp = NULL;
    fopen_s(&fp, filename, "rb");

    if (NULL == fp)
    {
        printf("open %s error %d", filename, GetLastError());
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
 * \brief   将utf8转成unicode
 * \param   [in]        const char      *src        源串
 * \param   [in]        unsigned int     src_len    源串长
 * \param   [out]       short           *dst        目标串
 * \param   [in/out]    unsigned int    *dst_len    目标串最大长,目标串长
 * \return  0-成功，其它失败
 */
int utf8_unicode(const char *src, unsigned int src_len, short *dst, unsigned int *dst_len)
{
    if (NULL == src || NULL == dst)
    {
        return -1;
    }

    // 转成unicode后的长度
    unsigned int len = MultiByteToWideChar(CP_UTF8, 0, src, src_len, NULL, 0);

    if (len >= *dst_len)
    {
        printf("dst_len too small %d>%d\n", len, *dst_len);
        return -2;
    }

    // 转成unicode
	MultiByteToWideChar(CP_UTF8, 0, src, src_len, dst, *dst_len);

    dst[len] = L'\0';

    *dst_len = len;
	return 0;
}

/**
 * \brief   解析bencode编码的字符串，格式：<字符串长度>字符串
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_info_head    info    信息
 * \return  0-成功，其它失败
 */
int bencode_str(const char *s, unsigned int len, p_info_head info)
{
    unsigned int str_len = 0;

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
            if ((info->last == ED2K) ||
                (info->last == FILEHASH) ||
                (info->last == PIECES))
            {
                info->last = 0;
                return i + str_len;
            }

            TCHAR        tmp[1024];
            unsigned int tmp_size = 1024;

            utf8_unicode(&s[i], str_len, tmp, &tmp_size);

            if (info->last == PATH_LIST)
            {
                info->name += _stprintf_s(info->name, 1024, L"%s\\", tmp);
            }
            else if (0 == lstrcmp(tmp, L"ed2k"))
            {
                info->last = ED2K;
            }
            else if (0 == lstrcmp(tmp, L"filehash"))
            {
                info->last = FILEHASH;
            }
            else if (0 == lstrcmp(tmp, L"name"))
            {
                info->last = NAME;
            }
            else  if(0 == lstrcmp(tmp, L"path"))
            {
                info->last = PATH;
            }
            else if (0 == lstrcmp(tmp, L"length"))
            {
                info->last = LENGTH;
            }
            else if (0 == lstrcmp(tmp, L"pieces"))
            {
                info->last = PIECES;
            }

            return i + str_len;
        }
        else
        {
            printf("string char error\n");
            return -100;
        }
    }

    printf("string error\n");
    return -101;
}

/**
 * \brief   解析bencode编码的整数，格式：i<整数>e
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_info_head    info    信息
 * \return  0-成功，其它失败
 */
int bencode_int(const char *s, unsigned int len, p_info_head info)
{
    if (s[0] != 'i')
    {
        printf("int flage error\n");
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
            if (info->last == LENGTH) // 上一个字符中是"length"
            {
                info->file[info->count].len  = num;
                info->file[info->count].name = info->name;
            }

            return i + 1;
        }
        else
        {
            printf("int num error\n");
            return -201;
        }
    }

    printf("int error\n");
    return -202;
}

/**
 * \brief   解析bencode编码的列表，格式：l<bencoding编码类型>e
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_info_head    info    信息
 * \return  0-成功，其它失败
 */
int bencode_list(const char *s, unsigned int len, p_info_head info)
{
    if (s[0] != 'l')
    {
        printf("list flage error\n");
        return -300;
    }

    if (info->last == PATH)
    {
        info->last = PATH_LIST;
    }

    int ret;

    for (unsigned int i = 1; i < len; )
    {
        if (s[i] == 'e')
        {
            if (info->last == PATH_LIST)
            {
                info->last = 0;
                info->count++;
                *(info->name - 1) = '\0'; // 结尾多了个'\\'
            }

            return i + 1;
        }
        else if (s[i] == 'd')
        {
            ret = bencode_dict(&s[i], len, info);
        }
        else if (s[i] == 'l')
        {
            ret = bencode_list(&s[i], len, info);
        }
        else if (s[i] == 'i')
        {
            ret = bencode_int(&s[i], len, info);
        }
        else
        {
            ret = bencode_str(&s[i], len, info);
        }

        if (ret <= 0)
        {
            printf("list sub item len error\n");
            return -301;
        }

        i += ret;
    }

    printf("list error\n");
    return -302;
}

/**
 * \brief   解析bencode编码的字典，格式：d<bencoding字符串><bencoding编码类型>e
 * \param   [in]     const char    *s       数据
 * \param   [in]     unsigned int   len     数据长
 * \param   [in/out] p_info_head    info    信息
 * \return  0-成功，其它失败
 */
int bencode_dict(const char *s, unsigned int len, p_info_head info)
{
    if (s[0] != 'd')
    {
        printf("dict flage error\n");
        return -400;
    }

    int ret = bencode_str(&s[1], len, info);

    if (ret <= 0)
    {
        printf("dict key len error\n");
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
            ret = bencode_dict(&s[i], len, info);
        }
        else if (s[i] == 'l')
        {
            ret = bencode_list(&s[i], len, info);
        }
        else if (s[i] == 'i')
        {
            ret = bencode_int(&s[i], len, info);
        }
        else
        {
            ret = bencode_str(&s[i], len, info);
        }

        if (ret <= 0)
        {
            printf("dict item len error\n");
            return -402;
        }

        i += ret;
    }

    printf("dict error\n");
    return -403;
}

int call_func()
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
    int ret = call_func();

    if (0 != ret)
    {
        return -1;
    }

    //char peerid[128];
    //strncpy_s(peerid, sizeof(peerid), g_send + 16, *(DWORD*)(g_send_tmp + 12));;
    //MessageBoxA(NULL, peerid, "peerid", MB_OK);

    p->func_id = XL_SetUserInfo;
    p->data[0] = 0x00;
    p->data[1] = 0x00;
    p->len = 0x0c;
    call_func();

    if (0 != ret)
    {
        return -2;
    }

    p->func_id = XL_SetGlobalExtInfo;
    p->arg1.len = sprintf_s(p->arg1.data, 100, "isvip=0,viptype=,viplevel=0,userchannel=100001");
    p->len = 8 + p->arg1.len;
    call_func();

    if (0 != ret)
    {
        return -3;
    }

    p->func_id = XL_SetDownloadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    call_func();

    if (0 != ret)
    {
        return -4;
    }

    p->func_id = XL_SetUploadSpeedLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    call_func();

    if (0 != ret)
    {
        return -5;
    }

    p->func_id = XL_SetGlobalConnectionLimit;
    p->data[0] = 0xffffffff;
    p->len = 0x08;
    call_func();

    if (0 != ret)
    {
        return -6;
    }

    p->func_id = XL_QueryGlobalStat;
    p->len = 0x04;
    call_func();

    if (0 != ret)
    {
        return -7;
    }

    return 0;
}

int download_torrent_file(HWND wnd, int *taskid, wchar_t *filename)
{
    if (!g_init)
    {
        return -1;
    }

    // 参数1,磁力URL,参数2,本地文件名,都是UNICODE编码
    p_data_head p = (p_data_head)g_recv_tmp;

    wchar_t *url = (wchar_t*)p->arg1.data;
    GetDlgItemTextW(wnd, IDC_EDIT, url, 1024*100);
    p->arg1.len = wcslen(url);

    MessageBox(wnd, url, _T("url"), MB_OK);

    p_arg_head arg2 = (p_arg_head)(p->arg1.data + p->arg1.len * 2);
    wchar_t *file = (wchar_t*)arg2->data;
    arg2->len = _stprintf_s(file, 100, _T("d:\\5.downloads\\%u.torrent"), GetTickCount());;

    MessageBox(wnd, file, _T("file"), MB_OK);

    p->func_id = XL_CreateMagnetTask;
    p->len = 8 + p->arg1.len * 2 + 4 + arg2->len * 2;

    int ret = call_func();

    if (0 != ret)
    {
        return -2;
    }

    *taskid = *(int*)(g_send_tmp + 12);
    _tcscpy_s(filename, 100, file);

    return 0;
}

int download_bt_file(HWND wnd, int *taskid, wchar_t *filename)
{
    if (!g_init)
    {
        return -1;
    }

    *taskid = *(int*)(g_send_tmp + 12);

    return 0;
}

int download_file_start(int taskid)
{
    p_data_head p = (p_data_head)g_recv_tmp;
    p->func_id = XL_SetTaskStrategy;
    p->data[0] = taskid;
    p->data[1] = 7;
    p->len = 0x0c;
    int ret = call_func();

    if (0 != ret)
    {
        return -1;
    }

    p->func_id = XL_SetTaskExtInfo;
    p->data[0] = taskid;
    p->data[1] = 0x2b;
    strcpy_s((char*)&(p->data[2]), 100, "parentid=109183876,taskorigin=newwindow_url");
    p->len = 0x0c + 0x2b;
    ret = call_func();

    if (0 != ret)
    {
        return -2;
    }

    p->func_id = XL_StartTask;
    p->data[0] = taskid;
    p->len = 0x08;
    ret = call_func();

    if (0 != ret)
    {
        return -3;
    }

    return 0;
}

void insert_list(int taskid, TCHAR *filename)
{
    TCHAR info[128];
    SP(_T("%d"), taskid);

    LVITEM item;
    item.mask = LVIF_TEXT;
    item.pszText = info;
    item.iItem = ListView_GetItemCount(g_list);
    item.iSubItem = 0;
    ListView_InsertItem(g_list, &item);

    item.pszText = filename;
    item.iSubItem = 1;
    ListView_SetItem(g_list, &item);

    item.pszText = _T("0.00%");
    item.iSubItem = 2;
    ListView_SetItem(g_list, &item);
}

void btn_download(HWND wnd)
{
    int     ret;
    int     taskid;
    TCHAR   info[128];
    wchar_t filename[MAX_PATH];

    if (IsWindowVisible(g_list))
    {
        ret = download_torrent_file(wnd, &taskid, filename);
    }
    else
    {
        ret = download_bt_file(wnd, &taskid, filename);
    }

    if (0 != ret)
    {
        SP(_T("download file error:%d"), ret);
        MessageBox(wnd, info, _T(""), MB_OK);
        return;
    }

    ret = download_file_start(taskid);

    if (0 != ret)
    {
        SP(_T("download start error:%d"), ret);
        MessageBox(wnd, info, _T(""), MB_OK);
    }

    insert_list(taskid, filename);
    return;
}

/**
 * \brief   定时任务
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_timer(HWND wnd)
{
    int ret;
    TCHAR info[128];

    for (int i = 0; i < ListView_GetItemCount(g_list); i++)
    {
        ListView_GetItemText(g_list, i, 2, info, SIZEOF(info));

        if (0 == _tcscmp(info, _T("100%")))
        {
            continue;
        }

        ListView_GetItemText(g_list, i, 0, info, SIZEOF(info));

        p_data_head p = (p_data_head)g_recv_tmp;
        p->func_id = XL_QueryTaskInfo;
        p->data[0] = _ttoi(info);
        p->len = 0x08;
        ret = call_func();

        if (0 != ret)
        {
            continue;
        }
/*
        DWORD count = *(DWORD*)g_send_tmp;
        TCHAR *ptr = info;

        for (DWORD i = 0; i < count; i++)
        {
            ptr += _stprintf_s(ptr, 10, _T("%02x "), g_send_tmp[i]);

            if (i % 64 == 63 && i != 0)
            {
                ptr += _stprintf_s(ptr, 10, _T("\n"));
            }
        }

        MessageBox(wnd, info, _T("XL_QueryTaskInfo"), MB_OK);
*/
        unsigned __int64 size = *(unsigned __int64*)(g_send_tmp + 0x18);
        unsigned __int64 down = *(unsigned __int64*)(g_send_tmp + 0x20);
        unsigned int     time = *(unsigned int*)(g_send_tmp + 0x28);

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 2;
        item.pszText = info;

        if (0 == size) // 下载完成
        {
            //p->func_id = XL_DeleteTask;
            //p->data[0] = _ttoi(info);
            //p->len = 0x08;
            //call_func();

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

    char filename[MAX_PATH];
    DragQueryFileA(drop, 0, filename, MAX_PATH);    // 0-只取第1个,0xFFFFFFFF-返回拖拽文件个数
    DragFinish(drop);

    char         *buff;
    unsigned int  size;

    if (0 != get_file_data(filename, &buff, &size))
    {
        return;
    }

    info_head data;
    data.name  = (TCHAR*)buff;
    data.last  = 0;
    data.count = 0;

    bencode_dict(buff, size, &data);

    TCHAR info[MAX_PATH];

    LVITEM item;
    item.mask = LVIF_TEXT;

    for (int i = 0; i < data.count; i++)
    {
        item.pszText = data.file[i].name;
        item.iItem = i;
        item.iSubItem = 0;
        ListView_InsertItem(g_torr, &item);

        double g = data.file[i].len / (1024.0 * 1024 * 1024);
        double m = data.file[i].len / (1024.0 * 1024);
        double k = data.file[i].len / (1024.0);

        if (g > 1.0)
        {
            SP(_T("%.2fGB"), g);
        }
        else if (m > 1.0)
        {
            SP(_T("%.2fMB"), m);
        }
        else if (k > 1.0)
        {
            SP(_T("%.2fKB"), k);
        }
        else
        {
            SP(_T("%lldB"), data.file[i].len);
        }

        item.pszText = info;
        item.iSubItem = 1;
        ListView_SetItem(g_torr, &item);
    }

    free(buff);

    ShowWindow(g_list, SW_HIDE);
    ShowWindow(g_torr, SW_SHOW);
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
    MoveWindow(g_torr, 0,       27,       w, h - 27, TRUE);

    LVCOLUMN col = {0};
    col.mask = LVCF_WIDTH;
    col.cx = 90;
    ListView_SetColumn(g_list, 0, &col);

    col.cx = w - 180;
    ListView_SetColumn(g_list, 1, &col);

    col.cx = 90;
    ListView_SetColumn(g_list, 2, &col);

    col.cx = w - 90;
    ListView_SetColumn(g_torr, 0, &col);

    col.cx = 70;
    ListView_SetColumn(g_torr, 1, &col);
}

/**
 * \brief   创建消息处理函数
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_create(HWND wnd, LPARAM l)
{
    CREATESTRUCT *arg = (CREATESTRUCT*)l;

    // 字体
    LOGFONT lf;
    lf.lfWeight         = 100;              // 粗细程度,0到1000,正常400，粗体700
    lf.lfHeight         = 13;               // 高度
    lf.lfWidth          = 7;                // 宽度
    lf.lfEscapement     = 0;                // 行角度900为90度
    lf.lfOrientation    = 0;                // 字符角度
    lf.lfItalic         = 0;                // 斜体
    lf.lfUnderline      = 0;                // 下划线
    lf.lfStrikeOut      = 0;                // 删除线
    lf.lfOutPrecision   = 0;                // 输出精度
    lf.lfClipPrecision  = 0;                // 剪辑精度
    lf.lfQuality        = 0;                // 输出质量
    lf.lfPitchAndFamily = 0;                // 字符间距和族s
    lf.lfCharSet        = DEFAULT_CHARSET;
    _tcscpy_s(lf.lfFaceName, SIZEOF(lf.lfFaceName), _T("宋体"));

    HFONT font = CreateFontIndirect(&lf);

    DragAcceptFiles(wnd, TRUE);             // 窗体可拖拽文件

    g_edit = CreateWindow(WC_EDIT,          // 控件类型
                          _T(""),           // 名称
                          WS_CHILD |
                          WS_VISIBLE |
                          WS_BORDER |
                          ES_AUTOHSCROLL,   // 属性
                          0, 0,             // 在父窗口位置
                          600, 25,          // 大小
                          wnd,              // 父窗口句柄
                          (HMENU)IDC_EDIT,  // 控件ID
                          NULL,             // 实例
                          NULL);            // 参数

    SendMessage(g_edit, WM_SETFONT, (WPARAM)font, (LPARAM)TRUE);

    g_butt = CreateWindow(WC_BUTTON,
                          _T("download"),
                          WS_CHILD |
                          WS_VISIBLE |
                          BS_PUSHBUTTON,
                          600, 00,
                          100, 25,
                          wnd,
                          (HMENU)IDC_BTN_DOWNLOAD,
                          NULL,
                          NULL);

    SendMessage(g_butt, WM_SETFONT, (WPARAM)font, (LPARAM)TRUE);

    g_list = CreateWindow(WC_LISTVIEW,
                          _T("listview"),
                          WS_CHILD |
                          WS_VISIBLE |
                          LVS_REPORT |
                          LVS_SHOWSELALWAYS,
                          0, 20,
                          600, 500,
                          wnd,
                          (HMENU)IDC_LIST,
                          NULL,
                          NULL);

    int style = LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES;
    ListView_SetExtendedListViewStyle(g_list, style);

    SendMessage(g_list, WM_SETFONT, (WPARAM)font, (LPARAM)TRUE);

    LVCOLUMN col = {0};
    col.mask = LVCF_TEXT;
    col.pszText = _T("任务");
    ListView_InsertColumn(g_list, 0, &col);

    col.pszText = _T("名称");
    ListView_InsertColumn(g_list, 1, &col);

    col.pszText = _T("进度");
    ListView_InsertColumn(g_list, 2, &col);

    g_torr = CreateWindow(WC_LISTVIEW,
                          _T("listview"),
                          WS_CHILD |
                          LVS_REPORT |
                          LVS_SHOWSELALWAYS,
                          0, 20,
                          600, 500,
                          wnd,
                          (HMENU)IDC_TORR,
                          NULL,
                          NULL);

    ListView_SetExtendedListViewStyle(g_torr, style);

    SendMessage(g_torr, WM_SETFONT, (WPARAM)font, (LPARAM)TRUE);

    col.pszText = _T("文件");
    ListView_InsertColumn(g_torr, 0, &col);

    col.pszText = _T("大小");
    ListView_InsertColumn(g_torr, 1, &col);
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
    int ret = MessageBox(wnd, _T("确定退出?"), _T(""), MB_ICONQUESTION | MB_YESNO);

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
        case WM_TIMER:       on_timer(wnd);         break;
        case WM_DROPFILES:   on_dropfiles(w);       break;
        case WM_COMMAND:     on_command(wnd, w);    break;
        case WM_SIZE:        on_size(l);            break;
        case WM_CREATE:      on_create(wnd, l);     break;
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
    // 窗体居中
    int cx = 800;
    int cy = 600;
    int x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;   // GetSystemMetrics得到屏幕大小
    int y = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;

    // 加载鼠标,笔刷,图标,菜单
    HCURSOR cursor   = LoadCursor(NULL, IDC_CROSS);
    HBRUSH  brush    = CreateSolidBrush(RGB(240, 240, 240));
    HICON   icon     = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GREEN));
    g_menu           = GetSubMenu(LoadMenu(hInstance, MAKEINTRESOURCE(IDC_MENU)), 0);

    // 窗体类
    WNDCLASS wc      = {0};
    wc.style         = CS_HREDRAW | CS_VREDRAW;         // 类型属性
    wc.lpfnWndProc   = window_proc;                     // 窗体消息处理函数
    wc.lpszClassName = _T("class_name");                // 类名称
    wc.hInstance     = hInstance;                       // 实例
    wc.hIcon         = icon;                            // 图标
    wc.hCursor       = cursor;                          // 鼠标指针
    wc.hbrBackground = brush;                           // 背景刷
    RegisterClass(&wc);

    // 创建窗体
    HWND wnd = CreateWindow(wc.lpszClassName,           // 类名称
                            _T("SDKStart"),             // 窗体名称
                            WS_OVERLAPPEDWINDOW,        // 窗体属性
                            x,  y,                      // 窗体位置
                            cx, cy,                     // 窗体大小
                            NULL,                       // 父窗句柄
                            NULL,                       // 菜单句柄
                            hInstance,                  // 实例句柄
                            NULL);                      // 参数,给WM_CREATE的lParam

    // 显示窗体
    ShowWindow(wnd, SW_SHOWNORMAL);

    // 重绘窗体
    UpdateWindow(wnd);

    // 系统托盘
    WM_MY_NOTIFY           = RegisterWindowMessage(_T("WM_MY_NOTIFYICON"));
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = wnd;                       // 指定接收托盘消息的句柄
    g_nid.hIcon            = icon;                      // 指定托盘图标
    g_nid.uFlags           = NIF_MESSAGE | NIF_ICON;    // 消息,图标
    g_nid.uCallbackMessage = WM_MY_NOTIFY;              // 消息ID

    Shell_NotifyIcon(NIM_ADD, &g_nid);

    for (int i = 0; i < 1; i++)
    {
        int ret = create_process(); // 创建子进程,启动DownloadSDKServer.exe

        if (0 != ret)
        {
            MessageBox(wnd, _T("run DownloadSDKServer.exe error"), _T("SDKStart"), MB_OK);
            break;
        }

        ret = create_mutex();

        if (0 != ret)
        {
            MessageBox(wnd, _T("create mutex error"), _T("SDKStart"), MB_OK);
            break;
        }

        ret = init_sys();

        if (0 != ret)
        {
            MessageBox(wnd, _T("init sys error"), _T("SDKStart"), MB_OK);
            break;
        }

        ret = init_sdk();

        if (0 != ret)
        {
            MessageBox(wnd, _T("init sdk error"), _T("SDKStart"), MB_OK);
            break;
        }

        g_init = TRUE;
    }

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