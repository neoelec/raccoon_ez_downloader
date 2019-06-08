/*
   Easy-Downloader V2.0 for ATMEL 89C51/52/55 (sdcc version)
   Copyright(c) 2004 Wichit Sirichote, kswichit@kmitl.ac.th, January 5, 2004

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

int i;
unsigned char ACCU,temp,blank,chip,VPP;
char command;
unsigned int address,chksum;

char code title[] = "\n\r Easy-Downloader V2.0 for ATMEL 89C51/52/55 (sdcc version)";
char code prompt[] = "\n\r >";
char code ok[] = "\n\r ok";
unsigned int count,bytes,nonblank;

#define LM317 P3_5
#define LE P3_7
#define prog P2_7
#define Vpp P3_3
#define rdy P3_2
#define xtal P3_1
#define p26 P3_4
#define p27 P3_5
#define p36 P3_6
#define p37 P3_7
#define p10 P1_0
#define p11 P1_1
#define p12 P1_2
#define p13 P1_3
#define p14 P1_4


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

delay10us()
{
    int i;
    for (i=0;i<1;i++);

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

send_hex(unsigned char n)
{
	if(n<=9) putchar(n+'0'); // send 0-9
	else putchar(n+55); // send A-F
}

puthex(unsigned char n)
{
	unsigned char temp;
	temp = n&0xf0;
	temp >>=4;
	send_hex(temp); 
	temp = n&0xf;
	send_hex(temp);
}

puthex16(unsigned int n)
{
	puthex(n>>8);
	puthex(n);
}	

putnum(unsigned int n)
{
	unsigned int k;
	char s[6];
	s[0] = n/10000+'0';
    k = n%10000;
	s[1] = k/1000+'0';
	k = k%1000;
	s[2] = k/100+'0';
	k = k%100;
	s[3] = k/10+'0';
	k = k%10;
	s[4] = k%10+'0';
	
	for(k = 0; s[k] == '0' && k<5; k++);
	if (k==5) k--; // if all bytes are '0', get back one position
	do putchar(s[k++]); 
	while (k<5);
}

getconsole()
{
    if (RI) command = getchr();
    else command = -1;
}

chkXOFF()     /* use XON and XOFF for controlling flow with host */

{
      if(getconsole() == xoff)
         {
            do;
            while(getconsole() != xon);
         }
}

erase()
{
    if (command == 'e')
    {
    Vpp = 1;
	p26 = 1;
	p27 = 0;
	p36 = 0;
	p37 = 0;
    delay10us();
    Vpp = 0;
    delay(100);
    for (i=0; i < 1; i++) /* erase entire PEROM array (2kB) */
    {
    prog = 0;
    delay(10);           /* 10 ms prog pulse */
    prog = 1;
    delay(10);
    }
    Vpp = 1;
    delay(100);
    putok();
    }
}

write()  /* call sequence before write: 's', 't','e','w' */
{
    if (command == 'w')
    {
	Vpp = 1;
	p26 = 0;
	p27 = 1;
	p36 = 1;
	p37 = 1;
	prog = 1;

    delay10us();
    if (VPP == 0xff)  /* skip if VPP != 0xff,i.e., need 5Vpp for programming  */
    Vpp = 0;
    delay(100); /* raise programming voltage to 12V */
    chksum = 0;
    for (i = 0; i < count; i++)
    {                 /* use XON & XOFF flow control */
    putchar(xon);       /* send XON */
    address = i;
    P1 = address;
	P2 = (address>>8)|0x80;
	
    delay10us();
    P0 = getchr();  /* read raw bytes from console */
    putchar(xoff);    /* send XOFF */
    chksum += P0;   /* summing chksum */
    pulseProg();

	while(!rdy)
		;	// wait until ready

    }
    Vpp = 1;    /* bring Vpp back to 5V */
    delay(100);
    putok();
    }
}

read()    /* read ACCUording to number of bytes by 'c' command  */
{  
    unsigned int i;
    if (command=='r')
    {
   chksum = 0;               /* reset check sum word */
    Vpp = 1;
	prog = 1;
	p26 = 0;
	p27 = 1;
	p36 = 1;
	p37 = 1;

    blank = '1';
  for(i = 0; i < count; i++)
	{
    address = i;
    P1 = address;
	P2 = (address>>8)|0x80;
	p27 = 0;

	delay10us();
    P0 = 0xff;
    chksum += P0;
    puthex(P0);     /*    read in HEX */
    p27 = 1;
	chkXOFF();
    }
    putok();
    }
}

