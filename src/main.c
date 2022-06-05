/*************************************************
 * Copyright:   XT Tech. Co., Ltd.
 * File name:   main.c
 * Author:      xt
 * Version:     1.0.0
 * Date:        2022-02-08
 * Code:        UTF-8(No BOM)
 * Description: 主模块实现
*************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <CommCtrl.h>
#include "xt_log.h"
#include "xt_http.h"
#include "xt_pinyin.h"
#include "config.h"
#include "resource.h"
#include "torrent.h"
#include "xl_sdk.h"


#define SIZEOF(x)   sizeof(x)/sizeof(x[0])
#define SP(...)     _stprintf_s(info, SIZEOF(info), __VA_ARGS__)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

enum    // 任务列表控件列ID
{
    LIST_TASK,
    LIST_FILE,
    LIST_SIZE,
    LIST_SPEE,
    LIST_PROG,
    LIST_TIME
};

enum    // 种子列表控件列ID
{
    TORR_SIZE,
    TORR_FILE
};

typedef struct _task_info {

    unsigned __int64    down;               // 已经下载的数量

    unsigned int        time;               // 用时秒数

    unsigned int        show_size;          // 是否已经显示大小

}task_info, *p_task_info;

HWND                    g_wnd;
HWND                    g_edit;
HWND                    g_down;
HWND                    g_list;
HWND                    g_torr;
HMENU                   g_menu;

torrent                 g_torrent       = {0};                              // 种子文件信息

task_info               g_task[128]     = {0};                              // 当前正在下载的任务信息

NOTIFYICONDATA          g_nid           = {0};                              // 任务栏图标数据结构

static const TCHAR     *g_title         = _T("DownloadSDKServerStart.exe"); // 标题

extern unsigned char   *g_pinyin;                                           // 拼音数据

int http_process_callback(const char *uri, const char *arg, int arg_count, char *content, int *content_len)
{
    strcpy_s(content, *content_len, "200");
    *content_len = 3;
    return 200;
}

/**
 * \brief   得到格式化后的信息
 * \param   [in]    unsigned __int64     data   数据
 * \param   [in]    TCHAR               *unit   单位
 * \param   [out]   TCHAR               *info   信息
 * \return  无
 */
void format_data(unsigned __int64 data, TCHAR *info)
{
    double g = data / 1024.0 / 1024.0 / 1024.0;
    double m = data / 1024.0 / 1024.0;
    double k = data / 1024.0;

    if (g > 1.0)
    {
        _stprintf_s(info, 16, _T("%.2fG"), g);
    }
    else if (m > 1.0)
    {
        _stprintf_s(info, 16, _T("%.2fM"), m);
    }
    else if (k > 1.0)
    {
        _stprintf_s(info, 16, _T("%.2fK"), k);
    }
    else
    {
        _stprintf_s(info, 16, _T("%I64u"), data);
    }
}

/**
 * \brief   得到格式化后的信息
 * \param   [out]   char   *list        列表
 * \param   [in]    int     list_size   列表空间
 * \param   [out]   short  *filename    最后一个选中的文件
 * \return  0-成功
 */
int get_select_list(char *list, int list_size, short *filename)
{
    if (NULL == list || list_size <= 0)
    {
        return -1;
    }

    int count = ListView_GetItemCount(g_torr);

    if (count >= list_size)
    {
        return -2;
    }

    for (int i = 0; i < count; i++)
    {
        if (ListView_GetCheckState(g_torr, i))
        {
            list[i] = '1';
            ListView_GetItemText(g_torr, i, TORR_FILE, filename, MAX_PATH);
        }
        else
        {
            list[i] = '0';
        }
    }

    char *ptr = strrchr(list, '1');

    if (NULL == ptr)
    {
        return -3;
    }

    *(++ptr) = '\0';

    return 0;
}

