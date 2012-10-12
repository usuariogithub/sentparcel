/*
AVR GPS 数据解码模块。
将GPS 的NMEMA 信息解码，然后通过I2C 总线传递到主机
*/

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#define INT_ON sei();
#define INT_OFF cli();

typedef struct tagGPSPosition
{
	//float fLon; // 经度 Longitude
	USHORT nLonHigh;
	USHORT nLonLow;
	BYTE WE;
	BYTE Reserved1; // 由于AVR 是8 位单片机，所以存储结构上可以按照1 字节对齐。
	// 但ARM 和PC 是32 位单片机，所以必须补齐一个空余的字节。
	
	//float fLat; // 纬度 Latitude
	USHORT nLatHigh;
	USHORT nLatLow;
	BYTE NS;
	BYTE Reserved2;
}GPSPOSITION;

#define NOP asm("nop");
#define TX_BUFFER_SIZE 20
#define UDR_EMPTY (1<<UDRE)

BOOL g_bState = FALSE; // GPS 定位状态
GPSPOSITION g_Position = {0};
BYTE tx_buffer[TX_BUFFER_SIZE]={0};
BYTE tx_wr_index=0;
BYTE tx_rd_index=0;
BYTE tx_counter=0;

void GPS_Decode(BYTE nData)
{
	/*
	解析这组信息：
	$GPRMC,121252.000,A,3958.3032,N,11629.6046,E,15.15,359.95,070306,,,A*54
	$GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>*hh<CR><LF>
	<1> UTC 时间，hhmmss（时分秒）格式
	<2> 定位状态，A=有效定位，V=无效定位
	<3> 纬度ddmm.mmmm（度分）格式（前面的0 也将被传输）
	<4> 纬度半球N（北半球）或S（南半球）
	<5> 经度dddmm.mmmm（度分）格式（前面的0 也将被传输）
	<6> 经度半球E（东经）或W（西经）
	<7> 地面速率（000.0~999.9 节，前面的0 也将被传输）
	<8> 地面航向（000.0~359.9 度，以真北为参考基准，前面的0 也将被传输）
	<9> UTC 日期，ddmmyy（日月年）格式
	<10> 磁偏角（000.0~180.0 度，前面的0 也将被传输）
	<11> 磁偏角方向，E（东）或W（西）
	<12> 模式指示（仅NMEA0183 3.00 版本输出，A=自主定位，D=差分，E=估算，N=数据无效）
	*/
	
	static int nDataIndex = 0;
	static BOOL bDataFieldOK = FALSE;
	static int nItemIndex = 0;
	static char szBuffer[15] = {0};
	static char *pBufferIndex = 0;
	BYTE i = 0;
	USHORT nData1 = 0;
	USHORT nData2 = 0;
	
	if (nData == '$')
	{
		// 新的一组数据开始。
		nDataIndex = 0;
		nItemIndex = 0;
		pBufferIndex = &szBuffer[0];
		bDataFieldOK = FALSE;
	}
	else
	{
		nDataIndex ++;
	}	
	if ((nDataIndex == 5) && (nData == 'C')) // 确认第6 个字符是C，也就是GPS 模块返回的$GPRMC 数据条目
	{
		if (!g_bState)
		{
			PORTB |= (1<<PB0);
		}
		// 可以扫描后续信息。
		bDataFieldOK = TRUE;
	}
	else if ((nDataIndex == 5) && (nData != 'C')) // 字段长度足够，但不是$GPRMC 数据条目
	{
		// 不是我们需要的字段，忽略不去处理。
	}
	
	if ((bDataFieldOK) && (nDataIndex > 6))
	{
		// 后续数据已经过来了，需要进行拆分。
		if ((nData == ',') || (nData == '*'))
		{
			PORTB &= ~(1<<PB0);
			*pBufferIndex = 0; // 写入终止符。
			// 分析上一组数据
			switch(nItemIndex)
			{			
				case 0:
					// UTC 时间，hhmmss
					//TRACE("UTC Time: %s",szBuffer);
					break;
				case 1:
					// 定位状态，A=有效定位，V=无效定位
					//TRACE("定位状态: %s",szBuffer);
					if (strcmp(szBuffer,"A") == 0)
					{
						PORTB |= (1<<PB2);
						g_bState = TRUE;
					}
					else
					{
						PORTB &= ~(1<<PB2);
						g_bState = FALSE;
					}
					break;
				case 2:
					// 纬度ddmm.mmmm（度分）格式（前面的0 也将被传输）
					// 由于AVR 和Intel32 位计算机的浮点数存储格式不大一样，所以不能直接转成浮点传递。
					//g_Position.fLat = atof(szBuffer);
					for (i=0;i<sizeof(szBuffer);i++)
					{
						if (szBuffer[i] == '.')
						{
							szBuffer[i] = 0;
							nData1 = atoi(szBuffer);
							nData2 = atoi(&szBuffer[i+1]);
							g_Position.nLatHigh = nData1;
							g_Position.nLatLow = nData2;
							break;
						}
					}
					break;
				case 3:
					// 纬度半球N（北半球）或S（南半球）
					g_Position.NS = szBuffer[0];
					break;
				case 4:
					// 经度dddmm.mmmm（度分）格式（前面的0 也将被传输）
					// g_Position.fLon = atof(szBuffer);
					// 由于AVR 和Intel32 位计算机的浮点数存储格式不大一样，所以不能直接转成浮点传递。
					for (i=0;i<sizeof(szBuffer);i++)
					{
						if (szBuffer[i] == '.')
						{
							szBuffer[i] = 0;
							nData1 = atoi(szBuffer);
							nData2 = atoi(&szBuffer[i+1]);
							g_Position.nLonHigh = nData1;
							g_Position.nLonLow = nData2;
							break;
						}
					}
					break;
				case 5:
					// 经度半球E（东经）或W（西经）
					g_Position.WE = szBuffer[0];
					break;
				case 6:
					// 地面速率（000.0~999.9 节，前面的0 也将被传输）
					break;
				case 7:
					// 地面航向（000.0~359.9 度，以真北为参考基准，前面的0 也将被传输）
					break;
				case 8:
					// UTC 日期，ddmmyy（日月年）格式
					break;
				case 9:
					// 磁偏角（000.0~180.0 度，前面的0 也将被传输）
					break;
				case 10:
					// 磁偏角方向，E（东）或W（西）
					break;
				case 11:
					// 模式指示（仅NMEA0183 3.00 版本输出，A=自主定位，D=差分，E=估算，N=数据无效）
					// 这是$GPRMC 的最后一项数据。
					// 我们需要的数据已经获得了，下面的数据就要等待下一次1s 后再来了。
					// 现在可以允许采集温度了。
					g_bAllowGetTemperature = FALSE;
					break;
				default:
					break;
}

			// 逗号，需要切换到下一组数据。
			nItemIndex++;
			// 重新设置缓冲区起始位置。
			pBufferIndex = &szBuffer[0];
		}
		else
		{
			// 数据写入缓冲区。
			*pBufferIndex = nData;
			pBufferIndex++;
		}
	}
}