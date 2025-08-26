/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 *
 * Ethernet remote device and sensor
 *
 * Title: Microchip ENC28J60 Ethernet Interface Driver
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/iom128a.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ip_arp_udp.h"
#include "rtl8019.h"
#include "net.h"

// please modify the following two lines. MAC and IP have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x00,0x06,0x98,0xf0,0x11,0x23};
static uint8_t myip[4] = {192,168,1,130};
static uint16_t myport =1200; // listen port for UDP

#define BUFFER_SIZE 250
static uint8_t buf[BUFFER_SIZE+1];

// the password string (only the first 3 char checked):
static char password[]="mar";

// 
uint8_t verify_password(char *str)
{
        // the first characters of the received string are
        // a simple password/cookie:
        if (strncmp(password,str,3)==0){
                return(1);
        }
        return(0);
}

int main(void){

        
        uint16_t plen;
        uint8_t cmd_pos=0;
        uint8_t payloadlen=0;
		uint16_t result;
        char str[30];
        char cmdval;
		DDRB |= _BV(PORTB6);
		ADMUX = (1<<REFS0);
     	
        cli();

		// Init External Memory
		MCUCR |= _BV(SRE);
        
        
		nicInit();
        
        //init the ethernet/ip layer:
        init_ip_arp_udp(mymac,myip);
		
		sei();
        while(1){
                // get the next new packet:
                plen = nicPoll(BUFFER_SIZE, buf);

                /*plen will be unequal to zero if there is a valid 
                 * packet (without crc error) */
                if(plen==0){
                        continue;
                }
                        
                // arp is broadcast if unknown but a host may also
                // verify the mac address by sending it to 
                // a unicast address.
                if(eth_type_is_arp_and_my_ip(buf,plen)){
                        make_arp_answer_from_request(buf,plen);
                        continue;
                }
                // check if ip packets (icmp or udp) are for us:
                if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                        continue;
                }
                
                if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
                        // a ping packet, let's send pong
                        make_echo_reply_from_request(buf,plen);
                        continue;
                }
                // we listen on port 1200=0x4B0
                if (buf[IP_PROTO_P]==IP_PROTO_UDP_V&&buf[UDP_DST_PORT_H_P]==4&&buf[UDP_DST_PORT_L_P]==0xb0){
                        payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
                        // you must sent a string starting with v
                        if (verify_password((char *)&(buf[UDP_DATA_P]))){
                                // find the first comma which indicates 
                                // the start of a command:
                                cmd_pos=0;
                                while(cmd_pos<payloadlen){
                                        cmd_pos++;
                                        if (buf[UDP_DATA_P+cmd_pos]==','){
                                                cmd_pos++; // put on start of cmd
                                                break;
                                        }
                                }
                                // a command is one char and a value. At
                                // least 3 characters long. It has an '=' on
                                // position 2:
                                if (cmd_pos<2 || cmd_pos>payloadlen-3 || buf[UDP_DATA_P+cmd_pos+1]!='='){
                                        strcpy(str,"err=noCommand");
                                        goto ANSWER;
                                }
                                // supported commands are
                                // c=0 c=1 c=2
                                if (buf[UDP_DATA_P+cmd_pos]=='c'){
                                        cmdval=buf[UDP_DATA_P+cmd_pos+2];
										if(cmdval=='0'){
                                                PORTB &= ~(1<<PORTB6) ;//LED OFF
                                                strcpy(str,"LED Is Off!");
                                                goto ANSWER;
                                        }else if(cmdval=='1'){
                                                PORTB |= _BV(PORTB6);//LED ON
                                                strcpy(str,"LED Is On!");
                                                goto ANSWER;
                                        }else if(cmdval=='2'){
												ADCSRA = (1<<ADEN) | (1<<ADSC) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);		
											    while (ADCSRA & (1<<ADSC));
												result=ADC;
											    itoa((result*400)/1024,str,10);
                                                goto ANSWER;		
                                        }
                                }
                                strcpy(str,"err=noSuchCmd");
                                goto ANSWER;
                        }
                        strcpy(str,"err=invalid_pw");
ANSWER:
                        make_udp_reply_from_request(buf,str,strlen(str),1200);
                }
        }
        return (0);
}
