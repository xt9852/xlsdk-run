/**
 *\file     main.c
 *\note     UTF-8
 *\author   xt
 *\version  1.0.0
 *\date     2022.02.08
 *\brief    主模块
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
#include "xt_notify.h"
#include "xt_http.h"
#include "xt_exe_ico.h"
#include "xt_character_set.h"
#include "xt_uri.h"
#include "xt_utitly.h"
#include "xl_sdk.h"
#include "torrent.h"

/// 首页页面
#define INDEX_PAGE "<meta charset='utf-8'>\n\
<style>\n\
    .tr_hover tr:hover {\n\
        background: #F0F0F0;\n\
    }\n\
</style>\n\
<table border='1' style='width:100%;border-collapse:collapse;font-family:宋体' class='tr_hover'>\n\
    <td width='60px'>大小</td>\n\
    <td width='60px'>进度</td>\n\
    <td width='60px'>速度</td>\n\
    <td>\n\
        <div style='display:flex'>\n\
            <input id ='input' style='flex:1;margin-right:1'>\n\
        </div>\n\
    </td>\n\
    <td width='43px'>\n\
        <button onclick='download()'>下载</button>\n\
    </td>\n\
</table>\n\
<script>\n\
    let data = [];\n\
    let torrent = [];\n\
    let input = document.getElementById('input');\n\
    let tbody = document.getElementsByTagName('tbody')[0];\n\
    function http(url, arg, callback) {\n\
        let req = new XMLHttpRequest();\n\
        req.open('GET', url + arg);\n\
        req.send(null);\n\
        req.onload = function() {\n\
            if (req.readyState != 4 || req.status != 200) {\n\
                alert('请求失败');\n\
            } else {\n\
                console.log('http ' + url + arg, JSON.parse(req.responseText));\n\
                callback(JSON.parse(req.responseText), arg);\n\
            }\n\
        }\n\
    }\n\
    function tr5(id, name) {\n\
        if (id != '') {\n\
            return '<button onclick=\"http(\\'/task\\',\\'?del=' + id + '\\',task_list)\">删除</button>';\n\
        } else if (name.slice(-8) == '.torrent') {\n\
            return '<button onclick=\"http(\\'/torrent_content?torrent=\\',\\'' + btoa(name) + '\\',torrent_content)\">打开</button>';\n\
        } else {\n\
            return '<input type=\"checkbox\">';\n\
        }\n\
    }\n\
    function insert(tb, id, size, prog, speed, name, t5) {\n\
        html = tr5(id, name);\n\
        tr = document.createElement('tr');\n\
        t0 = document.createElement('td');\n\
        t1 = document.createElement('td');\n\
        t2 = document.createElement('td');\n\
        t3 = document.createElement('td');\n\
        t4 = document.createElement('td');\n\
        t5 = document.createElement('dir');\n\
        tb.insertAdjacentElement('afterend', tr);\n\
        tr.appendChild(t0);\n\
        tr.appendChild(t1);\n\
        tr.appendChild(t2);\n\
        tr.appendChild(t3);\n\
        tr.appendChild(t4);\n\
        t4.appendChild(t5);\n\
        t0.innerText = size;\n\
        t1.innerText = prog;\n\
        t2.innerText = speed;\n\
        t3.innerText = name;\n\
        t5.outerHTML = html;\n\
    }\n\
    function update(tr, id, size, prog, speed, name) {\n\
        html = tr5(id, name);\n\
        tr.childNodes[0].innerText = size;\n\
        tr.childNodes[1].innerText = prog;\n\
        tr.childNodes[2].innerText = speed;\n\
        tr.childNodes[3].innerText = name;\n\
        tr.childNodes[4].childNodes[0].outerHTML = html;\n\
    }\n\
    function table(rsp) {\n\
        console.log('table', JSON.parse(JSON.stringify(rsp)));\n\
        tr = tbody.childNodes;\n\
        min = Math.min(rsp.length, tr.length - 1);\n\
        max = Math.max(rsp.length, tr.length - 1);\n\
        length = tr.length - 1;\n\
        for (let i = 0; i < min; i++) {\n\
            update(tr[i + 1], rsp[i].id, rsp[i].size, rsp[i].prog, rsp[i].speed, rsp[i].task);\n\
        }\n\
        for (let i = min; i < max && rsp.length > length; i++) {\n\
            insert(tbody.lastChild, rsp[i].id, rsp[i].size, rsp[i].prog, rsp[i].speed, rsp[i].task);\n\
        }\n\
        for (let i = min; i < max && rsp.length < length; i++) {\n\
            tbody.removeChild(tr[min + 1]);\n\
        }\n\
    }\n\
    function task_list(rsp, arg) {\n\
        input.value = '';\n\
        data = rsp;\n\
        for (let i = 0; i < data.length; i++) {\n\
            data[i].task = decodeURIComponent(atob(data[i].task));\n\
        }\n\
        table(data);\n\
    }\n\
    function torrent_content(rsp, arg) {\n\
        torrent = arg;\n\
        arg = decodeURIComponent(atob(arg));\n\
        for (let i = data.length - 1; i >= 0; i--) {\n\
            if (data[i].id == '' && data[i].prog == '') {\n\
                data.splice(i, 1);\n\
            }\n\
        }\n\
        if (data.length > 1) {\n\
            for (let i = 0; i < data.length; i++) {\n\
                if (data[i].task == arg) {\n\
                    for (let j = 0; j < rsp.length; j++) {\n\
                        data.splice(i + j + 1, 0, { id : '', size : rsp[j].size, prog : '', speed : '', task : decodeURIComponent(atob(rsp[j].filename)) });\n\
                    }\n\
                    break;\n\
                }\n\
            }\n\
        } else {\n\
            for (let i = 0; i < rsp.length; i++) {\n\
                data.push({ id : '', size : rsp[i].size, prog : '', speed : '', task : decodeURIComponent(atob(rsp[i].filename)) });\n\
            }\n\
        }\n\
        table(data);\n\
    }\n\
    function download() {\n\
        console.log('download');\n\
        let arg = btoa(input.value);\n\
        if (arg != '') {\n\
            arg = '?add=' + arg;\n\
        } else {\n\
            mask = '';\n\
            tr = tbody.childNodes;\n\
            for (let i = 1; i < tr.length; i++) {\n\
                if (tr[i].childNodes[4].childNodes[0].type == 'checkbox') {\n\
                    mask += tr[i].childNodes[4].childNodes[0].checked * 1;\n\
                }\n\
            }\n\
            if (mask != '') {\n\
                arg = '?add=' + torrent + '&mask=' + mask;\n\
            }\n\
        }\n\
        http('/task', arg, task_list);\n\
    }\n\
</script>"


char                g_path[MAX_PATH]        = "";       ///< 文件路径

char               *g_title                 = NULL;     ///< 标题

xt_log              g_log                   = {0};      ///< 日志文件

xt_http             g_http                  = {0};      ///< HTTP服务

bt_torrent          g_torrent               = {0};      ///< 种子数据

config              g_cfg                   = {&g_log}; ///< 配置数据

extern xl_task      g_task[TASK_SIZE];                  ///< 当前正在下载的任务信息

extern unsigned int g_task_count;                       ///< 当前正在下载的任务数量

/**
 *\brief                        HTTP回调函数,首页
 *\param[out]   data            HTTP的数据,data->len输入时为缓冲区长度,输出时为数据长度
 *\return       0               成功
 */
