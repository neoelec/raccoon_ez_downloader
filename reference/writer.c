/*
   Easy-Downloader V1.1 for ATMEL 89C2051/4051
   Copyright(c) 1998 By W.Sirichote

   Code for writer board was written by W.Sirichote
   PC code for downloading HEX file was written By Zong
*/

#include c:\mc51\8051io.h
#include c:\mc51\8051reg.h

#define xon 0x11
#define xoff 0x13

extern register char cputick;
register int i;
unsigned register char ACC,temp;
register char command;
char *title[] = "\n Easy-Downloader V1.1 for ATMEL 89C2051/4051"
char *prompt[] = "\n >"
char *ok[] = "\n ok"
register int count;

main()
{
/* define ASM EQU for assembly interfacing */
  asm"LM317 EQU $b5";
  asm"LE    EQU $b7";
  asm"prog  EQU $b2";
  asm"rdy   EQU $b3";
  asm"xtal  EQU $b4";
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
  asm " clr LE";
  initpowerup();
  sendprompt();

  while(1)
  {
    while ( cputick < 1)
    ;
    cputick = 0;
/* run the following tasks every 10 ms */
    getcommand();
    prompting();
    erase();
    write();
    read();
    lock();
    setcounter();
    }
}


getnum()

{
    char s[6]; /*  put to global variables instead five characters plus terminator */
    char c;
    int i;
    c = 0;
    for (i = 0; c != 10; i++)
    {
        putch(xon);
        c = getch();
        s[i] = c;
    }
  s[i] = '\0';
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

char getconsole()
{

    if ((SCON & 0x01) != 0)
  return(getchr());           /* use getchr() instead,ie. no echo */
    else return(-1);
}

putok()
{
    putstr(*ok);
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
	sendprompt();
       }
}

pulseLE()
{
    delay(1);
    asm" setb LE";
    delay(1);
    asm" clr LE";
    delay(1);
}

erase()
{
    if (command == 'e')
    {
    asm {
     setb p12     /* set erase mode */
     clr  p11
     clr  p10
       }
    pulseLE();
    asm {
	clr p14
	clr LM317
	}
    pulseLE();
    delay(100);    /* raise program supply up to 12V */
    for (i=0; i < 10; i++) /* erase entire PEROM array (2kB) */
    {
    asm " clr prog";
    delay(10);           /* 10 ms prog pulse */
    asm " setb prog";
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
    asm {
     clr  p12          /* set program mode */
     setb p11
     setb p10
       }
    pulseLE();
    asm {
      clr p14
      clr LM317
        }
    pulseLE();
    delay(100);       /* rise supply up 12V */
    asm" clr IE.7";
    for (i = 0; i < count; i++)
    {                 /* use XON & XOFF flow control */
    putch(xon);       /* send XON */
    P1 = getchr();
    putch(xoff);       /* send XOFF */
    pulseProg();
    /*
    asm " clr prog";
    asm " nop";
    asm " nop";
    asm " nop";
    asm " nop";           /* pulse prog ~4 microsecond */
    asm " setb prog"; */
    asm " nop";
    asm " jnb rdy,*";
    asm " nop";

    asm " setb xtal";
    delay (1);
    asm " clr xtal";
    }
    asm" setb IE.7";
    asm " setb LM317";
    asm " setb p14";
    pulseLE();
    putok();
    }
}

read()    /* read code with the number of bytes set by 's' command */
{
    if (command=='r')
    {
	initpowerup();
/*  delay(100);   */
	asm{
	    clr  prog
	    clr  p12
	    clr  p11
	    setb p10
	   }
	pulseLE();
  for(i = 0; i < count; i++)
	{
	asm" setb prog";
	asm" mov P1,#$FF";   /* put FF before read back */
   delay(1);
   printA();        /* read in HEX */
	asm" setb xtal";  /* next address */
  delay(1);
	asm" clr xtal";
/*  chkXOFF();  */      /* flow controlled by XON/XOFF */
  }
	putok();

    }
}

printA()
{
      ACC = P1;
      ACC >>= 4;  /* shift right 4 bits */
      putHEX();
      ACC = P1&15;
      putHEX();

}

putHEX()
{
     if (ACC > 9)
	putch(ACC+55);
      else putch(ACC+48);
}

/*
chkXOFF()     /* use XON and XOFF for controlling flow with host */

{
      if(getconsole() == xoff)
         {
            do;
            while(getconsole() != xon);
         }
}
*/


lock()  /* only protection mode 3,i.e., disabled further
program and verify, can be programmed */

{
    if (command == 'l')
    {
	P1 = 0x07;
	pulseLE();
	asm " clr LM317";
	delay(100);
	pulseProg();

	P1 = 0x06;
	pulseLE();
	pulseProg();
       
	asm " setb p14";
	asm " setb LM317";
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

initpowerup()
{
    asm{
  clr  prog
	clr  xtal
	setb LM317
	setb p14
	setb rdy
	}
    pulseLE();
    delay(100);
    asm{
  setb prog
	clr p14
	setb LM317
    }
    pulseLE();
    delay(100);

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