/**
 * \brief   下载按钮
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void btn_download(HWND wnd)
{
    int     ret;
    TCHAR   info[128];

    int     len;
    int     taskid;
    char    list[MAX_PATH];
    short   file[MAX_PATH];
    short   taskname[MAX_PATH];
    int     tasktype;
    short  *path   = L"D:\\5.downloads\\bt";

    GetDlgItemTextW(wnd, IDC_EDIT, file, sizeof(file));

    len = wcslen(file);

    if (0 == wcsncmp(file, L"magnet:?", 8))   // 磁力连接URL
    {
        tasktype = TASK_MAGNET;

        ret = xl_sdk_create_magnet_task(file, path, &taskid, taskname);
    }
    else if (0 == wcscmp(file + len - 8, L".torrent"))
    {
		wcsncpy_s(taskname, MAX_PATH - 1, file + wcslen(path) + 1, 41);

        tasktype = TASK_BT;

        ret = get_select_list(list, sizeof(list) - 1, taskname + 41);

        if (0 != ret)
        {
            SP(_T("get select list error:%d"), ret);
            MessageBox(wnd, info, _T("error"), MB_OK);
            return;
        }

        ret = xl_sdk_create_bt_task(file, path, list, g_torrent.announce_count,
                                    g_torrent.announce, g_torrent.announce_len,
                                    &taskid);
    }
    else
    {
        tasktype = TASK_URL;

        ret = xl_sdk_create_url_task(file, path, &taskid, taskname);
    }

    if (0 != ret)
    {
        SP(_T("create task error:%d"), ret);
        MessageBox(wnd, info, _T("error"), MB_OK);
        return;
    }

    ret = xl_sdk_start_download_file(taskid, tasktype);

    if (0 != ret)
    {
        SP(_T("download start error:%d"), ret);
        MessageBox(wnd, info, _T(""), MB_OK);
    }

    SP(_T("%d"), taskid);

    LVITEM item;
    item.mask = LVIF_TEXT;
    item.pszText = info;
    item.iItem = ListView_GetItemCount(g_list);
    item.iSubItem = LIST_TASK;
    ListView_InsertItem(g_list, &item);

    item.pszText = taskname;
    item.iSubItem = LIST_FILE;
    ListView_SetItem(g_list, &item);

    ShowWindow(g_list, SW_SHOW);
    ShowWindow(g_torr, SW_HIDE);
    return;
}

/**
 * \brief   定时任务
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_timer(HWND wnd)
{
    if (!IsWindowVisible(g_list))
    {
        return;
    }

    int              ret;
    TCHAR            info[128];

    int              taskid;
    unsigned int     show_size;
    unsigned int     time;
    unsigned int     last_time;
    unsigned __int64 down;
    unsigned __int64 last_down;
    unsigned __int64 size;

    for (int i = 0; i < ListView_GetItemCount(g_list); i++)
    {
        ListView_GetItemText(g_list, i, LIST_PROG, info, SIZEOF(info));

        if (0 == _tcscmp(info, _T("100")))  // 已经下载完成
        {
            continue;
        }

        ListView_GetItemText(g_list, i, LIST_TASK, info, SIZEOF(info));

        taskid    = _ttoi(info);
        last_down = g_task[taskid].down;
        last_time = g_task[taskid].time;
        show_size = g_task[taskid].show_size;

        ret = xl_sdk_get_task_info(taskid, &size, &down, &time);

        if (0 != ret)
        {
            SP(_T("XL_QueryTaskInfo:%d"), ret);
            MessageBox(wnd, info, _T("error"), MB_OK);
            continue;
        }

        if (size <= 0)  // 第一次可以出错
        {
            continue;
        }

        if (!show_size) // 显示文件大小
        {
            format_data(size, info);
            ListView_SetItemText(g_list, i, LIST_SIZE, info);

            g_task[taskid].show_size = TRUE;
        }

        if ((down == size) || (time == last_time && time != 0)) // 下载完成
        {
            ListView_SetItemText(g_list, i, LIST_SPEE, _T(""));
            ListView_SetItemText(g_list, i, LIST_PROG, _T("100"));
            continue;
        }

        if (down == last_down)  // 没有速度
        {
            ListView_SetItemText(g_list, i, LIST_SPEE, _T(""));
        }
        else
        {
            format_data((unsigned __int64)((double)(down - last_down) / (time - last_time)), info);
            ListView_SetItemText(g_list, i, LIST_SPEE, info);

            SP(_T("%0.2f"), (double)down / size * 100.0);
            ListView_SetItemText(g_list, i, LIST_PROG, info);

            g_task[taskid].down = down;
            g_task[taskid].time = time;
        }

        SP(_T("%d"), time);
        ListView_SetItemText(g_list, i, LIST_TIME, info);
    }
}

/**
 * \brief   拖拽文件
 * \param   [in]  WPARAM w 拖拽句柄
 * \return  无
 */
