/* SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __CPEABS_H__
#define __CPEABS_H__

#include <stdint.h>
#include <wdmp-c.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

char * getParamValue(char *paramName);
void getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus);
int rbus_GetValueFromDB( char* paramName, char** paramValue);
int rbus_StoreValueIntoDB(char *paramName, char *value);
int rbus_waitUntilSystemReady();
void cpeabs_free(void *ptr);
#endif

