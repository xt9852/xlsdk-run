/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         main.c
 *\author       xt
 *\version      1.0.0
 *\date         2022-02-08
 *\brief        主模块实现,UTF-8(No BOM)
 */
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include "resource.h"
#include "config.h"
#include "xt_log.h"
#include "xt_md5.h"
#include "xt_base64.h"
#include "xt_pinyin.h"
#include "xt_timer.h"
#include "xt_thread_pool.h"
#include "xt_memory_pool.h"
#include "xt_http.h"
#include "torrent.h"
#include "xl_sdk.h"

#define RT_ICONA         MAKEINTRESOURCEA(3)
#define RT_GROUP_ICONA   MAKEINTRESOURCEA((ULONG_PTR)(RT_ICON) + DIFFERENCE)

#define TITLE "DownloadSDKServerStart"          ///< 标题

/// 任务列表控件列ID
enum
{
    LIST_TASK,                                  ///< 任务ID
    LIST_FILE,                                  ///< 文件名
    LIST_SIZE,                                  ///< 文件大小
    LIST_SPEE,                                  ///< 下载速度
    LIST_PROG,                                  ///< 下载进度
    LIST_TIME                                   ///< 下载用时
};

/// 种子列表控件列ID
enum
{
    TORR_SIZE,                                  ///< 文件大小
    TORR_FILE                                   ///< 文件名
};

HWND                    g_wnd;                  ///< 主窗体句柄
HWND                    g_edit;                 ///< 输入框
HWND                    g_down;                 ///< "下载"按钮
HWND                    g_list;                 ///< 下在下载的文件列表控件
HWND                    g_torr;                 ///< 种子文件信息列表控件
HMENU                   g_menu;                 ///< 系统托盘菜单
NOTIFYICONDATA          g_nid           = {0};  ///< 任务栏图标数据结构

config                  g_cfg           = {0};  ///< 配置数据

xt_log                  g_log           = {0};  ///< 日志数据
xt_log                  g_test          = {0};  ///< 多日志测试

bt_torrent              g_torrent       = {0};  ///< 种子文件信息
xl_task                 g_task[128]     = {0};  ///< 当前正在下载的任务信息

/**
 *\brief        得到exe中图标数据
 *\param[in]    id              图片在图标中的id
 *\param[out]   content         图标数据
 *\param[out]   content_len     图标数据长
 *\return                       图标数据长
 */
int get_icon_data(int id, char *content, int *content_len)
{
    HRSRC    icon_res  = FindResourceA(NULL, MAKEINTRESOURCEA(id), RT_ICONA);
    HGLOBAL  icon_load = LoadResource(NULL, icon_res);
    char    *icon_data = LockResource(icon_load);
    int      icon_size = SizeofResource(NULL, icon_res);

    DBG("id:%d size:%d", id, icon_size);

    memcpy(content, icon_data, icon_size);

    *content_len += icon_size;

    return icon_size;
}

/**
 *\brief        exe中的图标和实际图标有差异,这个是通过BeyondCompare比较得到的数据
 *\param[out]   content         图标数据
 *\param[out]   group_data      图标组数据
 *\param[out]   group_size      图标组数据长
 *\return                       无
 */
void update_icon_data(char *content, char *group_data, int group_size)
{
    for (int i = 0, j = 0; i < group_size; i++, j++)
        {
            switch (i)
            {
                case 0x12:
                {
                    content[j++] = 0x46;
                    content[j++] = 0x00;
                    content[j]   = 0x00;
                    break;
                }
                case 0x20:
                {
                    content[j++] = 0x6E;
                    content[j++] = 0x01;
                    content[j]   = 0x00;
                    break;
                }
                case 0x2E:
                {
                    content[j++] = 0xD6;
                    content[j++] = 0x06;
                    content[j]   = 0x00;
                    break;
                }
                case 0x3C:
                {
                    content[j++] = 0x3E;
                    content[j++] = 0x0A;
                    content[j]   = 0x00;
                    break;
                }
                default:
                {
                    content[j] = group_data[i];
                }
            }
        }
}

/**
 *\brief        http回调函数,/favicon.ico
 *\param[out]   content_type    返回内容类型
 *\param[out]   content         返回内容
 *\param[out]   content_len     返回内容长度
 *\return       0               成功
 */
