
#include <rtthread.h>  
#include <rtdevice.h>
#include <mpu6xxx.h>
#include "paho_mqtt.h"
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include <U8g2lib.h>
#include "app_esp8266.h"
#include <sensor.h>
#include <ntp.h>
#define DBG_LEVEL   DBG_LOG
#include <rtdbg.h>
#define LOG_TAG                "example.hr"
#include "driver_max30205_basic.h"
static MQTTClient client;
#include "drv_dht11.h"
#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5
#define MQTT_Uri    "mqtts.heclouds.com"   // MQTT服务器的地址和端口号
#define ClientId    "534850"                // ClientId需要唯一
#define UserName    "ESP8266"                    // 用户名
#define PassWord    "1234567"                    // 用户名对应的密码
#define key_gpio 85
/*  静态线程1 的对象和运行时用到的栈 */  
static struct rt_thread thread1;
static rt_uint8_t thread1_stack[THREAD_STACK_SIZE]; 

/*  动态线程2 的对象 */  
static rt_thread_t thread2 = RT_NULL;
 #define ONENET_TOPIC_PROP_POST "$sys/" mqtt_pubid "/" mqtt_devid "/dp/post/json" //"$sys/" mqtt_pubid "/" mqtt_devid "/thing/property/post"
//接收下发属性设置主题
#define ONENET_TOPIC_PROP_SET  "$sys/" mqtt_pubid "/" mqtt_devid "/dp/post/json/+" //"$sys/" mqtt_pubid "/" mqtt_devid "/thing/property/set"
//接收下发属性设置成功的回复主题
#define ONENET_TOPIC_PROP_SET_REPLY "$sys/" mqtt_pubid "/" mqtt_devid "/dp/post/accepted" //"$sys/" mqtt_pubid "/" mqtt_devid "/thing/property/set_reply"
 
//接收设备属性获取命令主题
#define ONENET_TOPIC_PROP_GET "$sys/" mqtt_pubid "/" mqtt_devid "/cmd/request/+" //"$sys/" mqtt_pubid "/" mqtt_devid "/thing/property/get"
//接收设备属性获取命令成功的回复主题
#define ONENET_TOPIC_PROP_GET_REPLY "$sys/" mqtt_pubid "/" mqtt_devid "/cmd/response/+/+" //"$sys/" mqtt_pubid "/" mqtt_devid "/thing/property/get_reply"
 
//这是post上传数据使用的模板
#define ONENET_POST_BODY_FORMAT "{\"id\":%d,\"dp\":%s}"
//#define ONENET_POST_BODY_FORMAT
int postMsgId = 0; //记录已经post了多少条

/* Default configuration, please change according to the actual situation, support i2c and spi device name */
#define MPU6XXX_DEVICE_NAME  "i2c1"
/* 定时器的控制块 */
static rt_timer_t timer1;
static rt_timer_t timer2;


int16_t ax, ay, az;
int16_t gx, gy, gz;
float temp, humi;
int axoffs,ayoffs,azoffs;
float rax,ray,raz;
float ax0,ay0,az0;
float ax1,ay1,az1;

float bodytemp;
/* Test function */
double t1;
double t0;
double t;
int sum;
int heartRate;
int count=0;
/* Test function */
    struct mpu6xxx_device *dev;
    struct mpu6xxx_3axes accel, gyro;
    int ge,shi,bai,qian;
#define DHT11_DATA_PIN GET_PIN(B, 12)

 void read_temp()
{
    rt_device_t dev = RT_NULL;
    struct rt_sensor_data sensor_data;
    rt_size_t res;

    dev = rt_device_find(parameter);
    if (dev == RT_NULL)
    {
        rt_kprintf("Can't find device:%s\n", parameter);
        return;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("open device failed!\n");
        return;
    }
    rt_device_control(dev, RT_SENSOR_CTRL_SET_ODR, (void *)100);

        res = rt_device_read(dev, 0, &sensor_data, 1);
        if (res != 1)
        {
            rt_kprintf("read data failed!size is %d\n", res);
            rt_device_close(dev);
            return;
        }
        else
        {
            if (sensor_data.data.temp >= 0)
            {

							uint8_t temp = (sensor_data.data.temp & 0xffff) >> 0;      // get temp
              uint8_t humi = (sensor_data.data.temp & 0xffff0000) >> 16; // get humi
							rt_kprintf("temp:%d, humi:%d\n" ,temp, humi);
            }
        }
        rt_thread_mdelay(100);
    }
