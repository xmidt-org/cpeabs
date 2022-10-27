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
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <rbus/rbus.h>
#include "cpeabs.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SERIAL_NUMBER				"Device.DeviceInfo.SerialNumber"
#define FIRMWARE_VERSION			"Device.DeviceInfo.X_CISCO_COM_FirmwareName"
#define DEVICE_BOOT_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
#define MODEL_NAME				"Device.DeviceInfo.ModelName"
#ifdef WAN_FAILOVER_SUPPORTED
#define INTERFACE_NAME                          "Device.X_RDK_WanManager.CurrentActiveInterface"
#endif
#define PRODUCT_CLASS				"Device.DeviceInfo.ProductClass"
#define CONN_CLIENT_PARAM			"Device.X_RDK_Webpa.ConnectedClientNotify"
#define LAST_REBOOT_REASON			"Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason"
#define PARTNER_ID				"Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"
#define ACCOUNT_ID				"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
#define FIRMW_START_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeStartTime"
#define FIRMW_END_TIME			        "Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeEndTime" 

#if defined(_COSA_BCM_MIPS_)
#define DEVICE_MAC                   "Device.DPoE.Mac_address"
#elif defined(PLATFORM_RASPBERRYPI)
#define DEVICE_MAC                   "Device.Ethernet.Interface.5.MACAddress"
#elif defined(RDKB_EMU)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#elif defined(_HUB4_PRODUCT_REQ_)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#elif defined(_WNXL11BWL_PRODUCT_REQ_)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#else
#define DEVICE_MAC                   "Device.X_CISCO_COM_CableModem.MACAddress"
#endif

#define WEBCFG_URL_PARAM "Device.X_RDK_WebConfig.URL"
#define WEBCFG_PARAM_SUPPLEMENTARY_SERVICE   "Device.X_RDK_WebConfig.SupplementaryServiceUrls."
#define SYSTEM_READY_PARM "Device.CR.SystemReady"
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
static void systemReadyEventHandler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);
static void subscribeSystemReadyEvent();
static int rbus_checkIfSystemReady();
static int webcfgRbusRegisterWithCR();
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

#ifdef WAN_FAILOVER_SUPPORTED
char * getInterfaceName()
{
        char *interfaceName = NULL;
        interfaceName = getParamValue(INTERFACE_NAME);
        WebcfgDebug("interfaceName returned from lib is %s\n", interfaceName);
        return interfaceName;
}
#endif

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

int rbus_waitUntilSystemReady()
{
	int ret = 0;
	int fd;

	if(rbus_checkIfSystemReady())
	{
		WebcfgInfo("Checked CR - System is ready, proceed with webconfig startup\n");
		if((fd = creat("/var/tmp/webcfgcacheready", S_IRUSR | S_IWUSR)) == -1)
		{
			WebcfgError("/var/tmp/webcfgcacheready file creation failed with error:%d\n", errno);
		}
		else
		{
			close(fd);
		}
	}
	else
	{
		webcfgRbusRegisterWithCR();
		subscribeSystemReadyEvent();

		FILE *file;
		int wait_time = 0;
		int total_wait_time = 0;

		//Wait till Call back touches the indicator to proceed further
		while((file = fopen("/var/tmp/webcfgcacheready", "r")) == NULL)
		{
			WebcfgInfo("Waiting for system ready signal\n");
			//After waiting for 24 * 5 = 120s (2mins) send message to CR to query for system ready
			if(wait_time == 24)
			{
				wait_time = 0;
				if(rbus_checkIfSystemReady())
				{
				    WebcfgInfo("Checked CR - System is ready\n");
				    if((fd = creat("/var/tmp/webcfgcacheready", S_IRUSR | S_IWUSR)) == -1)
				    {
				       WebcfgError("/var/tmp/webcfgcacheready file creation failed, error:%d\n", errno);
				    }
				    else
				    {
				       close(fd);
				    }
				    break;
				}
				else
				{
				    WebcfgInfo("Queried CR for system ready after waiting for 2 mins, it is still not ready\n");
				    if(total_wait_time >= 84)
				    {
					    WebcfgInfo("Queried CR for system ready after waiting for 7 mins, it is still not ready. Proceeding ...\n");
                                            ret = 1;
					    break;
				    }
				}
			}
			sleep(5);
			wait_time++;
			total_wait_time++;
		};
		// In case of WebConfig restart, we should be having cacheready already touched.
		if(file != NULL)
		{
			WebcfgInfo("/var/tmp/webcfgcacheready file exists, proceed with webconfig start up\n");
			fclose(file);
		}
	}
        return ret;
}

