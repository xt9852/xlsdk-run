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
#include "xt_notify.h"
#include "xt_http.h"
#include "torrent.h"
#include "xl_sdk.h"

/// 程序图标组
#define RT_GROUP_ICONA   MAKEINTRESOURCEA((ULONG_PTR)(RT_ICON) + DIFFERENCE)

/// 程序图标
#define RT_ICONA         MAKEINTRESOURCEA(3)

/// 程序标题
#define TITLE "DownloadSDKServerStart"

/// 主页面
#define INDEX_PAGE "<meta charset='utf-8'>\
<script>\n\
    function download(){\n\
        filename = document.getElementById('file').value;\n\
        if (filename == '') {alert('请输入要下载的文件地址');return;}\n\
        url = '/download?file=' + filename;\n\
        file_list = document.getElementById('file-list');\n\
        if (file_list.style.display == '') {\n\
            list = '';\n\
            count = file_list.childNodes.length;\n\
            for (var i = 1; i < count; i++){\n\
                list = list + (file_list.childNodes[i].childNodes[0].childNodes[0].checked ? '1' : '0');\n\
            }\n\
            if (/^0+$/.test(list)) {alert('请选取要下载的文件'); return;}\n\
            url = url + '&list=' + list ;\n\
            file_list.style.display = 'none';\n\
        }\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            console.log(url + ' status:' + req.status);\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            down_list = document.getElementById('down-list');\n\
            while (down_list.childNodes.length > 1) { down_list.removeChild(down_list.childNodes[1]); }\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                tr = document.createElement('tr');\n\
                td = document.createElement('td');\n\
                td.innerText = item['id'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['torrent'];\n\
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
                td.innerText = item['progress'];\n\
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
                down_list.appendChild(tr);\n\
            }\n\
        }\n\
    }\n\
    function show_in_torrent_files(){\n\
        this.style.display = 'none';\n\
        filename = document.getElementById('file_' + this.i).innerText;\n\
        url = '/torrent-list?torrent=' + filename\n\
        req = new XMLHttpRequest();\n\
        req.open('GET', url);\n\
        req.send(null);\n\
        req.onload = function(){\n\
            console.log(url + ' status:' + req.status);\n\
            if (req.readyState != 4 || req.status != 200) {alert('http请求失败');return;}\n\
            rp = JSON.parse(req.responseText);\n\
            document.getElementById('file').value = filename;\n\
            file_list = document.getElementById('file-list');\n\
            file_list.style.display = '';\n\
            while (file_list.childNodes.length > 1) { file_list.removeChild(file_list.childNodes[1]); }\n\
            for (var i in rp) {\n\
                item = rp[i];\n\
                tr = document.createElement('tr');\n\
                td = document.createElement('td');\n\
                ip = document.createElement('input');\n\
                ip.type = 'checkbox';\n\
                td.appendChild(ip);\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['file'];\n\
                tr.appendChild(td);\n\
                td = document.createElement('td');\n\
                td.innerText = item['size'];\n\
                tr.appendChild(td);\n\
                file_list.appendChild(tr);\n\
            }\n\
        }\n\
    }\n\
    function on_check(checkbox){\n\
        file_list = document.getElementById('file-list');\n\
        for (var i = 1; i < file_list.childNodes.length; i++){\n\
            check = file_list.childNodes[i].childNodes[0].childNodes[0];\n\
            check.checked = checkbox.checked;\n\
        }\n\
    }\n\
</script>\n\
<input id='file'/>\
<button onclick='download()'>download</button>\
<table id='file-list' border='1' style='border-collapse:collapse;display:none'>\
<tr><th><input type='checkbox' name='check' onclick='on_check(this)' /></th><th>大小</th><th>文件</th></tr></table>\
<table id='down-list' border='1' style='border-collapse:collapse;'>\
<tr><th>任务</th><th>种子</th><th>文件</th><th>大小</th><th>速度</th><th>进度</th><th>用时</th><th>操作</th></tr>"

/// 主页面结束
#define INDEX_END "</table>"

config                  g_cfg           = {0};  ///< 配置数据

xt_log                  g_log           = {0};  ///< 日志数据
xt_log                  g_test          = {0};  ///< 多日志测试