void key_scan()
{
    int folag;
    if (gpio_level_low != gpio_get(key_gpio))
    delay_ms(10);
    if (gpio_level_low != gpio_get(key_gpio))
    folag++;
    oled_display(folag); 
    while (gpio_level_high != gpio_get(key_gpio));
    delay_ms(10);
    if (folag>=2)
    {
        folag=0;
    }
}
void draw1()
{
   timerAlarmEnable(tim1);//
  u8g2.clearBuffer();
  u8g2.setCursor(0, 31);
  u8g2.print("当前时间:");
  u8g2.setCursor(70, 31);
  u8g2.print("10:");
   u8g2.setCursor(90, 31);
  u8g2.print("35:");
   u8g2.setCursor(110, 31);
  u8g2.print(t);
 // u8g2.clearBuffer();
  u8g2.setCursor(0, 49);
  u8g2.print("2022/07/13");
  u8g2.setCursor(80, 49);
   u8g2.print("星期三");
  
  u8g2.setCursor(0, 15);
  u8g2.print("温度:");
  u8g2.setCursor(30 , 15);
  u8g2.print("tem");
  u8g2.setCursor(50, 15);
  u8g2.print("℃");

  u8g2.setCursor(65, 15);
  u8g2.print("湿度:");
  u8g2.setCursor(100, 15);
  u8g2.print("73");
  u8g2.setCursor(118, 15);
  u8g2.print("%");

  u8g2.setCursor(0, 64);
  u8g2.print("天气:");
  u8g2.setCursor(35, 64);
  u8g2.print("小雨");
  u8g2.sendBuffer();
   }

void oled_display()
{
    u8g2.begin();
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(48+3, 42);
    u8g2.print("体温：");    

    u8g2.setFont(u8g2_font_6x13_tr);            
    u8g2.drawStr(30, 60, tem);   
    u8g2.sendBuffer();

    sht3x_device_t  sht3x_device;
    sht3x_device = sht3x_init("i2c1", 0x44);

    rt_thread_mdelay(2000);

    int status = 0;
    char mstr[3];
    char hstr[3];
    time_t now;
    struct tm *p;
    int min = 0, hour = 0;
    int temperature = 0,humidity = 0;

    while(1)
    {
        switch(status)
        {
            case 0:
                now = time(RT_NULL);
                p=gmtime((const time_t*) &now);
                hour = p->tm_hour;
                min = p->tm_min;
                sprintf(mstr, "%02d", min);
                sprintf(hstr, "%02d", hour);


                u8g2.firstPage();
                do {
                     u8g2.setFont(u8g2_font_logisoso42_tn);
                     u8g2.drawStr(0,63,"脉搏");
                     u8g2.drawStr(50,63,":");
                     u8g2.drawStr(67,63,"心率：");
                   } while ( u8g2.nextPage() );


                rt_thread_mdelay(5000);
                status = 1;
                break;
          case 1：
          draw1();
           break;
        }
    }
}

static int mpu6xxx_st()
{
     /* Initialize mpu6xxx, The parameter is RT_NULL, means auto probing for i2c*/
    dev = mpu6xxx_init(MPU6XXX_DEVICE_NAME, RT_NULL);

    if (dev == RT_NULL)
    {
        rt_kprintf("mpu6xxx init failed\n");
        return -1;
    }
    rt_kprintf("mpu6xxx init succeed\n");

    
    return 1;
}
void jisuan()
{
      sum=abs(abs(ax1)+abs(ay1)+abs(az1));
      rt_kprintf("mpu6xxx %d\n",sum);
      if(sum>18)
      {
      count++;
      sum=0;
      }
      if(count<10) {ge=count; shi=0; bai=0;}
      if((count>=10)&&(count<100)) { ge=count%10; shi=count/10; bai=0;}
      if((count>=100)&&(count<1000)) {ge=count%100%10; shi=count%100/10; bai=count/100;}
      if(count>1000)          {ge=0;shi=0;bai=0;}
}
void getoffs()
{
      int16_t ax, ay, az;
      int16_t gx, gy, gz;
      long int axsum=0;
      long int aysum=0;
      long int azsum=0;
      int i;
          for(i=1;i<=2000;i++)
          {
              mpu6xxx_get_accel(dev, &accel);
                ax=accel.x;
                ay=accel.y;
                az=accel.z ;
              axsum=ax+axsum;
              aysum=ay+aysum;
              azsum=az+azsum-16384;
          }
      axoffs=-axsum/2000;
      ayoffs=-aysum/2000;
      azoffs=-azsum/2000;
}
static int mpu6xxx_test()
{

    int i;

        mpu6xxx_get_accel(dev, &accel);
        mpu6xxx_get_gyro(dev, &gyro);
    ax=accel.x;
    ay=accel.y;
    az=accel.z ;
        rt_kprintf("accel.x = %3d, accel.y = %3d, accel.z = %3d ", ax, ay, az );
       // rt_kprintf("gyro.x = %3d gyro.y = %3d, gyro.z = %3d\n", gyro.x, gyro.y, gyro.z);
    ax=ax+axoffs;
    ay=ay+ayoffs;
    az=az+azoffs;
    rax=ax;
    ray=ay;
    raz=az;
    ax1=(rax/16384)*9.80;
    ay1=(ray/16384)*9.80;
    az1=(raz/16384)*9.80;
     rt_kprintf("ax1 = %3d, ay1 = %3d, az1 = %3d ", ax1, ay1, az1 );
        //rt_thread_mdelay(1000);

    return 0;
}

