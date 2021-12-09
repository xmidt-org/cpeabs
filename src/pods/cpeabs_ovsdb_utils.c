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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "cpeabs_ovsdb_utils.h"
#include "cpeabs.h"

static int  g_ovsdb_handle = -1;
static char ovsdb_opt_db[32] = "Open_vSwitch";
static int  ovsdb_where_num = 0;
static char **ovsdb_where_expr = NULL;

int ovsdb_connect(void);
void ovsdb_close(int fd);
static json_t  *ovsdb_json_exec(char *method, json_t *params);
static bool     ovsdb_json_error(json_t *jres);
static bool ovsdb_parse_columns(json_t *columns, int colc, char *colv[]);
json_t *json_value(char *str);


/*
 * Initialize Ovsdb connection and return handle.
 */
int ovsdb_util_init()
{
    int db = -1;

    db = ovsdb_connect();
    if (db < 0) {
        WebcfgError("%s : Failed to connect to ovsdb. \n",__func__);
    }
    else
    {
        WebcfgError("%s : Successfully connected to ovsdb \n",__func__);
        g_ovsdb_handle = db;
    }
    return db;
}

bool ovsdb_util_fini()
{
    if (g_ovsdb_handle > 0)
    {
        ovsdb_close(g_ovsdb_handle);
        g_ovsdb_handle = -1;
    }
    return true;
}

int ovsdb_handle_get()
{
    return g_ovsdb_handle;
}

/*
 * ===========================================================================
 *  OVSDB communication functions
 * ===========================================================================
 */

/*
 * Connect to a OVSDB database
 */
int ovsdb_connect(void)
{
    struct sockaddr_un  addr;

    int fd = -1;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        WebcfgError("Unable to open OVSDB socket: %s\n", strerror(errno));
        goto error;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    char *sock_path = OVSDB_SOCK_PATH;

    if (strlen(sock_path) >= sizeof(addr.sun_path))
    {
        WebcfgError("Connect path too long!\n");
        goto error;
    }
    strcpy(addr.sun_path, sock_path);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        WebcfgError("Unable to connect to OVSDB: %s\n", strerror(errno));
        goto error;
    }

    return fd;

error:
    if (fd >= 0) close(fd);

    return -1;
}

void ovsdb_close(int fd)
{
    close(fd);
}

/*
 * OVSDB Select method
 */
bool ovsdb_select(char *table, json_t *where, int coln, char *colv[],
                  json_t **jresult, json_t **acolumns)
{
    *jresult = NULL;
    json_t *columns = json_array();
    *acolumns = columns;

    json_t *jparam = json_pack("[ s, { s:s, s:s, s:o } ]",
                               ovsdb_opt_db,
                               "op", "select",
                               "table", table,
                               "where", where);
    if (jparam == NULL)
    {
        WebcfgError("Error creating JSON-RPC parameters (SELECT).\n");
        return false;
    }
    if (coln > 0)
    {
        /* Parse columns */
        if (!ovsdb_parse_columns(columns, coln, colv))
        {
            WebcfgError("Error parsing columns. \n");
        }

        if (json_object_set_new(json_array_get(jparam, 1), "columns", columns) != 0)
        {
            WebcfgError("Error appending columns.\n");
            return false;
        }
    }
    json_t *jres = ovsdb_json_exec("transact", jparam);
    if (!ovsdb_json_error(jres))
    {
        return false;
    }
    *jresult = jres;
    return true;
}

/*
 * This functions splits a string into a left value, an operator and a right value. It can be used to
 * parse structures as a=b, or a!=b etc.
 *
 * This function modifies the string expr
 * Param op is taken from the "delim" array
 */
static bool str_parse_expr(char *expr, char *delim[], char **lval, char **op, char **rval)
{
    char *pop;
    char **pdelim;

    for (pdelim = delim; *pdelim != NULL; pdelim++)
    {
        /* Find the delimiter string */
        pop = strstr(expr, *pdelim);
        if (pop == NULL) continue;

        *op = *pdelim;

        *lval = expr;
        *rval = pop + strlen(*pdelim);
        *pop = '\0';

        return true;
    }

    /* Error parsing */
    return false;
}


/*
 * Check if an error was reported from OVSDB, if so, display it and return false. Return true otherwise.
 */
bool ovsdb_json_error(json_t *jobj)
{
    json_t *jres = json_object_get(jobj, "result");
    if (jres == NULL)
    {
        WebcfgError("Error: No \"result\" object is present.\n");
        return false;
    }

    /* Use the first object in the array */
    jres = json_array_get(jres, 0);

    json_t *jerror = json_object_get(jres, "error");
    if (jerror == NULL)
    {
        /* No error, return */
        return true;
    }
    WebcfgError("Error: %s\n", json_string_value(jerror));

    json_t *jdetails = json_object_get(jres, "details");
    if (jdetails != NULL)
    {
        WebcfgError("Details: %s\n", json_string_value(jdetails));
    }

    json_t *jsyntax = json_object_get(jres, "syntax");
    if (jsyntax != NULL)
    {
        WebcfgError("Syntax: %s\n", json_string_value(jsyntax));
    }

    return false;
}

