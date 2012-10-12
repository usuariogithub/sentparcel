#include <avr/io.h>
#include <avr/interrupt.h>
#include "ILI9325.H"
#include "ov7660.H"
#include "int.H"
#include "delay.h"


//CPU初始化//
int main(void) 
{
	
	EXCLK_DDR|=1<<EXCLK_BIT ;//en_exclk 引脚输出
	HC245_OE_DDR|=1<<HC245_OE_BIT;//en_245引脚输出
	CMOS_CLK_DDR|=1<<CMOS_CLK_BIT;//7660时钟允许引脚输出	
	DISEN_EXCLK;//禁止外部CLK，使单片机WR与TFT WR相连	
	DISEN_245;//禁止外部数据线，使单片机数据与TFT数据线相连
	CLK_init();//输出时钟到7660
	DATA_OUPUT(); //数据线输出，准备连数据线到TFT
    LCD_Init();
	//DATA_LCD_PORT=0xff;
	LCD_write_english_string(20,60,"Guanfu_Wang  2009-08-26",BLACK,RED);
	LCD_write_english_string(20,76,"Atmega32 & ILI9325 FOR OV7660 REV2.0",BLACK,RED);
	delay_ms(10000);
	LCD_write_english_string(20,96,"OV7660 Init......",BLACK,RED);
	while(1!=OV7660_init());//初始化ov7660
	LCD_write_english_string(20,96,"OV7660 Init  0K  ",BLACK,RED);
	delay_ms(10000);
	LCD_Clear(RED);
    DATA_INPUT(); 
	Init_INT0();
	/**/
    while(1)
    {
	}

}

