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
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <rbus-core/rbus_core.h>
#include <rbus-core/rbus_session_mgr.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <cjson/cJSON.h>
#include <jansson.h>
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
#define FIRMW_END_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeEndTime" 

#if defined(_COSA_BCM_MIPS_)
#define DEVICE_MAC                   "Device.DPoE.Mac_address"
#elif defined(PLATFORM_RASPBERRYPI)
#define DEVICE_MAC                   "Device.Ethernet.Interface.5.MACAddress"
#elif defined(RDKB_EMU)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#elif defined(_HUB4_PRODUCT_REQ_)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#else
#define DEVICE_MAC                   "Device.X_CISCO_COM_CableModem.MACAddress"
#endif

#define WEBCFG_RFC_PARAM "Device.X_RDK_WebConfig.RfcEnable"
#define PARAM_RFC_ENABLE "eRT.com.cisco.spvtg.ccsp.webpa.WebConfigRfcEnable"
#define WEBCFG_URL_PARAM "Device.X_RDK_WebConfig.URL"
#define WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM  "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry"
#define WEBCFG_PARAM_SUPPLEMENTARY_SERVICE   "Device.X_RDK_WebConfig.SupplementaryServiceUrls."
#define SYSTEM_READY_PARM "Device.CR.SystemReady"

#define WEBCFG_MAX_PARAM_LEN 128

#define WEBCFG_CJSON_FILE "/etc/partners_defaults_webcfg_video.json"

#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

#define WEBCONFIG_CONFIG_PS_FILE MAKE_STR(PS_FILE_PATH) WEBCFG_CJSON_FILE

#define WEBCFG_URL_FILE "/opt/webcfg_url"

#define RETURN_OK 0
#define RETURN_ERR -1

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
bool json_string_value_get(char *key, char* value_str, size_t len);
bool json_string_value_set(char *key, char* value_str);
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

