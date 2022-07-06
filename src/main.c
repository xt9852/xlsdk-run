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
#include "xl_sdk.h"
#include "torrent.h"
#define  PCRE2_STATIC
#define  PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

/// 程序标题
#define TITLE "DownloadSDKServerStart"

/// 主页面
#define INDEX_PAGE "<meta charset='utf-8'>\
<script>\n\
    function download(init){\n\
        filename = document.getElementById('file').value;\n\
        if (filename == '' && !init) {alert('请输入要下载的文件地址');return;}\n\
        url = '/download?file=' + btoa(filename);\n\
        file_table = document.getElementById('file-list');\n\
        if (file_table.style.display == '') {\n\
            list = '';\n\
            file_tbody = file_table.childNodes[0];\n\
            for (var i = 1; i < file_tbody.childNodes.length; i++){\n\
                list = list + (file_tbody.childNodes[i].childNodes[0].childNodes[0].checked ? '1' : '0');\n\
            }\n\
            if (/^0+$/.test(list)) {alert('请选取要下载的文件'); return;}\n\
            url = url + '&list=' + list ;\n\
            file_table.style.display = 'none';\n\
        }\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            console.log(url + ' status:' + req.status);\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            down_tbody = document.getElementById('down-list').childNodes[0];\n\
            while (down_tbody.childNodes.length > 1) { down_tbody.removeChild(down_tbody.childNodes[1]); }\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                tr = document.createElement('tr');\n\
                td = document.createElement('td');\n\
                td.innerText = item['id'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                file = decodeURIComponent(atob(item['file']));\n\
                td.id = 'file_' + i;\n\
                td.innerText = file;\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['size'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['prog'];\n\
                tr.appendChild(td);\n\
                down_tbody.appendChild(tr);\n\
                td = document.createElement('td');\n\
                td.innerText = item['speed'];\n\
                tr.appendChild(td);\n\
            }\n\
            width = document.getElementById('width').clientWidth + 37;\n\
            if (width < 400) width = 400;;\n\
            document.getElementById('file').style.width = width;\n\
        }\n\
    }\n\
    function show_torrent(edit){\n\
        console.log(edit);\n\
        filename = '';\n\
        \n\
        if (edit == true) {\n\
            filename = document.getElementById('file').value;\n\
        } else {\n\
            filename = document.getElementById('file_' + this.i).innerText;\n\
        }\n\
        \n\
        url = '/torrent-list?torrent=' + btoa(filename)\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            console.log(url + ' status:' + req.status);\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            input = document.getElementById('file');\n\
            input.value = filename;\n\
            input.size = filename.length;\n\
            file_table = document.getElementById('file-list');\n\
            file_table.style.display = '';\n\
            file_tbody = file_table.childNodes[0];\n\
            while (file_tbody.childNodes.length > 1) { file_tbody.removeChild(file_tbody.childNodes[1]); }\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                tr = document.createElement('tr');\n\
                td = document.createElement('td');\n\
                ip = document.createElement('input');\n\
                ip.type = 'checkbox';\n\
                td.appendChild(ip);\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = decodeURIComponent(atob(item['file']));\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['size'];\n\
                tr.appendChild(td);\n\
                file_tbody.appendChild(tr);\n\
            }\n\
        }\n\
    }\n\
    function on_check(checkbox){\n\
        file_tbody = document.getElementById('file-list').childNodes[0];\n\
        for (var i = 1; i < file_tbody.childNodes.length; i++){\n\
            file_tbody.childNodes[i].childNodes[0].childNodes[0].checked = checkbox.checked;\n\
        }\n\
    }\n\
</script>\n\
<body onload='download(true)'>\
<input id='file'/>\
<button onclick='download(false)'>download</button>\
<button onclick='show_torrent(true)'>open</button>\
<br><br>\
<table id='file-list' border='1' style='border-collapse:collapse;font-family:宋体;display:none'>\
<tr><th><input type='checkbox' name='check' onclick='on_check(this)' /></th><th>文件</th><th>大小</th></tr></table>\
<table id='down-list' border='1' style='border-collapse:collapse;font-family:宋体;'>\
<tr><th>任务</th><th id='width'>文件</th><th>大小</th><th>进度</th><th>速度</th></tr>\
</table>\
</body>"