void test_timer_01(void)
{
	/* 创建定时器1 */
	timer1 = rt_timer_create("timer1",  /* 定时器名字是 timer1 */
						jisuan, /* 超时时回调的处理函数 */
						RT_NULL,  /* 超时函数的入口参数 */
						1000,       /* 定时长度，以OS Tick为单位，即100个OS Tick */
						RT_TIMER_FLAG_PERIODIC); /* 周期性定时器 */
	/* 启动定时器 */
	if (timer1 != RT_NULL) rt_timer_start(timer1);

	
}
void ssd1306_wr(int sum_in ,int pos)
{  
    char str[25]; 
    ssd1306_Fill(Black);
    if(pos==1){
    ssd1306_SetCursor(2, 26 );
    ssd1306_WriteString("step:", Font_7x10, White);      
    sprintf(str, "%d", sum_in); //将100转为16进制表示的字符串。
    ssd1306_SetCursor(45, 26 );
    ssd1306_WriteString(str, Font_7x10, White);

    }
    if(pos==2){
    ssd1306_SetCursor(2, 44 );
    ssd1306_WriteString("heartRate:", Font_7x10, White);      
    sprintf(str, "%d", heartRate); //将100转为16进制表示的字符串。
    ssd1306_SetCursor(45, 44 );
    ssd1306_WriteString(str, Font_7x10, White);

    }
    if(pos==3){
    ssd1306_SetCursor(2, 62 );
    ssd1306_WriteString("bodyTemperature:", Font_7x10, White);      
    sprintf(str, "%d", bodytemp); //将100转为16进制表示的字符串。
    ssd1306_SetCursor(45, 64 );
    ssd1306_WriteString(str, Font_7x10, White);

    }

    ssd1306_UpdateScreen();
}




/*  线程 1入口  */  
static void thread1_entry (void* parameter) 
{ 
    
    mpu6xxx_st();
  getoffs();
  test_timer_01();
 rt_thread_mdelay(500);
 
    max30205_basic_init(MAX30205_ADDRESS_0);
     ssd1306_Init();
   

    while (1) 
    { 
         mpu6xxx_test();
         rt_kprintf("%d",count);  rt_kprintf("%d",bai);  rt_kprintf("%d",shi);  rt_kprintf("%d",ge);
  
                max30205_basic_read((float *)&bodytemp);

                 ssd1306_wr(count,1);
                 ssd1306_wr(heartRate ,2);
                  ssd1306_wr(bodytemp,3); 

				/*  延时100 个OS Tick */  
				rt_thread_delay(100); 
    } 
} 

/*  线程 2入口  */  
static void thread2_entry (void* parameter) 
{ 
     rt_device_t dev = rt_device_find("hr_max30102");
    if (dev == RT_NULL) {
        rt_kprintf("Find max30102 error");
        return ;
    }

    rt_device_open(dev, RT_DEVICE_FLAG_RDONLY);

    struct rt_sensor_data data;
    while (1) 
    { 
        if (rt_device_read(dev, 0, &data, sizeof(data)) == sizeof(data)) {
            heartRate=data.data.hr;
                        rt_kprintf("heart rate: %d",heartRate);

                    }

        /*  延时50个OS Tick */  
        rt_thread_delay(50); 
    } 
}

void test_thread_04(void)
{
	rt_err_t result; 

	result = rt_thread_init(&thread1, "thread1", 
			thread1_entry, RT_NULL, 
			&thread1_stack[0], sizeof(thread1_stack), 
			THREAD_PRIORITY, THREAD_TIMESLICE); 

	/*  启动线程1 */  
	if (result == RT_EOK) rt_thread_startup(&thread1); 
 
	thread2 = rt_thread_create( "thread2", thread2_entry, RT_NULL, 
			THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE); 

	/*  启动线程2 */  
	if (thread2 != RT_NULL) rt_thread_startup(thread2);
}
int main()
{
  test_thread_04();   
   //NTP自动对时
   time_t cur_time;
   cur_time = ntp_sync_to_rtc(NULL);
   if (cur_time)
   {
       rt_kprintf("Cur Time: %s", ctime((const time_t*) &cur_time));
   }
   else
   {
       rt_kprintf("NTP sync fail.\n");
   } 
  while (1)
  {    
    key_scan();
    read_temp();
    onenet_upload_cycle();
    oled_display();
    rt_thread_mdelay(100);  
 }
}


