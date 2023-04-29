/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         main.c
 *\author       xt
 *\version      1.0.0
 *\date         2022-02-08
 *\note         Encode:UTF-8
 *\brief        主模块
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
#include "xl_sdk.h"
#include "torrent.h"
#define  PCRE2_STATIC
#define  PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

/// 程序标题
#define TITLE "DownloadSDKServerStart"

/// 首页页面
#define INDEX_PAGE "<meta charset='utf-8'>"\
"<script>"\
    "function http(url, data, callback){"\
        "console.log(url + data);"\
        "req = new XMLHttpRequest();"\
        "req.open('GET', url + data);"\
        "req.send(null);"\
        "req.onload = function(){"\
            "if (req.readyState != 4 || req.status != 200) alert('请求失败');"\
            "else callback(JSON.parse(req.responseText), data);"\
        "}"\
    "}"\
    "function tbody(count, data){"\
        "document.getElementsByTagName('input')[0].value = (data == 'task' || data == 'torrent') ? '' : data;"\
        "for (tbd = document.getElementsByTagName('tbody')[0]; tbd.childNodes.length > 1;){tbd.removeChild(tbd.childNodes[1]);}"\
        "show = (data == 'task') ? '' : 'none';"\
        "title = tbd.childNodes[0].childNodes;"\
        "title[1].innerText = '任务(' + count + ')';"\
        "title[2].style.display = show;"\
        "title[3].style.display = show;"\
        "title[4].style.display = show;"\
        "return tbd;"\
    "}"\
    "function task_list(rsp, data){"\
        "tb = tbody(rsp.length, 'task');"\
        "for (i in rsp) {"\
            "tr = document.createElement('tr'); tb.appendChild(tr);"\
            "t1 = document.createElement('td'); tr.appendChild(t1);"\
            "t2 = document.createElement('td'); tr.appendChild(t2);"\
            "t3 = document.createElement('td'); tr.appendChild(t3);"\
            "t4 = document.createElement('td'); tr.appendChild(t4);"\
            "t5 = document.createElement('td'); tr.appendChild(t5);"\
            "t6 = document.createElement('td'); tr.appendChild(t6);"\
            "t3.align = 'right';"\
            "t4.align = 'right';"\
            "t5.align = 'right';"\
            "t1.innerText = rsp[i].id;"\
            "t2.innerText = decodeURIComponent(atob(rsp[i].task));"\
            "t3.innerText = rsp[i].size;"\
            "t4.innerText = rsp[i].prog + '%';"\
            "t5.innerText = rsp[i].speed;"\
            "t6.outerHTML = '<td><button onclick=\"http(\\'/task?del=\\',\\''+rsp[i].id+'\\', task_list)\">删除</button></td>';"\
        "}"\
    "}"\
    "function torrent_list(rsp, data){"\
        "tb = tbody(rsp.length, 'torrent');"\
        "for (i in rsp) {"\
            "tr = document.createElement('tr'); tb.appendChild(tr);"\
            "t1 = document.createElement('td'); tr.appendChild(t1);"\
            "t2 = document.createElement('td'); tr.appendChild(t2);"\
            "t3 = document.createElement('td'); tr.appendChild(t3);"\
            "t1.innerText = i;"\
            "t2.innerText = task_name = decodeURIComponent(atob(rsp[i].filename));"\
            "t3.outerHTML='<td><button onclick=\"http(\\'/file?torrent=\\',\\''+btoa(task_name)+'\\',torrent_file)\">打开</button></td>';"\
        "}"\
    "}"\
    "function torrent_file(rsp, data){"\
        "tb = tbody(rsp.length, atob(data));"\
        "for (i in rsp) {"\
            "tr = document.createElement('tr'); tb.appendChild(tr);"\
            "t1 = document.createElement('td'); tr.appendChild(t1);"\
            "t2 = document.createElement('td'); tr.appendChild(t2);"\
            "t3 = document.createElement('td'); tr.appendChild(t3);"\
            "t1.outerHTML = '<td><input type=\"checkbox\"></td>';"\
            "t2.innerText = decodeURIComponent(atob(rsp[i].file));"\
            "t3.innerText = rsp[i].size;"\
        "}"\
    "}"\
    "function download(){"\
        "data = btoa(document.getElementsByTagName('input')[0].value);"\
        "tr = document.getElementsByTagName('tr')[1];"\
        "if (tr && tr.childNodes[0].childNodes[0].type == 'checkbox') {"\
            "for (mask = '&msk='; tr; tr = tr.nextSibling) {mask += tr.childNodes[0].childNodes[0].checked * 1;}"\
            "if (/1+/.test(mask)) {data += mask;} else {data = '';}"\
        "}"\
        "http('/task?add=', data, task_list);"\
    "}"\