xt_http                 g_http          = {0};  ///< HTTP服务
xt_thread_pool          g_thread_pool   = {0};  ///< 线程池
xt_timer_set            g_timer_set     = {0};  ///< 定时器

bt_torrent              g_torrent       = {0};  ///< 种子文件信息
xl_task                 g_task[128]     = {0};  ///< 当前正在下载的任务信息
int                     g_task_count    = 0;    ///< 当前正在下载的任务数量

/**
 *\brief        得到格式化后的信息
 *\param[in]    data            数据
 *\param[out]   info            信息
 *\param[in]    info_size       缓冲区大小
 *\return                       无
 */
void format_data(unsigned __int64 data, char *info, int info_size)
{
    double g = data / 1024.0 / 1024.0 / 1024.0;
    double m = data / 1024.0 / 1024.0;
    double k = data / 1024.0;

    if (g > 1.0)
    {
        snprintf(info, info_size, "%.2fG", g);
    }
    else if (m > 1.0)
    {
        snprintf(info, info_size, "%.2fM", m);
    }
    else if (k > 1.0)
    {
        snprintf(info, info_size, "%.2fK", k);
    }
    else
    {
        snprintf(info, info_size, "%I64u", data);
    }
}

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
 *\brief        下载文件
 *\param[in]    filename        文件地址
 *\param[in]    list            下载BT文件时选中的要下载的文件,如:"10100",1-选中,0-末选
 *\return       0               成功
 */
int http_download(const char *filename, const char *list)
{
    int ret;
    int task_id;
    int task_type;
    char task_name[MAX_PATH];

    if (NULL == filename || 0 == strcmp(filename, ""))
    {
        DBG("filename:null or \"\"", filename);
        return 404;
    }

    DBG("task count:%d", g_task_count);

    for (int i = 0; i < g_task_count; i++)
    {
        if (0 == strcmp(filename, g_task[i].filename))  // 已经下载
        {
            DBG("have %s", filename);
            return 0;
        }
    }

    if (0 == strcmp(filename + strlen(filename) - 8, ".torrent"))   // BT下载
    {
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
        ERR("create task %s error:%d", filename, ret);
        return 404;
    }

    ret = xl_sdk_start_download_file(task_id, task_type);

    if (0 != ret)
    {
        ERR("download start %s error:%d", filename, ret);
        return 404;
    }

    ret = xl_sdk_get_task_info(task_id, &g_task[g_task_count].size, &g_task[g_task_count].down, &g_task[g_task_count].time);

    if (0 != ret)
    {
        ERR("get task info error:%d", ret);
        return 404;
    }

    char size[16];
    format_data(g_task[g_task_count].size, size, sizeof(size));

    DBG("task:%d size:%s", task_id, size);

    strcpy_s(g_task[g_task_count].filename, sizeof(g_task[g_task_count].filename), filename);
    g_task[g_task_count].id   = task_id;
    g_task[g_task_count].type = task_type;
    //g_task[g_task_count].size = 0;
    //g_task[g_task_count].down = 0;
    //g_task[g_task_count].time = 0;

    //////////////////////////////////// test
    strcpy_s(g_task[g_task_count].filename, sizeof(g_task[g_task_count].filename), "D:\\5.downloads\\bt\\7097B42EEBC037482B69056276858599ED9605B5.torrent");
    g_task[g_task_count].type = TASK_MAGNET;

    g_task_count++;

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
    char size[16];
    char oper[256];
    int len = sizeof(INDEX_PAGE) - 1;
    int pos = len;

    strcpy_s(content, *content_len, INDEX_PAGE);

    for (int i = 0; i < g_task_count; i++)
    {
        format_data(g_task[i].size, size, sizeof(size));

        snprintf(oper, sizeof(oper), "<button id='btn_%d' style='display:block;' onclick=\"show_in_torrent_files('%d')\">open</button>", i, i);

        len = snprintf(content + pos, *content_len - pos,
               "<tr><td>%d</td><td>%s</td><td id='file_%d'>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%u</td><td>%s</td></tr>",
               g_task[i].id, "", i, g_task[i].filename, size, "", "", g_task[i].time,
               (TASK_MAGNET == g_task[i].type) ? oper : "");

        pos += len;
    }

    strcpy_s(content + pos, *content_len - pos, INDEX_END);

    *content_type = HTTP_TYPE_HTML;
    *content_len = pos + sizeof(INDEX_END) - 1;
    return 0;
}

