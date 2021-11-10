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
#include <rbus-core/rbus_core.h>
#include <rbus-core/rbus_session_mgr.h>
#include "cpeabs.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SERIAL_NUMBER				"Device.DeviceInfo.SerialNumber"
#define FIRMWARE_VERSION			"Device.DeviceInfo.X_CISCO_COM_FirmwareName"
#define DEVICE_BOOT_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
#define MODEL_NAME				"Device.DeviceInfo.ModelName"
#define PRODUCT_CLASS				"Device.DeviceInfo.ProductClass"
#define CONN_CLIENT_PARAM			"Device.NotifyComponent.X_RDKCENTRAL-COM_Connected-Client"
#define LAST_REBOOT_REASON			"Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason"
#define PARTNER_ID				"Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"
#define ACCOUNT_ID				"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
#define FIRMW_START_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeStartTime"
#define FIRMW_END_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeEndTime" 

#if defined(_COSA_BCM_MIPS_)
#define DEVICE_MAC                   "Device.DPoE.Mac_address"
#elif defined(PLATFORM_RASPBERRYPI)
#define DEVICE_MAC                   "Device.Ethernet.Interface.5.MACAddress"
#elif defined(RDKB_EMU)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#else
#define DEVICE_MAC                   "Device.X_CISCO_COM_CableModem.MACAddress"
#endif

#define WEBCFG_URL_PARAM "Device.X_RDK_WebConfig.URL"
#define WEBCFG_PARAM_SUPPLEMENTARY_SERVICE   "Device.X_RDK_WebConfig.SupplementaryServiceUrls."

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct
{
    char *paramName;
    char *paramValue;
    rbusValueType_t type;
} rbusParamVal_t;
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static bool isRbus = false ;
char deviceMAC[32]={'\0'};
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void __attribute__((weak)) getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus);
static bool isRbusEnabled();
void macIDToLower(char macValue[],char macConverted[]);
void cpeabStrncpy(char *destStr, const char *srcStr, size_t destSize);
rbusHandle_t  __attribute__((weak)) get_global_rbus_handle(void);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void cpeabStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
}

void macIDToLower(char macValue[],char macConverted[])
{
	int i = 0;
	int j;
	char *token[32];
	char tmp[32];
	cpeabStrncpy(tmp, macValue, sizeof(tmp));
	token[i] = strtok(tmp, ":");
	if(token[i]!=NULL)
	{
	    cpeabStrncpy(macConverted, token[i], 32);
	    i++;
	}
	while ((token[i] = strtok(NULL, ":")) != NULL)
	{
	    strncat(macConverted, token[i], 31);
            macConverted[31]='\0';
	    i++;
	}
	macConverted[31]='\0';
	for(j = 0; macConverted[j]; j++)
	{
	    macConverted[j] = tolower(macConverted[j]);
	}
}

char* get_deviceMAC()
{
	if(strlen(deviceMAC) != 0)
	{
		WebcfgDebug("deviceMAC returned %s\n", deviceMAC);
		return deviceMAC;
	}

	char *macID = NULL;
	char deviceMACValue[32] = { '\0' };
	macID = getParamValue(DEVICE_MAC);
	if (macID != NULL)
	{
	    cpeabStrncpy(deviceMACValue, macID, strlen(macID)+1);
	    macIDToLower(deviceMACValue, deviceMAC);
	    WebcfgDebug("deviceMAC: %s\n",deviceMAC);
	    CPEABS_FREE(macID);
	}
	WebcfgDebug("deviceMAC returned from lib is %s\n", deviceMAC);
	return deviceMAC;
}

char * getSerialNumber()
{
	char *serialNum = NULL;
	serialNum = getParamValue(SERIAL_NUMBER);
	WebcfgDebug("serialNum returned from lib is %s\n", serialNum);
	return serialNum;
}

char * getDeviceBootTime()
{
	char *bootTime = NULL;
	bootTime = getParamValue(DEVICE_BOOT_TIME);
	WebcfgDebug("bootTime returned from lib is %s\n", bootTime);
	return bootTime;
}