blanktest() /* if all bytes are 0FFH then blank =FF, else blank = 0 */
{
     if (P0 != 0xff)
       { blank = '0';     /* full */
        nonblank++;
       }
}
  
lock()  /* only protection mode 4 can be programmed,i.e., disabled further
program, verify, and external execution */

{
    if (command == 'l')
    {
     Vpp = 0;
	 prog = 1;
	 p26 = p27 = p36 = p37 = 1;

	delay(100);
	pulseProg();
    delay(5);
	p36 = p37 = 0;

  delay10us();
  pulseProg();
  delay(5);
  p27 = 0;
  p36 = 1;
  p37 = 0;

    delay10us();
    pulseProg();
    delay(5);
	putok();
    }

}

printchksum()
{
    if (command == 'c')
    {
    putstr("\n\r CHKSUM = "); puthex16(~chksum+1);
    putok();
    }
}

signature()  /* read signature ATMEL chip 51/52/55 12V or 5V Vpp */
{
	  Vpp = prog = 1;
	  p26 = 0;
	  p27 = 1;
	  p36 = p37 = 0;

    address = 0x31;
    P1 = address;
	P2 = (address>>8)|0x80;
	p27 = 0;

    delay10us();
       P0 = 0xff;
	   chip = P0;
       p27 = 1;
     address++;
    P1 = address;
	P2 = (address>>8)|0x80;
	p27 = 0;
    delay10us();
       P0 = 0xff;   /* put FF before read back */
       VPP = P0; /* save Vpp 12V (FF) or 5V (5) into VPP */

}

testblank()  /* need to call signature function pior calling testblank */
{
    unsigned int i;
      signature();
      switch(chip){
        case (0x51): /* 89C51 */
        bytes = 4096;   /* if chip == 0x51 or 0x61 then bytes = 4kB */
        break;
        case (0x61): /* 89LV51 */
        bytes = 4096;
        break;
        case (0x52): /* 89C52 */
        bytes = 8192;   /* if chip == 0x52 or 0x62 then bytes = 8kB */
        break;
        case (0x62): /* 89LV52 */
        bytes = 8192;
        break;
        case (0x55): /* 89C55 */
        bytes = 20480;  /* if chip == 0x55 or 0x65 then bytes = 20kB */
        break;
        case (0x65): /* 89LV55 */
        bytes = 20480;
        break;
        case 0xff:
        bytes = 0;
        }
  chksum = 0;               /* reset check sum word */
  nonblank = 0;             /* reset nonblank bytes counter */
  Vpp = prog = 1;
  p26 = 0;
  p27 = p36 = p37 =1;
    blank = '1';
  for(i = 0; i < bytes; i++)
	{
    address = i;
    P1 = address;
	P2 = (address>>8)|0x80;
	p27 = 0;

    delay10us();
       P0 = 0xff;   /* put FF before read back */
       chksum += P0;
       blanktest();
       p27 = 1;  /* next address */
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

getinfo()
{
    if (command =='g')
    {
        testblank();
        if(chip!=0xff)
        {
        putstr("\n\r found 89C");
		puthex(chip);
		if (VPP==0xff)
        putstr("-12V");
          if (VPP==5)
        putstr("-5V");
        putstr("\n\r nonblank ");
		putnum(nonblank);
		putstr(" bytes");
		putstr("\n\r bytes counter ");
		putnum(count);
		}
        else(putstr("\n\r Not found 89Cxx"));
        putok();
    }
}

PGMparameters() /* for simple host interfacing */
{
    if (command=='p')
    {
        testblank();
		puthex(chip);
		putchar(',');
		putnum(nonblank);
		putchar(',');
		putnum(count); // use getnum instead of printf_fast to reduce size
        //printf_fast("%x,%u,%u",chip,nonblank,count);
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
  sendprompt();
 
  while(1)
  {
    getcommand();
    prompting();
    setcounter();
    erase();
    write();
    read();
    lock();
    printchksum();
    getinfo();
    PGMparameters();
    }
}

