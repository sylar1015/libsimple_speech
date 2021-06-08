/* Copyright (C) 2021-2021 Liang Sun <sylar1015@163.com>
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

    char ltasr_upload_url[1024];
    char ltasr_create_url[1024];
    char ltasr_query_url[1024];
    char ltasr_list_url[1204];
    //char ltasr_cancel_url[1024];

} sp_speech_handle_t;

/* module handle */
static sp_speech_handle_t s_handle;

/* helper functions */
static int build_ltasr_payload(sp_json_t **json, void *params);
static int build_rtasr_payload(sp_json_t **json, void *params);

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

        sp_string_append(s_handle.ltasr_create_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_create_url, "%s", "/ltasr/local/begin");

        sp_string_append(s_handle.ltasr_query_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_query_url, "%s", "/ltasr/result/info");

        sp_string_append(s_handle.ltasr_list_url, "%s", s_handle.api_gateway_url);
        sp_string_append(s_handle.ltasr_list_url, "%s", "/ltasr/list/info");
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

    sp_http_response_t *res = sp_http_post_file(s_handle.ltasr_upload_url, headers, 0, path);
    sp_return_val_if_fail(res != NULL, -1);

    int result = 0;
    sp_json_t *json = NULL;

    do {

        const char *body = sp_string_buffer_string(res->raw_body);
        sp_break_op_if_fail(body != NULL, result = -1);

        json = sp_json_parse(body);
        sp_break_op_if_fail(json != NULL, result = -1);

        sp_json_t *node = sp_json_object_item(json, "result");
        sp_break_op_if_fail(node != NULL, result = -1);

        node = sp_json_object_item(node, "fileUrl");
        sp_break_op_if_fail(node != NULL, result = -1);

        sp_string_clear(url);
        sp_string_copy(url, node->valuestring);

    } while (0);

    sp_op_if(json != NULL, sp_json_free(json));

    sp_op_if(res != NULL, sp_http_response_free(res));

    return result;
}

int sp_speech_asr_file_start(char* task_id /*out*/, void* params /*in*/)
{
    sp_return_val_if_fail(task_id && params, -1);

    sp_json_t *json = NULL;
    sp_return_val_if_fail(build_ltasr_payload(&json, params) == 0, -1);

    sp_http_response_t *res = sp_http_post_json(s_handle.ltasr_create_url, NULL, 0, json);
    sp_json_free(json);
    sp_return_val_if_fail(res, -1);

    json = sp_json_parse(sp_string_buffer_string(res->raw_body));

    sp_return_val_if_fail(json, -1);

    int result = 0;

    do {

        sp_json_t *node = sp_json_object_item(json, "result");
        sp_break_op_if_fail(node != NULL, result = -1);

        node = sp_json_object_item(node, "taskId");
        sp_break_op_if_fail(node != NULL, result = -1);

        sp_string_copy(task_id, node->valuestring);

    } while (0);

    sp_json_free(json);
        
    return result;
}

int sp_speech_asr_file_list(char **json_text /*out sp_free later*/, void *params /*in*/)
{
    sp_return_val_if_fail(json_text != NULL, -1);

    sp_json_t *payload = sp_json_object_new();
    sp_return_val_if_fail(payload, -1);

/*
 * "start_time"         :          ： 2021-05-17 02:13:48
 * "end_time"           :          ： 2021-06-07 02:13:48
 * "page"               :          ： 1
 * "size"               :          :  10
 * "file_name"          :          :  sp_speech.wav
 * "transcribe_status"  :          :  0
 */

    do {

        sp_break_if_fail(params != NULL);

        const char *start_time = sp_speech_params_get(params, "start_time");
        const char *end_time = sp_speech_params_get(params, "end_time");
        const char *page = sp_speech_params_get(params, "page");
        const char *size = sp_speech_params_get(params, "size");
        const char *file_name = sp_speech_params_get(params, "file_name");
        const char *transcribe_status = sp_speech_params_get(params, "transcribe_status");

        sp_op_if_unlikely(start_time,
            sp_json_object_add(payload, "startTime", sp_json_string(start_time)));

        sp_op_if_unlikely(end_time,
            sp_json_object_add(payload, "endTime", sp_json_string(end_time)));

        sp_op_if_unlikely(page,
            sp_json_object_add(payload, "page", sp_json_int(page)));

        sp_op_if_unlikely(size,
            sp_json_object_add(payload, "size", sp_json_int(size)));

        sp_op_if_unlikely(file_name,
            sp_json_object_add(payload, "fileName", sp_json_string(file_name)));

        sp_op_if_unlikely(transcribe_status,
            sp_json_object_add(payload, "transcribeStatus", sp_json_int(transcribe_status)));

    } while (0);

    sp_http_response_t *res = sp_http_post_json(s_handle.ltasr_list_url, NULL, 0, payload);

    sp_return_val_if_fail(res != NULL, -1);

    *json_text = sp_strdup(sp_string_buffer_string(res->raw_body));
    sp_http_response_free(res);

    return 0;
}

