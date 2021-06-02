/* Copyright (C) 2017-2017 Liang Sun <sylar1015@163.com>
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

typedef struct sp_speech_handle_t {

    sp_hashtable_t *params;
    sp_speech_asr_callback_t callbacks;

    char api_gateway_url[1024];
    char ltasr_upload_url[1024];
    char download_url[1024];

} sp_speech_handle_t;

static sp_speech_handle_t s_handle;

int sp_speech_init()
{
    sp_bzero(&s_handle, sizeof(sp_speech_handle_t));

    s_handle.params = sp_hashtable_new(64,
        sp_hashtable_str_hash,
        sp_hashtable_str_equal,
        free,
        free);

    sp_return_val_if_fail(s_handle.params != NULL, -1);

    return 0;
}

int sp_speech_fini()
{
    sp_return_val_if_fail(s_handle.params != NULL, 0);

    sp_hashtable_free(s_handle.params);

    sp_bzero(&s_handle, sizeof(sp_speech_handle_t));

    return 0;
}

int sp_speech_set_global_params(const char *key, const char *value)
{
    sp_hashtable_t *params = s_handle.params;
    sp_return_val_if_fail(params != NULL, -1);

    int res = sp_hashtable_put(params, sp_strdup(key), sp_strdup(value));
    sp_return_val_if_fail(res == 0, -1);

    if (sp_string_equal(key, SP_SPEECH_API_GATEWAY_URL))
    {
        sp_string_copy(s_handle.api_gateway_url, value);
        if (sp_string_endswith(value, "/"))
        {
            s_handle.api_gateway_url[sp_string_length(value) - 1] = 0;
        }

        sp_string_append(s_handle.ltasr_upload_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_upload_url, "%s", "/file/ltasr/upload");

        sp_string_append(s_handle.download_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.download_url, "%s", "/file/download/");
    }

    return 0;
}

int sp_speech_set_global_callbacks(sp_speech_asr_callback_t* callbacks)
{
    s_handle.callbacks = *callbacks;
    return 0;
}

void* sp_speech_params_new()
{
    return NULL;
}

int sp_speech_params_set(const char* key, const char* value)
{
    return 0;
}

const char* sp_speech_params_get(void *params /*in*/, const char* key /*in*/)
{
    return NULL;
}

void sp_speech_params_free(void *params /*in*/)
{

}

int sp_speech_asr_file_upload(char *url /*out*/, const char* path /*in*/)
{
    sp_return_val_if_fail(url && path, -1);

    sp_http_response_t *res = sp_http_upload(s_handle.ltasr_upload_url, NULL, 0, NULL, 0, path);

    sp_return_val_if_fail(res != NULL, -1);

    const char *body = sp_string_buffer_string(res->raw_body);
    sp_return_val_if_fail(body != NULL, -1);

    sp_json_t *json = sp_json_parse(body);
    sp_return_val_if_fail(json != NULL, -1);

    sp_json_t *node = sp_json_object_item(json, "result");
    sp_return_val_if_fail(node != NULL, -1);

    node = sp_json_object_item(node, "fileId");
    sp_return_val_if_fail(node != NULL, -1);

    sp_string_clear(url);
    sp_string_append(url, "%s%d", s_handle.download_url, node->valueint);

    sp_http_response_free(res);

    return 0;
}

int sp_speech_asr_file_start(char* task_id /*out*/, void* params /*in*/, const char* json_text /*in*/)
{
    return 0;
}

int sp_speech_asr_file_query(char** json_text /*out*/, const char* task_id /*in*/)
{
    return 0;
}

int sp_speech_asr_file_stop(const char* task_id /*in*/)
{
    return 0;
}

int sp_speech_asr_stream_start(char* task_id /*out*/, void* params /*in*/, const char* json_text /*in*/)
{
    return 0;
}