void on_dropfiles(HWND wnd, WPARAM w)
{
    HDROP drop = (HDROP)w;

    // iFile:0-只取第1个,0xFFFFFFFF-返回拖拽文件个数
    char filename[MAX_PATH];
    DragQueryFileA(drop, 0, filename, MAX_PATH);
    DragFinish(drop);

    TCHAR info[MAX_PATH];

    int ret = get_torrent_info(filename, &g_torrent);

    if (0 != ret)
    {
        SP(_T("get_torrent_info error:%d"), ret);
        MessageBox(wnd, info, _T("error"), MB_OK);
        return;
    }

    LVITEM item;
    item.mask = LVIF_TEXT;

    ListView_DeleteAllItems(g_torr);

    for (int i = 0; i < g_torrent.count; i++)
    {
        format_data(g_torrent.file[i].len, info);

        item.iItem = i;
        item.iSubItem = TORR_SIZE;
        item.pszText = info;
        ListView_InsertItem(g_torr, &item);

        item.iSubItem = TORR_FILE;
        item.pszText = g_torrent.file[i].name_unicode;
        ListView_SetItem(g_torr, &item);
    }

    SetWindowTextA(g_edit, filename);
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
    if (LOWORD(l) == WM_RBUTTONDOWN)
    {
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(g_wnd);
        TrackPopupMenu(g_menu, 0, pt.x, pt.y, 0, wnd, 0);
    }
}

/**
 * \brief   改变大小消息处理函数
 * \param   [in]  LPARAM l 窗体s宽高
 * \return  无
 */
