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
#include <stdio.h>
#include <ctype.h>
#include <sys/sysinfo.h>
#include "cpeabs.h"
#include "cpeabs_ovsdb_utils.h"

#define BFR_SIZE_16  16
#define BFR_SIZE_32  32
#define BFR_SIZE_64  64
#define BFR_SIZE_128 128
#define BFR_SIZE_256 256

#define ACCOUNT_FILE "account"
#define WEBCFG_CFG_FILE "webcfg.json"

#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

#define OSP_PSTORE_ACCOUNT MAKE_STR(PS_FILE_PATH) ACCOUNT_FILE
#define WEBCONFIG_CONFIG_PS_FILE MAKE_STR(PS_FILE_PATH) WEBCFG_CFG_FILE

#define OSP_PSTORE_REBOOT "/var/run/osp_reboot_reason"
#define WEBCFG_RFC_PARAM "Device.X_RDK_WebConfig.RfcEnable"
#define PARAM_RFC_ENABLE "eRT.com.cisco.spvtg.ccsp.webpa.WebConfigRfcEnable"
#define WEBCFG_URL_PARAM "Device.X_RDK_WebConfig.URL"
#define WEBCFG_SUPPLEMENTARY_TELEMETRY_PARAM  "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry"

#define WEBCFG_MAX_PARAM_LEN 128
#define RETURN_OK 0
#define RETURN_ERR -1

/*----------------------------------------------------------------------------*/
char deviceMAC[BFR_SIZE_32];
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void macIDToLower(char macValue[],char macConverted[]);
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
void cpeabStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
}