int http_process_icon(int *content_type, char *content, int *content_len)
{
    HRSRC    group_res  = FindResourceA(NULL, MAKEINTRESOURCEA(IDI_GREEN), RT_GROUP_ICONA);
    HGLOBAL  group_load = LoadResource(NULL, group_res);
    char    *group_data = LockResource(group_load);
    int      group_size = SizeofResource(NULL, group_res);

    update_icon_data(content, group_data, group_size);

    group_size += 8;
    content += group_size;
    *content_len = group_size;
    DBG("group size:%d", group_size);

    for (int i = 1; i < group_data[4] + 1; i++)
    {
        content += get_icon_data(i, content, content_len);
    }

    *content_type = HTTP_TYPE_ICON;
    return 0;
}

/**
 *\brief        http回调函数,主页
 *\param[out]   content_type    返回内容类型
 *\param[out]   content         返回内容
 *\param[out]   content_len     返回内容长度
 *\return       0               成功
 */
int http_process_index(int *content_type, char *content, int *content_len)
{
    strcpy_s(content, *content_len, "200");
    *content_type = HTTP_TYPE_HTML;
    *content_len = 3;
    return 0;
}

/**
 *\brief        http回调函数,文件
 *\param[in]    uri             URI地址
 *\param[out]   content_type    返回内容类型
 *\param[out]   content         返回内容
 *\param[out]   content_len     返回内容长度
 *\return       0               成功
 */
int http_process_file(const char *uri, int *content_type, char *content, int *content_len)
{
    char filename[512];
    sprintf_s(filename, sizeof(filename), "%s%s", g_cfg.http_path, uri);

    FILE *fp;
    fopen_s(&fp, filename, "rb");

    if (NULL == fp)
    {
        ERR("open %s fail", filename);
        return 404;
    }

    fseek(fp, 0, SEEK_END);

    *content_len = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    fread(content, 1, *content_len, fp);

    fclose(fp);

    DBG("%s size:%d", filename, *content_len);

    *content_type = HTTP_TYPE_HTML;
    return 0;
}

/**
 *\brief        http回调函数
 *\param[in]    uri             URI地址
 *\param[in]    arg_name        URI的参数名称
 *\param[in]    arg_data        URI的参数数据
 *\param[in]    arg_count       URI的参数数量
 *\param[out]   content_type    返回内容类型
 *\param[out]   content         返回内容
 *\param[out]   content_len     返回内容长度
 *\return       0               成功
 */
int http_process_callback(const char *uri, const char **arg_name, const char **arg_data, int arg_count,
                          int *content_type, char *content, int *content_len)
{
    for (int i = 0; i < arg_count; i++)
    {
        DBG("arg[%d]:%s:%s", i, arg_name[i], arg_data[i]);
    }

    if (0 == strcmp(uri, "/"))
    {
        return http_process_index(content_type, content, content_len);
    }
    else if (0 == strcmp(uri, "/favicon.ico"))
    {
        return http_process_icon(content_type, content, content_len);
    }
    else
    {
        return http_process_file(uri, content_type, content, content_len);
    }

    return 404;
}

/**
 *\brief        得到格式化后的信息
 *\param[in]    data            数据
 *\param[in]    unit            单位
 *\param[out]   info            信息
 *\return                       无
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
 *\brief        得到格式化后的信息
 *\param[in]    list_size       列表空间
 *\param[out]   list            列表
 *\param[out]   filename        最后一个选中的文件
 *\return       0               成功
 */
int get_select_list(int list_size, char *list, short *filename)
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
 *\brief        下载按钮
 *\param[in]    wnd             窗体句柄
 *\return                       无
 */
void btn_download(HWND wnd)
{/*
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

        ret = get_select_list(sizeof(list) - 1, list, taskname + 41);

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
        ERR("create task error:%d", ret);
        return;
    }

    ret = xl_sdk_start_download_file(taskid, tasktype);

    if (0 != ret)
    {
        ERR("download start error:%d", ret);
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
*/
}

/**
 *\brief        定时任务
 *\param[in]    wnd             窗体句柄
 *\return                       无
 */
