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

    sp_json_t *params;
    sp_speech_asr_callback_t callbacks;

    char api_gateway_url[1024];

    char download_url[1024];

    char ltasr_upload_url[1024];
    char ltasr_create_url[1024];
    char ltasr_query_url[1024];
    char ltasr_cancel_url[1024];

} sp_speech_handle_t;

static sp_speech_handle_t s_handle;

int sp_speech_init()
{
    sp_bzero(&s_handle, sizeof(sp_speech_handle_t));

    s_handle.params = sp_json_object_new();

    sp_return_val_if_fail(s_handle.params != NULL, -1);

    return 0;
}

int sp_speech_fini()
{
    sp_return_val_if_fail(s_handle.params != NULL, 0);

    sp_json_free(s_handle.params);

    sp_bzero(&s_handle, sizeof(sp_speech_handle_t));

    return 0;
}

int sp_speech_set_global_params(const char *key, const char *value)
{
    sp_json_t *params = s_handle.params;
    sp_return_val_if_fail(params != NULL, -1);

    sp_json_object_add(params, key, sp_json_string(value));

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

        sp_string_append(s_handle.ltasr_create_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_create_url, "%s", "/file/ltasr/create");

        sp_string_append(s_handle.ltasr_query_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_query_url, "%s", "/file/ltasr/query/");

        sp_string_append(s_handle.ltasr_cancel_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_cancel_url, "%s", "/file/ltasr/cancel/");
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
    return sp_json_object_new();
}

int sp_speech_params_set(void *params, const char* key, const char* value)
{
    sp_return_val_if_fail(params && key && value, -1);

    sp_json_object_add(params, key, sp_json_string(value));

    return 0;
}

const char* sp_speech_params_get(void *params /*in*/, const char* key /*in*/)
{
    sp_return_val_if_fail(params && key, -1);

    sp_json_t *node = sp_json_object_item(params, key);
    sp_return_val_if_fail(node != NULL, NULL);

    return node->valuestring;
}

void sp_speech_params_free(void *params /*in*/)
{
    sp_return_if_fail(params);

    sp_json_free(params);
}

int sp_speech_asr_file_upload(char *url /*out*/, const char* path /*in*/)
{
    sp_return_val_if_fail(url && path, -1);

    sp_json_t *headers = NULL;
    //headers = sp_json_object_new();
    //sp_json_object_add(headers, "Expect", sp_json_string(""));

    sp_http_response_t *res = sp_http_post_file(s_handle.ltasr_upload_url, headers, 0, path);
    //sp_json_free(headers);

    sp_return_val_if_fail(res != NULL, -1);

    const char *body = sp_string_buffer_string(res->raw_body);
    printf("%s\n", body);
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
    sp_return_val_if_fail(task_id && (params || json_text), -1);

    /* use params or json_text depends on non-empty */
    if (json_text)
    {
        sp_json_t *json = sp_json_parse(json_text);
        sp_return_val_if_fail(json, -1);

        sp_http_response_t *res = sp_http_post_json(s_handle.ltasr_create_url, NULL, 0, json);
        sp_return_val_if_fail(res, -1);

        sp_json_free(json);

        json = sp_json_parse(sp_string_buffer_string(res->raw_body));
        sp_return_val_if_fail(json, -1);

        sp_json_t *node = sp_json_object_item(json, "taskId");
        sp_return_val_if_fail(node, -1);

        sp_string_copy(task_id, node->valuestring);
    }
    else 
    {
        /* FixMe: add params handler */
        const char *audio_url = sp_json_object_item(params, "audioUrl");
        sp_return_val_if_fail(audio_url, -1);

        sp_json_t *file_data = sp_json_object_new();
        sp_json_object_add(file_data, "encoding", sp_json_string("pcm"));
        sp_json_object_add(file_data, "language", sp_json_string("MANDARIN"));
        sp_json_object_add(file_data, "sampleRateHertz", sp_json_int(16000));
        sp_json_object_add(file_data, "audioName", sp_json_string("sp_speech.wav"));

        sp_json_t *recognition_config = sp_json_object_new();
        sp_json_object_add(recognition_config, "model", sp_json_string("GENERAL"));
        sp_json_object_add(recognition_config, "enablePunctuation", sp_json_bool(true));

        sp_json_t *json = sp_json_object_new();
    }

    return 0;
}

int sp_speech_asr_file_query(char **json_text /*out*/, const char* task_id /*in*/)
{
    sp_return_val_if_fail(json_text && *json_text && task_id, -1);

    char url[1024];
    sp_string_clear(url);
    sp_string_append(url, "%s%s", s_handle.ltasr_query_url, task_id);

    sp_http_response_t *res = sp_http_get(url, NULL, 3);
    sp_return_val_if_fail(res, -1);

    *json_text = sp_strdup(sp_string_buffer_string(res->raw_body));

    sp_http_response_free(res);

    return 0;
}

int sp_speech_asr_file_stop(const char *task_id /*in*/)
{
    sp_return_val_if_fail(task_id, -1);

    char url[1024];
    sp_string_clear(url);
    sp_string_append(url, "%s%s", s_handle.ltasr_cancel_url, task_id);

    sp_http_response_t *res = sp_http_delete(url, NULL, 3);
    sp_return_val_if_fail(res, -1);

    sp_json_t *json = sp_json_parse(sp_string_buffer_string(res->raw_body));
    sp_http_response_free(res);

    sp_return_val_if_fail(json, -1);
    sp_json_t *node = sp_json_object_item(json, "rtn");
    sp_return_val_if_fail(node, -1);

    int result = node->valueint == 0 ? 0 : -1;

    sp_json_free(json);
    return result;
}

int sp_speech_asr_stream_start(char *task_id /*out*/, void *params /*in*/, const char *json_text /*in*/)
{
    return 0;
}
