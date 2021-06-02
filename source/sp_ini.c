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

typedef struct
{
    sp_json_t *ini;
} sp_ini_handle_t;

void *sp_ini_parse_file(const char *filename)
{
    sp_return_val_if_fail(filename, NULL);

    FILE *fp = fopen(filename, "r");
    sp_return_val_if_fail(fp, NULL);

    sp_ini_handle_t *h = (sp_ini_handle_t *)sp_calloc(1, sizeof(sp_ini_handle_t));

    h->ini = sp_json_object_new();

    char line[1024];
    size_t line_length = 1024;
    sp_json_t *current_section = NULL;
    while (fgets(line, line_length, fp))
    {
        if (sp_string_startswith(line, "#"))
        {
            continue;
        }

        if (sp_string_startswith(line, "["))
        {
            char section[256];
            if (!sp_string_between(line, "[", "]", section))
            {
                continue;
            }

            current_section = sp_json_object_new();
            sp_json_object_add(h->ini, section, current_section);
        }

        int pos = sp_string_find(line, "=");
        if (pos <= 0)
        {
            continue;
        }

        if (!current_section)
        {
            continue;
        }

        char key[256];
        char val[256];

        sp_string_ncopy(key, line, pos);
        sp_string_trim_inplace(key);

        sp_string_copy(val, line + pos + 1);
        sp_string_trim_inplace(val);

        sp_json_object_add(current_section, key, sp_json_string(val));
    }

    fclose(fp);

    return h;
}

int sp_ini_get_int(void *ini, const char *section, const char *key)
{
    char *value = sp_ini_get_string(ini, section, key);
    sp_return_val_if_fail(value, 0);

    return atoi(value);
}

void sp_ini_set_int(void *ini, 
    const char *section, const char *key, int value)
{
    sp_return_if_fail(ini && section && key);

    char temp[128] = {0};
    sprintf(temp, "%d", value);

    sp_ini_set_string(ini, section, key, temp);
}

char *sp_ini_get_string(void *ini, const char *section, const char *key)
{
    sp_return_val_if_fail(ini && section && key, NULL);

    sp_ini_handle_t *h = (sp_ini_handle_t *)ini;

    sp_json_t *item = sp_json_object_item(h->ini, section);

    sp_return_val_if_fail(item, NULL);

    item = sp_json_object_item(item, key);

    return item->valuestring;
}

void sp_ini_set_string(void *ini, 
    const char *section, const char *key, const char *value)
{
    sp_return_if_fail(ini && section && key && value);

    sp_ini_handle_t *h = (sp_ini_handle_t *)ini;

    sp_json_t *current_section = sp_json_object_item(h->ini, section);
    if (current_section == NULL)
    {
        current_section = sp_json_object_new();
        sp_json_object_add(h->ini, section, current_section);
    }

    sp_json_t *current_key = sp_json_object_item(current_section, key);
    if (current_key == NULL)
    {
        sp_json_object_add(current_section, key, sp_json_string(value));
    }
    else
    {
        sp_json_object_replace(current_section, key, sp_json_string(value));
    }
}


void sp_ini_free(void *ini)
{
    sp_return_if_fail(ini);

    sp_ini_handle_t *h = (sp_ini_handle_t *)ini;

    sp_json_free(h->ini);

    sp_free(h);
}

void sp_ini_save_file(void *ini, const char *filename)
{
    sp_return_if_fail(ini && filename);

    sp_ini_handle_t *h = (sp_ini_handle_t *)ini;

    sp_json_t *section = NULL;
    sp_json_t *key = NULL;

    FILE *fp = fopen(filename, "w");
    sp_return_if_fail(fp);

    sp_json_foreach(section, h->ini)
    {
        char section_str[128] = {0};
        sprintf(section_str, "[%s]\n", section->string);

        fwrite(section_str, 1, sp_string_length(section_str), fp);

        sp_json_foreach(key, section)
        {
            char key_str[256] = {0};
            sprintf(key_str, "%s=%s\n", key->string, key->valuestring);

            fwrite(key_str, 1, sp_string_length(key_str), fp);
        }
    }

    fclose(fp);
}