int http_proc_index(const p_xt_http_data data)
{
    D("index");
    data->type = HTTP_TYPE_HTML;
    data->len = sizeof(INDEX_PAGE) - 1;
    strcpy_s(data->content, sizeof(INDEX_PAGE), INDEX_PAGE);
    return 0;
}

/**
 *\brief                        HTTP回调函数,默认图标
 *\param[out]   data            HTTP的数据,data->len输入时为缓冲区长度,输出时为数据长度
 *\return       0               成功
 */
int http_proc_icon(const p_xt_http_data data)
{
    D("icon");
    data->type = HTTP_TYPE_ICO;
    exe_ico_get_data(IDI_GREEN, data->content, &(data->len));
    return 0;
}

/**
 *\brief                        将字符串转成uri编码再base64
 *\param[in]    input           输入字符串
 *\param[in]    input_len       输入字符串长度
 *\param[out]   output          输入字符串
 *\param[out]   output_len      输出字符串长度,输入时为缓冲区长度,输出时为数据长度
 *\return       0               成功
 */
int to_uri_base64(const char *input, int input_len, char *output, int *output_len)
{
    char tmp[10240];
    int  len = sizeof(tmp);

    if (NULL == input || NULL == output || NULL == output_len)
    {
        E("arg null");
    }

    D("task name %s %d %d", input, input_len, *output_len);

    if (0 != uri_encode(input, input_len, tmp, &len)) // js的atob不能解码unicode
    {
        E("uri_encode fail %s", input);
        return -2;
    }

    D("uri_encode %s %d", tmp, len);

    if (0 != base64_encode(tmp, len, output, output_len)) // 文件名中可能有json需要转码的字符
    {
        E("base64_encode fail %s", tmp);
        return -3;
    }

    D("base64_encode %s %d", output, *output_len);
    return 0;
}

