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
#include "cpeabs.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DEVICE_MAC          "Device.DeviceInfo.X_COMCAST-COM_STB_MAC"
#define MODEL_NAME          "Device.DeviceInfo.ModelName"
#define SERIAL_NUMBER       "Device.DeviceInfo.SerialNumber"
#define FIRMWARE_VERSION    "Device.DeviceInfo.X_RDK_FirmwareName"
#define DEVICE_BOOT_TIME    "Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
#define PRODUCT_CLASS       "Device.DeviceInfo.ProductClass"
#define LAST_REBOOT_REASON  "Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason"
#define PARTNER_ID          "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"
#define ACCOUNT_ID          "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"

#define WEBCFG_MAX_PARAM_LEN 128
#define WEBCFG_RFC_PARAM "Device.X_RDK_WebConfig.RfcEnable"
#define WEBCFG_URL_PARAM "Device.X_RDK_WebConfig.URL"
#define WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM  "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry"

#define PARAM_RFC_ENABLE "eRT.com.cisco.spvtg.ccsp.webpa.WebConfigRfcEnable"

#define WEBCFG_PARTNER_JSON_FILE "/etc/partners_defaults_webcfg_video.json"
#define WEBCFG_DB_STORE          "/opt/.webconfig.json"

#define DEF_WEB_URL "https://cpe-config.xdp.comcast.net/api/v1/device/{mac}/config"
#define DEF_SUPL_URL "https://cpe-profile.xdp.comcast.net/api/v1/device/{mac}/config"

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


typedef struct _webcfgValue
{
    char m_rfcStatus[16];
    char m_url[1024];
    char m_teleSuplUrl[1024];
} webCfgValue_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
char deviceMAC[32]={'\0'};
bool isInited = false;
webCfgValue_t webCfgPersist;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void __attribute__((weak)) getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus);
void macIDToLower(char macValue[],char macConverted[]);
void cpeabStrncpy(char *destStr, const char *srcStr, size_t destSize);
void load_partnerid_params();
void load_def_values();
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void cpeabStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    snprintf(destStr, destSize, "%s", srcStr);
}

