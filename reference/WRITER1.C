/*
  Easy-Downloader V1.3 for ATMEL 89C2051/4051 (sdcc version)
  Copyright(c) 2004 Wichit Sirichote, kswichit@kmitl.ac.th, January 02, 2004

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
   
 */

#include <at89x51.h> 
#include <stdio.h>

#define xon 0x11
#define xoff 0x13

#define LM317 P3_5
#define LE P3_7
#define prog P3_2
#define rdy P3_3
#define xtal P3_4
#define p10 P1_0
#define p11 P1_1
#define p12 P1_2
#define p13 P1_3
#define p14 P1_4

int i;
unsigned char ACCU,temp;
char command;
char code title[] = "\n\r Easy-Downloader V1.3 for ATMEL 89C2051/4051 (sdcc version)";
char code prompt[] = "\n\r >";
char code ok[] = "\n\r ok";
int count;

pulseProg()
{
	prog = 0;
	prog = 0;
	prog = 0;
	prog = 1;
}

time1ms()    /* 1 ms delay with XTAL 11.0592MHz */
{
    int i;
    for (i = 0; i<50; i++) // the value shown in this line, 50 was calibrated for 1ms 
    ;						// you may change it!
}

delay(int n)      /* do nothing n*1ms */
{
    int i;
    for (i=0; i< n ; i++)
    time1ms();
}

pulseLE()
{
    delay(1);
	LE =1;
    delay(1);
    LE=0;
	delay(1);
}


initpowerup()
{
    prog = 0;
	xtal = 0;
	LM317 = 1;
	p14 = 1;
	rdy = 1;
    pulseLE();
    delay(100);
	prog = 1;
	p14 = 0;
	LM317 = 1;
    pulseLE();
    delay(100);
}

char getchar(void)
{
	char c;
	while(!RI);
	RI =0;
	c = SBUF;
	putchar(c);    // echo to terminal
	return SBUF;
}

char getchr(void)
{
	while(!RI);
	RI =0;
	return SBUF;	// no echo for command write
}

// external function for sending character through serial port
void putchar(char c)
{
	while(!TI);  
	TI=0;
	SBUF = c;
}

// send ASCII string to screen, the string must be stored in code memory
putstr(char *s) 
{
	char i=0;
	char c;
	while((c=*(s+(i++)))!= 0) putchar(c); // while byte is not terminator, keep sending
}

sendprompt()
{
    putstr(prompt);
}

prompting()
{
    if (command == 0x0d) // send title and prompt when get ENTER key
       {
    	putstr(title);
	    sendprompt();
       }
}

// need flow control with host pc
unsigned int getnum()
{
    char s[6]; 
    char c;
    char i;
    unsigned int temp16;  
	c = 0;
	i=0;
	 for (i = 0; c != 0xa; i++) // loop until CR has entered
    {    
        putchar(xon); // send xon to signal host to send byte
        c = getchar(); // get character from serial port
		if(c == 0xd) c=0xa; // convert CR to LF to make it compatible with ez31 and ez41
        s[i] = c; // save character to array
    }
    s[i-1] = 0; // put terminator at the end of string

// convert ascii to integer	(atoi(s))
	 temp16 = 0;
 for(i=0; s[i] != 0; i++) temp16 = 10*temp16 + s[i]-'0';
    return temp16; // return 16-bit for number of byte counting
}


getcommand()
{
    if (RI) command = getchar(); // 
    else command = -1;             /* no cammand has entered */
}

putok()
{
    putstr(ok);
    sendprompt();
}

putHEX(unsigned char ACCU)
{
     if (ACCU > 9) putchar(ACCU+55); // convert to ASCII for A-F
      else putchar(ACCU+48); // and for ASCII 0-9
}

printA()
{
      ACCU = P1; // read byte from target
      ACCU >>= 4;  // shift right 4 bits
      putHEX(ACCU); // send 4-bit high nibble
      ACCU = P1&0xf; // get low nibble
      putHEX(ACCU); // send low nibble
}

erase()
{
    if (command == 'e')
    {
    p12 =1;
	p11=p10=0;
	pulseLE();
    p14 = 0;
	LM317 = 0;
	pulseLE();
    delay(100);    // raise program supply up to 12V
    for (i=0; i < 10; i++) // erase entire PEROM array (2kB)
    {
    prog = 0;
    delay(10);           /* 10 ms prog pulse */
    prog=1;
	delay(1);
    }
    initpowerup();    /* reset internal address counter to 000h */
  	putok();
    }
}

write()
{
    if (command == 'w')
    {
	p12=0;
	p11=1;
	p10=1;

    pulseLE();
    p14 =0;
	LM317 = 0;
	pulseLE();
    delay(100);       /* rise supply up 12V */
  
	for (i = 0; i < count; i++)
    {                 /* use XON & XOFF flow control */
    putchar(xon);       /* send XON */
    P1 = getchr();
    putchar(xoff);       /* send XOFF */
    pulseProg();
    
  	while(!rdy) // wait until ready pin = '1'
		;
	xtal = 1;	// next address
	delay(1);
	xtal = 0;
	}

	LM317 = 1;
	p14 = 1;

	pulseLE();
    putok();
    }
}

read()    /* read code with the number of bytes set by 's' command */
{
    if (command=='r')
    {
	initpowerup();
	prog =0;
	p12 =0;
	p11 = 0;
	p10 =1;
	pulseLE();
  for(i = 0; i < count; i++)
	{
	prog =1;
	P1 = 0xff;
	delay(1);
    printA();        /* read in HEX */
	xtal = 1;
    delay(1);
	xtal = 0;
   }
	putok();
    }
}


lock()  /* only protection mode 3,i.e., disabled further
program and verify, can be programmed */

{
    if (command == 'l')
    {
	P1 = 0x07;
	pulseLE();
	LM317 = 0;
	delay(100);
	pulseProg();

	P1 = 0x06;
	pulseLE();
	pulseProg();
    
	p14 = 1;
	LM317 = 1;
	pulseLE();
	putok();
    }

}

setcounter()
{
    if (command == 's')
    {
        count = getnum();
	    putok();
    }
}

void main(void)
{
  i=0;
  count =0;
  SCON = 0x52; // 8-bit UART mode
  TMOD = 0x20; // timer 1 mode 2 auto reload
  TH1= 0xfd; // 9600 8n1
  TR1 = 1; // run timer1
  getchar();
  putstr(title);
  LE = 0;
  initpowerup();
  sendprompt();

  while(1)
  {
    getcommand();
    prompting();
    erase();
    write();
    read();
    lock();
    setcounter();
    }
}