char * getProductClass()
{
	char *productClass = NULL;
	productClass = getParamValue(PRODUCT_CLASS);
	WebcfgDebug("productClass returned from lib is %s\n", productClass);
	return productClass;
}

char * getModelName()
{
	char *modelName = NULL;
	modelName = getParamValue(MODEL_NAME);
	WebcfgDebug("modelName returned from lib is %s\n", modelName);
	return modelName;
}

char * getFirmwareVersion()
{
	char *firmware = NULL;
	firmware = getParamValue(FIRMWARE_VERSION);
	WebcfgDebug("firmware returned from lib is %s\n", firmware);
	return firmware;
}

char * getConnClientParamName()
{
	return CONN_CLIENT_PARAM;
}

char * getRebootReason()
{
	char *reboot_reason = NULL;
	reboot_reason = getParamValue(LAST_REBOOT_REASON);
	WebcfgDebug("reboot_reason returned from lib is %s\n", reboot_reason);
	return reboot_reason;
}

char * getPartnerID()
{
	char *partnerId = NULL;
	partnerId = getParamValue(PARTNER_ID);
	WebcfgDebug("partnerId returned from lib is %s\n", partnerId);
	return partnerId;
}

char * getAccountID()
{
	char *accountId = NULL;
	accountId = getParamValue(ACCOUNT_ID);
	WebcfgDebug("accountId returned from lib is %s\n", accountId);
	return accountId;
}

char * getFirmwareUpgradeStartTime()
{
	char *FirmwareUpgradeStartTime = NULL;
	FirmwareUpgradeStartTime = getParamValue(FIRMW_START_TIME);
	WebcfgDebug("FirmwareUpgradeStartTime returned from lib is %s\n", FirmwareUpgradeStartTime);
	return FirmwareUpgradeStartTime;
}

char * getFirmwareUpgradeEndTime()
{
	char *FirmwareUpgradeEndTime = NULL;
	FirmwareUpgradeEndTime = getParamValue(FIRMW_END_TIME);
	WebcfgDebug("FirmwareUpgradeEndTime returned from lib is %s\n", FirmwareUpgradeEndTime);
	return FirmwareUpgradeEndTime;
}

bool isRbusEnabled()
{
	if(RBUS_ENABLED == rbus_checkStatus())
	{
		isRbus = true;
	}
	else
	{
		isRbus = false;
	}
	WebcfgDebug("Webconfig util RBUS mode active status = %s\n", isRbus ? "true":"false");
	return isRbus;
}

int Get_Webconfig_URL( char *pString)
{
	char *tempUrl = NULL;
	int retPsmGet = 0;
	if(isRbusEnabled())
	{
		retPsmGet = rbus_GetValueFromDB( WEBCFG_URL_PARAM, &tempUrl);
		WebcfgDebug("Get_Webconfig_URL. retPsmGet %d tempUrl %s\n", retPsmGet, tempUrl);
		if (retPsmGet == RBUS_ERROR_SUCCESS)
                {
			if(tempUrl !=NULL)
			{
				cpeabStrncpy(pString, tempUrl, strlen(tempUrl)+1);
			}
			WebcfgDebug("Get_Webconfig_URL. pString %s\n", pString);
		}
		else
                {
                        WebcfgError("psm_get failed ret %d for parameter %s\n", retPsmGet, WEBCFG_URL_PARAM);
                }
	}
	WebcfgDebug("Get_Webconfig_URL strong fn from lib\n");
	return retPsmGet;
}