void on_size(LPARAM l)
{
    int t = 0;
    int s = 0;
    int w = LOWORD(l);
    int h = HIWORD(l);

    MoveWindow(g_edit, 0,       1, w - 70,     20, TRUE);
    MoveWindow(g_down, w - 70,  1,     70,     20, TRUE);
    MoveWindow(g_list, 0,      22,      w, h - 22, TRUE);
    MoveWindow(g_torr, 0,      22,      w, h - 22, TRUE);

    t = 40;
    s += t;
    ListView_SetColumnWidth(g_list, LIST_TASK, t);    // 任务ID

    t = 50;
    s += t;
    ListView_SetColumnWidth(g_list, LIST_SIZE, t);    // 大小

    t = 65;
    s += t;
    ListView_SetColumnWidth(g_list, LIST_SPEE, t);    // 速度

    t = 50;
    s += t;
    ListView_SetColumnWidth(g_list, LIST_PROG, t);    // 进度

    t = 40;
    s += t;
    ListView_SetColumnWidth(g_list, LIST_TIME, t);    // 用时

    t = w - s;
    ListView_SetColumnWidth(g_list, LIST_FILE, t);    // 目录

    t = 80;
    ListView_SetColumnWidth(g_torr, TORR_SIZE, t);    // 大小

    t = w - t;
    ListView_SetColumnWidth(g_torr, TORR_FILE, t);    // 文件
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

    g_down = CreateWindow(WC_BUTTON,
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

    SendMessage(g_down, WM_SETFONT, (WPARAM)font, (LPARAM)TRUE);

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
    ListView_InsertColumn(g_list, LIST_TASK, &col);

    col.pszText = _T("文件");
    ListView_InsertColumn(g_list, LIST_FILE, &col);

    col.mask = LVCF_TEXT | LVCF_FMT;
    col.fmt = LVCFMT_RIGHT;
    col.pszText = _T("大小");
    ListView_InsertColumn(g_list, LIST_SIZE, &col);

    col.pszText = _T("速度");
    ListView_InsertColumn(g_list, LIST_SPEE, &col);

    col.pszText = _T("进度");
    ListView_InsertColumn(g_list, LIST_PROG, &col);

    col.pszText = _T("用时");
    ListView_InsertColumn(g_list, LIST_TIME, &col);

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

    col.mask = LVCF_TEXT;
    col.pszText = _T("大小");
    ListView_InsertColumn(g_torr, TORR_SIZE, &col);

    col.pszText = _T("文件");
    ListView_InsertColumn(g_torr, TORR_FILE, &col);
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
    TCHAR info[MAX_PATH];

    for (int i = 0; i < ListView_GetItemCount(g_list); i++)
    {
        ListView_GetItemText(g_list, i, 0, info, SIZEOF(info));

        xl_sdk_stop_download_file(_ttoi(info));
    }

    DestroyWindow(wnd);
}

/**
 * \brief   窗体关闭处理函数
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_exit(HWND wnd)
{
    int ret = MessageBox(wnd, _T("确定退出?"), _T("消息"), MB_ICONQUESTION | MB_YESNO);

    if (IDNO == ret)
    {
        return;
    }

    DestroyWindow(wnd);

    TCHAR info[MAX_PATH];

    for (int i = 0; i < ListView_GetItemCount(g_list); i++)
    {
        ListView_GetItemText(g_list, i, 0, info, SIZEOF(info));

        xl_sdk_stop_download_file(_ttoi(info));
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
 * \brief   窗体显示函数
 * \param   [in]  HWND wnd 窗体句柄
 * \return  无
 */
void on_show(HWND wnd)
{
    ShowWindow(wnd, IsWindowVisible(wnd)?SW_HIDE:SW_SHOW);
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
        case IDM_EXIT:          on_exit(wnd);       break;
        case IDM_SHOW:          on_show(wnd);       break;
        case IDC_BTN_DOWNLOAD:  btn_download(wnd);  break;
    }
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
    if (g_nid.uCallbackMessage == msg)
    {
        on_sys_notify(wnd, l);
    }

    switch(msg)
    {
        case WM_TIMER:       on_timer(wnd);         break;
        case WM_DROPFILES:   on_dropfiles(wnd, w);  break;
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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR info[MAX_PATH];

    int ret = config_init("DownloadSDKServerStart.json");

    if (ret != 0)
    {
        SP(_T("config init fail %d"), ret);
        MessageBox(NULL, info, g_title, MB_OK);
        return -1;
    }

    DBG("config init ok, download path:%s", config_get_path());

    ret = http_init(80, http_process_callback);

    if (ret != 0)
    {
        ERR("http init fail %d", ret);
        return -2;
    }

    DBG("http init ok");

    // 加载拼音数据
    ret = pinyin_init_res("PINYIN", IDR_PINYIN);

    if (0 != ret)
    {
        ERR("pinyin init fail %d", ret);
        return -3;
    }

    DBG("pinyin init ok");

    // 初始化SDK
    ret = xl_sdk_init();

    if (0 != ret)
    {
        ERR("init error:%d", ret);
        return -4;
    }

    DBG("sdk init ok");

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
    wc.style         = CS_HREDRAW | CS_VREDRAW;     // 类型属性
    wc.lpfnWndProc   = window_proc;                 // 窗体消息处理函数
    wc.lpszClassName = _T("class_name");            // 类名称
    wc.hInstance     = hInstance;                   // 实例
    wc.hIcon         = icon;                        // 图标
    wc.hCursor       = cursor;                      // 鼠标指针
    wc.hbrBackground = brush;                       // 背景刷
    RegisterClass(&wc);

    // 创建窗体
    g_wnd = CreateWindow(wc.lpszClassName,          // 类名称
                         g_title,                   // 窗体名称
                         WS_OVERLAPPEDWINDOW,       // 窗体属性
                         x,  y,                     // 窗体位置
                         cx, cy,                    // 窗体大小
                         NULL,                      // 父窗句柄
                         NULL,                      // 菜单句柄
                         hInstance,                 // 实例句柄
                         NULL);                     // 参数,给WM_CREATE的lParam

    // 显示窗体
    ShowWindow(g_wnd, SW_SHOWNORMAL);

    // 重绘窗体
    UpdateWindow(g_wnd);

    // 系统托盘
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = g_wnd;                                         // 指定接收托盘消息的句柄
    g_nid.hIcon            = icon;                                          // 指定托盘图标
    g_nid.uFlags           = NIF_MESSAGE | NIF_ICON;                        // 消息,图标
    g_nid.uCallbackMessage = RegisterWindowMessage(_T("WM_MY_NOTIFYICON")); // 消息ID

    Shell_NotifyIcon(NIM_ADD, &g_nid);                                      // 添加系统托盘图标

    // 定时器
    SetTimer(g_wnd, 1, 5000, NULL);

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