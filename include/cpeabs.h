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
#ifndef __CPEABS_H__
#define __CPEABS_H__

#include <stdint.h>
#include <wdmp-c.h>

#if ! defined(DEVICE_EXTENDER)
#include <cimplog.h>
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define CPEABS_FREE(__x__) if(__x__ != NULL) { free((void*)(__x__)); __x__ = NULL;} else {printf("Trying to free null pointer\n");}
#define UNUSED(x) (void )(x)
#define MAX_BUFF_SIZE 256

/**
 * @brief Enables or disables debug logs.
 */
#if defined DEVICE_GATEWAY && defined BUILD_YOCTO || defined DEVICE_VIDEO

#define WEBCFG_LOG_MODULE                     "WEBCONFIG"
#define WEBCFG_RDK_LOG_MODULE                 "LOG.RDK.WEBCONFIG"

#define WebConfigLog(...)       __cimplog_rdk_generic(WEBCFG_RDK_LOG_MODULE, WEBCFG_LOG_MODULE, LEVEL_INFO, __VA_ARGS__)

#define WebcfgError(...)	__cimplog_rdk_generic(WEBCFG_RDK_LOG_MODULE, WEBCFG_LOG_MODULE, LEVEL_ERROR, __VA_ARGS__)
#define WebcfgInfo(...)		__cimplog_rdk_generic(WEBCFG_RDK_LOG_MODULE, WEBCFG_LOG_MODULE, LEVEL_INFO, __VA_ARGS__)
#define WebcfgDebug(...)	__cimplog_rdk_generic(WEBCFG_RDK_LOG_MODULE, WEBCFG_LOG_MODULE, LEVEL_DEBUG, __VA_ARGS__)
#else
#define WebConfigLog(...)       printf(__VA_ARGS__)

#define WebcfgError(...)	printf(__VA_ARGS__)
#define WebcfgInfo(...)		printf(__VA_ARGS__)
#define WebcfgDebug(...)	printf(__VA_ARGS__)

#endif
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
#endif

