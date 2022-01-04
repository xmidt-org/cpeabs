/* SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __CPEABS_RDKB_H__
#define __CPEABS_RDKB_H__

#include <stdint.h>
#include <wdmp-c.h>
#include <cimplog.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define UNUSED(x) (void )(x)
#define MAX_BUFF_SIZE 256

/**
 * @brief Enables or disables debug logs.
 */
#if defined BUILD_YOCTO

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
/* none */
#endif

