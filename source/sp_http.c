/* Copyright (C) 2017-2018 Liang Sun <sylar1015@163.com>
   This file is part of the Simple C Library
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
 
   You should have received a copy of the GNU Lesser General Public
   License along with the Simple Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "sp.h"
#include <curl/curl.h>

typedef struct
{
    CURL *curl;
} sp_http_session_t;

static struct curl_slist *sp_headers_2_curl_headers(sp_json_t *headers);
static size_t sp_http_write_callback(void *data, size_t size, size_t nmemb, void *arg);
static size_t sp_http_header_callback(void *data, size_t size, size_t nmemb, void *arg);
static size_t sp_http_read_callback(void *data, size_t size, size_t nmemb, void *arg);
static sp_http_response_t *sp_http_session_perform(sp_http_session_t *session,
    const char *url, sp_json_t *headers, int timeout);
static sp_http_response_t *sp_http_response_new();


sp_http_response_t *sp_http_get(const char *url, sp_json_t *headers, int timeout)
{
    sp_return_val_if_fail(url, NULL);

    sp_http_session_t *session = sp_http_session_new();
    sp_return_val_if_fail(session, NULL);

    sp_http_response_t *res = sp_http_session_get(session, url, headers, timeout);

    sp_http_session_free(session);
    
    return res;
}

sp_http_response_t *sp_http_delete(const char *url, sp_json_t *headers, int timeout)
{
    sp_return_val_if_fail(url, NULL);

    sp_http_session_t *session = sp_http_session_new();
    sp_return_val_if_fail(session, NULL);

    sp_http_response_t *res = sp_http_session_delete(session, url, headers, timeout);

    sp_http_session_free(session);
    
    return res;
}

sp_http_response_t *sp_http_post(const char *url, sp_json_t *headers,
    int timeout, const char *payload, int length)
{
    sp_return_val_if_fail(url, NULL);

    sp_http_session_t *session = sp_http_session_new();
    sp_return_val_if_fail(session, NULL);

    sp_http_response_t *res = sp_http_session_post(session, url, headers, timeout, payload, length);

    sp_http_session_free(session);

    return res;
}

sp_http_response_t *sp_http_post_file(const char *url,
    sp_json_t *headers, int timeout, const char *path)
{
    sp_return_val_if_fail(url && path && !sp_string_empty(url), NULL);

    sp_http_session_t *session = sp_http_session_new();
    sp_return_val_if_fail(session, NULL);

    CURL *curl = session->curl;

    curl_mime *form = curl_mime_init(curl);
    curl_mimepart *field = curl_mime_addpart(form);

    curl_mime_name(field, "file");
    curl_mime_filedata(field, path);

    //field = curl_mime_addpart(form);
    //curl_mime_name(field, "filename");
    //curl_mime_data(field, "postit2.c", CURL_ZERO_TERMINATED);

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    //curl_easy_setopt(curl, CURLOPT_POST, 1L);
    /* set where to read from (on Windows you need to use READFUNCTION too) */
    //curl_easy_setopt(curl, CURLOPT_READDATA, fp);
    //curl_easy_setopt(curl, CURLOPT_READFUNCTION, sp_http_read_callback);
    //curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);
    //curl_easy_setopt(curl, CURLOPT_INFILE, fp);

    sp_http_response_t *res = sp_http_session_perform(session, url, headers, timeout);

    curl_mime_free(form);

    sp_http_session_free(session);

    return res;
}

sp_http_response_t *sp_http_post_json(const char *url,
    sp_json_t *headers, int timeout, sp_json_t *json)
{
    sp_return_val_if_fail(url && json, NULL);
    const char *text = sp_json_text(json);

    int length = sp_string_length(text);
    sp_http_response_t *res = sp_http_post(url, headers, timeout, text, length);

    sp_free(text);

    return res;
}

sp_http_response_t *sp_http_post_soap(const char *url,
    int timeout, const char *payload)
{
    sp_return_val_if_fail(url && payload, NULL);

    int length = sp_string_length(payload);

    sp_json_t *headers = sp_json_object_new();
    sp_json_object_add(headers, "Content-Type", sp_json_string("application/soap+xml; charset=utf-8"));

    sp_http_response_t *res = sp_http_post(url, headers, timeout, payload, length);

    sp_json_free(headers);

    return res;
}

