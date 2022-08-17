/**
 *\copyright    XT Tech. Co., Ltd.
 *\file         main.c
 *\author       xt
 *\version      1.0.0
 *\date         2022-02-08
 *\brief        主模块,UTF-8(No BOM)
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
#define INDEX_PAGE "<meta charset='utf-8'>\
<script>\n\
    function task(arg){\n\
        addr = document.getElementById('addr');\n\
        url = '/task?' + arg;\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            down_tbody = document.getElementById('down').childNodes[0];\n\
            while (down_tbody.childNodes.length > 1) { down_tbody.removeChild(down_tbody.childNodes[1]); }\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                task_name = decodeURIComponent(atob(item['task']));\n\
                tr = document.createElement('tr');\n\
                td = document.createElement('td');\n\
                td.innerText = item['id'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = task_name;\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['size'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['prog'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['speed'];\n\
                tr.appendChild(td);\n\
                bt = document.createElement('button');\n\
                bt.onclick = del;\n\
                bt.innerText = 'del';\n\
                bt.task_id = item['id'];\n\
                tr.appendChild(bt);\n\
                if (/\\.torrent$/.test(task_name)) {\n\
                    bt = document.createElement('button');\n\
                    bt.onclick = open;\n\
                    bt.innerText = 'open';\n\
                    bt.task_name = task_name;\n\
                    tr.appendChild(bt);\n\
                }\n\
                down_tbody.appendChild(tr);\n\
            }\n\
            task_th = down_tbody.childNodes[0].childNodes[1];\n\
            task_th.innerText = '任务(' + rp.length + ')';\n\
            width = task_th.clientWidth + 21 + 2;\n\
            addr.style.width = (width < 600) ? 600 : width;\n\
        }\n\
    }\n\
    function add(){\n\
        addr = document.getElementById('addr');\n\
        arg = 'add=' + btoa(addr.value);\n\
        torr_table = document.getElementById('torr');\n\
        if (torr_table.style.display == '') {\n\
            mask = '';\n\
            torr_tbody = torr_table.childNodes[0];\n\
            for (var i = 1; i < torr_tbody.childNodes.length; i++){\n\
                mask = mask + (torr_tbody.childNodes[i].childNodes[0].checked ? '1' : '0');\n\
            }\n\
            if (/^0+$/.test(mask)) {alert('请选取要下载的文件'); return;}\n\
            arg = arg + '&msk=' + mask ;\n\
            torr_table.style.display = 'none';\n\
        }\n\
        task(arg);\n\
    }\n\
    function del(){\n\
        arg = 'del=' + this.task_id;\n\
        task(arg);\n\
    }\n\
    function open(){\n\
        document.getElementById('addr').value = this.task_name;\n\
        addr = document.getElementById('addr');\n\
        url = '/file?torrent=' + btoa(addr.value)\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            torr_table = document.getElementById('torr');\n\
            torr_table.style.display = '';\n\
            torr_tbody = torr_table.childNodes[0];\n\
            while (torr_tbody.childNodes.length > 1) { torr_tbody.removeChild(torr_tbody.childNodes[1]); }\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                tr = document.createElement('tr');\n\
                ip = document.createElement('input');\n\
                ip.type = 'checkbox';\n\
                tr.appendChild(ip);\n\
                td = document.createElement('td');\n\
                td.innerText = decodeURIComponent(atob(item['file']));\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['size'];\n\
                tr.appendChild(td);\n\
                torr_tbody.appendChild(tr);\n\
            }\n\
        }\n\
    }\n\
    function torrent(){\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', '/torrent');\n\
        req.send(null);\n\
        req.onload = function(){\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            down_tbody = document.getElementById('down').childNodes[0];\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                task_name = decodeURIComponent(atob(item['filename']));\n\
                tr = document.createElement('tr');\n\
                td = document.createElement('td');\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = task_name;\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                tr.appendChild(td);\n\
                if (/\\.torrent$/.test(task_name)) {\n\
                    bt = document.createElement('button');\n\
                    bt.onclick = open;\n\
                    bt.innerText = 'open';\n\
                    bt.task_name = task_name;\n\
                    tr.appendChild(bt);\n\
                }\n\
                down_tbody.appendChild(tr);\n\
            }\n\
            task_th = down_tbody.childNodes[0].childNodes[1];\n\
            task_th.innerText = '任务(' + rp.length + ')';\n\
            width = task_th.clientWidth + 21 + 2;\n\
            addr.style.width = (width < 600) ? 600 : width;\n\
        }\n\
    }\n\
    function on_check(chk){\n\
        torr_tbody = document.getElementById('torr').childNodes[0];\n\
        for (var i = 1; i < torr_tbody.childNodes.length; i++){\n\
            torr_tbody.childNodes[i].childNodes[0].checked = chk.checked;\n\
        }\n\
    }\n\
</script>\n\
<body onload='task()'>\
<table id='down' border='1' style='border-collapse:collapse;font-family:宋体;'>\
<tr><th>ID</th><th>任务</th><th>大小</th><th>进度</th><th>速度</th><th>操作</th></tr>\
<table id='torr' border='1' style='border-collapse:collapse;font-family:宋体;display:none'>\
<tr><th><input type='checkbox' onclick='on_check(this)' /></th><th>文件</th><th>大小</th></tr></table>\
<input id='addr' style='width:600'/>\
<button onclick='add()'>download</button>\
<button onclick='torrent()'>torrent</button>\
</table>\
</body>"

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

    ret = http_init(g_cfg.http_ip, g_cfg.http_port, g_cfg.http_ipv4, http_proc_callback, &g_http);

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