/**
 *\brief                        查找本地种子文件
 *\param[out]   buf             输入字符串
 *\param[out]   len             输出字符串长度,输入时为缓冲区长度,输出时为数据长度
 *\param[in]    task            是否是任务数据
 *\param[in]    count           初始数值
 *\return       0               成功
 */
int get_local_torrent(char *buf, int *len, int task, int count)
{
    char  tmp[10240];
    char  size[16];
    char  filename[MAX_PATH];
    int   filename_len;
    int   buf_size = *len;
    int   pos = 0;

    sprintf_s(filename, MAX_PATH, "%s\\*.torrent", g_cfg.path_tmp);

    WIN32_FIND_DATAA wfd;

    HANDLE find = FindFirstFileA(filename, &wfd);

    if (INVALID_HANDLE_VALUE == find)
    {
        E("FindFirstFileA path %s error %d", filename, GetLastError());
        return -1;
    }

    while (true)
    {
        bool have = false;

        filename_len = sizeof(filename);

        sprintf_s(tmp, MAX_PATH, "%s\\%s", g_cfg.path_tmp, wfd.cFileName);

        if (0 != gbk_utf8(tmp, strlen(tmp), filename, &filename_len))
        {
            E("ansi_utf8 fail %s", wfd.cFileName);
            return -2;
        }

        // 排除正下载的任务
        for (unsigned int i = 0; i < g_task_count; i++)
        {
            if (0 == strcmp(filename, g_task[i].torrent_filename))
            {
                have = true;
                break;
            }
        }

        if (!have)
        {
            *len = sizeof(tmp);

            to_uri_base64(filename, filename_len, tmp, len);

            format_data((unsigned __int64)(wfd.nFileSizeHigh) << 32 | wfd.nFileSizeLow, size, sizeof(size));

            if (!task)
            {
                pos += snprintf(buf + pos, buf_size - pos, "%s{\"filename\":\"%s\",\"size\":\"%s\"}", (0 == count ? "" : ","), tmp, size);
            }
            else
            {
                pos += snprintf(buf + pos, buf_size - pos, "%s{\"id\":\"\",\"size\":\"%s\",\"prog\":\"100%%\",\"speed\":\"\",\"task\":\"%s\"}",
                                (0 == count ? "" : ","), size, tmp);
            }

            count++;
        }

        if (!FindNextFileA(find, &wfd)) break;
    }

    FindClose(find);

    *len = pos;
    return 0;
}

