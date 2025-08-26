#include <util/delay.h>

#include "rtl8019.h"
#include <avr/io.h>
#include "nic.h"
typedef uint8_t u08;

/*****************************************************************************
*  Module Name:       Realtek 8019AS Driver
*  
*  Created By:        Louis Beaudoin (www.embedded-creations.com)
*
*  Original Release:  September 21, 2002 
*
*  Module Description:  
*  Provides functions to initialize the Realtek 8019AS, and send and retreive
*  packets
*
*  November 16, 2003 - Louis Beaudoin
*    The rtl8019Write and Read functions/macros were changed to support
*      three methods of communcating with the NIC
*    Interfacing with the AVR ports changed from sbi/cbi/etc functions
*      to direct port names
*    Renamed functions to be more consistant with the two NIC drivers
*    Overrun function now retransmits if resend is set (thanks Krzysztof)
* 
*  November 15, 2002 - Louis Beaudoin
*    processRTL8019Interrupt() - bit mask mistake fixed
*
*  November 8, 2003 - Louis Beaudoin
*    Changed delay library function calls
*
*  September 30, 2002 - Louis Beaudoin
*    Receive functions modified to handle errors encountered when receiving a
*      fast data stream.  Functions now manually retreive data instead of
*      using the send packet command.  Interface improved by checking for
*      overruns and data in the buffer internally.
*    Corrected the overrun function - overrun flag was not reset after overrun
*    Added support for the Imagecraft Compiler
*    Added support to communicate with the NIC using general I/O ports
*
*****************************************************************************/


/*****************************************************************************
*  rtl8019Write( RTL_ADDRESS, RTL_DATA )
*  Args:        1. unsigned char RTL_ADDRESS - register offset of RTL register
*               2. unsigned char RTL_DATA - data to write to register
*  Created By:  Louis Beaudoin
*  Date:        September 21, 2002
*  Description: Writes byte to RTL8019 register.
*
*  Notes - If using the External SRAM Interface, performs a write to
*            address MEMORY_MAPPED_RTL8019_OFFSET + (RTL_ADDRESS<<8)
*            The address is sent in the non-multiplxed upper address port so
*            no latch is required.
*
*          If using general I/O ports, the data port is left in the input
*            state with pullups enabled
*
*****************************************************************************/
#if NIC_CONNECTION == MEMORY_MAPPED_HIGHADDR
#define rtl8019Write(RTL_ADDRESS,RTL_DATA) do{ *(volatile unsigned char *) \
                             (MEMORY_MAPPED_RTL8019_OFFSET \
                             + (((unsigned char)(RTL_ADDRESS)) << 8)) = \
                             (unsigned char)(RTL_DATA); } while(0)

#endif
                             
#if NIC_CONNECTION == MEMORY_MAPPED
#define rtl8019Write(RTL_ADDRESS,RTL_DATA) do{ *(volatile unsigned char *) \
                             (MEMORY_MAPPED_RTL8019_OFFSET \
                             + (unsigned char)(RTL_ADDRESS)) = \
                             (unsigned char)(RTL_DATA); } while(0)

#endif

#if NIC_CONNECTION == GENERAL_IO

void rtl8019Write(unsigned char address, unsigned char data)
{
    // assert the address, leaving the non-address pins intact
    address |= (RTL8019_ADDRESS_PORT & ~RTL8019_ADDRESS_MASK);
    RTL8019_ADDRESS_PORT = address;
    
	// set data bus as output and place data on bus
    RTL8019_DATA_DDR = 0xFF;
    RTL8019_DATA_PORT = data;
    
	// toggle write pin
    RTL8019_CONTROL_PORT &= ~_BV(RTL8019_CONTROL_WRITEPIN);
    nop();
    RTL8019_CONTROL_PORT |= _BV(RTL8019_CONTROL_WRITEPIN);

    
	// set data port back to input with pullups enabled
    RTL8019_DATA_DDR = 0x00;
    RTL8019_DATA_PORT = 0xFF;
}

#endif

