/* SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#include <jansson.h>

#define OVSDB_COL_NUM    256
#define OVSDB_COL_STR   1024
#define UUID_STR_LEN     36
#define OVSDB_SOCK_PATH  "/var/run/db.sock"

bool ovsdb_string_value_get(char *table,json_t *where, int coln, char * colv[],char *result_str, unsigned int len );
bool ovsdb_parse_where(json_t *where, char *_str, bool is_parent_where);
void cpeabStrncpy(char *destStr, const char *srcStr, size_t destSize);