/**
 *\brief                        http回调函数,任务信息
 *\param[out]   data            HTTP的数据,data->len输入时为缓冲区长度,输出时为数据长度
 *\return       0               成功
 */
int http_proc_task(const p_xt_http_data data)
{
    const char *del = NULL;    // 可以为空
    const char *add = NULL;    // 可以为空
    const char *msk = NULL;    // 可以为空
    unsigned int addr_len = 0;

    for (unsigned int i = 0; i < data->arg_count; i++)
    {
        if (data->arg[i].key   != NULL && 0 == strcmp(data->arg[i].key, "add") &&
            data->arg[i].value != NULL && 0 != strcmp(data->arg[i].value, ""))
        {
            add = data->arg[i].value;
            addr_len = data->arg[i].value_len;
        }
        else if (data->arg[i].key   != NULL && 0 == strcmp(data->arg[i].key, "mask") &&
                 data->arg[i].value != NULL && 0 != strcmp(data->arg[i].value, ""))
        {
            msk = data->arg[i].value;
        }
        else if (data->arg[i].key   != NULL && 0 == strcmp(data->arg[i].key, "del") &&
                 data->arg[i].value != NULL && 0 != strcmp(data->arg[i].value, ""))
        {
            del = data->arg[i].value;
        }
    }

    if (NULL != del)
    {
        D("del:%s", del);

        unsigned int task_id = atoi(del);

        for (unsigned int i = 0; i < g_task_count; i++)
        {
            if (g_task[i].type == TASK_BT && g_task[i].id == task_id && g_task[i].prog == 100)
            {
                D("prog:%f", g_task[i].prog);

                int len;
                int src_len;
                int dst_len;
                char path[MAX_PATH];
                short src[MAX_PATH];
                short dst[MAX_PATH];
                char *temp = &(g_task[i].name[41]);
                char *name = strtok_s(temp, "|", &temp);

                for (i = 0; NULL != name; i++)
                {
                    src_len = MAX_PATH - 1;
                    len = snprintf(path, sizeof(path), "%s\\%s", g_cfg.path_tmp, name);

                    if (0 != utf8_unicode(path, len, src, &src_len))
                    {
                        E("utf8_unicode error %s", path);
                        break;
                    }

                    dst_len = MAX_PATH - 1;
                    len = snprintf(path, sizeof(path), "%s\\%s", g_cfg.path_download, name);

                    if (0 != utf8_unicode(path, len, dst, &dst_len))
                    {
                        E("utf8_unicode error %s", path);
                        break;
                    }

                    MoveFileW(src, dst);

                    D("move:%s", path);

                    name = strtok_s(NULL, "|", &temp);
                }

                break;
            }
        }

        xl_sdk_stop_task(task_id);
        xl_sdk_del_task(task_id);
        D("del task:%s", del);
    }

    int  len;
    char tmp[10240];

    if (NULL != add)
    {
        D("add:%s", add);

        len = sizeof(tmp);

        if (0 != base64_decode(add, addr_len, tmp, &len))
        {
            E("base64_decode fail %s", add);
            return -1;
        }

        if (0 != xl_sdk_download((0 != strncmp(add, "http", 4)) ? g_cfg.path_tmp : g_cfg.path_download, tmp, msk, &g_torrent))
        {
            E("xl_sdk_download fail");
            return -2;
        }
    }

    int    pos = 1;
    char   size[16];
    char   speed[16];
    char  *content = data->content;

    for (unsigned int i = 0; i < g_task_count; i++)
    {
        len = sizeof(tmp);

        to_uri_base64(g_task[i].name, g_task[i].name_len, tmp, &len);

        format_data(g_task[i].size, size, sizeof(size));
        format_data(g_task[i].speed, speed, sizeof(speed));

        pos += snprintf(content + pos, data->len - pos,
                       "%s{\"id\":%d,\"size\":\"%s\",\"prog\":\"%.2f%%\",\"speed\":\"%s\",\"task\":\"%s\"}",
                       (0 == i ? "": ","), g_task[i].id, size, g_task[i].prog, speed, tmp);
    }

    len = data->len - pos;
    get_local_torrent(content + pos, &len, 1, g_task_count);
    pos += len;

    content[0] = '[';
    content[pos++] = ']';
    data->type = HTTP_TYPE_HTML;
    data->len = pos;
    return 0;
}