/*****************************************************************************
*  rtl8019Read(RTL_ADDRESS)
*  Args:        unsigned char RTL_ADDRESS - register offset of RTL register
*  Created By:  Louis Beaudoin_MAR
*  Date:        September 21, 2002
*  Description: Reads byte from RTL8019 register
*
*  Notes - If using the External SRAM Interface, performs a read from
*            address MEMORY_MAPPED_RTL8019_OFFSET + (RTL_ADDRESS<<8)
*            The address is sent in the non-multiplxed upper address port so
*            no latch is required.
*
*          If using general I/O ports, the data port is assumed to already be
*            an input, and is left as an input port when done
*
*****************************************************************************/
#if NIC_CONNECTION == MEMORY_MAPPED_HIGHADDR
#define rtl8019Read(RTL_ADDRESS) (*(volatile unsigned char *) \
                       (MEMORY_MAPPED_RTL8019_OFFSET \
                       + (((unsigned char)(RTL_ADDRESS)) << 8)) )
#endif
                             
#if NIC_CONNECTION == MEMORY_MAPPED

#define rtl8019Read(RTL_ADDRESS) (*(volatile unsigned char *) \
                       (MEMORY_MAPPED_RTL8019_OFFSET \
                       + (unsigned char)(RTL_ADDRESS)) )
#endif

#if NIC_CONNECTION == GENERAL_IO

unsigned char rtl8019Read(unsigned char address)
{
    unsigned char byte;
   
    // assert the address, leaving the non-address pins intact
    address |= (RTL8019_ADDRESS_PORT & ~RTL8019_ADDRESS_MASK);
    RTL8019_ADDRESS_PORT = address;
   
    // assert read
    RTL8019_CONTROL_PORT &= ~_BV(RTL8019_CONTROL_READPIN);
    nop();
   
    // read in the data
    byte = RTL8019_DATA_PIN;

    // negate read
    RTL8019_CONTROL_PORT |= _BV(RTL8019_CONTROL_READPIN);

    return byte;
}

#endif                       



/*****************************************************************************
*  rtl8019SetupPorts(void);
*
*  Created By:  Louis Beaudoin
*  Date:        September 21, 2002
*  Description: Sets up the ports used for communication with the RTL8019 NIC
*                 (data bus, address bus, read, write, and reset)
*****************************************************************************/
void rtl8019SetupPorts(void)
{

#if NIC_CONNECTION == GENERAL_IO

    // make the address port output
	RTL8019_ADDRESS_DDR = RTL8019_ADDRESS_MASK;
    
    // make the data port input with pull-ups
    RTL8019_DATA_PORT = 0xFF;

	// make the control port read and write pins outputs and asserted
	RTL8019_CONTROL_DDR |= _BV(RTL8019_CONTROL_READPIN);
    RTL8019_CONTROL_DDR |= _BV(RTL8019_CONTROL_WRITEPIN);
	          
	RTL8019_CONTROL_PORT |= RTL8019_CONTROL_READPIN;
	RTL8019_CONTROL_PORT |= RTL8019_CONTROL_WRITEPIN;

/*#else

  	// enable external SRAM interface - no wait states
    MCUCR |= _BV(SRE);*/

#endif

	// enable output pin for Resetting the RTL8019
	RTL8019_RESET_DDR |= RTL8019_RESET_PIN;
}



/*****************************************************************************
*  HARD_RESET_RTL8019()
*
*  Created By:  Louis Beaudoin
*  Date:        September 21, 2002
*  Description: Simply toggles the pin that resets the NIC
*****************************************************************************/
#define HARD_RESET_RTL8019() do{ RTL8019_RESET_PORT |= RTL8019_RESET_PIN; \
                                delay_ms(10); \
                                RTL8019_RESET_PORT &= ~RTL8019_RESET_PIN;} \
                                while(0)



/*****************************************************************************
*  rtl8019Overrun(void);
*
*  Created By:  Louis Beaudoin
*  Date:        September 21, 2002
*  Description: "Canned" receive buffer overrun function originally from
*                 a National Semiconductor appnote
*  Notes:       This function must be called before retreiving packets from
*                 the NIC if there is a buffer overrun
*****************************************************************************/
void rtl8019Overrun(void);