void on_timer(HWND wnd)
{/*
    if (!IsWindowVisible(g_list))
    {
        return;
    }

    int              ret;
    TCHAR            info[128];

    int              taskid;
    bool             show_size;     // 是否已经显示文件大小
    unsigned int     time;          // 当前用时
    unsigned int     last_time;     // 上次刷新显示用时
    unsigned __int64 speed;         // 下载速度,第一次时,显示的是已经下载文件大小
    unsigned __int64 size;          // 文件总大小
    unsigned __int64 down;          // 已经下载的大小
    unsigned __int64 last_down;     // 上次刷新显示时已经下载的大小

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
            ERR("XL_QueryTaskInfo:%d", ret);
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
            speed = (0 == last_time) ? down : (unsigned __int64)((double)(down - last_down) / (time - last_time));

            format_data(speed, info);

            ListView_SetItemText(g_list, i, LIST_SPEE, info);

            SP(_T("%0.2f"), (double)down / size * 100.0);

            ListView_SetItemText(g_list, i, LIST_PROG, info);

            g_task[taskid].down = down;
            g_task[taskid].time = time;
        }

        SP(_T("%d"), time);
        ListView_SetItemText(g_list, i, LIST_TIME, info);
    }
*/
}

/**
 *\brief        拖拽文件
 *\param[in]    wnd             窗体句柄
 *\param[in]    w               拖拽句柄
 *\return                       无
 */
void on_dropfiles(HWND wnd, WPARAM w)
{/*
    HDROP drop = (HDROP)w;

    // iFile:0-只取第1个,0xFFFFFFFF-返回拖拽文件个数
    char filename[MAX_PATH];
    DragQueryFileA(drop, 0, filename, MAX_PATH);
    DragFinish(drop);

    DBG(filename);

    TCHAR info[MAX_PATH];

    int ret = get_torrent_info(filename, &g_torrent);

    if (0 != ret)
    {
        ERR("get torrent info error:%d", ret);
        return;
    }

    DBG(filename);

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

    DBG(filename);*/
}

/**
 *\brief        系统托盘消息处理函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    l               操作
 *\return                       无
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
 *\brief        改变大小消息处理函数
 *\param[in]    l               窗体s宽高
 *\return                       无
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
 *\brief        创建消息处理函数
 *\param[in]    wnd             窗体句柄
 *\return                       无
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
 *\brief        窗体关闭处理函数 \n
                当用户点击窗体上的关闭按钮时 \n
                系统发出WM_CLOSE消息,自己执行DestroyWindow关闭窗口 \n
                然后发送WM_DESTROY消息,自己执行PostQuitMessage关闭应用程序 \n
                最后发出WM_QUIT消息来关闭消息循环
 *\param[in]    wnd             窗体句柄
 *\return                       无
 */
void on_close(HWND wnd)
{
    TCHAR info[MAX_PATH];

    for (int i = 0; i < ListView_GetItemCount(g_list); i++)
    {
        ListView_GetItemText(g_list, i, 0, info, SIZEOF(info));

        //xl_sdk_stop_download_file(_ttoi(info));
    }

    DestroyWindow(wnd);
}

/**
 *\brief        窗体关闭处理函数
 *\param[in]    wnd             窗体句柄
 *\return                       无
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

        //xl_sdk_stop_download_file(_ttoi(info));
    }
}

/**
 *\brief        窗体消毁处理函数
 *\param[in]    wnd             窗体句柄
 *\return                       无
 */
void on_destory(HWND wnd)
{
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    PostQuitMessage(0);
}

/**
 *\brief        窗体显示函数
 *\param[in]    wnd             窗体句柄
 *\return                       无
 */
void on_show(HWND wnd)
{
    ShowWindow(wnd, IsWindowVisible(wnd)?SW_HIDE:SW_SHOW);
}

/**
 *\brief        命令消息处理函数,菜单,按钮都会发此消息
 *\param[in]    wnd             窗体句柄
 *\param[in]    w               消息参数
 *\return                       无
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
 *\brief        窗体类消息处理回调函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    msg             消息ID
 *\param[in]    w               消息参数
 *\param[in]    l               消息参数
 *\return                       消息处理结果,它与发送的消息有关
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
 *\brief    定时器任务回调
 *\param    [in]  param         自定义参数
 *\return                       无
 */
void timer_callback(void *param)
{
    DBG("param:%s", (char*)param);
}

