/*
   Easy-Downloader V2.0 for ATMEL 89C51/52/55
   Copyright(c) 1998 By W.Sirichote

   Code for writer board was written with 'C' by W.Sirichote
   PC code for downloading HEX file was written with 'PASCAL' By Zong
*/

#include c:\mc51\8051io.h
#include c:\mc51\8051reg.h

#define xon 0x11
#define xoff 0x13

extern register char cputick;
register int i;
unsigned register char ACC,temp,blank,chip,VPP;
register char command;
register unsigned int address,chksum;

char *title[] = "\n Easy-Downloader V2.0 for ATMEL 89C51/52/55"
char *prompt[] = "\n >"
char *ok[] = "\n ok"
unsigned register int count,bytes,nonblank;

main()
{
/* define ASM EQU for assembly interfacing */
  asm"LM317 EQU $b5";
  asm"LE    EQU $b7";
  asm"prog  EQU $a7";   /* active low */
  asm"Vpp   EQU $b3";   /* set Vpp = 5V clear Vpp = 12V */
  asm"rdy   EQU $b2";   /* set when ready clear busy */
  asm"xtal  EQU $b1";
  asm"p26   EQU $b4";
  asm"p27   EQU $b5";
  asm"p36   EQU $b6";
  asm"p37   EQU $b7";
  asm"p10  EQU $90";
  asm"p11  EQU $91";
  asm"p12  EQU $92";
  asm"p13  EQU $93";
  asm"p14  EQU $94";

  cputick = 0;
  i = 0;
  count = 0;
  serinit(9600);
  getch();
  putstr(*title);
 /* putok(); */
  sendprompt();
  while(1)
  {
    do
    {
       ;       /* put tasks that require 51's speed here */
    }
    while ( cputick < 1);
    cputick = 0;
/* run the following tasks every 10 ms */
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


getnum()

{
    char s[6];     /*  five characters plus terminator */
    char c;
    int i;
    c = 0;
    for (i = 0; c != 10; i++)
    {
       putch(xon);
       c = getch();
       s[i] = c;
    }
    s[i] = '\0';    /* put end-of-string to the last byte */
    if (i==0)
    return (-1);
    else
    return (_atoi(s));

}

getcommand()
{

    if ((SCON & 0x01) != 0)
	command = getch();
    else command = -1;             /* no cammand has entered */
}

getconsole()
{

    if ((SCON & 0x01) != 0)
  command = getchr();
    else command = -1;
}

putok()
{
    putstr(*ok);
   /* putch(' ');  */
    sendprompt();
}

sendprompt()
{
    putstr(*prompt);
}

prompting()

{
    if (command == '\n')
       {
	putstr(*title);
 /* putok(); */
  sendprompt();
       }
}

erase()
{
    if (command == 'e')
    {
    asm {
     setb Vpp
     setb p26
     clr  p27
     clr  p36
     clr  p37
       }
    delay10us();
    asm" clr Vpp";
    delay(100);
    for (i=0; i < 1; i++) /* erase entire PEROM array (2kB) */
    {
    asm " clr prog";
    delay(10);           /* 10 ms prog pulse */
    asm " setb prog";
    delay(10);
    }
    asm " setb Vpp";
    delay(100);
    putok();
    }
}

write()  /* call sequence before write: 's', 't','e','w' */
{
    if (command == 'w')
    {
    asm {
     setb Vpp         /* set write code mode */
     clr  p26
     setb p27
     setb p36
     setb p37
     setb prog
       }
    delay10us();
    if (VPP == 0xff)  /* skip if VPP != 0xff,i.e., need 5Vpp for programming  */
    asm" clr Vpp";
    delay(100); /* raise programming voltage to 12V */
    chksum = 0;
    for (i = 0; i < count; i++)
    {                 /* use XON & XOFF flow control */
    putch(xon);       /* send XON */
    address = i;
    asm{
       mov P1,address  /* put address to P1 and P2 */
       mov A,address+1
       orl A,#$80
       mov P2,A
       }
    delay10us();
    P0 = getchr();  /* read raw bytes from console */
    putch(xoff);    /* send XOFF */
    chksum += P0;   /* summing chksum */
    pulseProg();
    asm " nop";
    asm " jnb rdy,*";  /* until ready */
    asm " nop";
    }
    asm " setb Vpp";    /* bring Vpp back to 5V */
    delay(100);
    putok();
    }
}

read()    /* read according to number of bytes by 'c' command  */
{  
    unsigned int i;
    if (command=='r')
    {
 /*  temp = getch(); */
  chksum = 0;               /* reset check sum word */
  asm{                      /* set mode read code */
      setb Vpp
      setb prog
      clr  p26
      setb p27
      setb p36
      setb p37
	   }
     blank = '1';
  for(i = 0; i < count; i++)
	{
    address = i;
    asm{
       mov P1,address
       mov A,address+1
       orl A,#$80
       mov P2,A
       clr p27
       }

    delay10us();
       asm" mov P0,#$FF";   /* put FF before read back */
       chksum += P0;
       printA(P0);     /*    read in HEX */
    asm" setb p27";  /* next address */
    chkXOFF();
    }
    putok();
    }
}

printA(n)
char n;
{
      ACC = n;
      ACC >>= 4;  /* shift right 4 bits */
      putHEX();
      ACC = n&15;
      putHEX();
   /*   delay(2);     */

}

chkXOFF()     /* use XON and XOFF for controlling flow with host */

{
      if(getconsole() == xoff)
         {
            do;
            while(getconsole() != xon);
         }
}

putHEX()
{
     if (ACC > 9)
	putch(ACC+55);
      else putch(ACC+48);
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
     asm{
        clr  Vpp
        setb prog
        setb p26
        setb p27
        setb p36
        setb p37
    }
	delay(100);
	pulseProg();
  delay(5);
  asm {
        clr  p36
        clr  p37
    }
  delay10us();
  pulseProg();
  delay(5);
  asm {
       clr  p27
       setb p36
       clr  p37
    }
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
    printf("\n CHKSUM = %04x",~chksum+1);
    putok();
    }
}

signature()  /* read signature ATMEL chip 51/52/55 12V or 5V Vpp */

{
     asm{                      /* set mode read code */
      setb Vpp
      setb prog
      clr  p26
      setb p27
      clr  p36
      clr  p37
	   }
    address = 0x31;
    asm{
       mov P1,address
       mov A,address+1
       orl A,#$80
       mov P2,A
       clr p27
       }
    delay10us();
       asm" mov P0,#$FF";   /* put FF before read back */
       chip = P0;
       asm" setb p27";
     address++;
    asm{
       mov P1,address
       mov A,address+1
       orl A,#$80
       mov P2,A
       clr p27
       }
    delay10us();
       asm" mov P0,#$FF";   /* put FF before read back */
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
  asm{                      /* set mode read code */
      setb Vpp
      setb prog
      clr  p26
      setb p27
      setb p36
      setb p37
	   }
   blank = '1';
  for(i = 0; i < bytes; i++)
	{
    address = i;
    asm{
       mov P1,address
       mov A,address+1
       orl A,#$80
       mov P2,A
       clr p27
       }

    delay10us();
       asm" mov P0,#$FF";   /* put FF before read back */
       chksum += P0;
       blanktest();
    asm" setb p27";  /* next address */
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
        printf("\n found 89C%2x",chip);
          if (VPP==0xff)
        putstr("-12V");
          if (VPP==5)
        putstr("-5V");
        printf("\n nonblank %u bytes",nonblank);
        printf("\n bytes counter %u",count);
        }
        else(putstr("\n Not found 89Cxx"));
        putok();
    }
}

PGMparameters() /* for simple host interfacing */
{
    if (command=='p')
    {
        testblank();
        printf("%x,%u,%u",chip,nonblank,count);
        putok();
    }

}



/*
printhelp()
{
    if (command == '?')
    {
	putstr("\n e  erase");
	putstr("\n rb read BIN");
	putstr("\n rh read HEX");
	putstr("\n w  write");
	putstr("\n l  lock");
     }
	putok();

}
*/

pulseProg()

{
	asm " clr prog";
	asm " nop";
	asm " nop";
	asm " nop";
	asm " setb prog";

}

time1ms()    /* 1 ms delay with XTAL 11.0592MHz */
{
    int i;
    for (i = 0; i < 8 ; i++)
    ;
}

delay(n)      /* do nothing n*1ms */
int n;
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
