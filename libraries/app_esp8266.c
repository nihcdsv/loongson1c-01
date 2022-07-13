/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       zx
 * 2020-05-08            the first version
 */
 
#include "app_esp8266.h"
#include <rtthread.h>  
#include <rtdevice.h> 
#include "at.h"
/*使能esp8266 */
void Set_Esp8266_En(void)
{
    rt_pin_mode(23, PIN_MODE_OUTPUT);
    rt_pin_write(23, 1);
    rt_kprintf("esp8266_Enable,PB8=1\r\n");
}
INIT_PREV_EXPORT(Set_Esp8266_En);
/*重启esp8266 */
void Reset_Esp8266(void)
{
    rt_pin_mode(24, PIN_MODE_OUTPUT);
    rt_pin_write(24, 0);
    rt_thread_mdelay(200);
    rt_pin_write(24, 1);
    rt_kprintf("Reset_Esp8266\r\n");
}
INIT_PREV_EXPORT(Reset_Esp8266);
 
/*esp8266结构体回调函数 */
void CallBack(char *Cmd,float Cycle)
{
}
 
/*初始化esp8266命令结构体，回调函数暂时没用到 */
typEsp8266_t typEsp8266CmdTable[]=
{
    {"AT+CWMODE=1",                                 CallBack},          //1：工作在客户端模式
    {"AT+CWJAP=\"QIANTAI\",\"12345678#\"",          CallBack},          //2: 连接无线
    {"AT+CIFSR",                                    CallBack},          //3: 获取自身的IP及Mac地址
    {"AT+CIPMUX=0",                                 CallBack},          //4：关闭多连接
    {"AT+CIPSTART=\"TCP\",\"121.36.63.46\",10086",  CallBack},          //5：连接服务器IP端口
    {"AT+CIPMODE=1",                                CallBack},          //6：透传模式
    {"AT+CIPSEND",                                  CallBack}           //7：启动透传，之后esp8266进入透传状态，发送的数据将转发向服务器，不再识别AT指令
};
 
/*重新配置esp8266 */
int esp8266_init_Reconfiguring(void)
{
    at_response_t resp = RT_NULL;
    const char *line_buffer = RT_NULL;
 rt_size_t line_num;
int i;

    /* 创建响应结构体，设置最大支持响应数据长度为 512 字节，响应数据行数无限制，超时时间为8 秒 */
    resp = at_create_resp(512, 0, rt_tick_from_millisecond(8000));
    if (!resp)
    {
        LOG_E("No memory for response structure!");
        return -RT_ENOMEM;
    }
    /* 发送 AT 命令并接收 AT Server 响应数据，数据及信息存放在 resp 结构体中并打印出来 */
    for(  i=0;i<array_sizeof(typEsp8266CmdTable);i++)
    {
        if (at_exec_cmd(resp, typEsp8266CmdTable[i].CmdString) != RT_EOK)               //发送AT指令
        {
            LOG_E("%s指令没收到esp8266的回应",typEsp8266CmdTable[i].CmdString);
        }
        else
        {
            for(  line_num = 1; line_num <= resp->line_counts; line_num++)      //将回应的数据打印出来
                {
                    if((line_buffer = at_resp_get_line(resp, line_num)) != RT_NULL)
                    {
                        LOG_D("%s line%d Response buf: %s",typEsp8266CmdTable[i].CmdString,line_num, line_buffer);
                    }
                    else
                    {
                        LOG_E("Parse line buffer error!");
                    }
                }
        }
    }
    rt_thread_mdelay(20);
    at_exec_cmd( resp, "由开发板发来的第一次测试数据1");
    at_exec_cmd( resp, "由开发板发来的第二次测试数据2");
 
    /* 命令发送成功 */
    LOG_D("AT Client send commands to AT Server success!");
 
    /* 删除响应结构体 */
    at_delete_resp(resp);
 
    return RT_EOK;
}
 