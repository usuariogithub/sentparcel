//
//TWI BootLoader.c
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
// 参数配置，包括地址和亮灯的IO 口代码，程序会反复调用BL_LIGHT1 和BL_LIGHT2
//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
#ifdef GPSBOARD
	#define TWI_ADDR 30
	
	#define BL_PORT_INIT() DDRB = (1<< DDB0) | (1<<PB1) | (1<<PB2);
	#define BL_LIGHT1() PORTB = (1<<PB0)|(1<<PB1);
	#define BL_LIGHT2() PORTB = (1<<PB2)|(1<<PB1);
#elif DRIVERBOARD
	#define TWI_ADDR 20
	
	#define BL_PORT_INIT() DDRC = (1<<DDC0) | (1<<DDC1);
	#define BL_LIGHT1() PORTC = (1<<PC0);
	#define BL_LIGHT2() PORTC = (1<<PC1);
#else
	#define TWI_ADDR 31
	
	#define BL_PORT_INIT() DDRB = (1<<DDB3) | (1<<DDB1) | (1<< DDB0) |(1<<DDB2);
	#define BL_LIGHT1() PORTB = (1<<PB1) | (1<<PB0) | (1<<PB2) | (1<<PB3);
	#define BL_LIGHT2() PORTB = 0;
#endif
#define BL_ADDR 0x1800 // BootLoader 的地址
//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
// TWI_Slave.h 文件内容
//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

/****************************************************************************
TWI Status/Control register definitions
****************************************************************************/

#define TWI_BUFFER_SIZE (SPM_PAGESIZE+2) // Reserves memory for the driverstransceiver buffer.
// Set this to the largest message size that will be sentincluding address byte.

/****************************************************************************
Global definitions
****************************************************************************/

union TWI_statusReg // Status byte holding flags.
{
	unsigned char all;
	struct
	{
		unsigned char lastTransOK:1;
		unsigned char RxDataInBuf:1;
		unsigned char genAddressCall:1; // TRUE = General call,
		FALSE = TWI Address;
		unsigned char unusedBits:5;
	};
};

//extern union TWI_statusReg TWI_statusReg;

/****************************************************************************
Function definitions
****************************************************************************/

static void TWI_Slave_Initialise( unsigned char );
static unsigned char TWI_Transceiver_Busy( void );
//static unsigned char TWI_Get_State_Info( void );
static void TWI_Start_Transceiver_With_Data( unsigned char * , unsigned int );
static void TWI_Start_Transceiver( void );
//static unsigned char TWI_Get_Data_From_Transceiver( unsigned char *, unsigned char );
static unsigned char* TWI_Get_Data_Pointer_From_Transceiver(void);
static void TWI_Get_Data_Pointer_From_Transceiver_Release(void);

//static void TWI_Prepare_Transceiver_CleanDataSize(void);
//static void TWI_Prepare_Transceiver_With_Data( unsigned char *msg, unsigned int msgSize);
//static void TWI_Start_TransceiverData(void);
static void TWI_ISR(void);
static void TWIProcess(void);

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
typedef unsigned int UINT;
typedef unsigned char BYTE;

typedef struct tagInfo
{
	BYTE szSign[2];
	BYTE nSize;
	BYTE PageSize;
	BYTE nBLVersion;
	BYTE CurrentAddr;
}INFO;

UINT g_BL_FlashAddr = 0;
INFO g_BL_CurrentInfo;

//用户程序起始地
#define PROG_START 0x0000

#define BOOTLOADER_ADDR_START_USER_APP 1 // 启动用户程序
#define BOOTLOADER_ADDR_GET_INFO 2 // 获得芯片信息
#define BOOTLOADER_ADDR_UPLOAD 3 // 上传代码
#define BOOTLOADER_ADDR_RESET 4 // 复位指针


/****************************************************************************
Bit and byte definitions
****************************************************************************/
#define TWI_READ_BIT 0 // Bit position for R/W bit in "address byte".
#define TWI_ADR_BITS 1 // Bit position for LSB of the slave address bits in the init byte.
#define TWI_GEN_BIT 0 // Bit position for LSB of the general call bit in the init byte.

#define TRUE 1
#define FALSE 0