/**
 *\brief        http回调函数,主页
 *\param[in]    filename        文件地址
 *\param[in]    list            下载BT文件时选中的要下载的文件,如:"10100",1-选中,0-末选
 *\param[out]   content_type    返回内容类型
 *\param[out]   content         返回内容
 *\param[out]   content_len     返回内容长度
 *\return       0               成功
 */
int http_process_download(const char *filename, const char *list, int *content_type, char *content, int *content_len)
{
    if (0 != http_download(filename, list))
    {
        return 404;
    }

    int pos = 1;
    int len = 0;
    char size[16];
    char base64[MAX_PATH];
    content[0] = '[';

    for (int i = 0; i < g_task_count; i++)
    {
        format_data(g_task[i].size, size, sizeof(size));

        len = sizeof(base64);

        base64_to(g_task[i].filename, strlen(g_task[i].filename), base64, &len);

        len = snprintf(content + pos, *content_len - pos, "{\"id\":%d,\"torrent\":\"%s\",\"file\":\"%s\",\"size\":\"%s\",\"speed\":\"%s\",\"progress\":\"%s\",\"time\":%d},",
                       g_task[i].id, "", base64, size, "1.23M", "12.3", g_task[i].time);

        pos += len;
    }

    content[pos - 1] = ']';

    *content_type = HTTP_TYPE_HTML;
    *content_len = pos;
    return 0;
}

/**
 *\brief        http回调函数,得到种子中文件信息
 *\param[in]    torrent         种子文件本地地址
 *\param[out]   content_type    返回内容类型
 *\param[out]   content         返回内容
 *\param[out]   content_len     返回内容长度
 *\return       0               成功
 */