/*
 * Return true if string is encased in quotation marks, either ' or "
 */
bool str_is_quoted(char *str)
{
    size_t len = strlen(str);

    if ((str[0] == '"' && str[len - 1] == '"') ||
            (str[0] == '\'' && str[len - 1] == '\''))
    {
        return true;
    }

    return false;
}

/*
 * Guess the type of "str" and return a JSON object of the corresponding type.
 *
 * If str is:
 *      "\"...\"" -> Everything that starts with a quote and ends in a quote is assumed to be a string.
 *                   The resulting object is the value without quotes
 *      "true" -> bool, value set to true
 *      "false" -> bool, value set to false
 *      "XXXX" -> where X are digits only -> integer
 *      ["map", -> object
 *      ["set", -> object
 *      ["uuid", -> object
 *
 *      Everything else is assumed to be a string.
 */
json_t *json_value(char *str)
{
    if (str[0] == '\0') return json_string("");

    /*
     * Check for quoted string first
     */
    if (str_is_quoted(str))
    {
        return json_stringn(str + 1, strlen(str) - 2);
    }

    if (strcmp(str, "true") == 0)
    {
        return json_true();
    }
    else if (strcmp(str, "false") == 0)
    {
        return json_false();
    }

    /*
     * Try to convert it to an integer
     */
    char *pend;
    json_int_t lval = strtoll(str, &pend, 0);

    /* All character were consumed during conversion -- it's an int */
    if (*pend == '\0')
    {
        return json_integer(lval);
    }
    // check if object
    char *str_map = "[\"map\",";
    char *str_set = "[\"set\",";
    char *str_uuid = "[\"uuid\",";
    if (!strncmp(str, str_map, strlen(str_map))
            || !strncmp(str, str_set, strlen(str_set))
            || !strncmp(str, str_uuid, strlen(str_uuid)))
    {
        // Try to parse rval as a json object
        json_t *jobj = json_loads(str, 0, NULL);
        if (jobj) {
            return jobj;
        }
        WebcfgError("Error decoding as object value: %s\n", str);
        // fallthrough, treat as string for backward compatibility, eventually treat as error
    }

    /* Everything else is a string */
    return json_string(str);
}

/*
 * Parse a list of columns, generate an array object out of it
 */
static bool ovsdb_parse_columns(json_t *columns, int colc, char *colv[])
{
    int ii;

    json_t *jrval = NULL;

    for (ii = 0; ii < colc; ii++)
    {
        char    col[OVSDB_COL_STR];

        /*
         * Use only the "left" value of an expression
         */
        char *lval, *op, *rval;

        static char *delim[] =
        {
            ":=",
            "::",
            "~=",
            NULL
        };

        if (strlen(colv[ii]) + 1 > sizeof(col))
        {
            WebcfgError("Column too big: %s\n", colv[ii]);
            return false;
        }
        strcpy(col, colv[ii]);
        if (!str_parse_expr(col, delim, &lval, &op, &rval))
        {
            /*Not an expression, use whole line as left value */
            lval = colv[ii];
            rval = NULL;
        }
        else
        {
            /* Depending on the operator, do different things with rval */
            if (strcmp(op, ":=") == 0)
            {
                /* := standard right value processing */
                jrval = json_value(rval);
            }
            else if (strcmp(op, "::") == 0)
            {
                /* Try to parse rval as a json object */
                jrval = json_loads(rval, 0, NULL);
                if (jrval == NULL)
                {
                    WebcfgError("Error decoding column right value: %s\n", rval);
                    return false;
                }
            }
            else if (strcmp(op, "~=") == 0)
            {
                /* Force right value to string */
                jrval = json_string(rval);
            }
            else
            {
                WebcfgError("Error decoding column operator: %s\n", op);
                return false;
            }
        }

        if (json_is_array(columns))
        {
            /* Append left value only */
            if (json_array_append_new(columns, json_string(lval)) != 0)
            {
                goto error;
            }
        }
        else if (json_is_object(columns) && rval != NULL)
        {
            if (json_object_set_new(columns, lval, jrval) != 0)
            {
                goto error;
            }
        }
        else
        {
            WebcfgError("Parameter is not an object or array.\n");
            goto error;
        }

        jrval = NULL;
    }
    return true;

error:
    if (jrval != NULL) json_decref(jrval);
    return false;
}

bool ovsdb_json_get_result_rows(json_t *jobj, json_t **jrows)
{
    *jrows = NULL;

    json_t *jres = json_object_get(jobj, "result");
    if (jres == NULL)
    {
        WebcfgError("Error: No \"result\" object is present.\n");
        return false;
    }

    /* Use the first object in the array */
    jres = json_array_get(jres, 0);
    if (jres == NULL)
    {
        WebcfgError("Unexpected JSON result: Got empty array.\n");
        return false;
    }

    *jrows = json_object_get(jres, "rows");
    if (*jrows == NULL)
    {
        WebcfgError("Error: Expected \"rows\" object in response.\n");
        return false;
    }
    return true;
}

