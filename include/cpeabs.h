/* SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __CPEABS_H__
#define __CPEABS_H__

#include <stdint.h>
#include <wdmp-c.h>

#if ! defined(DEVICE_EXTENDER)
#include <cimplog.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <rbus-core/rbus_core.h>
#include <rbus-core/rbus_session_mgr.h>
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
#if defined DEVICE_GATEWAY && defined BUILD_YOCTO

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