/**
 *\brief        窗体类程序主函数
 *\param[in]    hInstance       当前实例句柄
 *\param[in]    hPrevInstance   先前实例句柄
 *\param[in]    lpCmdLine       命令行参数
 *\param[in]    nCmdShow        显示状态(最小化,最大化,隐藏)
 *\return                       exe程序返回值
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char m[MAX_PATH];

    int ret = config_init(TITLE".json", &g_cfg);

    if (ret != 0)
    {
        sprintf_s(m, sizeof(m), "init config fail %d", ret);
        MessageBoxA(NULL, m, TITLE, MB_OK);
        return -10;
    }

    strncpy_s(g_log.filename, sizeof(g_log.filename), g_cfg.log_filename, sizeof(g_log.filename) - 1);
    g_log.level  = g_cfg.log_level;
    g_log.cycle  = g_cfg.log_cycle;
    g_log.backup = g_cfg.log_backup;
    g_log.clean  = g_cfg.log_clean;

    ret = log_init(&g_log);

    if (ret != 0)
    {
        sprintf_s(m, sizeof(m), "init log fail %d", ret);
        MessageBoxA(NULL, m, TITLE, MB_OK);
        return -20;
    }

    DBG("g_log init ok");

    strncpy_s(g_test.filename, sizeof(g_test.filename), TITLE".test", sizeof(g_test.filename) - 1);
    g_test.level  = g_cfg.log_level;
    g_test.cycle  = g_cfg.log_cycle;
    g_test.backup = g_cfg.log_backup;
    g_test.clean  = g_cfg.log_clean;

    ret = log_init(&g_test);

    if (ret != 0)
    {
        ERR("init log test fail %d", ret);
        return -21;
    }

    DBG("g_log init ok");
    DBG("g_log init ok");
    DBG("g_log init ok");
    DBG("g_log init ok");

    D(&g_test, "g_test init ok");
    D(&g_test, "g_test init ok");
    D(&g_test, "g_test init ok");
    D(&g_test, "g_test init ok");
    D(&g_test, "g_test init ok");

    DBG("--------------------------------------------------------------------");

    xt_md5 md5;
    char   md5_out[128];
    char  *md5_in = "1234567890";

    ret = md5_get(md5_in, strlen(md5_in), &md5);

    if (ret != 0)
    {
        ERR("init log test fail %d", ret);
        return -30;
    }

    DBG("md5.A:%x B:%x C:%x D:%x", md5.A, md5.B, md5.C, md5.D);

    ret = md5_get_str(md5_in, strlen(md5_in), md5_out);

    if (ret != 0)
    {
        ERR("init log test fail %d", ret);
        return -31;
    }

    DBG("md5=%s", md5_out);

    md5_in = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890";

    ret = md5_get_str(md5_in, strlen(md5_in), md5_out);

    if (ret != 0)
    {
        ERR("init log test fail %d", ret);
        return -32;
    }

    DBG("md5=%s", md5_out);

    DBG("--------------------------------------------------------------------");

    int len;
    char base64[64];
    char output[64];
    char data[][10] = { "1", "12", "123", "1234" };

    for (int i = 0; i < 4; i++)
    {
        len = sizeof(base64);

        ret = base64_to(data[i], strlen(data[i]), base64, &len);

        if (ret != 0)
        {
            ERR("init log test fail %d", ret);
            return -40;
        }

        DBG("data:%s base64:%s len:%d", data[i], base64, len);

        ret = base64_from(base64, len, output, &len);

        if (ret != 0)
        {
            ERR("init log test fail %d", ret);
            return -41;
        }

        DBG("base64:%s data:%s len:%d", base64, output, len);
    }

    DBG("--------------------------------------------------------------------");

    ret = pinyin_init_res("PINYIN", IDR_PINYIN);

    if (ret != 0)
    {
        ERR("init pinyin fail %d", ret);
        return -50;
    }

    DBG("--------------------------------------------------------------------");

    xt_memory_pool mem_pool;

    ret = memory_pool_init(&mem_pool, 1024, 100);

    if (ret != 0)
    {
        ERR("init pinyin fail %d", ret);
        return -60;
    }

    void *mem = NULL;

    for (int i = 0; i < 2000; i++)
    {
        ret = memory_pool_get(&mem_pool, &mem);

        if (ret != 0)
        {
            ERR("memory_pool_get fail %d", ret);
            return -61;
        }

        DBG("memory_pool_get ret:%d count:%d list-size:%d count:%d head:%d tail:%d", ret, mem_pool.count,
        mem_pool.free.size, mem_pool.free.count, mem_pool.free.head, mem_pool.free.tail);
    }

    ret = memory_pool_put(&mem_pool, mem);

    if (ret != 0)
    {
        ERR("memory_pool_put fail %d", ret);
        return -62;
    }

    DBG("memory_pool_put ret:%d memory-pool-count:%d list-size:%d count:%d head:%d tail:%d", ret, mem_pool.count,
        mem_pool.free.size, mem_pool.free.count, mem_pool.free.head, mem_pool.free.tail);

    ret = memory_pool_uninit(&mem_pool);

    if (ret != 0)
    {
        ERR("memory_pool_uninit fail %d", ret);
        return -63;
    }

    DBG("--------------------------------------------------------------------");

    xt_thread_pool thread_pool;

    ret = thread_pool_init(&thread_pool, 10);

    if (ret != 0)
    {
        ERR("memory_pool_uninit fail %d", ret);
        return -70;
    }

    DBG("thread_pool_init=%d", ret);

    DBG("--------------------------------------------------------------------");

    xt_timer_manager manager;

    ret = timer_init(&manager);

    if (ret != 0)
    {
        ERR("memory_pool_uninit fail %d", ret);
        return -80;
    }

    DBG("timer_init=%d", ret);

    ret = timer_add_cycle(&manager, "timer_0",  5, &thread_pool, timer_callback, "timer_0_param");

    if (ret != 0)
    {
        ERR("memory_pool_uninit fail %d", ret);
        return -81;
    }

    DBG("timer_add_cycle=%d", ret);

    ret = timer_add_cycle(&manager, "timer_1", 10, &thread_pool, timer_callback, "timer_1_param");

    if (ret != 0)
    {
        ERR("memory_pool_uninit fail %d", ret);
        return -82;
    }

    DBG("timer_add_cycle=%d", ret);

    ret = timer_add_cron(&manager, "timer_2", TIMER_CRON_MINUTE, 0, 0, 0, 0, 0, 0, 0, &thread_pool, timer_callback, "timer_2_param");

    if (ret != 0)
    {
        ERR("memory_pool_uninit fail %d", ret);
        return -83;
    }

    DBG("timer_add_cron=%d", ret);

    DBG("--------------------------------------------------------------------");

    xt_http http;
    http.port = 80;
    http.run  = true;
    http.proc = http_process_callback;

    ret = http_init(&http);

    if (ret != 0)
    {
        ERR("http init fail %d", ret);
        return -90;
    }

    DBG("http init ok");

    DBG("--------------------------------------------------------------------");

    // 初始化SDK
    ret = xl_sdk_init();

    if (0 != ret)
    {
        ERR("init error:%d", ret);
        return -100;
    }

    DBG("sdk init ok");

/*
    int error;
    PCRE2_SIZE offset;
    PCRE2_SIZE *ovector;

    char *pattern = "{0-9}{5}";

    pcre2_code *pcre_data = pcre2_compile((PCRE2_SPTR)pattern, 0, 0, &error, &offset, NULL);

    if (pcre_data == NULL)
    {
        PCRE2_UCHAR info[256];
        //pcre2_get_error_message(error, info, sizeof(info));
        ERR("PCRE init fail pattern:%s offste:%d err:%s", pattern, offset, error, info);
    }

    DBG("pcre2_compile ok pattern:\"%s\"", pattern);

    char *subject = "abcdefghijkl";

    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(pcre_data, NULL);

    ret = pcre2_match(pcre_data, (PCRE2_SPTR)subject, strlen(subject), 0, 0, match_data, NULL); // <0发生错误，==0没有匹配上，>0返回匹配到的元素数量

    if (ret > 0)
    {
        ovector = pcre2_get_ovector_pointer(match_data);

        for (int i = 0; i < ret; i++)
        {
            //int substring_length = ovector[2 * i + 1] - ovector[2 * i];
            //char* substring_start = subject + ovector[2*i];
            //DBG("%d: %d %.*s", i, substring_length, substring_start);
            DBG("%d:%d %d", i, ovector[2 * i], ovector[2 * i + 1]);
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(pcre_data);

    DBG("pcre2_match subject:\"%s\" ret:%d", subject, ret);
*/
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
    g_wnd = CreateWindow(wc.lpszClassName,              // 类名称
                         _T(TITLE),                     // 窗体名称
                         WS_OVERLAPPEDWINDOW,           // 窗体属性
                         x,  y,                         // 窗体位置
                         cx, cy,                        // 窗体大小
                         NULL,                          // 父窗句柄
                         NULL,                          // 菜单句柄
                         hInstance,                     // 实例句柄
                         NULL);                         // 参数,给WM_CREATE的lParam

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