"</script>"\
"<div style='display:flex;margin-right:2'><input style='flex:1;margin-right:1'/><button onclick='download()'>下载</button></div>"\
"<table width='100%' border='1' style='border-collapse:collapse'>"\
    "<th width='30px'>ID</th><th>任务</th><th width='61px'>大小</th><th width='61px'>进度</th><th width='61px'>速度</th>"\
    "<th width='43px'><button onclick=\"http(\'/torrent\', \'\', torrent_list)\">种子</button></th>"\
"</table>"

config              g_cfg                   = {0};  ///< 配置数据

xt_log              g_log                   = {0};  ///< 日志数据

xt_http             g_http                  = {0};  ///< HTTP服务

bt_torrent          g_torrent               = {0};  ///< 种子数据

extern xl_task      g_task[TASK_SIZE];              ///< 当前正在下载的任务信息

extern unsigned int g_task_count;                   ///< 当前正在下载的任务数量

/**
 *\brief        http回调函数,得到种子中文件信息
 *\param[out]   data            HTTP的数据
 *\return       0               成功
 */
int http_proc_file(const p_xt_http_data data)
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

    char buf[20480];
    int  len = sizeof(buf);

    if (0 != base64_decode(torrent_filename, data->arg[0].value_len, buf, &len))
    {
        E("base64_decode fail %s", torrent_filename);
        return -3;
    }

    if (0 != get_torrent_info(buf, &g_torrent))
    {
        E("get torrent:%s info error", buf);
        return -4;
    }

    int   pos;
    int   base64_len;
    char  size[16];
    char *content;

    pos = 1;
    content = data->content;
    content[0] = '[';

    for (int i = 0; i < g_torrent.count; i++)
    {
        format_data(g_torrent.file[i].size, size, sizeof(size));

        pos += snprintf(content + pos, data->len - pos, "{\"size\":\"%s\",\"file\":\"", size);

        len = sizeof(buf);

        if (0 != uri_encode(g_torrent.file[i].name, g_torrent.file[i].name_len, buf, &len)) // js的atob不能解码unicode
        {
            E("uri_encode fail %s", g_torrent.file[i].name);
            return -5;
        }

        base64_len = data->len - pos;

        if (0 != base64_encode(buf, len, content + pos, &base64_len)) // 文件名中可能有json需要转码的字符
        {
            E("base64_encode fail %s", buf);
            return -6;
        }

        pos += base64_len;
        pos += snprintf(content + pos, data->len - pos, "\"},");
    }

    if (g_torrent.count > 0)
    {
        content[pos - 1] = ']';
    }
    else
    {
        content[pos++] = ']';
    }

    data->type = HTTP_TYPE_HTML;
    data->len = pos;
    return 0;
}

/**
 *\brief        http回调函数,任务信息
 *\param[out]   data            HTTP的数据
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
        else if (data->arg[i].key   != NULL && 0 == strcmp(data->arg[i].key, "msk") &&
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
        xl_sdk_stop_task(task_id);
        xl_sdk_del_task(task_id);
    }

    int  len;
    char buf[20480];

    if (NULL != add)
    {
        D("add:%s", add);

        len = sizeof(buf);

        if (0 != base64_decode(add, addr_len, buf, &len))
        {
            E("base64_decode fail %s", add);
            return -1;
        }

        if (0 != xl_sdk_download(g_cfg.download_path, buf, msk, &g_torrent))
        {
            E("xl_sdk_download fail");
            return -2;
        }
    }

    int    pos;
    int    base64_len;
    char   size[16];
    char   speed[16];
    char  *content = data->content;

    pos = 1;
    content[0] = '[';

    for (unsigned int i = 0; i < g_task_count; i++)
    {
        format_data(g_task[i].size, size, sizeof(size));
        format_data(g_task[i].speed, speed, sizeof(speed));

        pos += snprintf(content + pos, data->len - pos,
                       "{\"id\":%d,\"size\":\"%s\",\"prog\":\"%.2f\",\"speed\":\"%s\",\"task\":\"",
                       g_task[i].id, size, g_task[i].prog, speed);

        len = sizeof(buf);

        if (0 != uri_encode(g_task[i].name, g_task[i].name_len, buf, &len)) // js的atob不能解码unicode
        {
            E("uri_encode fail %s", g_task[i].name);
            return -3;
        }

        base64_len = data->len - pos;

        if (0 != base64_encode(buf, len, content + pos, &base64_len)) // 文件名中可能有json需要转码的字符
        {
            E("base64_encode fail %s", buf);
            return -4;
        }

        pos += base64_len;
        pos += snprintf(content + pos, data->len - pos, "\"},");
    }

    if (g_task_count > 0)
    {
        content[pos - 1] = ']';
    }
    else
    {
        content[pos++] = ']';
    }

    data->type = HTTP_TYPE_HTML;
    data->len = pos;
    return 0;
}

/**
 *\brief        http回调函数,种子文件列表
 *\param[out]   data            HTTP的数据
 *\return       0               成功
 */
