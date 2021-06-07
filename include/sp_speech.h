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

#ifndef _SP_SPEECH_H_
#define _SP_SPEECH_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* global params */
#define SP_SPEECH_API_GATEWAY_URL "SP_SPEECH_API_GATEWAY_URL"

typedef struct sp_speech_asr_callback_t
{
   /* audio stream asr */
   void (*on_partial_transcription)(const char *task_id, const char *name, const char *content);
   void (*on_final_transcription)(const char *task_id, const char *name, const char *content);

   /* common */
   void (*on_error)(const char*task_id, int error_code);
} sp_speech_asr_callback_t;

/* global apis */
int sp_speech_init();
int sp_speech_fini();
int sp_speech_set_global_params(const char* key, const char* value);
int sp_speech_set_global_callbacks(sp_speech_asr_callback_t* callbacks);

/* params */
void* sp_speech_params_new();
int sp_speech_params_set(void *params, const char* key, const char* value);
const char* sp_speech_params_get(void *params /*in*/, const char* key /*in*/);
void sp_speech_params_free(void *params /*in*/);

/* file asr api */
int sp_speech_asr_file_upload(char *url /*out*/, const char *path /*in*/);
/*
 * @param key 参考识别参数
 * ---------------------------------
 * key                  : required ： example value  
 * ---------------------------------
 * "audio_url"          :   must   ： http://sp_speech.h or file:///sp_speech.h
 * "audio_name"         :          ： sp_speech.h
 * "sample_rate"        :          ： 16000
 * "encoding"           :          :  pcm
 * "language"           :          :  MANDARIN
 * "model"              :          :  GENERAL
 * "speech_contexts"    :          :  [测试, 热词]
 * "enable_diarization" :          :  true
 * "speaker_number"     :          :  2
 * ""
 */
int sp_speech_asr_file_start(char *task_id /*out*/, void *params /*in*/);
int sp_speech_asr_file_query(char **json_text /*out sp_free later*/, const char *task_id /*in*/);
int sp_speech_asr_file_stop(const char *task_id /*in*/);

/* stream asr api */
int sp_speech_asr_stream_start(char *task_id /*out*/, void *params /*in*/, const char *json_text /*in*/);
int sp_speech_asr_stream_stop(const char *task_id);

#ifdef __cplusplus
}
#endif

#endif