//******************************************************************
//*	REALTEK CONTROL REGISTER OFFSETS
//*   All offsets in Page 0 unless otherwise specified
//*	  All functions accessing CR must leave CR in page 0 upon exit
//******************************************************************
#define CR		 	0x00	// Command Register
#define PSTART		0x01	// Page Start Register
#define PSTOP		0x02	// Page Stop Register
#define BNRY		0x03	// Boundary Pointer
#define RDMAPORT  	0x10	// DMA Data Port
#define MEMR		0x14	// MII/EEPROM Access Register
#define TR			0x15	// Test Register
#define SPP_DPR    	0x18	// Standard Printer Port Data
#define SSP_SPR		0x19	// Standard Printer Port Status
#define SSP_CPR		0x1A	// Standard Printer Port Control
// Page 0 - Read
#define TSR			0x04	// Transmit Status Register
#define NCR			0x05	// Number of Collisions Register
#define ISRR			0x07	// Interrupt Status Register
#define CRDA0		0x08	// Current Remote DMA Address 0
#define CRDA1		0x09	// Current Remote DMA Address 1
#define RSR			0x0C	// Receive Status Register
#define CNTR0		0x0D
#define CNTR1		0x0E
#define CNTR2		0x0F
#define GPI			0x17	// General-Purpose Input
#define RSTPORT		0x1F	// Reset
// Page 0 - Write
#define TPSR		0x04	// Transmit Page Start Address
#define TBCR0		0x05	// Transmit Byte Count Register 0
#define TBCR1		0x06	// Transmit Byte Count Register 1
#define RSAR0		0x08	// Remote Start Address Register 0
#define RSAR1		0x09	// Remote Start Address Register 1
#define RBCR0		0x0A	// Remote Byte Count 0
#define RBCR1		0x0B	// Remote Byte Count 1
#define RCR			0x0C	// Receive Config Register
#define TCR			0x0D	// Transmit Config Register
#define DCR			0x0E	// Data Config Register
#define IMR			0x0F	// Interrupt Mask Register
#define GPOC		0x17	// General-Purpose Output Control
// Page 1 - Read/Write
#define PAR0      	0x01	// Physical Address Register 0
#define PAR1      	0x02	// Physical Address Register 1
#define PAR2      	0x03	// Physical Address Register 2
#define PAR3      	0x04	// Physical Address Register 3
#define PAR4      	0x05	// Physical Address Register 4
#define PAR5      	0x06	// Physical Address Register 5
#define CURR		0x07	// Page 1
#define CPR			0x07	// Current Page Register

#define RTL_EECR	0x01    // page 3
#define CR9346    	0x01    // Page 3
#define CONFIG2     0x05    // page 3
#define CONFIG3     0x06    // page 3

// RTL8019/NE2000 CR Register Bit Definitions
#define  PS1		0x80 
#define  PS0		0x40 
#define  RD2		0x20 
#define  RD1		0x10 
#define  RD0		0x08 
#define  TXP		0x04 
#define  START		0x02 
#define  STOP		0x01 
// RTL8019/NE2000 ISRR Register Bit Definitions
#define  RST		0x80
#define  RDC		0x40
#define  OVW		0x10
#define  RXE		0x08
#define  TXE		0x04
#define  PTX		0x02
#define  PRX		0x01
// RTL8019/NE2000 RCR Register Bit Definitions
#define  MON		0x20
#define  PRO		0x10
#define  AM			0x08
#define  AB			0x04
#define  AR			0x02
#define  SEP		0x01
// RTL8019/NE2000 TCR Register Bit Definitions
#define  FDU		0x80	// full duplex
#define  PD			0x40	// pad disable
#define  RLO		0x20	// retry of late collisions
#define  LB1		0x04	// loopback 1
#define  LB0		0x02	// loopback 0
#define  CRC		0x01	// generate CRC
// RTL8019 EECR Register Bit Definitions
#define  EEM1		0x80
#define  EEM0		0x40
#define  EECS		0x08
#define  EESK		0x04
#define  EEDI		0x02
#define  EEDO		0x01