int http_process_torrent(const char *torrent, int *content_type, char *content, int *content_len)
{
    if (NULL == torrent || 0 == strcmp(torrent, ""))
    {
        DBG("torrent:null or \"\"", torrent);
        return -1;
    }

    if (0 != get_torrent_info(torrent, &g_torrent))
    {
        ERR("get torrent info error");
        return 404;
    }

    int pos = 1;
    int len = 0;
    char size[16];
    content[0] = '[';

    for (int i = 0; i < g_torrent.count + 10; i++)
    {
        format_data(g_torrent.file[0].len, size, sizeof(size));

        DBG("file:%s size:%s", g_torrent.file[0].name, size);

        len = snprintf(content + pos, *content_len - pos, "{\"file\":\"%s\",\"size\":\"%s\"},", g_torrent.file[0].name, size);

        pos += len;
    }

    content[pos - 1] = ']';

    *content_type = HTTP_TYPE_HTML;
    *content_len = pos;
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
int http_process_callback(const char *uri, const char *arg_name[], const char *arg_data[], int arg_count,
                          int *content_type, char *content, int *content_len)
{
    DBG("uri:%s", uri);

    if (0 == strcmp(uri, "/"))
    {
        return http_process_index(content_type, content, content_len);
    }
    else if (0 == strcmp(uri, "/download"))
    {
        const char *file = (arg_count >= 1 && 0 == strcmp(arg_name[0], "file")) ? arg_data[0] : NULL;
        const char *list = (arg_count >= 2 && 0 == strcmp(arg_name[1], "list")) ? arg_data[1] : NULL;
        return http_process_download(file, list, content_type, content, content_len);
    }
    else if (0 == strcmp(uri, "/torrent-list"))
    {
        const char *torrent = (arg_count >= 1 && 0 == strcmp(arg_name[0], "torrent")) ? arg_data[0] : NULL;
        return http_process_torrent(torrent, content_type, content, content_len);
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
 *\brief    定时器任务回调
 *\param    [in]  param         自定义参数
 *\return                       无
 */
void timer_callback(void *param)
{
    DBG("param:%s", (char*)param);
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
        ERR("get md5 fail %d", ret);
        return -30;
    }

    DBG("str:%s md5.A:%x B:%x C:%x D:%x", md5_in, md5.A, md5.B, md5.C, md5.D);

    ret = md5_get_str(md5_in, strlen(md5_in), md5_out);

    if (ret != 0)
    {
        ERR("get md5 str fail %d", ret);
        return -31;
    }

    DBG("str:%s md5:%s", md5_in, md5_out);

    md5_in = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"\
             "1234567890";

    ret = md5_get_str(md5_in, strlen(md5_in), md5_out);

    if (ret != 0)
    {
        ERR("get md5 str fail %d", ret);
        return -32;
    }

    DBG("str:%s md5:%s", md5_in, md5_out);

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
            ERR("get base64 str fail %d", ret);
            return -40;
        }

        DBG("data:%s base64:%s len:%d", data[i], base64, len);

        ret = base64_from(base64, len, output, &len);

        if (ret != 0)
        {
            ERR("from base64 get data fail %d", ret);
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
        ERR("init memory pool fail %d", ret);
        return -60;
    }

    void *mem = NULL;

    for (int i = 0; i < 2000; i++)
    {
        ret = memory_pool_get(&mem_pool, &mem);

        if (ret != 0)
        {
            ERR("memory pool get fail %d", ret);
            return -61;
        }

        DBG("memory_pool_get ret:%d count:%d list-size:%d count:%d head:%d tail:%d", ret, mem_pool.count,
        mem_pool.free.size, mem_pool.free.count, mem_pool.free.head, mem_pool.free.tail);
    }

    ret = memory_pool_put(&mem_pool, mem);

    if (ret != 0)
    {
        ERR("memory pool put fail %d", ret);
        return -62;
    }

    DBG("memory_pool_put ret:%d memory-pool-count:%d list-size:%d count:%d head:%d tail:%d", ret, mem_pool.count,
        mem_pool.free.size, mem_pool.free.count, mem_pool.free.head, mem_pool.free.tail);

    ret = memory_pool_uninit(&mem_pool);

    if (ret != 0)
    {
        ERR("memory pool uninit fail %d", ret);
        return -63;
    }

    DBG("--------------------------------------------------------------------");

    ret = thread_pool_init(&g_thread_pool, 10);

    if (ret != 0)
    {
        ERR("thread pool init fail %d", ret);
        return -70;
    }

    DBG("--------------------------------------------------------------------");

    ret = timer_init(&g_timer_set);

    if (ret != 0)
    {
        ERR("timer init fail %d", ret);
        return -80;
    }

    ret = timer_add_cycle(&g_timer_set, "timer_0",  5, &g_thread_pool, timer_callback, "timer_0_param");

    if (ret != 0)
    {
        ERR("add cycle timer fail %d", ret);
        return -81;
    }

    ret = timer_add_cycle(&g_timer_set, "timer_1", 10, &g_thread_pool, timer_callback, "timer_1_param");

    if (ret != 0)
    {
        ERR("add cycle timer fail %d", ret);
        return -82;
    }

    ret = timer_add_cron(&g_timer_set, "timer_2", TIMER_CRON_MINUTE, 0, 0, 0, 0, 0, 0, 0, &g_thread_pool, timer_callback, "timer_2_param");

    if (ret != 0)
    {
        ERR("add cron timer fail %d", ret);
        return -83;
    }

    DBG("--------------------------------------------------------------------");

    g_http.run  = true;
    g_http.port = g_cfg.http_port;
    g_http.proc = http_process_callback;

    ret = http_init(&g_http);

    if (ret != 0)
    {
        ERR("http init fail %d", ret);
        return -90;
    }

    DBG("--------------------------------------------------------------------");

    // 初始化SDK
    ret = xl_sdk_init();

    if (0 != ret)
    {
        ERR("init error:%d", ret);
        return -100;
    }

    DBG("--------------------------------------------------------------------");

    ret = get_torrent_info("D:\\5.downloads\\bt\\7097B42EEBC037482B69056276858599ED9605B5.torrent", &g_torrent);

    if (0 != ret)
    {
        ERR("init error:%d", ret);
        return -110;
    }

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

    notify_init(hInstance, IDI_GREEN, SIZEOF(menu), menu);

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
