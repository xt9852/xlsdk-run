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
#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

/// 程序标题
#define TITLE "DownloadSDKServerStart"

/// 主页面
#define INDEX_PAGE "<meta charset='utf-8'>\
<script>\n\
    function download(init){\n\
        filename = document.getElementById('file').value;\n\
        if (filename == '' && !init) {alert('请输入要下载的文件地址');return;}\n\
        url = '/download?file=' + filename;\n\
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
                td.innerText = item['speed'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['prog'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['time'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                if (/\\.torrent$/.test(file)) {\n\
                    btn = document.createElement('button');\n\
                    btn.innerText = 'open';\n\
                    btn.i = i;\n\
                    btn.id = 'btn_' + i;\n\
                    btn.style='display:block;';\n\
                    btn.onclick = show_in_torrent_files;\n\
                    td.appendChild(btn);\n\
                }\n\
                tr.appendChild(td);\n\
                down_tbody.appendChild(tr);\n\
            }\n\
        }\n\
    }\n\
    function show_in_torrent_files(){\n\
        filename = document.getElementById('file_' + this.i).innerText;\n\
        url = '/torrent-list?torrent=' + filename\n\
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
<table id='file-list' border='1' style='border-collapse:collapse;display:none'>\
<tr><th><input type='checkbox' name='check' onclick='on_check(this)' /></th><th>大小</th><th>文件</th></tr></table>\
<table id='down-list' border='1' style='border-collapse:collapse;'>\
<tr><th>任务</th><th>文件</th><th>大小</th><th>速度</th><th>进度</th><th>用时</th><th>操作</th></tr>\
</table>\
</body>"
config              g_cfg                   = {0};  ///< 配置数据

xt_list             g_monitor_event_list    = {0};  ///< 监控事件队列

xt_memory_pool      g_memory_pool           = {0};  ///< 内存池

xt_log              g_log                   = {0};  ///< 日志数据
xt_log              g_test                  = {0};  ///< 多日志测试

xt_http             g_http                  = {0};  ///< HTTP服务
xt_thread_pool      g_thread_pool           = {0};  ///< 线程池
xt_timer_set        g_timer_set             = {0};  ///< 定时器

bt_torrent          g_torrent               = {0};  ///< 种子文件信息
xl_task             g_task[128]             = {0};  ///< 当前正在下载的任务信息
int                 g_task_count            = 0;    ///< 当前正在下载的任务数量


/**
 *\brief        下载文件
 *\param[in]    filename        文件地址
 *\param[in]    list            下载BT文件时选中的要下载的文件,如:"10100",1-选中,0-末选
 *\return       0               成功
 */
int xl_download(const char *filename, const char *list)
{
    int ret;
    int task_id;
    int task_type;
    char task_name[MAX_PATH];

    D("task count:%d", g_task_count);

    for (int i = 0; i < g_task_count; i++)
    {
        if (0 == strcmp(filename, g_task[i].filename))  // 已经下载
        {
            D("have %s", filename);
            return 0;
        }
    }

    if (0 == strcmp(filename + strlen(filename) - 8, ".torrent"))   // BT下载
    {
        if (NULL == list)
        {
            return -1;
        }

		task_type = TASK_BT;

        ret = xl_sdk_create_bt_task(filename, g_cfg.download_path, list, &task_id, task_name, sizeof(task_name));
    }
    else if (0 == strncmp(filename, "magnet:?", 8))                 // 磁力下载
    {
        task_type = TASK_MAGNET;

        ret = xl_sdk_create_magnet_task(filename, g_cfg.download_path, &task_id, task_name, sizeof(task_name));
    }
    else                                                            // 普通下载
    {
        task_type = TASK_URL;

        ret = xl_sdk_create_url_task(filename, g_cfg.download_path, &task_id, task_name, sizeof(task_name));
    }

    if (0 != ret)
    {
        E("create task %s Eor:%d", filename, ret);
        return -2;
    }

    ret = xl_sdk_start_download_file(task_id, task_type);

    if (0 != ret)
    {
        E("download start %s Eor:%d", filename, ret);
        return -3;
    }

    ret = xl_sdk_get_task_info(task_id, &g_task[g_task_count].size, &g_task[g_task_count].down, &g_task[g_task_count].time);

    if (0 != ret)
    {
        E("get task info Eor:%d", ret);
        return -4;
    }

    /* ------------------test--------------------- */
    if (0 == strcmp(filename, "http://127.0.0.1/"))
    {
        strcpy_s(g_task[g_task_count].filename, sizeof(g_task[g_task_count].filename), "C:\\Program Files\\7-Zip\\1\\DB2FE78374A1A18C1A5EFCC5E961901A1BCACFD2.torrent");
        g_task[g_task_count].type      = TASK_MAGNET;
        g_task[g_task_count].size      = 1024*1024*1024 + 235*1024*1024;
        g_task[g_task_count].down      = 1024*1024*1024;
        g_task[g_task_count].time      = 123;
        g_task[g_task_count].last_down = 0;
        g_task[g_task_count].last_time = 0;
    }
    else if (0 == strcmp(filename, "http://127.0.0.2/"))
    {
        strcpy_s(g_task[g_task_count].filename, sizeof(g_task[g_task_count].filename), "D:\\5.downloads\\bt\\7097B42EEBC037482B69056276858599ED9605B5.torrent");
        g_task[g_task_count].type      = TASK_MAGNET;
        g_task[g_task_count].size      = 4*1024*1024*1024ll + 567*1024*1024;
        g_task[g_task_count].down      = 1024*1024*1024;
        g_task[g_task_count].time      = 456;
        g_task[g_task_count].last_down = 0;
        g_task[g_task_count].last_time = 0;
    }
    else
    {
        strcpy_s(g_task[g_task_count].filename, sizeof(g_task[g_task_count].filename), task_name);
        g_task[g_task_count].id        = task_id;
        g_task[g_task_count].type      = task_type;
        g_task[g_task_count].last_down = 0;
        g_task[g_task_count].last_time = 0;
    }

    g_task_count++;

    return 0;
}

/**
 *\brief        http回调函数,下载接口
 *\param[in]    arg             URI的参数
 *\param[out]   content         返回内容
 *\return       0               成功
 */
int http_proc_download(const p_xt_http_arg arg, p_xt_http_content content)
{
    const char *file = NULL;
    const char *list = NULL;

    if (arg->count >= 1 && NULL != arg->name[0] && 0 == strcmp(arg->name[0], "file") && 0 != strcmp(arg->data[0], ""))
    {
        file = arg->data[0];
    }

    if (arg->count >= 2 && NULL != arg->name[1] && 0 == strcmp(arg->name[1], "list") && 0 != strcmp(arg->data[1], ""))
    {
        list = arg->data[1];
    }

    if (NULL != file && 0 != xl_download(file, list))
    {
        E("xl_download fail");
        return -1;
    }

    int    pos = 1;
    int    len;
    int    encode_len;
    int    base64_len;
    int    time;
    char   size[16];
    char   speed[16];
    char   encode[MAX_PATH];
    char   base64[MAX_PATH];
    char  *data = content->data;
    double prog;

    data[0] = '[';

    for (int i = 0; i < g_task_count; i++)
    {
        encode_len = sizeof(encode);
        base64_len = sizeof(base64);

        uri_encode(g_task[i].filename, strlen(g_task[i].filename), encode, &encode_len); // js的atob不能解码unicode
        base64_encode(encode, encode_len, base64, &base64_len); // 文件名中可能有json需要转码的字符
        format_data(g_task[i].size, size, sizeof(size));

        time = g_task[i].time - g_task[i].last_time;
        if (0 == time) time = 1;
        format_data((g_task[i].down - g_task[i].last_down) / time, speed, sizeof(speed));

        prog = (0 == g_task[i].size) ? 0 : g_task[i].down * 100.0 / g_task[i].size;

        len = snprintf(data + pos, content->len - pos,
                       "{\"id\":%d,\"file\":\"%s\",\"size\":\"%s\",\"speed\":\"%s\",\"prog\":\"%.2f\",\"time\":%d},",
                       g_task[i].id, base64, size, speed, prog, g_task[i].time);
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
    if (arg->count <= 0 || NULL == arg->name[0] || 0 != strcmp(arg->name[0], "torrent"))
    {
        return -1;
    }

    const char *file = arg->data[0];

    if (NULL == file || 0 == strcmp(file, ""))
    {
        D("file:null or \"\"");
        return -2;
    }

    if (0 != get_torrent_info(file, &g_torrent))
    {
        E("get torrent:%s info Eor", file);
        return -3;
    }

    int   pos = 1;
    int   len;
    int   encode_len;
    int   base64_len;
    char  size[16];
    char  encode[MAX_PATH];
    char  base64[MAX_PATH];
    char *data = content->data;

    data[0] = '[';

    for (int i = 0; i < g_torrent.count; i++)
    {
        encode_len = sizeof(encode);
        base64_len = sizeof(base64);

        uri_encode(g_torrent.file[i].name, strlen(g_torrent.file[i].name), encode, &encode_len); // js的atob不能解码unicode
        base64_encode(encode, encode_len, base64, &base64_len); // 文件名中可能有json需要转码的字符
        format_data(g_torrent.file[i].size, size, sizeof(size));

        D("i:%d file:%s uri_encode:%s base64(uri_encode):%s", i, g_torrent.file[i].name, encode, base64);

        len = snprintf(data + pos, content->len - pos, "{\"file\":\"%s\",\"size\":\"%s\"},", base64, size);
        pos += len;
    }

    if (g_torrent.count > 0)
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
 *\brief    定时器任务回调
 *\param    [in]  param         自定义参数
 *\return                       无
 */
void timer_callback(void *param)
{
    D("param:%s", (char*)param);
}

/**
 *\brief        初始化
 *\return       0               成功
 */
int init()
{
    char m[MAX_PATH];

    int ret = config_init(TITLE".json", &g_cfg);

    if (ret != 0)
    {
        sprintf_s(m, sizeof(m), "init config fail %d", ret);
        MessageBoxA(NULL, m, TITLE, MB_OK);
        return -10;
    }

    ret = log_init_ex(TITLE".test", g_cfg.log_level, g_cfg.log_cycle, g_cfg.log_backup, g_cfg.log_clean, 38, &g_test);

    if (ret != 0)
    {
        sprintf_s(m, sizeof(m), "init log test fail %d", ret);
        MessageBoxA(NULL, m, TITLE, MB_OK);
        return -20;
    }

    DD(&g_test, "g_test init ok");

    ret = log_init_ex(g_cfg.log_filename, g_cfg.log_level, g_cfg.log_cycle, g_cfg.log_backup, g_cfg.log_clean, 38, &g_log);

    if (ret != 0)
    {
        E("init log fail %d", ret);
        return -21;
    }

    D("g_log init ok");

    D("--------------------------------------------------------------------");

    xt_md5 md5;
    char   md5_out[128];
    char  *md5_in = "1234567890";

    ret = md5_get(md5_in, strlen(md5_in), &md5);

    if (ret != 0)
    {
        E("get md5 fail %d", ret);
        return -30;
    }

    D("str:%s md5.A:%x B:%x C:%x D:%x", md5_in, md5.A, md5.B, md5.C, md5.D);

    ret = md5_get_str(md5_in, strlen(md5_in), md5_out);

    if (ret != 0)
    {
        E("get md5 str fail %d", ret);
        return -31;
    }

    D("str:%s md5:%s", md5_in, md5_out);

    md5_in = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890";

    ret = md5_get_str(md5_in, strlen(md5_in), md5_out);

    if (ret != 0)
    {
        E("get md5 str fail %d", ret);
        return -32;
    }

    D("str:%s md5:%s", md5_in, md5_out);

    D("--------------------------------------------------------------------");

    int len;
    char base64[64];
    char output[64];
    char data[][10] = { "1", "12", "123", "1234" };

    for (int i = 0; i < 4; i++)
    {
        len = sizeof(base64);

        ret = base64_encode(data[i], strlen(data[i]), base64, &len);

        if (ret != 0)
        {
            E("get base64 str fail %d", ret);
            return -40;
        }

        D("data:%s base64:%s len:%d", data[i], base64, len);

        ret = base64_decode(base64, len, output, &len);

        if (ret != 0)
        {
            E("from base64 get data fail %d", ret);
            return -41;
        }

        D("base64:%s data:%s len:%d", base64, output, len);
    }

    D("--------------------------------------------------------------------");

    ret = pinyin_init_res("PINYIN", IDR_PINYIN);

    if (ret != 0)
    {
        E("init pinyin fail %d", ret);
        return -50;
    }

    D("--------------------------------------------------------------------");

    xt_memory_pool mem_pool;

    ret = memory_pool_init(&mem_pool, 1024, 100);

    if (ret != 0)
    {
        E("init memory pool fail %d", ret);
        return -60;
    }

    void *mem = NULL;

    for (int i = 0; i < 2000; i++)
    {
        ret = memory_pool_get(&mem_pool, &mem);

        if (ret != 0)
        {
            E("memory pool get fail %d", ret);
            return -61;
        }

        D("memory_pool_get ret:%d count:%d list-size:%d count:%d head:%d tail:%d", ret, mem_pool.count,
        mem_pool.free.size, mem_pool.free.count, mem_pool.free.head, mem_pool.free.tail);
    }

    ret = memory_pool_put(&mem_pool, mem);

    if (ret != 0)
    {
        E("memory pool put fail %d", ret);
        return -62;
    }

    D("memory_pool_put ret:%d memory-pool-count:%d list-size:%d count:%d head:%d tail:%d", ret, mem_pool.count,
        mem_pool.free.size, mem_pool.free.count, mem_pool.free.head, mem_pool.free.tail);

    ret = memory_pool_uninit(&mem_pool);

    if (ret != 0)
    {
        E("memory pool uninit fail %d", ret);
        return -63;
    }

    D("--------------------------------------------------------------------");

    ret = thread_pool_init(&g_thread_pool, 10);

    if (ret != 0)
    {
        E("thread pool init fail %d", ret);
        return -70;
    }

    D("--------------------------------------------------------------------");

    ret = timer_init(&g_timer_set);

    if (ret != 0)
    {
        E("timer init fail %d", ret);
        return -80;
    }

    ret = timer_add_cycle(&g_timer_set, "timer_0",  5, &g_thread_pool, timer_callback, "timer_0_param");

    if (ret != 0)
    {
        E("add cycle timer fail %d", ret);
        return -81;
    }

    ret = timer_add_cycle(&g_timer_set, "timer_1", 10, &g_thread_pool, timer_callback, "timer_1_param");

    if (ret != 0)
    {
        E("add cycle timer fail %d", ret);
        return -82;
    }

    ret = timer_add_cron(&g_timer_set, "timer_2", TIMER_CRON_MINUTE, 0, 0, 0, 0, 0, 0, 0, &g_thread_pool, timer_callback, "timer_2_param");

    if (ret != 0)
    {
        E("add cron timer fail %d", ret);
        return -83;
    }

    D("--------------------------------------------------------------------");

    ret = http_init(g_cfg.http_port, http_proc_callback, &g_http);

    if (ret != 0)
    {
        E("http init fail %d", ret);
        return -90;
    }

    D("--------------------------------------------------------------------");

    ret = xl_sdk_init();

    if (0 != ret)
    {
        E("init Eor:%d", ret);
        return -100;
    }

    D("--------------------------------------------------------------------");

    ret = get_torrent_info("D:\\5.downloads\\bt\\7097B42EEBC037482B69056276858599ED9605B5.torrent", &g_torrent);

    if (0 != ret)
    {
        E("init Eor:%d", ret);
        return -110;
    }

    D("--------------------------------------------------------------------");

    int error;
    PCRE2_SIZE offset;

    PCRE2_SPTR reg = "[0-9]{5}[a-z]+[0-9]{5}";

    pcre2_code *pcre_data = pcre2_compile(reg, PCRE2_ZERO_TERMINATED, 0, &error, &offset, NULL);

    if (pcre_data == NULL)
    {
        PCRE2_UCHAR info[256];
        pcre2_get_error_message(error, info, sizeof(info));
        E("pcre2_compile:fail reg:%s offste:%d error:%d info:%s", reg, offset, error, info);
        return 0;
    }

    D("pcre2_compile:ok reg:%s", reg);

    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(pcre_data, NULL);

    if (NULL == match_data)
    {
        PCRE2_UCHAR info[256];
        pcre2_get_error_message(error, info, sizeof(info));
        E("pcre2_match_data_create_from_pattern:fail reg:%s", reg);
        return 0;
    }

    D("pcre2_match_data_create_from_pattern:ok reg:%s", reg);

    PCRE2_SPTR txt = "abcde12345abcde12345-=";

    ret = pcre2_match(pcre_data, txt, strlen(txt), 0, 0, match_data, NULL); // <0发生错误，==0没有匹配上，>0返回匹配到的元素数量

    if (ret < 0)
    {
        E("pcre2_match:fail txt:%s ret:%d", txt, ret);
        return 0;
    }
    else if (ret == 0)
    {
        E("pcre2_match:ok txt:%s ret:%d", txt, ret);
    }
    else
    {
        E("pcre2_match:ok txt:%s ret:%d", txt, ret);

        int len;
        char format[512];

        PCRE2_SIZE *pos = pcre2_get_ovector_pointer(match_data);

        for (int i = 0; i < ret; i++)
        {
            len = pos[2 * i + 1] - pos[2 * i];
            sprintf_s(format, sizeof(format), "i:%%d pos:%%d-%%d len:%%d str:%%.%ds", len);
            D(format, i, pos[2 * i], pos[2 * i + 1], len, txt + pos[2 * i]);
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(pcre_data);
    return 0;
}

/**
 *\brief        窗体显示函数
 *\param[in]    wnd             窗体句柄
 *\param[in]    param           自定义参数
 *\return                       无
 */
void on_menu_show(HWND wnd, void *param)
{
    ShowWindow(wnd, IsWindowVisible(wnd) ? SW_HIDE : SW_SHOW);
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

    DestroyWindow(wnd);
}

#include "cunit.h"
#include "Automated.h"

void case_1(void)
{
    //CU_FAIL("This is a failure.");
    CU_ASSERT(1); // 0-失败,!0-成功
    CU_ASSERT_TRUE(CU_TRUE);
    CU_ASSERT_FALSE(CU_FALSE);
    CU_TEST_FATAL(CU_TRUE);
}

void case_2(void)
{
    CU_ASSERT_EQUAL(10, 10);
    CU_ASSERT_NOT_EQUAL(10, 11);
}

void case_3(void)
{
    CU_ASSERT_PTR_EQUAL((void*)0x100, (void*)0x100);
    CU_ASSERT_PTR_NOT_EQUAL((void*)0x100, (void*)0x101);
    CU_ASSERT_PTR_NULL((void*)(NULL));
    CU_ASSERT_PTR_NOT_NULL((void*)0x100);
}

void case_4(void)
{
    char *str1 = "1234567";
    char *str2 = "1234567";
    char *str3 = "123----";
    CU_ASSERT_STRING_EQUAL(str1, str2);
    CU_ASSERT_STRING_NOT_EQUAL(str1, str3);
    CU_ASSERT_NSTRING_EQUAL(str1, str2, strlen(str1));
    CU_ASSERT_NSTRING_NOT_EQUAL(str1, str3, 4);
}

void case_5(void)
{
    CU_ASSERT_DOUBLE_EQUAL(10, 10.0001, 0.0001);
    CU_ASSERT_DOUBLE_NOT_EQUAL(10, 10.001, 0.0001);
}

int suite_init(void)
{
    return 0;
}

int suite_clean(void)
{
    return 0;
}

#define CU(x) { "\"" #x "\"", x }

int cunit()
{
    CU_TestInfo cases[] =
    {
        CU(case_1),
        CU(case_2),
        CU(case_3),
        CU(case_4),
        CU(case_5),
        CU_TEST_INFO_NULL
    };

    CU_SuiteInfo suites[] =
    {
        {TITLE, suite_init, suite_clean, NULL, NULL, cases}, CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_initialize_registry())
    {
        E("init cunit fail");
        return -1;
    }

    if (CUE_SUCCESS != CU_register_suites(suites))
    {
        E("reg suites fail:%s", CU_get_error_msg());
        return -2;
    }

    CU_set_output_filename("D:\\2.code\\CUnit\\"TITLE".cunit");
    CU_list_tests_to_file();
    CU_automated_run_tests();
    CU_cleanup_registry();
    return 0;
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
    if (init() != 0)
    {
        return -1;
    }

    notify_menu_info menu[2] = { {0, L"显示(&S)", NULL, on_menu_show},{1, L"退出(&E)", NULL, on_menu_exit} };

    notify_init(hInstance, IDI_GREEN, TITLE, SIZEOF(menu), menu);

    cunit();

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