bool json_string_value_get(char *key, char* value_str, size_t len)
{
        json_t *json;
        json_error_t error;
        json_t *value;
        bool ret = false;

        json = json_load_file(WEBCONFIG_CONFIG_PS_FILE, 0, &error);
        if(!json) 
        {
                WebcfgError("Failed to load %s",WEBCONFIG_CONFIG_PS_FILE);
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

        json = json_load_file(WEBCONFIG_CONFIG_PS_FILE, 0, &error);
        if(!json)
	{
                WebcfgError("Failed to load %s",WEBCONFIG_CONFIG_PS_FILE);
                return ret;
        }

        value_json = json_string(value_str);

        if (json_object_set(json, key, value_json) == RETURN_OK)
        {
                WebcfgInfo("%s : Successfully set : [%s] = [%s] \n",__func__,key,value_str);
                if (json_dump_file(json,WEBCONFIG_CONFIG_PS_FILE,0) == RETURN_OK )
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
        char *token[BFR_SIZE_32];
        char tmp[BFR_SIZE_32];

        cpeabStrncpy(tmp, macValue,sizeof(tmp));
        token[i] = strtok(tmp, ":");
        if(token[i]!=NULL)
        {
                cpeabStrncpy(macConverted, token[i],BFR_SIZE_32);
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

        json_t *where = json_array();
        char *table = "Wifi_Inet_State";
        char *colv[] = {"hwaddr"};
        char buff[BFR_SIZE_32];
        char *where_str = "if_name==eth0";

        if (!ovsdb_parse_where(where,where_str, false))
        {
                WebcfgError("Error parsing WHERE statement: %s",where_str);
        }
        memset(deviceMAC,0,sizeof(deviceMAC));
        memset(buff,0,sizeof(buff));
        WebcfgDebug("Reaching after memset of buff inside get devicemac\n");
        if (ovsdb_string_value_get(table,where,1,colv,buff,sizeof(buff)))
        {
                WebcfgDebug("Reaching after ovsdb_string_value_get success inside get devicemac\n");
                macIDToLower(buff, deviceMAC);
                WebcfgDebug("deviceMAC returned from lib in converted format is: %s\n",deviceMAC);
                return deviceMAC;
        }
        else
        {
                WebcfgError("Failed to GetValue for deviceMAC\n");
                return NULL;
        }
}

char * getSerialNumber()
{
        json_t *where = json_array();
        char *table = "AWLAN_Node";
        char *colv[] = {"serial_number"};
        char buff[BFR_SIZE_64];
        char *serialNum = NULL;
  
        serialNum = malloc(BFR_SIZE_64*sizeof(char));
        if (!serialNum)
        {
                WebcfgError("%s : serialNum couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        memset(buff,0,sizeof(buff));

        if (ovsdb_string_value_get(table,where,1,colv,buff,sizeof(buff)))
        {
                WebcfgDebug("serialNum returned from lib is %s\n", buff);
                cpeabStrncpy(serialNum,buff,BFR_SIZE_64*sizeof(char));
                return serialNum;
        }
        else
        {
                WebcfgError("Failed to GetValue for serialNum\n");
                return NULL;
        }
}

char * getDeviceBootTime()
{
        json_t *where = json_array();
        char *table = "AWLAN_Node";
        char *colv[] = {"boot_time"};
        char buff[BFR_SIZE_64];
        char *bootTime = NULL;

        bootTime = malloc(BFR_SIZE_64*sizeof(char));
        if (!bootTime)
        {
                WebcfgError("%s : bootTime couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        memset(buff,0,sizeof(buff));

        if (ovsdb_string_value_get(table,where,1,colv,buff,sizeof(buff)))
        {
                WebcfgDebug("bootTime returned from lib is %s\n", buff);
                cpeabStrncpy(bootTime,buff,BFR_SIZE_64*sizeof(char));
                return bootTime;
        }
        else
        {
                WebcfgError("Failed to GetValue for bootTime\n");
                return NULL;
        }
}

char * getProductClass()
{
        char *productClass = NULL;
        productClass = malloc(BFR_SIZE_16*sizeof(char));
        if (!productClass)
        {
                WebcfgError("%s : productClass couldnt be assigned with memory\n",__func__);
                return NULL;
        }
        cpeabStrncpy(productClass,"XE2",BFR_SIZE_16*sizeof(char));
        return productClass;
}

char * getModelName()
{
        json_t *where = json_array();
        char *table = "AWLAN_Node";
        char *colv[] = {"model"};
        char buff[BFR_SIZE_64];
        char *modelName = NULL;

        modelName = malloc(BFR_SIZE_64*sizeof(char));
        if (!modelName)
        {
                WebcfgError("%s : modelName couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        memset(buff,0,sizeof(buff));

        if (ovsdb_string_value_get(table,where,1,colv,buff,sizeof(buff)))
        {
                WebcfgDebug("modelName returned from lib is %s\n", buff);
                cpeabStrncpy(modelName,buff,BFR_SIZE_64*sizeof(char));
                return modelName;
        }
        else
        {
                WebcfgError("Failed to GetValue for modelName\n");
                return NULL;
        }
}

char * getFirmwareVersion()
{
        json_t *where = json_array();
        char *table = "AWLAN_Node";
        char *colv[] = {"firmware_version"};
        char buff[BFR_SIZE_64];
        char *firmware_version = NULL;

        firmware_version = malloc(BFR_SIZE_64*sizeof(char));
        if (!firmware_version)
        {
                WebcfgError("%s : firmware_version couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        memset(buff,0,sizeof(buff));

        if (ovsdb_string_value_get(table,where,1,colv,buff,sizeof(buff)))
        {
                WebcfgDebug("firmwareVersion returned from lib is %s\n", buff);
                cpeabStrncpy(firmware_version,buff,BFR_SIZE_64*sizeof(char));
                return firmware_version;
        }
        else
        {
                WebcfgError("Failed to GetValue for firmwareVersion\n");
                return NULL;
        }
}

char * getConnClientParamName()
{
        return NULL;
}

char * getRebootReason()
{
        FILE *rebptr = NULL;
        char reboot_type[BFR_SIZE_64];
        char reboot_str[BFR_SIZE_32];
        char reboot_osprsn[BFR_SIZE_128];
        char reboot_fileresp[BFR_SIZE_256];
        char * reboot_reason = NULL;

        reboot_reason = malloc(BFR_SIZE_64*sizeof(char));
        if (!reboot_reason)
        {
                WebcfgError("%s : reboot_reason couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        memset(reboot_type,0,sizeof(reboot_type));
        memset(reboot_str,0,sizeof(reboot_str));
        memset(reboot_fileresp, 0, sizeof(reboot_fileresp));
        memset(reboot_osprsn, 0, sizeof(reboot_osprsn));
        if ((rebptr = fopen(OSP_PSTORE_REBOOT, "r")) == NULL) 
        {
                WebcfgError("Error! File %s cannot be opened.\n", OSP_PSTORE_REBOOT);
                return NULL;
        }
        if( fgets(reboot_fileresp, sizeof(reboot_fileresp), rebptr) != NULL )
        {
                sscanf(reboot_fileresp, "%31s  %63s %127[^\n]", reboot_str, reboot_type, reboot_osprsn);
        }
        else
        {
                WebcfgError("Error! Output from file %s couldnt be parsed.\n", OSP_PSTORE_REBOOT);
                return NULL;
        }
        cpeabStrncpy(reboot_reason,reboot_type,BFR_SIZE_64*sizeof(char));
        if (strlen(reboot_reason) > 0)
        {
                WebcfgDebug("Reboot reason returned from lib is %s\n", reboot_reason);
                return reboot_reason;
        }
        else
        {
                WebcfgError("Failed to GetValue for reboot_reason\n");
                return NULL;
        }
}

char * cutting_delimiters(char *pstore_content, char * PATTERN1, char *PATTERN2)
{
        char *start, *end;
        char *target = NULL;
        if((start = strstr(pstore_content, PATTERN1)))
        {
                start += strlen(PATTERN1);
                if ((end = strstr(start, PATTERN2)))
                {
                        target = (char *)malloc(end-start+1);
                        memcpy(target,start,end-start);
                        target[end-start] = '\0';
                }
        }
        return target;
}

void get_id_pstore(int id_chk, char *id_type)
{
        char pstore_content[BFR_SIZE_256];
        char acc_id[BFR_SIZE_64];
        char partner_id[BFR_SIZE_64];
        char *acc_id_ret = NULL;
        char *partner_id_ret = NULL;

        FILE *fptr = NULL;
        if ((fptr = fopen(OSP_PSTORE_ACCOUNT, "r")) == NULL)
        {
                WebcfgError("Error! File %s cannot be opened.",OSP_PSTORE_ACCOUNT);
                return;
        }
        memset(pstore_content,0,sizeof(pstore_content));
        fscanf(fptr, "%[^\n]", pstore_content);
        WebcfgDebug("Data from the file:\n%s\n", pstore_content);
        fclose(fptr);

        memset(acc_id,0,sizeof(acc_id));
        memset(partner_id,0,sizeof(partner_id));

        if((strchr(pstore_content, ',') != NULL) && (strlen(pstore_content) > 0))
        {
                sscanf(pstore_content, "%*[^:]%*c%[^,]%*[^:]%*c%[^}]", acc_id, partner_id);
                acc_id_ret = cutting_delimiters(acc_id, "\"", "\"");
                partner_id_ret = cutting_delimiters(partner_id, "\"", "\"");
        }
        else if(strlen(pstore_content) > 0)
        {
                sscanf(pstore_content, "%*[^:]%*c%[^}]", acc_id);
                acc_id_ret = cutting_delimiters(acc_id, "\"", "\"");
        }
        else
        {
                WebcfgError("Error!The file is present but is empty!\n");
                return;
        }
        if ((id_chk == 1) && (strlen(partner_id) > 0))
        {
                cpeabStrncpy(id_type,partner_id_ret,BFR_SIZE_64*sizeof(char));
                return;
        }
        else
        {
                WebcfgError("Partner ID couldnt be parsed\n");
                return;
        }

        if ((id_chk == 0) && (strlen(acc_id) > 0))
        {
                cpeabStrncpy(id_type,acc_id_ret,BFR_SIZE_64*sizeof(char));
                return;
        }
        else
        {
                WebcfgError("Account ID couldnt be parsed\n");
                return;
        }

}
char * getPartnerID()
{
        int partner_id_chk = 1;
        char *partner_id = NULL;
 
  	partner_id = malloc(BFR_SIZE_64*sizeof(char));
        if (!partner_id)
        {
                WebcfgError("%s : partner_id couldnt be assigned with memory\n",__func__);
                return NULL;
        }
        get_id_pstore(partner_id_chk,partner_id);
        if (strlen(partner_id) > 0)
        {
                WebcfgDebug("PartnerID returned from lib is %s\n", partner_id);
                return partner_id;
        }
        else
        {
                WebcfgError("Failed to GetValue for PartnerID\n");
                return NULL;
        }
}

char * getAccountID()
{
        int acc_id_chk = 0;
        char *account_id = NULL;
   
        account_id = malloc(BFR_SIZE_64*sizeof(char));
        if (!account_id)
        {
                WebcfgError("%s : account_id couldnt be assigned with memory\n",__func__);
                return NULL;
        }
        get_id_pstore(acc_id_chk,account_id);
        if (strlen(account_id) > 0)
        {
                 WebcfgDebug("AccountID returned from lib is %s\n", account_id);
                 return account_id;
        }
        else
        {
                WebcfgError("Failed to GetValue for AccountID\n");
                return NULL;
        }
}

char * getFirmwareUpgradeStartTime()
{
        char * start_time = NULL;

	start_time = malloc(BFR_SIZE_32*sizeof(char));
        if (!start_time)
        {
                WebcfgError("%s : start_time couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        cpeabStrncpy(start_time,"7200",strlen("7200")+1);
        return start_time;
}

char * getFirmwareUpgradeEndTime()
{
        char * end_time = NULL;
        
	end_time = malloc(BFR_SIZE_32*sizeof(char));
        if (!end_time)
        {
                WebcfgError("%s : end_time couldnt be assigned with memory\n",__func__);
                return NULL;
        }

        cpeabStrncpy(end_time,"14400",strlen("14400")+1);
        return end_time;
}

int Get_Webconfig_URL( char *pString)
{
        char url[128];
        int ret = RETURN_ERR;

        memset(url,0,sizeof(url));

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

/**
 * To persist TR181 parameter values in PSM DB.
 */
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
                if (json_string_value_set(PARAM_RFC_ENABLE,value) == RETURN_OK)
                {
                        WebcfgDebug("%s : Successfully stored [%s] = [%s]. \n",__func__,PARAM_RFC_ENABLE,value);
                        ret = RETURN_OK;
                }
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
                WebcfgError("Invalid Param Name \n");
        }
        return ret;
}
/**
 * To fetch TR181 parameter values from PSM DB.
 */
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
                if (json_string_value_get(PARAM_RFC_ENABLE,value_str, sizeof(value_str)))
                {
                        *paramValue = strdup(value_str);
                        WebcfgDebug("%s : Successfully fetched [%s] = [%s]. \n",__func__,PARAM_RFC_ENABLE,*paramValue);
                        ret = RETURN_OK;
                }
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
                WebcfgError("Invalid Param Name \n");
        }
        return ret;
}

int rbus_waitUntilSystemReady()
{
        return RETURN_OK;
}
