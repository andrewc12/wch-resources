/* 以下程序节选于LPC2114开发套件 2004.07 */
/* lpc21xx（飞利浦的ARM）目标板特殊的代码，包括异常处理程序和目标板初始化程序 */

#include "LPC2294.H"

/*     本例子的配置             */
/* 系统设置, Fosc、Fcclk、Fcco、Fpclk必须定义*/
#define Fosc            11059200                    //晶振频率,10MHz~25MHz，应当与实际一至
#define Fcclk           (Fosc * 4)                  //系统频率，必须为Fosc的整数倍(1~32)，且<=60MHZ
#define Fcco            (Fcclk * 4)                 //CCO频率，必须为Fcclk的2、4、8、16倍，范围为156MHz~320MHz
#define Fpclk           (Fcclk / 4) * 1             //VPB时钟频率，只能为(Fcclk / 4)的1、2、4倍


void	__irq	IRQ_Exception( void )				/* 中断异常处理程序，用户根据需要自己改变程序 */
{
    while(1);                   // 这一句替换为自己的代码
}

void	FIQ_Exception( void )						/* 快速中断异常处理程序，用户根据需要自己改变程序 */
{
    while(1);                   // 这一句替换为自己的代码
}

void	TargetResetInit( void )						/* 调用main函数前目标板初始化代码，根据需要改变，不能删除 */
{
    MEMMAP = 0x1;                   //remap
    /* 设置系统各部分时钟 */
    PLLCON = 1;
#if (Fpclk / (Fcclk / 4)) == 1
    VPBDIV = 0;
#endif
#if (Fpclk / (Fcclk / 4)) == 2
    VPBDIV = 2;
#endif
#if (Fpclk / (Fcclk / 4)) == 4
    VPBDIV = 1;
#endif
#if (Fcco / Fcclk) == 2
    PLLCFG = ((Fcclk / Fosc) - 1) | (0 << 5);
#endif
#if (Fcco / Fcclk) == 4
    PLLCFG = ((Fcclk / Fosc) - 1) | (1 << 5);
#endif
#if (Fcco / Fcclk) == 8
    PLLCFG = ((Fcclk / Fosc) - 1) | (2 << 5);
#endif
#if (Fcco / Fcclk) == 16
    PLLCFG = ((Fcclk / Fosc) - 1) | (3 << 5);
#endif
    PLLFEED = 0xaa;
    PLLFEED = 0x55;
    while((PLLSTAT & (1 << 10)) == 0);
    PLLCON = 3;
    PLLFEED = 0xaa;
    PLLFEED = 0x55;
    /* 设置存储器加速模块 */
    MAMCR = 0;
#if Fcclk < 20000000
    MAMTIM = 1;
#else
#if Fcclk < 40000000
    MAMTIM = 2;
#else
    MAMTIM = 3;
#endif
#endif
    MAMCR = 2;  
    /* 初始化VIC */
    VICIntEnClr = 0xffffffff;
    VICVectAddr = 0;
    VICIntSelect = 0;
    /* 添加自己的代码 */
}