int Set_Webconfig_URL( char *pString)
{
    int retPsmSet = 0;
    if(isRbusEnabled())
    {
    	retPsmSet = rbus_StoreValueIntoDB( WEBCFG_URL_PARAM, pString );
	WebcfgDebug("Set_Webconfig_URL. retPsmSet %d pString %s\n", retPsmSet, pString);

	if (retPsmSet != RBUS_ERROR_SUCCESS)
        {
		WebcfgError("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, WEBCFG_URL_PARAM, pString);
                return 0;
        }
        else
        {
		WebcfgDebug("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, WEBCFG_URL_PARAM, pString);
        }
    }
    return retPsmSet;
}

int Get_Supplementary_URL( char *name, char *pString)
{
	char *tempUrl = NULL;
	int retPsmGet = 0;
	if(isRbusEnabled())
	{
		char *tempParam = (char *) malloc (sizeof(char)*MAX_BUFF_SIZE);
		if(tempParam !=NULL)
		{
			snprintf(tempParam, MAX_BUFF_SIZE, "%s%s", WEBCFG_PARAM_SUPPLEMENTARY_SERVICE, name);
			WebcfgDebug("tempParam is %s\n", tempParam);
			retPsmGet = rbus_GetValueFromDB( tempParam, &tempUrl);
			if (retPsmGet == RBUS_ERROR_SUCCESS)
			{
				WebcfgDebug("Get_Supplementary_URL. retPsmGet %d tempUrl %s\n", retPsmGet, tempUrl);
				if(tempUrl !=NULL)
				{
					cpeabStrncpy(pString, tempUrl, strlen(tempUrl)+1);
				}
				WebcfgDebug("Get_Supplementary_URL. pString %s\n", pString);
				CPEABS_FREE(tempParam);
			}
			else
			{
				WebcfgError("psm_get failed ret %d for parameter %s\n", retPsmGet, tempParam);
				CPEABS_FREE(tempParam);
			}
		}
	}
	return retPsmGet;
}

int Set_Supplementary_URL( char *name, char *pString)
{
    int retPsmSet = 0;
    if(isRbusEnabled())
    {
	char *tempParam = (char *) malloc (sizeof(char)*MAX_BUFF_SIZE);
	if(tempParam !=NULL)
	{
		if ((name != NULL) && (strncmp(name, "Telemetry",strlen(name)+1)) == 0)
		{
			snprintf(tempParam, MAX_BUFF_SIZE, "%s%s", WEBCFG_PARAM_SUPPLEMENTARY_SERVICE, name);
			WebcfgDebug("tempParam is %s\n", tempParam);
			retPsmSet = rbus_StoreValueIntoDB( tempParam, pString );
			if (retPsmSet != RBUS_ERROR_SUCCESS)
			{
				WebcfgError("psm_set failed ret %d for parameter %s%s and value %s\n", retPsmSet, WEBCFG_PARAM_SUPPLEMENTARY_SERVICE, name, pString);
				CPEABS_FREE(tempParam);
				return 0;
			}
			else
			{
				WebcfgDebug("psm_set success ret %d for parameter %s%s and value %s\n",retPsmSet, WEBCFG_PARAM_SUPPLEMENTARY_SERVICE, name, pString);
			}
		}
		else
		{
			WebcfgError("Invalid supplementary doc name\n");
		}
		CPEABS_FREE(tempParam);
	}
    }
    return retPsmSet;
}

char * getParamValue(char *paramName)
{
	if(isRbusEnabled())
	{
		int paramCount=0;
		WDMP_STATUS ret = WDMP_FAILURE;
		int count=0;
		const char *getParamList[1];
		getParamList[0] = paramName;

		char *paramValue = (char *) malloc(sizeof(char)*64);
		paramCount = sizeof(getParamList)/sizeof(getParamList[0]);
		param_t **parametervalArr = (param_t **) malloc(sizeof(param_t *) * paramCount);

		WebcfgDebug("paramName : %s paramCount %d\n",getParamList[0], paramCount);
		getValues_rbus(getParamList, paramCount, 0, NULL, &parametervalArr, &count, &ret);

		if (ret == WDMP_SUCCESS )
		{
			cpeabStrncpy(paramValue, parametervalArr[0]->value,64);
			CPEABS_FREE(parametervalArr[0]->name);
			CPEABS_FREE(parametervalArr[0]->value);
			CPEABS_FREE(parametervalArr[0]);
		}
		else
		{
			WebcfgError("Failed to GetValue for %s\n", getParamList[0]);
			CPEABS_FREE(paramValue);
		}
		CPEABS_FREE(parametervalArr);
		WebcfgDebug("getParamValue : paramValue is %s\n", paramValue);
		return paramValue;
	}
	WebcfgError("getParamValue : returns NULL\n");
	return NULL;
}

/**
 * To persist TR181 parameter values in PSM DB.
 */
int rbus_StoreValueIntoDB(char *paramName, char *value)
{
	rbusHandle_t rbus_handle;
	rbusObject_t inParams;
	rbusObject_t outParams;
	rbusValue_t setvalue;
	int rc = RBUS_ERROR_SUCCESS;

	rbus_handle = get_global_rbus_handle();
	if(!rbus_handle)
	{
		WebcfgError("rbus_StoreValueIntoDB failed as rbus_handle is empty\n");
		return 1;
	}

	rbusObject_Init(&inParams, NULL);
	rbusValue_Init(&setvalue);
	rbusValue_SetString(setvalue, value);
	rbusObject_SetValue(inParams, paramName, setvalue);
	rbusValue_Release(setvalue);

	rc = rbusMethod_Invoke(rbus_handle, "SetPSMRecordValue()", inParams, &outParams);
	rbusObject_Release(inParams);
	if(rc != RBUS_ERROR_SUCCESS)
	{
		WebcfgError("SetPSMRecordValue failed with err %d: %s\n", rc, rbusError_ToString(rc));
	}
	else
	{
		WebcfgDebug("SetPSMRecordValue is success\n");
		rbusObject_Release(outParams);
		return 0;
	}
	return 1;
}

/**
 * To fetch TR181 parameter values from PSM DB.
 */
int rbus_GetValueFromDB( char* paramName, char** paramValue)
{
	rbusHandle_t rbus_handle;
	rbusObject_t inParams;
	rbusObject_t outParams;
	rbusValue_t setvalue;
	int rc = RBUS_ERROR_SUCCESS;

	rbus_handle = get_global_rbus_handle();
	if(!rbus_handle)
	{
		WebcfgError("rbus_GetValueFromDB failed as rbus_handle is empty\n");
		return 1;
	}

	rbusObject_Init(&inParams, NULL);
	rbusValue_Init(&setvalue);
	rbusValue_SetString(setvalue, "value");
	rbusObject_SetValue(inParams, paramName, setvalue);
	rbusValue_Release(setvalue);

	rc = rbusMethod_Invoke(rbus_handle, "GetPSMRecordValue()", inParams, &outParams);
	rbusObject_Release(inParams);
	if(rc != RBUS_ERROR_SUCCESS)
	{
		WebcfgError("GetPSMRecordValue failed with err %d: %s\n", rc, rbusError_ToString(rc));
	}
	else
	{
		WebcfgDebug("GetPSMRecordValue is success\n");
		rbusProperty_t prop = NULL;
		rbusValue_t value = NULL;
		char *str_value = NULL;
		prop = rbusObject_GetProperties(outParams);
		while(prop)
		{
			value = rbusProperty_GetValue(prop);
			if(value)
			{
				str_value = rbusValue_ToString(value,NULL,0);
				if(str_value)
				{
					WebcfgDebug("Parameter Name : %s\n", rbusProperty_GetName(prop));
					WebcfgDebug("Parameter Value fetched: %s\n", str_value);
				}
			}
			prop = rbusProperty_GetNext(prop);
		}
		if(str_value !=NULL)
		{
			*paramValue = strdup(str_value);
			CPEABS_FREE(str_value);
			WebcfgDebug("Requested param DB value [%s]\n", *paramValue);
		}
		rbusObject_Release(outParams);
		return 0;
	}
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

rbusHandle_t get_global_rbus_handle(void)
{
	return 0;
}