config              g_cfg                   = {0};  ///< 配置数据

xt_log              g_log                   = {0};  ///< 日志数据

xt_http             g_http                  = {0};  ///< HTTP服务

extern xl_task      g_task[TASK_SIZE];              ///< 当前正在下载的任务信息

extern unsigned int g_task_count;                   ///< 当前正在下载的任务数量

/**
 *\brief        http回调函数,下载接口
 *\param[in]    arg             URI的参数
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_download(const p_xt_http_arg arg, p_xt_http_content content)
{
    const char *file = NULL;    // 可以为空
    const char *list = NULL;    // 可以为空

    if (arg->count >= 1 &&
        NULL != arg->item[0].name &&
        0 == strcmp(arg->item[0].name, "file") &&
        0 != strcmp(arg->item[0].data, ""))
    {
        file = arg->item[0].data;
    }

    if (arg->count >= 2 &&
        NULL != arg->item[1].name &&
        0 == strcmp(arg->item[1].name, "list") &&
        0 != strcmp(arg->item[1].data, ""))
    {
        list = arg->item[1].data;
    }

    int len = 1024;
    char filename[1024];
    base64_decode(file, arg->item[0].data_len, filename, &len);

    if (NULL != file && 0 != xl_sdk_download(g_cfg.download_path, filename, list))
    {
        E("xl_sdk_download fail");
        return -1;
    }

    int    pos;
    int    encode_len;
    int    base64_len;
    char   size[16];
    char   speed[16];
    char   encode[20480];
    char   base64[20480];
    char  *data = content->data;

    pos = 1;
    data[0] = '[';

    for (unsigned int i = 0; i < g_task_count; i++)
    {
        encode_len = sizeof(encode);
        base64_len = sizeof(base64);

        if (0 != uri_encode(g_task[i].name, g_task[i].name_len, encode, &encode_len)) // js的atob不能解码unicode
        {
            return -2;
        }

        if (0 != base64_encode(encode, encode_len, base64, &base64_len)) // 文件名中可能有json需要转码的字符
        {
            return -3;
        }

        format_data(g_task[i].size, size, sizeof(size));
        format_data(g_task[i].speed, speed, sizeof(speed));

        D("id:%u, name:%s uri:%s base64:%s size:%s speed:%s prog:%f",
          g_task[i].id, g_task[i].name, encode, base64, size, speed, g_task[i].prog);

        len = snprintf(data + pos, content->len - pos,
                       "{\"id\":%d,\"file\":\"%s\",\"size\":\"%s\",\"prog\":\"%.2f\",\"speed\":\"%s\"},",
                       g_task[i].id, base64, size, g_task[i].prog, speed);
        pos += len;
    }

    if (g_task_count > 0)
    {
        data[pos - 1] = ']';
    }
    else
    {
        data[pos++] = ']';
    }

    content->type = HTTP_TYPE_HTML;
    content->len = pos;
    return 0;
}

/**
 *\brief        http回调函数,得到种子中文件信息
 *\param[in]    arg             URI的参数
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_torrent(const p_xt_http_arg arg, p_xt_http_content content)
{
    if (arg->count <= 0 || NULL == arg->item[0].name || 0 != strcmp(arg->item[0].name, "torrent"))
    {
        D("file:null or \"\"");
        return -1;
    }

    const char *file = arg->item[0].data;

    if (NULL == file || 0 == strcmp(file, ""))
    {
        D("file:null or \"\"");
        return -2;
    }

    int len = 1024;
    char filename[1024];
    bt_torrent torrent = {0};

    base64_decode(file, arg->item[0].data_len, filename, &len);

    if (0 != get_torrent_info(filename, &torrent))
    {
        E("get torrent:%s info error", file);
        return -3;
    }

    int   pos;
    int   encode_len;
    int   base64_len;
    char  size[16];
    char  encode[20480];
    char  base64[20480];
    char *data = content->data;

    pos = 1;
    data[0] = '[';

    for (int i = 0; i < torrent.count; i++)
    {
        encode_len = sizeof(encode);
        base64_len = sizeof(base64);

        uri_encode(torrent.file[i].name, torrent.file[i].name_len, encode, &encode_len); // js的atob不能解码unicode
        base64_encode(encode, encode_len, base64, &base64_len); // 文件名中可能有json需要转码的字符
        format_data(torrent.file[i].size, size, sizeof(size));

        D("i:%d file:%s", i, torrent.file[i].name);

        len = snprintf(data + pos, content->len - pos, "{\"file\":\"%s\",\"size\":\"%s\"},", base64, size);
        pos += len;
    }

    D("5");

    if (torrent.count > 0)
    {
        data[pos - 1] = ']';
    }
    else
    {
        data[pos++] = ']';
    }

    content->type = HTTP_TYPE_HTML;
    content->len = pos;
    return 0;
}

/**
 *\brief        http回调函数,文件
 *\param[in]    uri             URI地址
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_other(const char *uri, p_xt_http_content content)
{
    char file[512];
    sprintf_s(file, sizeof(file), "%s%s", g_cfg.http_path, uri);

    FILE *fp;
    fopen_s(&fp, file, "rb");

    if (NULL == fp)
    {
        E("open %s fail", file);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    content->len = ftell(fp);

    fseek(fp, 0, SEEK_SET);
    fread(content->data, 1, content->len, fp);
    fclose(fp);

    D("%s size:%d", file, content->len);

    char *ext = strrchr(file, '.');

    if (NULL == ext)
    {
        content->type = HTTP_TYPE_HTML;
        return 0;
    }

    if (0 == strcmp(ext, ".xml"))
    {
        content->type = HTTP_TYPE_XML;
    }
    else if (0 == strcmp(ext, ".ico"))
    {
        content->type = HTTP_TYPE_ICO;
    }
    else if (0 == strcmp(ext, ".png"))
    {
        content->type = HTTP_TYPE_PNG;
    }
    else if (0 == strcmp(ext, ".jpg"))
    {
        content->type = HTTP_TYPE_JPG;
    }
    else if (0 == strcmp(ext, ".jpeg"))
    {
        content->type = HTTP_TYPE_JPEG;
    }
    else
    {
        content->type = HTTP_TYPE_HTML;
    }

    return 0;
}

/**
 *\brief        HTTP回调函数,/favicon.ico
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_icon(p_xt_http_content content)
{
    content->type = HTTP_TYPE_ICO;
    exe_ico_get_data(IDI_GREEN, content->data, &(content->len));
    return 0;
}

/**
 *\brief        HTTP回调函数,主页
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_index(p_xt_http_content content)
{
    content->type = HTTP_TYPE_HTML;
    content->len = sizeof(INDEX_PAGE) - 1;
    strcpy_s(content->data, sizeof(INDEX_PAGE), INDEX_PAGE);
    return 0;
}

/**
 *\brief        HTTP回调函数
 *\param[in]    uri             URI地址
 *\param[in]    arg             URI的参数,参数使用的是conten.data指向的内存
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_callback(const char *uri, const p_xt_http_arg arg, p_xt_http_content content)
{
    D("uri:%s", uri);

    if (0 == strcmp(uri, "/"))
    {
        return http_proc_index(content);
    }
    else if (0 == strcmp(uri, "/download"))
    {
        return http_proc_download(arg, content);
    }
    else if (0 == strcmp(uri, "/torrent-list"))
    {
        return http_proc_torrent(arg, content);
    }
    else if (0 == strcmp(uri, "/favicon.ico"))
    {
        return http_proc_icon(content);
    }
    else
    {
        return http_proc_other(uri, content);
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
        xl_sdk_stop_download_file(g_task[i].id);
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

    ret = http_init(g_cfg.http_port, http_proc_callback, &g_http);

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
