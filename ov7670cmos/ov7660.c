#include "ov7660.h"
#include "delay.h"
#include "ov7660config.c"

////////////////////////////
//功能：提供7660时钟
//返回：无
void CLK_init(void)
{
   CLK_ON;
}

////////////////////////////
//功能：写OV7660寄存器
//返回：1-成功	0-失败
uchar wrOV7660Reg(uchar regID, uchar regDat)
{
	startSCCB();
	if(0==SCCBwriteByte(0x42))
	{
		stopSCCB();
		return(0);
	}
	delay_us(100);
  	if(0==SCCBwriteByte(regID))
	{
		stopSCCB();
		return(0);
	}
	delay_us(100);
  	if(0==SCCBwriteByte(regDat))
	{
		stopSCCB();
		return(0);
	}
  	stopSCCB();
	
  	return(1);
}
////////////////////////////
//功能：读OV7660寄存器
//返回：1-成功	0-失败
uchar rdOV7660Reg(uchar regID, uchar *regDat)
{
	//通过写操作设置寄存器地址
	startSCCB();
	if(0==SCCBwriteByte(0x42))
	{
		stopSCCB();
		return(0);
	}
	delay_us(100);
  	if(0==SCCBwriteByte(regID))
	{
		stopSCCB();
		return(0);
	}
	stopSCCB();
	
	delay_us(100);
	
	//设置寄存器地址后，才是读
	startSCCB();
	if(0==SCCBwriteByte(0x43))
	{
		stopSCCB();
		return(0);
	}
	delay_us(100);
  	*regDat=SCCBreadByte();
  	noAck();
  	stopSCCB();
  	return(1);
}


//(140,16,640,480) is good for VGA
//(272,16,320,240) is good for QVGA
/* config_OV7660_window */
void OV7660_config_window(uint startx,uint starty,uint width, uint height)
{
	uint endx;
	uint endy;// "v*2"必须
	uchar temp_reg1, temp_reg2;
	uchar state,temp;
	
	endx=(startx+width);
	endy=(starty+height+height);// "v*2"必须
	state = rdOV7660Reg(0x03, &temp_reg1 );
	temp_reg1 &= 0xf0;
	state = rdOV7660Reg(0x32, &temp_reg2 );
	temp_reg2 &= 0xc0;
	
	// Horizontal
	temp = temp_reg2|((endx&0x7)<<3)|(startx&0x7);
	state = wrOV7660Reg(0x32, temp );
	temp = (startx&0x7F8)>>3;
	state = wrOV7660Reg(0x17, temp );
	temp = (endx&0x7F8)>>3;
	state = wrOV7660Reg(0x18, temp );
	
	// Vertical
	temp = temp_reg1|((endy&0x3)<<2)|(starty&0x3);
	state = wrOV7660Reg(0x03, temp );
	temp = starty>>2;
	state = wrOV7660Reg(0x19, temp );
	temp = endy>>2;
	state = wrOV7660Reg(0x1A, temp );
}



/* OV7660_init() */
//返回1成功，返回0失败
uchar OV7660_init(void)
{
	uchar temp;
	
	uint i=0;

	//uchar ovidmsb=0,ovidlsb=0;
	
	InitSCCB();//io init..

	temp=0x80;
	if(0==wrOV7660Reg(0x12, temp)) //Reset SCCB
	{
		return 0 ;
	}
	delay_ms(10);
/*
   
    wrOV7660Reg(0x39 ,0x57); //?? 

    wrOV7660Reg(0xa1 ,0x07); //  ??
     
    wrOV7660Reg(0x69 ,0x80);   
    wrOV7660Reg(0x43 ,0xf0);   
    wrOV7660Reg(0x44 ,0x10);   
    wrOV7660Reg(0x45 ,0x78);   
    wrOV7660Reg(0x46 ,0xa8);   
    wrOV7660Reg(0x47 ,0x60);   
    wrOV7660Reg(0x48 ,0x80);   
    wrOV7660Reg(0x59 ,0xba);   
    wrOV7660Reg(0x5a ,0x9a);   
    wrOV7660Reg(0x5b ,0x22);   
    wrOV7660Reg(0x5c ,0xb9);   
    wrOV7660Reg(0x5d, 0x9b);   
    wrOV7660Reg(0x5e ,0x10);   
    wrOV7660Reg(0x5f ,0xe0);   
    wrOV7660Reg(0x60 ,0x85);// ;05 for advanced AWB   
    wrOV7660Reg(0x61 ,0x60);   
    wrOV7660Reg(0x9f ,0x9d);   
    wrOV7660Reg(0xa0 ,0xa0);  //
  */
	for(i=0;i<CHANGE_REG_NUM;i++)
	{
		if( 0==wrOV7660Reg(pgm_read_byte( &change_reg[i][0]),pgm_read_byte( &change_reg[i][1]) ))
		{
			return 0;
		}
	}

	// OV7660_config_window(272,12,320,240);// set 240*320
	return 0x01; //ok
} 