/****************************************************************************
TWI State codes
****************************************************************************/
// General TWI Master staus codes
#define TWI_START 0x08 // START has been transmitted
#define TWI_REP_START 0x10 // Repeated START has been transmitted
#define TWI_ARB_LOST 0x38 // Arbitration lost

// TWI Master Transmitter staus codes
#define TWI_MTX_ADR_ACK 0x18 // SLA+W has been tramsmitted and ACK received
#define TWI_MTX_ADR_NACK 0x20 // SLA+W has been tramsmitted and NACK received
#define TWI_MTX_DATA_ACK 0x28 // Data byte has been tramsmitted and ACK received
#define TWI_MTX_DATA_NACK 0x30 // Data byte has been tramsmitted and NACK received

// TWI Master Receiver staus codes
#define TWI_MRX_ADR_ACK 0x40 // SLA+R has been tramsmitted and ACK received
#define TWI_MRX_ADR_NACK 0x48 // SLA+R has been tramsmitted and NACK received
#define TWI_MRX_DATA_ACK 0x50 // Data byte has been received and ACK tramsmitted
#define TWI_MRX_DATA_NACK 0x58 // Data byte has been received and NACK tramsmitted

// TWI Slave Transmitter staus codes
#define TWI_STX_ADR_ACK 0xA8 // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0 // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK 0xB8 // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK 0xC0 // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8 // Last data byte in TWDR has been transmitted (TWEA = ??; ACK has been received

// TWI Slave Receiver staus codes
#define TWI_SRX_ADR_ACK 0x60 // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68 // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK 0x70 // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78 // Arbitration lost in SLA+R/W as Master;General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK 0x80 // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK 0x88 // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK 0x90 // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK 0x98 // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART 0xA0 // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE 0xF8 // No relevant state information available;TWINT = ??
#define TWI_BUS_ERROR 0x00 // Bus error due to an illegal START or STOP condition


//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
// TWI_Slave.c 文件内容
//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
static unsigned char TWI_buf[TWI_BUFFER_SIZE]; // Transceiver buffer. Set the size in the header file
static unsigned int TWI_msgSize = 0; // Number of bytes to be transmitted.
static unsigned char TWI_state = TWI_NO_STATE; // State byte. Default set to TWI_NO_STATE.

union TWI_statusReg TWI_statusReg = {0}; // TWI_statusReg is defined in TWI_Slave.h


BOOTLOADER_SECTION int main(void)
{
	int nCount=0;
	
	cli();
	BL_PORT_INIT();
	
	memset(TWI_buf,0,TWI_BUFFER_SIZE);
	g_BL_FlashAddr = 0;
	g_BL_CurrentInfo.szSign[0] = 'B';
	g_BL_CurrentInfo.szSign[1] = 'L';
	g_BL_CurrentInfo.nSize = sizeof(g_BL_CurrentInfo);
	g_BL_CurrentInfo.nBLVersion = 1;
	g_BL_CurrentInfo.CurrentAddr = g_BL_FlashAddr;
	g_BL_CurrentInfo.PageSize = SPM_PAGESIZE;
	
	TWI_Slave_Initialise((TWI_ADDR<<TWI_ADR_BITS) | (0<<TWI_GEN_BIT));
	
	TWI_Start_Transceiver();
	
	while (1)
	{
		//while (TWI_Transceiver_Busy())
		if (TWCR & (1<<TWIE))
		{
			if (TWCR & (1<<TWINT))
			{
				TWI_ISR();
			}
		}
		
		TWIProcess();
		
		nCount++;
		
		if ((nCount >= 10000) && (nCount < 20000))
		{
			BL_LIGHT1();
		}
		else if (nCount >= 20000)
		{
			BL_LIGHT2();
			nCount = 0;
		}
	}
	return 0;
}