cJSON* parse_json_file()
{
    long len = 0;
    cJSON *json;
    char *JSON_content;

    FILE* fp = fopen(WEBCFG_CJSON_FILE, "rb");
    if(!fp)
    {
        printf("open file %s failed.\n", WEBCFG_CJSON_FILE);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if(0 == len)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    JSON_content = (char*) malloc(sizeof(char) * len);
    fread(JSON_content, 1, len, fp);
    
    json = cJSON_Parse(JSON_content);
    if (json == NULL)
    {
        printf("jsoni file - parse null\n");
        return NULL;
    }

    fclose(fp);
    return json;
}

bool json_string_value_get(char *key, char* value_str, size_t len)
{
        json_t *json;
        json_error_t error;
        json_t *value;
        bool ret = false;

        json = json_load_file(WEBCFG_URL_FILE, 0, &error);
        if(!json) 
        {
                WebcfgError("Failed to load %s",WEBCFG_URL_FILE);
                return ret;
        }

        value = json_object_get(json, key);
        if (value)
        {
                cpeabStrncpy(value_str,json_string_value(value),len);
                WebcfgInfo("%s : Fetched [%s] = [%s]. \n",__func__,key,value_str);
                ret = true;
        }
        else
        {
                WebcfgError("Json value is Null.\n");
        }
        json_decref(json);
        return ret;
}

bool json_string_value_set(char *key, char* value_str)
{
        json_t *json;
        json_error_t error;
        json_t *value_json;
        bool ret = false;

        json = json_load_file(WEBCFG_URL_FILE, 0, &error);
        if(!json)
	{
                WebcfgError("Failed to load %s",WEBCFG_URL_FILE);
                return ret;
        }

        value_json = json_string(value_str);

        if (json_object_set(json, key, value_json) == RETURN_OK)
        {
                WebcfgInfo("%s : Successfully set : [%s] = [%s] \n",__func__,key,value_str);
                if (json_dump_file(json,WEBCFG_URL_FILE,0) == RETURN_OK )
                {
                        WebcfgInfo("Successfully Written to file. \n");
                        ret = true;
                }
        }
        else
        {
                WebcfgError("Json value is Null.\n");
        }
        json_decref(value_json);
        json_decref(json);
        return ret;
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

int opt_file_cpy()
{
	char *partner_id = NULL;
        partner_id=getPartnerID();
	
	cJSON *parser=parse_json_file();
	cJSON *item = cJSON_GetObjectItem(parser,partner_id);
        if(item!=NULL){
          printf(" %s print items of partner id\n",__func__);
          char *str=cJSON_Print(item);
          printf("Data in partner id =%s\n",str);
        }

        FILE* fpw = fopen(WEBCFG_URL_FILE, "wb");
        if (fpw == NULL) {
            printf("Failed to open file- %s",WEBCFG_URL_FILE);
            return -1;
        }
        //write the value to opt file
        char *cjValue = cJSON_Print(item);
        int wret = fwrite(cjValue, sizeof(char), strlen(cjValue), fpw);
        if (wret == 0) {
            printf("Failed to write in file- %s",WEBCFG_URL_FILE);
            return -1;
         }
         fclose(fpw);
         free(cjValue);
	return 0;
}

int Get_Webconfig_URL( char *pString)
{
        char url[128];
        int ret = RETURN_ERR;

        memset(url,0,sizeof(url));
	
        FILE *file;
        if((file = fopen(WEBCFG_URL_FILE,"r"))==NULL)
        {
        //opt file doesn't exist, so fetch data from json file
		int f_ret=opt_file_cpy();
		if(f_ret !=0 )
			return ret;
	}
        if (json_string_value_get(WEBCFG_URL_PARAM,url, sizeof(url)))
        {
                 cpeabStrncpy(pString,url,sizeof(url));
                 WebcfgDebug("Successfully fetched Webconfig URL : [%s]. \n", url);
                 ret = RETURN_OK;
        }
        else{
                 WebcfgError("Error! Failed to fetch Webconfig URL\n");
        } 
        return ret;
}

int Set_Webconfig_URL( char *pString)
{
        int ret = RETURN_ERR;
	
        FILE *file;
        if((file = fopen(WEBCFG_URL_FILE,"r"))==NULL)
        {
        //opt file doesn't exist, so fetch data from json file
		int f_ret=opt_file_cpy();
		if(f_ret !=0 )
			return ret;
	}

         if (json_string_value_set(WEBCFG_URL_PARAM, pString) == true)
         {
                 WebcfgDebug("Successfully set Webconfig URL \n");
                 ret = RETURN_OK;
         }
         else{
                 WebcfgError("Error! Failed to set Webconfig URL\n");
         }
         return ret;
}

int Get_Supplementary_URL( char *name, char *pString)
{
        char url[128];
        int ret = RETURN_ERR;

        memset(url,0,sizeof(url));

        FILE *file;
        if((file = fopen(WEBCFG_URL_FILE,"r"))==NULL)
        {
        //opt file doesn't exist, so fetch data from json file
                int f_ret=opt_file_cpy();
                if(f_ret !=0 )
                        return ret;
        }

        if (json_string_value_get(WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,url, sizeof(url)))
        {
                WebcfgDebug("Successfully fetched Webconfig Supp URL : [%s] \n", url);
                cpeabStrncpy(pString,url,sizeof(url));
                ret = RETURN_OK;
        }
        else{
                WebcfgError("Error! Failed to fetch Webconfig Supp URL\n");
        }
        return ret;
}

int Set_Supplementary_URL( char *name, char *pString)
{
        int ret = RETURN_ERR;

        FILE *file;
        if((file = fopen(WEBCFG_URL_FILE,"r"))==NULL)
        {
        //opt file doesn't exist, so fetch data from json file
                int f_ret=opt_file_cpy();
                if(f_ret !=0 )
                        return ret;
        }

        if (json_string_value_set(WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM, pString) == true)
        {
                WebcfgDebug("Successfully set Webconfig URL \n");
                ret = RETURN_OK;
        }
        else{
                WebcfgError("Error! Failed to set Webconfig Supp URL\n");
        }
        return ret;
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

int rbus_StoreValueIntoDB(char *paramName, char *value)
{
        int ret = RETURN_ERR;

        if (strncmp(paramName,WEBCFG_RFC_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
        {
                if (json_string_value_set(WEBCFG_RFC_PARAM,value) == RETURN_OK)
                {
                        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,WEBCFG_RFC_PARAM,value);
                        ret = RETURN_OK;
                }
        }
        else if (strncmp(paramName,PARAM_RFC_ENABLE,WEBCFG_MAX_PARAM_LEN) == 0)
        {
        	WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,PARAM_RFC_ENABLE,value);
                ret = RETURN_OK;
        }
        else if (strncmp(paramName,WEBCFG_URL_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
        {
                if (Set_Webconfig_URL(value) == RETURN_OK)
                {
                        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,WEBCFG_URL_PARAM,value);
                        ret = RETURN_OK;
                }
        }
        else if (strncmp(paramName,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
        {
                if (Set_Supplementary_URL(NULL,value) == RETURN_OK)
                {
                        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,value);
                        ret = RETURN_OK;
                }
        }
        else {
                WebcfgError("Invalid Param Name (set)= %s\n",paramName);
        }
        return ret;
}

int rbus_GetValueFromDB( char* paramName, char** paramValue)
{
        char value_str[256];
        int ret = RETURN_ERR;

        memset(value_str,0,sizeof(value_str));

        if (strncmp(paramName,WEBCFG_RFC_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
        {
                if (json_string_value_get(WEBCFG_RFC_PARAM,value_str, sizeof(value_str)))
                {
                        *paramValue = strdup(value_str);
                        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,WEBCFG_RFC_PARAM,*paramValue);
                        ret = RETURN_OK;
                }
        }
        else if (strncmp(paramName,PARAM_RFC_ENABLE,WEBCFG_MAX_PARAM_LEN) == 0)
        {
                *paramValue = strdup("true");
                WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,PARAM_RFC_ENABLE,*paramValue);
                ret = RETURN_OK;
        }
        else if (strncmp(paramName,WEBCFG_URL_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)

        {
                if (Get_Webconfig_URL(value_str) == RETURN_OK)
                {
                        *paramValue = strdup(value_str);
                        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,WEBCFG_URL_PARAM,*paramValue);
                        ret = RETURN_OK;
                }
        }
        else if (strncmp(paramName,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
        {
                if (Get_Supplementary_URL(NULL,value_str) == RETURN_OK)
                {
                        *paramValue = strdup(value_str);
                        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,*paramValue);
                        ret = RETURN_OK;
                }
        }
        else
        {
                WebcfgError("Invalid Param Name (get)= %s\n",paramName);
        }
        return ret;
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
