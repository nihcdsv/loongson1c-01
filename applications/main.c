
#include <rtthread.h>  
#include <rtdevice.h>
#include <mpu6xxx.h>

#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
 
#include "app_esp8266.h"
#include <sensor.h>

#define DBG_LEVEL   DBG_LOG
#include <rtdbg.h>
#define LOG_TAG                "example.hr"
#include "driver_max30205_basic.h"

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5

/*  静态线程1 的对象和运行时用到的栈 */  
static struct rt_thread thread1;
static rt_uint8_t thread1_stack[THREAD_STACK_SIZE]; 

/*  动态线程2 的对象 */  
static rt_thread_t thread2 = RT_NULL;
 

/* Default configuration, please change according to the actual situation, support i2c and spi device name */
#define MPU6XXX_DEVICE_NAME  "i2c1"
/* 定时器的控制块 */
static rt_timer_t timer1;
static rt_timer_t timer2;


int16_t ax, ay, az;
int16_t gx, gy, gz;
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
   

   // mpu6xxx_deinit(dev);

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

	/*  初始化线程1 */ 
	/*  线程的入口是thread1_entry ，参数是RT_NULL 
	 *  线程栈是thread1_stack 栈空间是512 ，  
	 *  优先级是25 ，时间片是5个OS Tick 
	 */  
	result = rt_thread_init(&thread1, "thread1", 
			thread1_entry, RT_NULL, 
			&thread1_stack[0], sizeof(thread1_stack), 
			THREAD_PRIORITY, THREAD_TIMESLICE); 

	/*  启动线程1 */  
	if (result == RT_EOK) rt_thread_startup(&thread1); 

	/*  创建线程2 */ 
	/*  线程的入口是thread2_entry,  参数是RT_NULL 
	 *  栈空间是512 ，优先级是25 ，时间片是5个OS Tick 
	 */  
	thread2 = rt_thread_create( "thread2", thread2_entry, RT_NULL, 
			THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE); 

	/*  启动线程2 */  
	if (thread2 != RT_NULL) rt_thread_startup(thread2);
}
int main()
{
  test_thread_04();    
  while (1)
  {    
  rt_thread_mdelay(1000);  
}
}