/****************************************************************************
Call this function to set up the TWI slave to its initial standby state.
Remember to enable interrupts from the main application after initializing the TWI.
Pass both the slave address and the requrements for triggering on a general call in the
same byte. Use e.g. this notation when calling this function:
TWI_Slave_Initialise( (TWI_slaveAddress<<TWI_ADR_BITS) | (TRUE<<TWI_GEN_BIT) );
The TWI module is configured to NACK on any requests. Use a TWI_Start_Transceiver function to
start the TWI.
****************************************************************************/
static BOOTLOADER_SECTION void TWI_Slave_Initialise( unsigned char TWI_ownAddress )
{
	TWAR = TWI_ownAddress; // Set own TWI slave address.
	Accept TWI General Calls.
	TWDR = 0xFF; // Default content = SDA released.
	TWCR = (1<<TWEN)| // Enable TWI‐interface and
	release TWI pins.
		(0<<TWIE)|(0<<TWINT)| // Disable TWI Interupt.
		(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Do not ACK on any requests,
	yet.
		(0<<TWWC); //
}

/****************************************************************************
Call this function to test if the TWI_ISR is busy transmitting.
****************************************************************************/
static BOOTLOADER_SECTION unsigned char TWI_Transceiver_Busy( void )
{
	return ( TWCR & (1<<TWIE) ); // IF TWI interrupt is enabled then the Transceiver is busy
}

/****************************************************************************
Call this function to fetch the state information of the previous operation. The function will hold
execution (loop)
until the TWI_ISR has completed with the previous operation. If there was an error, then the
function
will return the TWI State code.
****************************************************************************/
/*
static BOOTLOADER_SECTION unsigned char TWI_Get_State_Info( void )
{
	while ( TWI_Transceiver_Busy() ); // Wait until TWI has completed the transmission.
	return ( TWI_state ); // Return error state.
}
*/
/****************************************************************************
Call this function to send a prepared message, or start the Transceiver for reception. Include
a pointer to the data to be sent if a SLA+W is received. The data will be copied to the TWI buffer.
Also include how many bytes that should be sent. Note that unlike the similar Master function,
the
Address byte is not included in the message buffers.
The function will hold execution (loop) until the TWI_ISR has completed with the previous
operation,
then initialize the next operation and return.
****************************************************************************/
static BOOTLOADER_SECTION void TWI_Start_Transceiver_With_Data( unsigned char *msg,unsigned int msgSize )
{
	unsigned int temp;
	
	while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	
	TWI_msgSize = msgSize; // Number of data to transmit.
	for ( temp = 0; temp < msgSize; temp++ ) // Copy data that may be transmitted if the
	TWI Master requests data.
	TWI_buf[ temp ] = msg[ temp ];
	TWI_statusReg.all = 0;
	TWI_state = TWI_NO_STATE ;
	TWCR = (1<<TWEN)| // TWI Interface enabled.
	(1<<TWIE)|(1<<TWINT)| // Enable TWI Interupt and clear the flag.
	(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Prepare to ACK next time the Slave is addressed.
	(0<<TWWC); //
}

/*
static BOOTLOADER_SECTION void TWI_Prepare_Transceiver_CleanDataSize()
{
	while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	TWI_msgSize = 0;
}
*/

/*
static BOOTLOADER_SECTION void TWI_Prepare_Transceiver_With_Data( unsigned char *msg,unsigned int msgSize )
{
	unsigned int temp;
	
	while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	
	for ( temp = TWI_msgSize; temp < TWI_msgSize+msgSize; temp++ ) // Copy data that may be transmitted if the TWI Master requests data.
	{
		TWI_buf[temp] = msg[temp‐TWI_msgSize];
	}
	TWI_msgSize += msgSize;
}*/

/*
static BOOTLOADER_SECTION void TWI_Start_TransceiverData()
{
	while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	TWI_statusReg.all = 0;
	TWI_state = TWI_NO_STATE ;
	TWCR = (1<<TWEN)| // TWI Interface enabled.
		(1<<TWIE)|(1<<TWINT)| // Enable TWI Interupt and clear the flag.
		(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Prepare to ACK next time the Slave is addressed.
		(0<<TWWC); //
}*/

/****************************************************************************
Call this function to start the Transceiver without specifing new transmission data. Usefull for
restarting
a transmission, or just starting the transceiver for reception. The driver will reuse the data
previously put
in the transceiver buffers. The function will hold execution (loop) until the TWI_ISR has
completed with the
previous operation, then initialize the next operation and return.
****************************************************************************/
static BOOTLOADER_SECTION void TWI_Start_Transceiver( void )
{
	while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	TWI_statusReg.all = 0;
	TWI_state = TWI_NO_STATE ;
	TWCR = (1<<TWEN)| // TWI Interface enabled.
		(1<<TWIE)|(1<<TWINT)| // Enable TWI Interupt and clear the flag.
		(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Prepare to ACK next time the Slave is addressed.
		(0<<TWWC); //
}
/****************************************************************************
Call this function to read out the received data from the TWI transceiver buffer. I.e. first call
TWI_Start_Transceiver to get the TWI Transceiver to fetch data. Then Run this function to collect
the
data when they have arrived. Include a pointer to where to place the data and the number of
bytes
to fetch in the function call. The function will hold execution (loop) until the TWI_ISR has
completed
with the previous operation, before reading out the data and returning.
If there was an error in the previous transmission the function will return the TWI State code.
****************************************************************************/
/*
static BOOTLOADER_SECTION unsigned char TWI_Get_Data_From_Transceiver( unsigned char
*msg, unsigned char msgSize )
{
	unsigned int i;
	
	while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	
	if( TWI_statusReg.lastTransOK ) // Last transmission competed successfully.
	{
		for ( i=0; i<msgSize; i++ ) // Copy data from Transceiver buffer.
		{
			msg[ i ] = TWI_buf[ i ];
		}
	TWI_statusReg.RxDataInBuf = FALSE; // Slave Receive data has been read from buffer.
	}
	return( TWI_statusReg.lastTransOK );
}
*/

static BOOTLOADER_SECTION unsigned char* TWI_Get_Data_Pointer_From_Transceiver()
{
	//while ( TWI_Transceiver_Busy() ); // Wait until TWI is ready for next transmission.
	
	if( TWI_statusReg.lastTransOK ) // Last transmission competed successfully.
	{
		return TWI_buf;
	}
	else
	{
	return 0;
	}
}

static BOOTLOADER_SECTION void TWI_Get_Data_Pointer_From_Transceiver_Release()
{
	TWI_statusReg.RxDataInBuf = FALSE; // Slave Receive data has been read from buffer.
}


// ********** Interrupt Handlers ********** //
/****************************************************************************
This function is the Interrupt Service Routine (ISR), and called when the TWI interrupt is
triggered;
that is whenever a TWI event has occurred. This function should not be called directly from the
main
application.
****************************************************************************/
//BOOTLOADER_SECTION ISR(TWI_vect)
static BOOTLOADER_SECTION void TWI_ISR(void)
{
	static unsigned int TWI_bufPtr;
	switch (TWSR)
	{
	case TWI_STX_ADR_ACK: // Own SLA+R has been received; ACK has been returned
	// case TWI_STX_ADR_ACK_M_ARB_LOST: // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
	TWI_bufPtr = 0; // Set buffer pointer to first data location
	case TWI_STX_DATA_ACK: // Data byte in TWDR has been transmitted; ACK has been received
	TWDR = TWI_buf[TWI_bufPtr++];
	TWCR = (1<<TWEN)| // TWI Interface enabled
		(1<<TWIE)|(1<<TWINT)| // Enable TWI Interupt and clear the flag to send byte
		(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| //
		(0<<TWWC); //
	break;
	case TWI_STX_DATA_NACK: // Data byte in TWDR has been transmitted; NACKhas been received.
	// I.e. this could be the end of the transmission.
	if (TWI_bufPtr == TWI_msgSize) // Have we transceived all expected data?
	{
	TWI_statusReg.lastTransOK = TRUE; // Set status bits to completed successfully.
	}else // Master has sent a NACK before all data where sent.
	{
	TWI_state = TWSR; // Store TWI State as errormessage.
	}
	// Put TWI Transceiver in passive mode.
	TWCR = (1<<TWEN)| // Enable TWI‐interface and release TWI pins
		(0<<TWIE)|(0<<TWINT)| // Disable Interupt
		(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Do not acknowledge on any new requests.
		(0<<TWWC); //
	break;
	case TWI_SRX_GEN_ACK: // General call address has been received; ACK has been returned 
	// case TWI_SRX_GEN_ACK_M_ARB_LOST: 
	// Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
	TWI_statusReg.genAddressCall = TRUE;
	case TWI_SRX_ADR_ACK: // Own SLA+W has been received ACK has been returned
	// case TWI_SRX_ADR_ACK_M_ARB_LOST: // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
	// Dont need to clear TWI_S_statusRegister.generalAddressCall due to that it is the default state.
	TWI_statusReg.RxDataInBuf = TRUE;
	TWI_bufPtr = 0; // Set buffer pointer to first data location
	// Reset the TWI Interupt to wait for a new event
	TWCR = (1<<TWEN)| // TWI Interface enabled
			(1<<TWIE)|(1<<TWINT)| // Enable TWI Interupt and clear the flag to send byte
			(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Expect ACK on this transmission
			(0<<TWWC); //
	break;
	case TWI_SRX_ADR_DATA_ACK: // Previously addressed with own SLA+W; data has been received; ACK has been returned
	case TWI_SRX_GEN_DATA_ACK: // Previously addressed with general call; data has been received; ACK has been returned
	TWI_buf[TWI_bufPtr++] = TWDR;
	TWI_statusReg.lastTransOK = TRUE; // Set flag transmission successfull.
	// Reset the TWI Interupt to wait for a new event.
	TWCR = (1<<TWEN)| // TWI Interface enabled
			(1<<TWIE)|(1<<TWINT)| // Enable TWI Interupt and clear the flag to send byte
			(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Send ACK after next reception
			(0<<TWWC); //
	break;
	case TWI_SRX_STOP_RESTART: // A STOP condition or repeated START condition has been received while still addressed as Slave
	// Put TWI Transceiver in passive mode.
	TWCR = (1<<TWEN)| // Enable TWI‐interface and release TWI pins
			(0<<TWIE)|(0<<TWINT)| // Disable Interupt
			(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| // Do not acknowledge on any new requests.
			(0<<TWWC); //
	break;
	case TWI_SRX_ADR_DATA_NACK: // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
	case TWI_SRX_GEN_DATA_NACK: // Previously addressed with general call; data has been received; NOT ACK has been returned
	case TWI_STX_DATA_ACK_LAST_BYTE: // Last data byte in TWDR has been transmitted (TWEA = ??; ACK has been received
	// case TWI_NO_STATE // No relevant state information available; TWINT= ??
	case TWI_BUS_ERROR: // Bus error due to an illegal START or STOP condition
	default:
	TWI_state = TWSR; // Store TWI State as errormessage, operation also clears the Success bit.
	TWCR = (1<<TWEN)| // Enable TWI‐interface and release TWI pins
			(0<<TWIE)|(0<<TWINT)| // Disable Interupt
			(0<<TWEA)|(0<<TWSTA)|(1<<TWSTO)| // Do not acknowledge on any new requests.
	(0<<TWWC); //
	}
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
// 使用静态函数确保BootLoader 的main 位于bootloader 端的最开始，
// 这样可以通过直接从Bootloader 段启动的融丝位直接跑到BootLoader 里。
//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

//更新一个Flash 页
static BOOTLOADER_SECTION void BootLoader_WriteOnePage(BYTE *pBuffer)
{
	UINT PagePointer;
	if (g_BL_FlashAddr < BL_ADDR)
	{
		boot_page_erase(g_BL_FlashAddr); //擦除一个Flash 页
		boot_spm_busy_wait();
		for (PagePointer = 0; PagePointer < SPM_PAGESIZE; PagePointer += 2)
		{
			boot_page_fill(g_BL_FlashAddr + PagePointer, pBuffer[PagePointer] |
			((pBuffer[PagePointer + 1] << 8))); // 一次写入一个WORD，2 个BYTE。
		}
	boot_page_write(g_BL_FlashAddr); //将缓冲页数据写入一个Flash 页
	boot_spm_busy_wait(); //等待页编程完成
	g_BL_FlashAddr += SPM_PAGESIZE;
	g_BL_CurrentInfo.CurrentAddr = g_BL_FlashAddr/SPM_PAGESIZE;
	}
}

// CRC16 校验
static BOOTLOADER_SECTION void CRC16(BYTE *buf,int nSize,BYTE *pHighByte,BYTE *pLowByte)
{
	unsigned int j;
	unsigned char i;
	unsigned int t;
	unsigned int crc;
	crc = 0;
	for(j = nSize; j > 0; j‐‐)
	{
		//标准CRC 校验
		crc = (crc ^ (((unsigned int) *buf) << 8));
		for(i = 8; i > 0; i‐‐)
		{
			t = crc << 1;
			if(crc & 0x8000)
			t = t ^ 0x1021;
			crc = t;
		}
		buf++;
	}
	*pHighByte = crc / 256;
	*pLowByte = crc % 256;
}

//跳转到用户程序
static BOOTLOADER_SECTION void quit()
{
	boot_rww_enable(); //允许用户程序区读写
	TWCR = (0<<TWEN)| // Disable TWI‐interface and release TWI pins
		(0<<TWIE)|(1<<TWINT)| // Disable Interupt
		(0<<TWEA)|(0<<TWSTA)|(1<<TWSTO)| // Do not acknowledge on any new requests.
		(0<<TWWC); //
	(*((void(*)(void))PROG_START))(); //跳转，这样比'jmp 0'节省空间
}

static BOOTLOADER_SECTION void TWIProcess()
{
	BYTE *pMessageBuf = 0;
	BYTE CRCHigh = 0;
	BYTE CRCLow = 0;
	BYTE nInternalAddr = 0;
	// TWI 接收器是否忙？
	if (!TWI_Transceiver_Busy())
	{
		// 上次操作是否成功？
		if (TWI_statusReg.lastTransOK)
		{
			// 最后一次操作是否收到了请求？
				if (TWI_statusReg.RxDataInBuf)
				{
					pMessageBuf = TWI_Get_Data_Pointer_From_Transceiver();
					nInternalAddr = pMessageBuf[0];
					pMessageBuf = &pMessageBuf[1];
					//TWI_Get_Data_From_Transceiver(messageBuf, 2);
					// 是否为一个0 地址的广播消息？
					if (TWI_statusReg.genAddressCall)
					{
						// 0 地址的广播消息，我们不去理会。
					}
					else
					{
						// 亮灯提示进度
						/*
						if (PINB & (1<<PB2))
						{
							PORTB &= ~(1<<PB2);
						}
						else
						{
							PORTB |= (1<<PB2);
						}
						*/
						
						// 地址正确，下面解析收到的命令
						
						//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
						// 写命令部分
						if (nInternalAddr == BOOTLOADER_ADDR_START_USER_APP)
						{
							// 跳转到用户代码区。
							quit();
							
							while(1);
						}
						else if (nInternalAddr == BOOTLOADER_ADDR_RESET)
						{
							g_BL_FlashAddr = 0;
							g_BL_CurrentInfo.CurrentAddr = 0;
						}
						else if (nInternalAddr == BOOTLOADER_ADDR_UPLOAD)
						{
							// 上传用户代码
							// 前SPM_PAGESIZE 个字节为数据，后2 个字节为CRC16 校验。
							CRC16(pMessageBuf,SPM_PAGESIZE,&CRCHigh,&CRCLow);
							if ((CRCHigh == pMessageBuf[SPM_PAGESIZE]) && (CRCLow ==pMessageBuf[SPM_PAGESIZE+1]))
							{
								// CRC 正确，将舵量信息保存下来。
								// 将这个扇区的内容写入。
								BootLoader_WriteOnePage(pMessageBuf);
							}
							TWI_Get_Data_Pointer_From_Transceiver_Release();
						}
						//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
						//读命令部分
						else if (nInternalAddr == BOOTLOADER_ADDR_GET_INFO)
						{
							TWI_Start_Transceiver_With_Data((BYTE*)&g_BL_CurrentInfo,sizeof(g_BL_CurrentInfo));
							TWI_Get_Data_Pointer_From_Transceiver_Release();
						}
					}
				}
				// Check if the TWI Transceiver has already been started.
				// If not then restart it to prepare it for new receptions.
				if (!TWI_Transceiver_Busy())
				{
					TWI_Start_Transceiver();
				}
		}
		else
		{
			// 上次操作不成功，重新启动接收器
			//TWI_Act_On_Failure_In_Last_Transmission( TWI_Get_State_Info() );
			TWI_Start_Transceiver();
		}
	}
}
/* 测试用
int main(void)
{
	while(1);
	main1();
	DDRB = (1<<DDB2) | (1<<DDB1) | (1<< DDB0);
	while (1)
	{
		PORTB = (1<<PB2);
		_delay_ms(200);
		PORTB = 0;
		_delay_ms(200);
	}
}
*/
