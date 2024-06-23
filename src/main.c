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
#include "xl_sdk.h"
#include "torrent.h"

/// 首页页面
#define INDEX_PAGE "<meta charset='utf-8'>\n\
<script>\n\
    function http(url, data, callback){\n\
        console.log(url + data);\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url + data);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            if (req.readyState != 4 || req.status != 200) alert('请求失败');\n\
            else callback(JSON.parse(req.responseText), data);\n\
        }\n\
    }\n\
    function tbody(count, data){\n\
        document.getElementsByTagName('input')[0].value = (data == 'task' || data == 'torrent') ? '' : data;\n\
        for (tbd = document.getElementsByTagName('tbody')[0]; tbd.childNodes.length > 1;){tbd.removeChild(tbd.childNodes[1]);}\n\
        title = tbd.childNodes[0].childNodes;\n\
        title[8].innerText = '任务(' + count + ')';\n\
        if (data == 'task') {\n\
            title[2].style.display = '';\n\
            title[4].style.display = '';\n\
            title[6].style.display = '';\n\
        } else if (data == 'torrent') {\n\
            title[2].style.display = 'none';\n\
            title[4].style.display = 'none';\n\
            title[6].style.display = 'none';\n\
        } else {\n\
            title[2].style.display = '';\n\
        }\n\
        return tbd;\n\
    }\n\
    function task_list(rsp, data){\n\
        tb = tbody(rsp.length, 'task');\n\
        for (i in rsp) {\n\
            tr = document.createElement('tr'); tb.appendChild(tr);\n\
            t1 = document.createElement('td'); tr.appendChild(t1);\n\
            t2 = document.createElement('td'); tr.appendChild(t2);\n\
            t3 = document.createElement('td'); tr.appendChild(t3);\n\
            t4 = document.createElement('td'); tr.appendChild(t4);\n\
            t5 = document.createElement('td'); tr.appendChild(t5);\n\
            t2.align = 'right';\n\
            t3.align = 'right';\n\
            t4.align = 'right';\n\
            t1.outerHTML = '<td><button onclick=\"http(\\'/task?del=\\',\\''+rsp[i].id+'\\',task_list)\">删除</button></td>';\n\
            t2.innerText = rsp[i].size;\n\
            t3.innerText = rsp[i].prog;\n\
            t4.innerText = rsp[i].speed;\n\
            t5.innerText = decodeURIComponent(atob(rsp[i].task));\n\
        }\n\
    }\n\
    function torrent_list(rsp, data){\n\
        tb = tbody(rsp.length, 'torrent');\n\
        for (i in rsp) {\n\
            task_name = decodeURIComponent(atob(rsp[i].filename));\n\
            tr = document.createElement('tr'); tb.appendChild(tr);\n\
            t1 = document.createElement('td'); tr.appendChild(t1);\n\
            t2 = document.createElement('td'); tr.appendChild(t2);\n\
            t1.outerHTML='<td><button onclick=\"http(\\'/file?torrent=\\',\\''+btoa(task_name)+'\\',torrent_file)\">打开</button></td>';\n\
            t2.innerText = task_name;\n\
        }\n\
    }\n\
    function torrent_file(rsp, data){\n\
        tb = tbody(rsp.length, atob(data));\n\
        for (i in rsp) {\n\
            tr = document.createElement('tr'); tb.appendChild(tr);\n\
            t1 = document.createElement('td'); tr.appendChild(t1);\n\
            t2 = document.createElement('td'); tr.appendChild(t2);\n\
            t3 = document.createElement('td'); tr.appendChild(t3);\n\
            t1.outerHTML = '<td><input type=\"checkbox\"></td>';\n\
            t2.innerText = rsp[i].size;\n\
            t3.innerText = decodeURIComponent(atob(rsp[i].file));\n\
        }\n\
    }\n\
    function download(){\n\
        data = btoa(document.getElementsByTagName('input')[0].value);\n\
        tr = document.getElementsByTagName('tr')[1];\n\
        if (tr && tr.childNodes[0].childNodes[0].type == 'checkbox') {\n\
            for (mask = '&msk='; tr; tr = tr.nextSibling) {mask += tr.childNodes[0].childNodes[0].checked * 1;}\n\
            if (/1+/.test(mask)) {data += mask;} else {data = '';}\n\
        }\n\
        http('/task?add=', data, task_list);\n\
    }\n\