int sp_speech_asr_file_query(char **json_text /*out*/, const char* task_id /*in*/)
{
    sp_return_val_if_fail(json_text && task_id, -1);

    sp_json_t *payload = sp_json_object_new();
    sp_json_object_add(payload, "taskId", sp_json_string(task_id));

    sp_http_response_t *res = sp_http_post_json(s_handle.ltasr_query_url, NULL, 0, payload);
    sp_json_free(payload);
    sp_return_val_if_fail(res, -1);

    *json_text = sp_strdup(sp_string_buffer_string(res->raw_body));

    sp_http_response_free(res);

    return 0;
}

int sp_speech_asr_file_stop(const char *task_id /*in*/)
{
    sp_return_val_if_fail(task_id, -1);
    return 0;

#if 0
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
#endif
}

int sp_speech_asr_stream_start(char *task_id /*out*/, void *params /*in*/)
{
    return 0;
}

static int build_ltasr_payload(sp_json_t **out_json, void *params)
{
    sp_return_val_if_fail(out_json && params, -1);

    sp_json_t *node = sp_json_object_item(params, "audio_url");
    sp_return_val_if_fail(node && node->valuestring, -1);
    const char *audio_url = node->valuestring;

    node = sp_json_object_item(params, "audio_name");
    const char *audio_name = node ? node->valuestring : "sp_speech.wav";

    node = sp_json_object_item(params, "speech_contexts");
    const char *speech_context = node ? node->valuestring : "[]";

    node = sp_json_object_item(params, "enable_diarization");
    bool enable_diarization = node ? sp_string_equal(node->valuestring, "true") : false;

    node = sp_json_object_item(params, "speaker_number");
    int speaker_number = node ? atoi(node->valuestring) : 1;

    node = sp_json_object_item(params, "enable_voiceprint");
    bool enable_voiceprint = node ? sp_string_equal(node->valuestring, "true") : false;

    node = sp_json_object_item(params, "sample_rate");
    int sample_rate = node ? node->valueint : 16000;

    char buf[64] = {0};
    const char *file_id = sp_string_between(audio_url, "/download/", NULL, buf);

    node = sp_json_object_item(params, "replace");
    const char *replace_config = node ? node->valuestring : "[]";

    sp_json_t *json = sp_json_object_new();

    sp_json_object_add(json, "fileId", sp_json_string(file_id));
    sp_json_object_add(json, "fileName", sp_json_string(audio_name));
    sp_json_object_add(json, "fileUrl", sp_json_string(audio_url));
    sp_json_object_add(json, "enableChannelSeparation", sp_json_bool(false));
    sp_json_object_add(json, "enablePunctuation", sp_json_bool(true));
    sp_json_object_add(json, "enableItn", sp_json_bool(true));
    sp_json_object_add(json, "enableVoiceprint", sp_json_bool(enable_voiceprint));
    sp_json_object_add(json, "enableWordTimeOffsets", sp_json_bool(false));
    sp_json_object_add(json, "encoding", sp_json_string("pcm"));
    sp_json_object_add(json, "language", sp_json_string("MANDARIN"));
    sp_json_object_add(json, "sampleRateHertz", sp_json_int(sample_rate));
    sp_json_object_add(json, "model", sp_json_string("GENERAL"));
    sp_json_object_add(json, "speechContexts", sp_json_parse(speech_context));
    sp_json_object_add(json, "speakerNumber", sp_json_int(speaker_number));
    sp_json_object_add(json, "replaceConfigDTOList", sp_json_parse(replace_config));

    *out_json = json;

    return 0;
}

static int build_rtasr_payload(sp_json_t **json, void *params)
{
    return 0;
}