/**
 *\brief                        http回调函数,种子文件列表
 *\param[out]   data            HTTP的数据,data->len输入时为缓冲区长度,输出时为数据长度
 *\return       0               成功
 */
int http_proc_torrent_list(const p_xt_http_data data)
{
    int pos = 1;
    int len = data->len - pos;
    char *content = data->content;

    get_local_torrent(content + pos, &len, 0, 0);
    pos += len;

    content[0] = '[';
    content[pos++] = ']';
    data->type = HTTP_TYPE_HTML;
    data->len = pos;
    return 0;
}

/**
 *\brief                        http回调函数,种子中文件信息
 *\param[out]   data            HTTP的数据,data->len输入时为缓冲区长度,输出时为数据长度
 *\return       0               成功
 */
int http_proc_torrent_content(const p_xt_http_data data)
{
    if (data->arg_count <= 0 || NULL == data->arg[0].key || 0 != strcmp(data->arg[0].key, "torrent"))
    {
        D("torrent:null or \"\"");
        return -1;
    }

    const char *torrent_filename = data->arg[0].value;

    if (NULL == torrent_filename || 0 == strcmp(torrent_filename, ""))
    {
        D("torrent:null or \"\"");
        return -2;
    }

    char tmp[10240];
    int  len = sizeof(tmp);

    if (0 != base64_decode(torrent_filename, data->arg[0].value_len, tmp, &len))
    {
        E("base64_decode fail %s", torrent_filename);
        return -3;
    }

    if (0 != get_torrent_info(tmp, &g_torrent))
    {
        E("get torrent:%s info error", tmp);
        return -4;
    }

    int   pos = 1;
    char  size[16];
    char *content = data->content;

    for (int i = 0; i < g_torrent.count; i++)
    {
        len = sizeof(tmp);

        to_uri_base64(g_torrent.file[i].name, g_torrent.file[i].name_len, tmp, &len);

        format_data(g_torrent.file[i].size, size, sizeof(size));

        pos += snprintf(content + pos, data->len - pos,
                       "%s{\"filename\":\"%s\",\"size\":\"%s\"}",
                       (0 == i ? "" : ","), tmp, size);
    }

    content[0] = '[';
    content[pos++] = ']';
    data->type = HTTP_TYPE_HTML;
    data->len = pos;
    return 0;
}

/**
 *\brief                        HTTP回调函数
 *\param[out]   data            HTTP的数据
 *\return       0               成功
 */
int http_proc_callback(const p_xt_http_data data)
{
    D("uri:%s", data->uri);

    if (0 == strcmp(data->uri, "/"))
    {
        return http_proc_index(data);
    }
    else if (0 == strcmp(data->uri, "/favicon.ico"))
    {
        return http_proc_icon(data);
    }
    else if (0 == strcmp(data->uri, "/task"))
    {
        return http_proc_task(data);
    }
    else if (0 == strcmp(data->uri, "/torrent_list"))
    {
        return http_proc_torrent_list(data);
    }
    else if (0 == strcmp(data->uri, "/torrent_content"))
    {
        return http_proc_torrent_content(data);
    }

    return 404;
}

/**
 *\brief                        打开临时目录处理函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    param           自定义参数
 *\return                       无
 */
void on_menu_tmp(HWND wnd, void *param)
{
    ShellExecuteA(NULL, "open", g_cfg.path_tmp, NULL, NULL, SW_SHOWNORMAL);
}