void *sp_http_session_new()
{
    sp_http_session_t *session = sp_calloc(1, sizeof(sp_http_session_t));

    session->curl = curl_easy_init();
    sp_return_val_if_fail(session->curl, NULL);

    return session;
}

void sp_http_session_free(void *session)
{
    sp_return_if_fail(session);

    sp_http_session_t *s = (sp_http_session_t *)session;

    curl_easy_cleanup(s->curl);

    sp_free(session);
}

static sp_http_response_t *sp_http_session_perform(sp_http_session_t *session,
    const char *url, sp_json_t *headers, int timeout)
{
    sp_return_val_if_fail(session && url, NULL);

    sp_http_session_t *s = (sp_http_session_t *)session;
    sp_return_val_if_fail(s->curl, NULL);

    CURL *curl = s->curl;

    curl_easy_setopt(curl, CURLOPT_URL, url);

    /* headers */
    struct curl_slist *curl_headers = sp_headers_2_curl_headers(headers);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);

    /* ignore signal */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    /* timeout */
    if (timeout > 0)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);        
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);
    }

    sp_http_response_t *res = sp_http_response_new();

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
                   sp_http_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, res);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sp_http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res);

    do
    {
        CURLcode ret = curl_easy_perform(curl);

        if (ret != CURLE_OK)
        {
            sp_http_response_free(res);
            res = NULL;
            break;
        }

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        res->status_code = status_code;
        
    } while(0);

    curl_slist_free_all(curl_headers);
    curl_easy_reset(curl);
    return res;
}

sp_http_response_t *sp_http_session_get(void *session, const char *url,
    sp_json_t *headers, int timeout)
{
    return sp_http_session_perform(session, url, headers, timeout);
}

sp_http_response_t *sp_http_session_delete(void *session, const char *url,
    sp_json_t *headers, int timeout)
{
    sp_return_val_if_fail(session && url, NULL);

    sp_http_session_t *s = (sp_http_session_t *)session;

    CURL *curl = s->curl;
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    return sp_http_session_perform(session, url, headers, timeout);
}

sp_http_response_t *sp_http_session_post(void *session, const char *url,
    sp_json_t *headers, int timeout, const char *payload, int length)
{
    sp_return_val_if_fail(session && url, NULL);

    sp_http_session_t *s = (sp_http_session_t *)session;

    CURL *curl = s->curl;
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, length);

    return sp_http_session_perform(session, url, headers, timeout);
}

sp_http_response_t *sp_http_response_new()
{
    sp_http_response_t *res = sp_calloc(1, sizeof(sp_http_response_t));

    res->raw_headers = sp_string_buffer_new(0);
    res->raw_body = sp_string_buffer_new(0);

    return res;
}

void sp_http_response_free(sp_http_response_t *res)
{
    sp_return_if_fail(res);

    sp_string_buffer_free(res->raw_body);
    sp_string_buffer_free(res->raw_headers);

    sp_free(res);
}

static struct curl_slist *sp_headers_2_curl_headers(sp_json_t *headers)
{
    sp_return_val_if_fail(headers, NULL);

    struct curl_slist *curl_headers = NULL;

    sp_json_t *item = NULL;
    sp_json_foreach(item, headers)
    {
        char header_kv[1024];
        sp_string_clear(header_kv);
        sp_string_append(header_kv, "%s: %s", item->string, item->valuestring);
        curl_headers = curl_slist_append(curl_headers, header_kv);
    }

    return curl_headers;
}

static size_t sp_http_write_callback(void *data, size_t size, size_t nmemb, void *arg)
{
    sp_http_response_t *res = (sp_http_response_t *)arg;

    size_t length = size * nmemb;
    sp_string_buffer_append(res->raw_body, data, length);

    return length;
}

static size_t sp_http_read_callback(void *data, size_t size, size_t nmemb, void *arg)
{
    size_t sizes = fread(data, size, nmemb, (FILE *)arg);
	return sizes;
}

static size_t sp_http_header_callback(void *data, size_t size, size_t nmemb, void *arg)
{    
    sp_http_response_t *res = (sp_http_response_t *)arg;

    size_t length = size * nmemb;
    sp_string_buffer_append(res->raw_headers, data, length);

    return length;
}

void sp_http_fini()
{
    curl_global_cleanup();
}