// RTL8019 Initial Register Values
// RCR : INT trigger active high and Accept Broadcast ENET packets
#define RCR_INIT		(PRO)
#define DCR_INIT		0x58	// FIFO thrsh. 8bits, 8bit DMA transfer
// TCR : default transmit operation - CRC is generated
#define TCR_INIT		0x00
// IMR : interrupt enabled for receive and overrun events
#define IMR_INIT		0x11    // PRX and OVW interrupt enabled
// buffer boundaries
//	transmit has 6 256-byte pages
//	receive has 26 256-byte pages
//	entire available packet buffer space is allocated
#define TXSTART_INIT   	0x40
#define RXSTART_INIT   	0x46
#define RXSTOP_INIT    	0x60

void nicInit(void)
{
	rtl8019Init();
}

void nicSend(unsigned int len, unsigned char* packet)
{
	rtl8019BeginPacketSend(len);
	rtl8019SendPacketData(packet, len);
	rtl8019EndPacketSend();
}

unsigned int nicPoll(unsigned int maxlen, unsigned char* packet)
{
	unsigned int packetLength;
	
	packetLength = rtl8019BeginPacketRetreive();

	// if there's no packet or an error - exit without ending the operation
	if( !packetLength )
		return 0;

	// drop anything too big for the buffer
	if( packetLength > maxlen )
	{
		rtl8019EndPacketRetreive();
		return 0;
	}
	
	// copy the packet data into the packet buffer
	rtl8019RetreivePacketData( packet, packetLength );
	rtl8019EndPacketRetreive();
		
	return packetLength;
}

void nicGetMacAddress(u08* macaddr)
{
	u08 tempCR;
	// switch register pages
	tempCR = rtl8019Read(CR);
	rtl8019Write(CR,tempCR|PS0);
	// read MAC address registers
	*macaddr++ = rtl8019Read(PAR0);
	*macaddr++ = rtl8019Read(PAR1);
	*macaddr++ = rtl8019Read(PAR2);
	*macaddr++ = rtl8019Read(PAR3);
	*macaddr++ = rtl8019Read(PAR4);
	*macaddr++ = rtl8019Read(PAR5);
	// switch register pages back
	rtl8019Write(CR,tempCR);
}

void nicSetMacAddress(u08* macaddr)
{
	u08 tempCR;
	// switch register pages
	tempCR = rtl8019Read(CR);
	rtl8019Write(CR,tempCR|PS0);
	// write MAC address registers
	rtl8019Write(PAR0, *macaddr++);
	rtl8019Write(PAR1, *macaddr++);
	rtl8019Write(PAR2, *macaddr++);
	rtl8019Write(PAR3, *macaddr++);
	rtl8019Write(PAR4, *macaddr++);
	rtl8019Write(PAR5, *macaddr++);
	// switch register pages back
	rtl8019Write(CR,tempCR);
}


void rtl8019BeginPacketSend(unsigned int packetLength)
{
	unsigned int sendPacketLength;
	sendPacketLength = (packetLength>=ETHERNET_MIN_PACKET_LENGTH)?
						(packetLength):ETHERNET_MIN_PACKET_LENGTH;
	
	//start the NIC
	rtl8019Write(CR, (RD2|START));
	
	// still transmitting a packet - wait for it to finish
	while( rtl8019Read(CR) & TXP );

	// load beginning page for transmit buffer
	rtl8019Write(TPSR,TXSTART_INIT);
	
	// set start address for remote DMA operation
	rtl8019Write(RSAR0,0x00);
	rtl8019Write(RSAR1,0x40);
	
	// clear the packet stored interrupt
	rtl8019Write(ISRR,PTX);

	// load data byte count for remote DMA
	rtl8019Write(RBCR0, (unsigned char)(packetLength));
	rtl8019Write(RBCR1, (unsigned char)(packetLength>>8));

	rtl8019Write(TBCR0, (unsigned char)(sendPacketLength));
	rtl8019Write(TBCR1, (unsigned char)((sendPacketLength)>>8));
	
	// do remote write operation
	rtl8019Write(CR,(RD1|START));
}



void rtl8019SendPacketData(unsigned char * localBuffer, unsigned int length)
{
	unsigned int i;
	
	// write data to DMA port
	for(i=0;i<length;i++)
		rtl8019Write(RDMAPORT, localBuffer[i]);
}



void rtl8019EndPacketSend(void)
{
	//send the contents of the transmit buffer onto the network
	rtl8019Write(CR,(RD2|TXP));
	// clear the remote DMA interrupt
	rtl8019Write(ISRR, RDC);
}