/**
 *\brief                        打开下载目录处理函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    param           自定义参数
 *\return                       无
 */
void on_menu_down(HWND wnd, void *param)
{
    ShellExecuteA(NULL, "open", g_cfg.path_download, NULL, NULL, SW_SHOWNORMAL);
}

/**
 *\brief                        打开页面处理函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    param           自定义参数
 *\return                       无
 */
void on_menu_page(HWND wnd, void *param)
{
    char tmp[MAX_PATH];
    snprintf(tmp, sizeof(tmp), "http://%s:%d", g_cfg.http_ip, g_cfg.http_port);
    ShellExecuteA(NULL, "open", tmp, NULL, NULL, SW_HIDE);
}

/**
 *\brief                        打开配置处理函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    param           自定义参数
 *\return                       无
 */
void on_menu_config(HWND wnd, void *param)
{
    char tmp[MAX_PATH];
    snprintf(tmp, sizeof(tmp), "%s\\%s.json", g_path, g_title);
    ShellExecuteA(NULL, "open", tmp, NULL, NULL, SW_HIDE);
}

/**
 *\brief                        窗体关闭处理函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    param           自定义参数
 *\return                       无
 */
void on_menu_exit(HWND wnd, void *param)
{
    if (IDNO == MessageBoxW(wnd, L"确定退出?", L"消息", MB_ICONQUESTION | MB_YESNO))
    {
        return;
    }

    for (unsigned int i = 0; i < g_task_count; i++)
    {
        xl_sdk_stop_task(g_task[i].id);
    }

    DestroyWindow(wnd);
}

/**
 *\brief                        窗体类程序主函数
 *\param[in]    hInstance       当前实例句柄
 *\param[in]    hPrevInstance   先前实例句柄
 *\param[in]    lpCmdLine       命令行参数
 *\param[in]    nCmdShow        显示状态(最小化,最大化,隐藏)
 *\return                       exe程序返回值
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    GetModuleFileNameA(hInstance, g_path, sizeof(g_path));

    g_title = strrchr(g_path, '\\');
    *g_title++ = '\0';

    char *end = strrchr(g_title, '.');
    *end = '\0';

    char tmp[MAX_PATH];
    snprintf(tmp, sizeof(tmp), "%s\\%s.json", g_path, g_title);

    int ret = config_init(tmp, &g_cfg);

    if (ret != 0)
    {
        MessageBoxW(NULL, L"配置错误", L"错误", MB_OK);
        return -1;
    }

    ret = log_init(g_path, 26, g_cfg.log);  // 26是当前代码的根目录长度,日志中只保留代码的相对路径

    if (ret != 0)
    {
        MessageBoxW(NULL, L"日志错误", L"错误", MB_OK);
        return -2;
    }

    ret = http_init(g_cfg.http_ip, g_cfg.http_port, http_proc_callback, &g_http);

    if (ret != 0)
    {
        E("http init fail %d", ret);
        MessageBoxW(NULL, L"HTTP错误", L"错误", MB_OK);
        return -3;
    }

    ret = xl_sdk_init();

    if (0 != ret)
    {
        E("init error:%d", ret);
        MessageBoxW(NULL, L"SDK错误", L"错误", MB_OK);
        return -4;
    }

    notify_menu_info menu[] = {
        {L"打开页面(&I)", NULL, on_menu_page},
        {L"打开配置(&C)", NULL, on_menu_config},
        {L"临时目录(&T)", NULL, on_menu_tmp},
        {L"下载目录(&D)", NULL, on_menu_down},
        {L"退出程序(&Q)", NULL, on_menu_exit} };

    ret = notify_init(hInstance, IDI_GREEN, "DownloadSDKServerRun", SIZEOF(menu), menu);

    if (0 != ret)
    {
        MessageBoxW(NULL, L"菜单错误", L"错误", MB_OK);
        return -5;
    }

    return notify_loop_msg();
}