/*
 * Execute a JSON RPC command, close the connection and return
 */
json_t *ovsdb_json_exec(char *method, json_t *params)
{
    char buf[2048];

    char   *str = NULL;
    json_t *jres = NULL;
    int     db = -1;

    json_t *jexec = json_pack("{ s:s, s:o, s:i }", "method", method, "params", params, "id", getpid());

    str = json_dumps(jexec, 0);
    WebcfgDebug(">>>>>>> %s\n", str);
    db = g_ovsdb_handle;

    ssize_t len = strlen(str);
    if (write(db, str, len) < len)
    {
        WebcfgError("Error writing to OVSDB: %s\n", strerror(errno));
        goto error;
    }

    ssize_t nrd;
    nrd = read(db, buf, sizeof(buf));
    if (nrd < 0)
    {
        goto error;
    }

    buf[nrd] = '\0';

    WebcfgDebug("<<<<<<< %s\n", buf);

    jres = json_loads(buf, JSON_PRESERVE_ORDER, NULL);

error:
    if (str != NULL) free(str);
    if (db > 0) ovsdb_close(db);

    return jres;
}

bool ovsdb_parse_where(json_t *where, char *_str, bool is_parent_where)
{
    bool retval = false;
    char str[OVSDB_COL_STR];

    if (strlen(_str) + 1 > sizeof(str))
    {
        /* String too long */
        return false;
    }

    strncpy(str, _str, sizeof(str));

    static char *where_delims[] =
    {
        "==",
        "!=",
        ":inc:",
        ":exc:",
        NULL,
    };

    char *lval, *op, *rval;
    if (!str_parse_expr(str, where_delims, &lval, &op, &rval))
    {
        WebcfgError("Error parsing expression: %s\n", _str);
        return false;
    }

    /* Convert ovsh opt o ovsdb op */
    if (strcmp(op, ":inc:") == 0)
    {
        op = "includes";
    }
    else if (strcmp(op, ":exc:") == 0)
    {
        op = "excludes";
    }
    else
    {
        /* Other ovsh operators (where_delims) match 1:1 to ovsdb operators */
    }

    /*
     * Create a JSON array from values
     */
    json_t *jop = json_array();
    if (jop == NULL)
    {
        WebcfgError("Unable to create JSON array\n");
        return false;
    }

    if (json_array_append_new(jop, json_string(lval)) != 0)
    {
        WebcfgError("Unable to append JSON array (lval)\n");
        goto error;
    }

    if (json_array_append_new(jop, json_string(op)) != 0)
    {
        WebcfgError("Unable to append JSON array (op)\n");
        goto error;
    }

    if (json_array_append_new(jop, json_value(rval)) != 0)
    {
        WebcfgError("Unable to append JSON array (rval)\n");
        goto error;
    }

    if (json_array_append(where, jop) != 0)
    {
        WebcfgError("Unable to append JSON array (where)\n");
        goto error;
    }

    retval = true;

    /* Append to global where expr table, but only if this is a regular where
     * statement (not part of a --parent argument)  */
    if (!is_parent_where)
    {
        ovsdb_where_num++;
        ovsdb_where_expr = (char**)realloc(ovsdb_where_expr, sizeof(char**) * ovsdb_where_num);
        ovsdb_where_expr[ovsdb_where_num-1] = strdup(_str);
    }

error:
    if (jop != NULL) json_decref(jop);

    return retval;
}

bool ovsdb_string_value_get(char *table,json_t *where, int coln, char * colv[],char *result_str, unsigned int len )

{
    int handle = -1;
    bool ret = false;
    json_t *jres = NULL;
    json_t *columns = NULL;
    json_t *jrows = NULL;
    json_t *result = NULL;

    handle = ovsdb_util_init();
    if (handle < 0) {
        goto exit;
    }
    if (!ovsdb_select(table, where, coln, colv, &jres, &columns)) {
        WebcfgError("ovsdb_select failed.\n");
        goto exit;
    }
    if (!ovsdb_json_get_result_rows(jres, &jrows)) {
        WebcfgError("ovsdb_json_get_result_rows failed.\n");
        goto exit;
    }

    if (jrows != NULL || (json_array_size(jrows) > 0)) {
        result = json_object_get(json_array_get(jrows,0),colv[0]);
        if (result)
        {
            if (json_is_string(result)) {
                strncpy(result_str,json_string_value(result),len);
                ret = true;
            } else if (json_is_integer(result)) {
                snprintf(result_str,len, "%lld",json_integer_value(result));
                ret = true;
            } else {
                WebcfgError(" Un supported value type.\n");
            }
        }
        else {
            WebcfgError("JROWS is null or Json Array size is less than 0\n");
        }
    }
exit :
        ovsdb_util_fini();
        return ret;
}
