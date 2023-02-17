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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
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
char* get_deviceWanMAC()
{
	//TODO:return the wan mac value
	return NULL;
}

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