static void subscribeSystemReadyEvent()
{
	int rc = RBUS_ERROR_SUCCESS;
	rbusHandle_t rbus_handle;

	rbus_handle = get_global_rbus_handle();
	if(!rbus_handle)
	{
		WebcfgError("subscribeSystemReadyEvent failed as rbus_handle is empty\n");
		return;
	}

	rc = rbusEvent_Subscribe(rbus_handle,SYSTEM_READY_PARM,systemReadyEventHandler,"webconfig",0);
	if(rc != RBUS_ERROR_SUCCESS)
		WebcfgError("systemready rbusEvent_Subscribe failed: %d, %s\n", rc, rbusError_ToString(rc));
	else
		WebcfgInfo("systemready rbusEvent_Subscribe was successful\n");
}

static void systemReadyEventHandler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
	(void)handle;
	(void)subscription;
	int eventValue = 0;
	rbusValue_t value = NULL;
	int fd;

	value = rbusObject_GetValue(event->data, "value");
	eventValue = (int) rbusValue_GetBoolean(value);
	WebcfgDebug("eventValue is %d\n", eventValue);
	if(eventValue)
	{
		if((fd = creat("/var/tmp/webcfgcacheready", S_IRUSR | S_IWUSR)) == -1)
		{
			WebcfgError("Failed to create /var/tmp/webcfgcacheready file, error:%d\n", errno);
		}
		else
		{
			close(fd);
		}
		WebcfgInfo("Received system ready signal, created /var/tmp/webcfgcacheready file\n");
	}
}

/**
 * rbus_checkIfSystemReady Function to query CR and check if system is ready.
 * This is just in case webconfig registers for the systemReadySignal event late.
 * If SystemReadySignal is already sent then this will return 1 indicating system is ready.
 */
static int rbus_checkIfSystemReady()
{
	int rc = -1;
	rbusHandle_t rbus_handle;
	int sysVal = 0;

	rbus_handle = get_global_rbus_handle();
	if(!rbus_handle)
	{
		WebcfgError("rbus_checkIfSystemReady failed as rbus_handle is empty\n");
		return rc;
	}

	rbusValue_t value;
	rc = rbus_get(rbus_handle, SYSTEM_READY_PARM, &value);
	if(rc != RBUS_ERROR_SUCCESS)
        {
		WebcfgError("rbus_checkIfSystemReady failed with err %d: %s\n", rc, rbusError_ToString(rc));
		return rc;
	}
	else
	{
		WebcfgDebug("rbus_checkIfSystemReady returns %d\n", rbusValue_GetBoolean(value));
		sysVal = (int) rbusValue_GetBoolean(value);
		WebcfgDebug("sysVal is %d\n", sysVal);
        }
	rbusValue_Release(value);
	if(sysVal)
	{
		WebcfgDebug("SystemReady returns 1\n");
		return 1;
	}
	else
	{
		WebcfgDebug("SystemReady returns 0\n");
	}
	return rc;
}

/**
 * To register webconfig component with component Registery CR.
 */
static int webcfgRbusRegisterWithCR()
{
	rbusHandle_t rbus_handle;
	rbusObject_t inParams;
	rbusObject_t outParams;
	rbusValue_t value;
	int rc = RBUS_ERROR_SUCCESS;

	rbus_handle = get_global_rbus_handle();
	if(!rbus_handle)
	{
		WebcfgError("webcfgRbusRegisterWithCR failed as rbus_handle is empty\n");
		return 1;
	}

	rbusObject_Init(&inParams, NULL);

	rbusValue_Init(&value);
	rbusValue_SetString(value, "webconfig");
	rbusObject_SetValue(inParams, "name", value);
	rbusValue_Release(value);

	rbusValue_Init(&value);
	rbusValue_SetInt32(value, 1);
	rbusObject_SetValue(inParams, "version", value);
	rbusValue_Release(value);

	rc = rbusMethod_Invoke(rbus_handle, "Device.CR.RegisterComponent()", inParams, &outParams);
	rbusObject_Release(inParams);
	if(rc != RBUS_ERROR_SUCCESS)
	{
		WebcfgError("Device.CR.RegisterComponent failed with err %d: %s\n", rc, rbusError_ToString(rc));
	}
	else
	{
		WebcfgInfo("Device.CR.RegisterComponent is success\n");
		rbusObject_Release(outParams);
		return 0;
	}
	return 1;
}
