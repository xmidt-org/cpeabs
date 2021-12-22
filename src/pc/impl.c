/* SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <rbus-core/rbus_core.h>
#include <rbus-core/rbus_session_mgr.h>
#include "cpeabs.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
char * getParamValue(char *paramName)
{
	UNUSED(paramName);
	return NULL;
}

/**
 * To persist TR181 parameter values in DB.
 */
int rbus_StoreValueIntoDB(char *paramName, char *value)
{
	UNUSED(paramName);
	UNUSED(value);
	return 1;
}

/**
 * To fetch TR181 parameter values from DB.
 */
int rbus_GetValueFromDB( char* paramName, char** paramValue)
{
	UNUSED(paramName);
	UNUSED(paramValue);
	return 1;
}

void getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus)
{
	UNUSED(paramName);
	UNUSED(paramCount);
	UNUSED(index);
	UNUSED(timeSpan);
	UNUSED(paramArr);
	UNUSED(retValCount);
	UNUSED(retStatus);
	return;
}