// pointers to locations in the RTL8019 receive buffer
static unsigned char NextPage;
static unsigned int CurrentRetreiveAddress;

// location of items in the RTL8019's page header
#define  PKTHEADER_STATUS		0x00	// packet status
#define  PKTHEADER_NEXTPAGE		0x01	// next buffer page
#define	 PKTHEADER_PKTLENL		0x02	// packet length low
#define	 PKTHEADER_PKTLENH		0x03	// packet length high

unsigned int rtl8019BeginPacketRetreive(void)
{
	unsigned char i;
	unsigned char bnry;
	
	unsigned char pageheader[4];
	unsigned int rxlen;
	
	// check for and handle an overflow
	rtl8019ProcessInterrupt();
	
	// read CPR from page 1
	rtl8019Write(CR,(PS0|RD2|START));
	i = rtl8019Read(CPR);
	
	// return to page 0
	rtl8019Write(CR,(RD2|START));
	
	// read the boundary register - pointing to the beginning of the packet
	bnry = rtl8019Read(BNRY) ;
	
	// return if there is no packet in the buffer
	if( bnry == i )
		return 0;

	// clear the packet received interrupt flag
	rtl8019Write(ISRR, PRX);
	
	// if boundary pointer is invalid
	if( (bnry >= RXSTOP_INIT) || (bnry < RXSTART_INIT) )
	{
		// reset the contents of the buffer and exit
		rtl8019Write(BNRY, RXSTART_INIT);
		rtl8019Write(CR, (PS0|RD2|START));
		rtl8019Write(CPR, RXSTART_INIT);
		rtl8019Write(CR, (RD2|START));
		return 0;
	}

	// initiate DMA to transfer the RTL8019 packet header
	rtl8019Write(RBCR0, 4);
	rtl8019Write(RBCR1, 0);
	rtl8019Write(RSAR0, 0);
	rtl8019Write(RSAR1, bnry);
	rtl8019Write(CR, (RD0|START));
	// transfer packet header
	for(i=0;i<4;i++)
		pageheader[i] = rtl8019Read(RDMAPORT);
	// end the DMA operation
	rtl8019Write(CR, (RD2|START));
	// wait for remote DMA complete
	for(i = 0; i < 20; i++)
		if(rtl8019Read(ISRR) & RDC)
			break;
	rtl8019Write(ISRR, RDC);

	rxlen = (pageheader[PKTHEADER_PKTLENH]<<8) + pageheader[PKTHEADER_PKTLENL];
	NextPage = pageheader[PKTHEADER_NEXTPAGE];
	
	CurrentRetreiveAddress = (bnry<<8) + 4;
	
	// if the NextPage pointer is invalid, the packet is not ready yet - exit
	if( (NextPage >= RXSTOP_INIT) || (NextPage < RXSTART_INIT) )
		return 0;
    
	return rxlen-4;
}


void rtl8019RetreivePacketData(unsigned char * localBuffer, unsigned int length)
{
	unsigned int i;
	
	// initiate DMA to transfer the data
	rtl8019Write(RBCR0, (unsigned char)length);
	rtl8019Write(RBCR1, (unsigned char)(length>>8));
	rtl8019Write(RSAR0, (unsigned char)CurrentRetreiveAddress);
	rtl8019Write(RSAR1, (unsigned char)(CurrentRetreiveAddress>>8));
	rtl8019Write(CR, (RD0|START));
	// transfer packet data
	for(i=0;i<length;i++)
		localBuffer[i] = rtl8019Read(RDMAPORT);
	// end the DMA operation
	rtl8019Write(CR, (RD2|START));
	// wait for remote DMA complete
	for(i=0; i<20; i++)
		if(rtl8019Read(ISRR) & RDC)
			break;
	rtl8019Write(ISRR, RDC);
	// keep track of current address
    CurrentRetreiveAddress += length;
    if( CurrentRetreiveAddress >= 0x6000 )
    	CurrentRetreiveAddress = CurrentRetreiveAddress - (0x6000-0x4600) ;
}



