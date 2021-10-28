#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <rbus/rbus.h>
#include <rbus/rbus_value.h>
#include <rtMessageHeader.h>
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
void do_something();
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int cpeabs()
{
  /* req_struct *req = NULL;
   req = (req_struct *) malloc (sizeof(req_struct)); 
   do_something();*/
   //wdmp_parse_generic_request("Hi", WDMP_TR181, &req);
   rtMessageHeader* testrt = NULL;
   testrt = (rtMessageHeader*)malloc(sizeof(rtMessageHeader));
   
   rtMessageHeader_Init(testrt);
   
   do_something();
   return 0;
}

void do_something()
{
    printf("I do something well.\n");
}