int http_proc_torrent(const p_xt_http_data data)
{
    char  buf[20480];
    char  filename[MAX_PATH];
    int   filename_len;
    int   len;
    int   pos = 1;
    int   count = 0;
    int   base64_len;
    char *content = data->content;

    content[0] = '[';
    sprintf_s(filename, MAX_PATH, "%s\\*.torrent", g_cfg.download_path);

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

        sprintf_s(buf, MAX_PATH, "%s\\%s", g_cfg.download_path, wfd.cFileName);

        if (0 != ansi_utf8(buf, strlen(buf), filename, &filename_len))
        {
            E("ansi_utf8 fail %s", wfd.cFileName);
            return -2;
        }

        D(filename);

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
            count++;

            pos += snprintf(content + pos, data->len - pos, "{\"filename\":\"");

            len = sizeof(buf);

            if (0 != uri_encode(filename, filename_len, buf, &len)) // js的atob不能解码unicode
            {
                E("uri_encode fail %s", filename);
                return -3;
            }

            base64_len = data->len - pos;

            if (0 != base64_encode(buf, len, content + pos, &base64_len)) // 文件名中可能有json需要转码的字符
            {
                E("base64_encode fail %s", buf);
                return -4;
            }

            pos += base64_len;
            pos += snprintf(content + pos, data->len - pos, "\"},");
        }

        if (!FindNextFileA(find, &wfd)) break;
    }

    FindClose(find);

    if (count > 0)
    {
        content[pos - 1] = ']';
    }
    else
    {
        content[pos++] = ']';
    }

    data->type = HTTP_TYPE_HTML;
    data->len = pos;
    return 0;
}

/**
 *\brief        HTTP回调函数,默认图标
 *\param[out]   data            HTTP的数据
 *\return       0               成功
 */
int http_proc_icon(const p_xt_http_data data)
{
    data->type = HTTP_TYPE_ICO;
    exe_ico_get_data(IDI_GREEN, data->content, &(data->len));
    return 0;
}

/**
 *\brief        HTTP回调函数,首页
 *\param[out]   data            HTTP的数据
 *\return       0               成功
 */
int http_proc_index(const p_xt_http_data data)
{
    data->type = HTTP_TYPE_HTML;
    data->len = sizeof(INDEX_PAGE) - 1;
    strcpy_s(data->content, sizeof(INDEX_PAGE), INDEX_PAGE);
    return 0;
}

/**
 *\brief        HTTP回调函数
 *\param[out]   data            HTTP的数据
 *\return       0               成功
 */
int http_proc_callback(const p_xt_http_data data)
{
    if (0 == strcmp(data->uri, "/"))
    {
        return http_proc_index(data);
    }
    else if (0 == strcmp(data->uri, "/file"))
    {
        return http_proc_file(data);
    }
    else if (0 == strcmp(data->uri, "/task"))
    {
        return http_proc_task(data);
    }
    else if (0 == strcmp(data->uri, "/torrent"))
    {
        return http_proc_torrent(data);
    }
    else if (0 == strcmp(data->uri, "/favicon.ico"))
    {
        return http_proc_icon(data);
    }

    return 404;
}

/**
 *\brief        窗体关闭处理函数
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
        return -1;
    }

    ret = log_init_ex(TITLE, g_cfg.log_level, g_cfg.log_cycle, g_cfg.log_backup, g_cfg.log_clean, 38, &g_log);

    if (ret != 0)
    {
        sprintf_s(m, sizeof(m), "init log test fail %d", ret);
        MessageBoxA(NULL, m, TITLE, MB_OK);
        return -2;
    }

    ret = http_init(g_cfg.http_ip, g_cfg.http_port, http_proc_callback, &g_http);

    if (ret != 0)
    {
        E("http init fail %d", ret);
        return -3;
    }

    ret = xl_sdk_init();

    if (0 != ret)
    {
        E("init error:%d", ret);
        return -4;
    }

    notify_menu_info menu[] = { {0, L"退出(&E)", NULL, on_menu_exit} };

    ret = notify_init(hInstance, IDI_GREEN, TITLE, SIZEOF(menu), menu);

    if (0 != ret)
    {
        return -5;
    }

    return notify_loop_msg();
}