void rtl8019EndPacketRetreive(void)
{
	unsigned char i;

	// end the DMA operation
	rtl8019Write(CR, (RD2|START));
	// wait for remote DMA complete
	for(i=0; i<20; i++)
		if(rtl8019Read(ISRR) & RDC)
			break;
	rtl8019Write(ISRR, RDC);

	// set the boundary register to point to the start of the next packet
	rtl8019Write(BNRY, NextPage);
}

void rtl8019ProcessInterrupt(void)
{
	unsigned char byte = rtl8019Read(ISRR);
	
	if( byte & OVW )
		rtl8019Overrun();
}


void rtl8019Overrun(void)
{
	unsigned char data_L, resend;	

	data_L = rtl8019Read(CR);
	rtl8019Write(CR, 0x21);
	_delay_ms(2);
	rtl8019Write(RBCR0, 0x00);
	rtl8019Write(RBCR1, 0x00);
	if(!(data_L & 0x04))
		resend = 0;
	else if(data_L & 0x04)
	{
		data_L = rtl8019Read(ISRR);
		if((data_L & 0x02) || (data_L & 0x08))
	    	resend = 0;
	    else
	    	resend = 1;
	}
	
	rtl8019Write(TCR, 0x02);
	rtl8019Write(CR, 0x22);
	rtl8019Write(BNRY, RXSTART_INIT);
	rtl8019Write(CR, 0x62);
	rtl8019Write(CPR, RXSTART_INIT);
	rtl8019Write(CR, 0x22);
	rtl8019Write(ISRR, 0x10);
	rtl8019Write(TCR, TCR_INIT);
	
    if(resend)
        rtl8019Write(CR, 0x26);

    rtl8019Write(ISRR, 0xFF);
}


void rtl8019Init(void)
{
	
	
	// clear interrupt state
	rtl8019Write( ISRR, rtl8019Read(ISRR) );
	_delay_ms(50);

	// switch to page 3 to load config registers
	rtl8019Write(CR, (PS0|PS1|RD2|STOP));

	// disable EEPROM write protect of config registers
	rtl8019Write(RTL_EECR, (EEM1|EEM0));

    // set network type to 10 Base-T link test
	rtl8019Write(CONFIG2, 0x20);

    // disable powerdown and sleep
	rtl8019Write(CONFIG3, 0);
	_delay_ms(255);

    // reenable EEPROM write protect
	rtl8019Write(RTL_EECR, 0);

    // go back to page 0, stop NIC, abort DMA
	rtl8019Write(CR, (RD2|STOP));
	_delay_ms(2);					// wait for traffic to complete
	rtl8019Write(DCR, DCR_INIT);
	rtl8019Write(RBCR0,0x00);
	rtl8019Write(RBCR1,0x00);
	
	#ifdef RTL8019_PROMISCUOUS
	rtl8019Write(RCR, AB | AM | PRO);
	#else
	rtl8019Write(RCR, AB);
	#endif
	rtl8019Write(TPSR, TXSTART_INIT);
	rtl8019Write(TCR, LB0);
	rtl8019Write(PSTART, RXSTART_INIT);
	rtl8019Write(BNRY, RXSTART_INIT);
	rtl8019Write(PSTOP, RXSTOP_INIT);
	rtl8019Write(CR, (PS0|RD2|STOP));	// switch to page 1
	_delay_ms(2);
	rtl8019Write(CPR, RXSTART_INIT);
	
	// set MAC address
	rtl8019Write(PAR0, MYMAC_0);
	rtl8019Write(PAR1, MYMAC_1);
	rtl8019Write(PAR2, MYMAC_2);
	rtl8019Write(PAR3, MYMAC_3);
	rtl8019Write(PAR4, MYMAC_4);
	rtl8019Write(PAR5, MYMAC_5);
    
	// initialize sequence per NE2000 spec
	rtl8019Write(CR, (RD2|STOP));
	rtl8019Write(DCR, DCR_INIT);
	rtl8019Write(CR, (RD2|START));
	rtl8019Write(ISRR,0xFF);			// clear all interrupts
	rtl8019Write(IMR, IMR_INIT);
	rtl8019Write(TCR, TCR_INIT);
	
	rtl8019Write(CR, (RD2|START));	// start the NIC
}