</script>\n\
<div style='display:flex;margin:0 0 10 2'>\n\
    <button onclick='download()'>下载</button>\n\
    <input style='flex:1;margin-left:1'/>\n\
</div>\n\
<table border='1' style='width:100%;border-collapse:collapse;font-family:宋体'>\n\
    <td width='43px'><button onclick=\"http(\'/torrent\',\'\',torrent_list)\">种子</button></td>\n\
    <th width='60px'>大小</th>\n\
    <th width='60px'>进度</th>\n\
    <th width='60px'>速度</th>\n\
    <th>任务</th>\n\
</table>"

char                g_path[MAX_PATH]        = "";   ///< 文件路径

config              g_cfg                   = {0};  ///< 配置数据

xt_log              g_log                   = {0};  ///< 日志文件

xt_http             g_http                  = {0};  ///< HTTP服务

bt_torrent          g_torrent               = {0};  ///< 种子数据

extern xl_task      g_task[TASK_SIZE];              ///< 当前正在下载的任务信息

extern unsigned int g_task_count;                   ///< 当前正在下载的任务数量

/**
 *\brief                        http回调函数,得到种子中文件信息
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
 *\brief                        http回调函数,任务信息
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
                char *temp = &(g_task[i].name[5]);
                char *name = strtok_s(temp, "|", &temp);

                for (i = 0; NULL != name; i++)
                {
                    src_len = MAX_PATH - 1;
                    len = snprintf(path, sizeof(path), "%s\\%s", g_cfg.path_download, name);

                    if (0 != utf8_unicode(path, len, src, &src_len))
                    {
                        E("utf8_unicode error %s", path);
                        break;
                    }

                    dst_len = MAX_PATH - 1;
                    len = snprintf(path, sizeof(path), "%s\\%s", g_cfg.path_move, name);

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

        if (0 != xl_sdk_download(g_cfg.path_download, buf, msk, &g_torrent))
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
                       "{\"id\":%d,\"size\":\"%s\",\"prog\":\"%.2f%%\",\"speed\":\"%s\",\"task\":\"",
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
 *\brief                        http回调函数,种子文件列表
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
    sprintf_s(filename, MAX_PATH, "%s\\*.torrent", g_cfg.path_download);

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

        sprintf_s(buf, MAX_PATH, "%s\\%s", g_cfg.path_download, wfd.cFileName);

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
 *\brief                        HTTP回调函数,默认图标
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
 *\brief                        HTTP回调函数,首页
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
 *\brief                        HTTP回调函数
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
    char message[MAX_PATH];

    GetModuleFileNameA(hInstance, g_log.path, MAX_PATH);

    char *title = strrchr(g_log.path, '\\');
    *title++ = '\0';

    char *end = strrchr(title, '.');
    *end = '\0';

    char filename[MAX_PATH];

    snprintf(filename, MAX_PATH, "%s.json", title);

    g_cfg.log = &g_log;

    int ret = config_init(filename, &g_cfg);

    if (ret != 0)
    {
        sprintf_s(message, sizeof(message), "init config fail %d", ret);
        MessageBoxA(NULL, message, title, MB_OK);
        return -1;
    }

    // 22是当前代码的根目录长度,日志中只保留代码的相对路径
    ret = log_init(g_cfg.log);

    if (ret != 0)
    {
        sprintf_s(message, sizeof(message), "init log fail %d", ret);
        MessageBoxA(NULL, message, title, MB_OK);
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

    ret = notify_init(hInstance, IDI_GREEN, "DownloadSDKServerRun", SIZEOF(menu), menu);

    if (0 != ret)
    {
        return -5;
    }

    return notify_loop_msg();
}
