/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       zx
 * 2020-05-08            the first version
 */
#ifndef APPLICATIONS_APP_ESP8266_H_
#define APPLICATIONS_APP_ESP8266_H_
 
#define array_sizeof(a) (sizeof(a) / sizeof(a[0]))
/* 服务函数结构体*/
typedef struct typ_Esp8266_handler
{
  char  *CmdString; //命令字符串
  void  (*CmdOperate)(char *Cmd,float Cycle);//命令执行的功能操作
} typEsp8266_t;
 
extern int esp8266_init_Reconfiguring(void);
 
#endif /* APPLICATIONS_APP_ESP8266_H_ */
 