cJSON* load_partner_json_file(const char* fileName)
{
    long len = 0;
    cJSON *json;
    char *JSON_content;

    FILE* fp = fopen(fileName, "rb");
    if(!fp)
    {
        WebcfgError("open file %s failed.\n", WEBCFG_PARTNER_JSON_FILE);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if(0 == len)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    JSON_content = (char*) malloc((len +1));
    fread(JSON_content, 1, len, fp);
    
    JSON_content[len-1] = '\0';
    json = cJSON_Parse(JSON_content);
    if (json == NULL)
    {
        WebcfgInfo("Json file - parse null\n");
        return NULL;
    }

    fclose(fp);
    return json;
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
    return NULL;
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
	return NULL;
}

char * getFirmwareUpgradeEndTime()
{
	return NULL;
}

char * getParamValue(char *paramName)
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

void writeToFile(char* pText)
{
    FILE* fpw = fopen(WEBCFG_DB_STORE, "wb");
    if (fpw == NULL) {
        WebcfgError("Failed to open file- %s",WEBCFG_DB_STORE);
        return;
    }
    int wret = fwrite(pText, 1, strlen(pText), fpw);
    if (wret == 0) {
        WebcfgError("Failed to write in file- %s", WEBCFG_DB_STORE);
        return;
    }
    fwrite("\n", 1, strlen("\n"), fpw);
    fclose(fpw);
}


cJSON * convertWebCfgDataToJson()
{
    cJSON *pWebCfg = cJSON_CreateObject();
    if (pWebCfg)
    {
        cJSON *pRfc = cJSON_CreateString(webCfgPersist.m_rfcStatus);
        cJSON *pUrl= cJSON_CreateString(webCfgPersist.m_url);
        cJSON *pTeleSuplUrl = cJSON_CreateString(webCfgPersist.m_teleSuplUrl);

        cJSON_AddItemToObject(pWebCfg, WEBCFG_URL_PARAM, pUrl);
        cJSON_AddItemToObject(pWebCfg, WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM, pTeleSuplUrl);
        cJSON_AddItemToObject(pWebCfg, WEBCFG_RFC_PARAM, pRfc);
        return pWebCfg;
    }
    return NULL;
}


void populatePersistenceData()
{
    if (access(WEBCFG_DB_STORE, F_OK) == 0)
    {
        cJSON *pParser = load_partner_json_file(WEBCFG_DB_STORE);
        if(!pParser)
        {
            WebcfgInfo("DB file present, but contents/parse empty. Regenerate data\n");
            load_partnerid_params();
        }
        else
        {
            cJSON *pUrl = cJSON_GetObjectItem(pParser, WEBCFG_URL_PARAM);
            if(!pUrl)
            {
                WebcfgInfo("WebUrl empty in db json file. Set default value\n");
                snprintf(webCfgPersist.m_url, 1024, "%s", DEF_WEB_URL);
            }
            else
            {
                WebcfgInfo("WebUrl present in db json file\n");
                snprintf(webCfgPersist.m_url, 1024, "%s",cJSON_GetStringValue(pUrl));
            }

            cJSON *pTeleSuplUrl = cJSON_GetObjectItem(pParser, WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM);
            if(!pTeleSuplUrl)
            {
                WebcfgInfo("Supl_url empty in db json file. Set default value\n");
                snprintf(webCfgPersist.m_teleSuplUrl, 1024, "%s", DEF_SUPL_URL);
            }
            else
            {
                WebcfgInfo("Sup_url present in db json file\n");
                snprintf(webCfgPersist.m_teleSuplUrl, 1024, "%s", cJSON_GetStringValue(pTeleSuplUrl));
            }

            cJSON *pRfc = cJSON_GetObjectItem(pParser, WEBCFG_RFC_PARAM);
            if(!pRfc)
            {
                WebcfgInfo("Rfc empty in db json file. Set default value\n");
                snprintf(webCfgPersist.m_rfcStatus, 16, "%s", "true");
            }
            else
            {
                WebcfgInfo("Rfc present in db json file\n");
                snprintf(webCfgPersist.m_rfcStatus, 16, "%s", cJSON_GetStringValue(pRfc));
            }
        }
    }
    else
    {
        WebcfgInfo("DB file is not present. Regenerate file & contents\n");
        load_partnerid_params();
    }
}


void load_partnerid_params()
{
    char *pPartnerId = NULL;
    pPartnerId = getPartnerID();
    if(!pPartnerId)
    {
        pPartnerId="default";
        WebcfgInfo("Partner id is default.\n");
    }
    cJSON *pParser = load_partner_json_file(WEBCFG_PARTNER_JSON_FILE);
    if(!pParser)
    {
        WebcfgInfo("Parsing from main json file is empty. Load default values\n");
        load_def_values();
        return;
    }
    cJSON *pItem = cJSON_GetObjectItem(pParser, pPartnerId);
    if(pItem)
    {
        cJSON *name = cJSON_CreateString("true");
        cJSON_AddItemToObject(pItem, WEBCFG_RFC_PARAM, name);
        char *pValue2 = cJSON_Print(pItem);
        if((pValue2) && (strlen(pValue2)))
        {
            WebcfgInfo("Parsing item/get item from parser\n");
            cJSON *pUrl = cJSON_GetObjectItemCaseSensitive(pItem, WEBCFG_URL_PARAM);
            cJSON *pTeleSuplUrl = cJSON_GetObjectItemCaseSensitive(pItem, WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM);

            snprintf(webCfgPersist.m_rfcStatus, 16, "%s", "true");
            snprintf(webCfgPersist.m_url, 1024, "%s", cJSON_GetStringValue(pUrl));
            snprintf(webCfgPersist.m_teleSuplUrl, 1024, "%s", cJSON_GetStringValue(pTeleSuplUrl));

            WebcfgInfo("Writing from json obj to db file.\n");
            writeToFile(pValue2);
        }
    }
    else
    {
        WebcfgInfo("pItem/get item from parser is empty. Load default values\n");
        load_def_values();
        return;
    }

}

void load_def_values()
{
    WebcfgInfo("Load default values to db json\n");
    cJSON *pItem=NULL;
    cJSON *name = cJSON_CreateString("true");
    cJSON_AddItemToObject(pItem, WEBCFG_RFC_PARAM, name);
    char *pValue2 = cJSON_Print(pItem);
    if((pValue2) && (strlen(pValue2)))
    {
        snprintf(webCfgPersist.m_rfcStatus, 16, "%s", "true");
        snprintf(webCfgPersist.m_url, 1024, "%s", DEF_WEB_URL);
        snprintf(webCfgPersist.m_teleSuplUrl, 1024, "%s", DEF_SUPL_URL);
        cJSON *pWebCfg = convertWebCfgDataToJson();
        if (pWebCfg)
        {
            char* pString = cJSON_Print(pWebCfg);
            WebcfgInfo("Writing default values to db file.\n");
            writeToFile(pString);
            cJSON_Delete(pWebCfg);
        }
    }
}

int Get_Webconfig_URL( char *pString)
{
    if (!isInited)
    {
        populatePersistenceData();
        isInited = true;
    }
    snprintf (pString, 1024, "%s", webCfgPersist.m_url);
    return RETURN_OK;
}

int Set_Webconfig_URL( char *pString)
{
    snprintf(webCfgPersist.m_url, 1024, "%s", pString);
    cJSON * pWebCfg = convertWebCfgDataToJson();
    if (pWebCfg)
    {
            char* pString = cJSON_Print(pWebCfg);
            writeToFile(pString);
            cJSON_Delete(pWebCfg);
    }

    return RETURN_OK;
}

int Get_Supplementary_URL( char *name, char *pString)
{
    if (!isInited)
    {
        populatePersistenceData();
        isInited = true;
    }
    (void) name;
    snprintf (pString, 1024, "%s", webCfgPersist.m_teleSuplUrl);
    return RETURN_OK;
}


int Set_Supplementary_URL( char *name, char *pString)
{
    (void) name;
    snprintf(webCfgPersist.m_teleSuplUrl, 1024, "%s", pString);
    cJSON * pWebCfg = convertWebCfgDataToJson();
    if (pWebCfg)
    {
            char* pString = cJSON_Print(pWebCfg);
            writeToFile(pString);
            cJSON_Delete(pWebCfg);
    }
    return RETURN_OK;
}


int rbus_StoreValueIntoDB(char *paramName, char *value)
{
    int ret = RETURN_ERR;

    if (strncmp(paramName,PARAM_RFC_ENABLE,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        snprintf(webCfgPersist.m_rfcStatus, 16, "%s", value);
        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,PARAM_RFC_ENABLE,value);
        ret = RETURN_OK;
    }
    else if (strncmp(paramName,WEBCFG_RFC_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        snprintf(webCfgPersist.m_rfcStatus, 16, "%s", value);
        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__, WEBCFG_RFC_PARAM, value);
        ret = RETURN_OK;
    }
    else if (strncmp(paramName,WEBCFG_URL_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        snprintf(webCfgPersist.m_url, 1024, "%s", value);
        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,WEBCFG_URL_PARAM,value);
        ret = RETURN_OK;
    }
    else if (strncmp(paramName,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        snprintf(webCfgPersist.m_teleSuplUrl, 1024, "%s", value);
        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,value);
        ret = RETURN_OK;
    }
    else
    {
        WebcfgError("Invalid Param Name (set)= %s\n",paramName);
        goto error2;
    }

    cJSON * pWebCfg = convertWebCfgDataToJson();
    if (pWebCfg)
    {
            char* pString = cJSON_Print(pWebCfg);
            writeToFile(pString);
            cJSON_Delete(pWebCfg);
    }


error2:
    return ret;
}

int rbus_GetValueFromDB( char* paramName, char** paramValue)
{
    char value_str[256];
    int ret = RETURN_ERR;

    if (!isInited)
    {
        populatePersistenceData();
        isInited = true;
    }

    memset(value_str,0,sizeof(value_str));
    if (strncmp(paramName,WEBCFG_RFC_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        *paramValue = strdup(webCfgPersist.m_rfcStatus);
        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,WEBCFG_RFC_PARAM,*paramValue);
        ret = RETURN_OK;
    }
    else if (strncmp(paramName,PARAM_RFC_ENABLE,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        *paramValue = strdup(webCfgPersist.m_rfcStatus);
        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__, PARAM_RFC_ENABLE, *paramValue);
        ret = RETURN_OK;
    }
    else if (strncmp(paramName,WEBCFG_URL_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        *paramValue = strdup(webCfgPersist.m_url);
        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,WEBCFG_URL_PARAM,*paramValue);
        ret = RETURN_OK;
    }
    else if (strncmp(paramName,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,WEBCFG_MAX_PARAM_LEN) == 0)
    {
        *paramValue = strdup(webCfgPersist.m_teleSuplUrl);
        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM,*paramValue);
        ret = RETURN_OK;
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

int rbus_waitUntilSystemReady()
{
    int ret = RETURN_OK;
    int fd;

    WebcfgInfo("Checked CR - System is ready, proceed with webconfig startup\n");
    if((fd = creat("/var/tmp/webcfgcacheready", S_IRUSR | S_IWUSR)) == -1)
    {
        WebcfgError("/var/tmp/webcfgcacheready file creation failed with error:%d\n", errno);
    }
    else
    {
        close(fd);
    }

    return ret;
}
