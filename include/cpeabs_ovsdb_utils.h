/*
 * Copyright 2021 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jansson.h>

#define OVSDB_COL_NUM    256
#define OVSDB_COL_STR   1024
#define UUID_STR_LEN     36
#define OVSDB_SOCK_PATH  "/var/run/db.sock"

bool ovsdb_string_value_get(char *table,json_t *where, int coln, char * colv[],char *result_str, unsigned int len );
bool ovsdb_parse_where(json_t *where, char *_str, bool is_parent_where);
