/*
   Easy-Downloader V2.1 for ATMEL 89C51/52/55WD
   Copyright(c) 2002 By Wichit Sirichote kswichit@kmitl.ac.th

   The source code was modified from ez52.c for programming the 89C55WD.

   89C55WD uses A14 tied to P3.4 not P3.0 as 89C55.

   The source code was compiled with Dunfiled Micro-C for 8051.

   13 December 2013, modified for Microchip FlashFlex 8051 MCU

   21 December 2013, complete test for 89S52
                     add warning for programming lock bits!
   27 December 2013, modify code for SST89E516RD for using with EZDL4
                     ReadID must be executed for ARMING operation
                     Block 1 for bootloader must be selected.

                     The EZDL4 will see the chip as 89C52 and will erase
                     block 1, write the bootloader of NXP89V51RD2
                     User can then use the Flashmagic to program the
                     SST89E516 directly!
                 

   18 January 2014, test DC-to-DC converter Mc34063 that produces +12V
                    from +5V
                    new file name, flex2.c

                    change VPP control, setbit at VCON will produce +12V
                                        clearbit at VCON will produce +5V


                    after power reset, set VPP to +5V by clearing VCON


*/

#include <8051io.h>
#include <8051reg.h>
#include <8051bit.h>

#define xon 0x11
#define xoff 0x13

#define Vpp P3.3   // set Vpp = +5V clear Vpp = +12.5V
#define prog P3.2

// map materchip i/o port to the mode selection on ZIF

#define p26 P3.4  
#define p27 P3.5

#define p33 P2.7

#define p36 P3.6
#define p37 P3.7

register int i;
unsigned register char ACCU,temp,blank,chip,VPP,ID,bcc;
register char command;
register unsigned int address,chksum;

char title[] = "\n Easy-Downloader V5.0 for AT89C52/89S52 FlashFlexMCU"
char prompt[] = "\n >"
char ok[] = "\n ok"

unsigned register int count,bytes,nonblank,load_address;

register char buffer[50]; // for Intel hex file record buffer
register char eof;

main()
{
  delay(100);
  clrbit(Vpp)

  i = 0;
  count = 0;
  serinit(9600);
  getch();
  putstr(title);
  sendprompt();
  setbit(prog)
  delay(100);
  

  for(;;)
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
    read_ID();
    block_select();  // for SST chips
    manual_VPP();
    read_lockbit();
    printhelp();
    get_record(); // read hex record
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
       c = putchr(getch());
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
        command = putchr(getch());
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
    putstr(ok);
   /* putch(' ');  */
    sendprompt();
}

sendprompt()
{
    putstr(prompt);
}

prompting()

{
    if (command == '\n')
       {
        putstr(title);
 /* putok(); */
  sendprompt();
       }
}

gets()

{
    char c;

    for (i = 0; c != 10; i++)
    {
       c = getch();
       buffer[i] = c;
    }
    buffer[i] = '\0';    /* put end-of-string to the last byte */

}


char to_hex(char hi_nib,char low_nib)
{
   temp = hi_nib-'0';

   if(temp>9) temp-=7;
   hi_nib=temp<<=4;

   temp = low_nib-'0';
   if(temp>9) temp-=7;

   return (hi_nib|temp);

}

get_record()
{
  if(command=='h')
  {
   putstr("\nwating hex file");

   eof=0;

   while(eof==0)
   {
//   _getstr(buffer,sizeof(buffer));

   putch(xon);
   _getstr(buffer,sizeof(buffer));
   putch(xoff);

   printf("\n%i %d",buffer,sizeof(buffer));

// :100060007105F583710543B14075B5AAF5B4858373

   // check end of file

   eof = to_hex(buffer[7],buffer[8]);


   bcc =0;

   bytes= to_hex(buffer[1],buffer[2]); // get number of byte

   bcc+=bytes;

 //  printf("\n byte = %d ",bytes);

   address=to_hex(buffer[3],buffer[4]);

   bcc +=address;

   address<<=8;

   address|=to_hex(buffer[5],buffer[6]);

   bcc +=address&0x00ff;

//   printf("\n %d %04x",bytes,address);

   for(i=0; i<bytes; i++)
   {

    temp = to_hex(buffer[i*2+9],buffer[i*2+10]);

    bcc+=temp;

//    printf(" %02x",temp);

   }

   bcc=~bcc+1;

//   printf(" %02x",bcc);


   }


   sendprompt();




  }




}


read_lockbit() // for 89S52
{
  if(command=='q')
  {
   setbit(prog)
   clrbit(Vpp)
   setbit(p26)
   setbit(p27)
   clrbit(p33)
   setbit(p36)
   clrbit(p37)
   delay(10);
   P0 = 0xff;
   temp=P0;
   printf("\n%08b",temp);
   sendprompt();
  }
}

manual_VPP()
{

  if(command=='v')
  {

   putstr("\nVPP =+12V");
   setbit(Vpp)
   putstr("\npress any key to turn VPP off");
   getch();
   clrbit(Vpp)
   sendprompt();


  }


}



read_ID()
{
  if(command=='i')
  {

    readID();
   putok();
   sendprompt();



  }

}

block_select()
{
  char c;

  if(command=='b')
  {

    putstr("\nBlock 0 or 1>");
    c=putchr(getch());

    if (c==0x31) temp = 0xA5;
    else temp = 0x55;


    P2 = temp;

    setbit(p37)
    clrbit(p36)
    clrbit(p27)
    setbit(p26)

    pulseProg();
    delay(100);
    putok();
    sendprompt();


  }


}

erase()
{
    if (command == 'e')
    {

    if(chip ==0x52)
    {

    setbit(Vpp);

    delay(100);

    setbit(p26)
    clrbit(p27)
    setbit(p33)
    clrbit(p36)
    clrbit(p37)


    delay(10);       // raise Vpp from +5V to +12.5V

    pulseProg();

    delay(100);
    clrbit(Vpp);
    }
    // select memory block beforehand

    if(chip ==0x93)
    {
    setbit(Vpp);

    delay(100);

    setbit(p26)   // erase memory block
    clrbit(p27)
    setbit(p36)
    setbit(p37)


    delay(10);       // raise Vpp from +5V to +12.5V

    pulseProg();

    delay(100);
    clrbit(Vpp);
    }
    putok();
    }
}

write()  /* call sequence before write: 's', 't','e','w' */
{
    if (command == 'w')
    {

    if(chip == 0x52 || chip == 0x93)
    {

    clrbit(Vpp)

    clrbit(p26)
    setbit(p27)
    setbit(p33)
    setbit(p36)
    setbit(p37)

    setbit(prog)

    delay10us();

    setbit(Vpp)

    delay(100); /* raise programming voltage to 12V */
    chksum = 0;
    for (i = 0; i < count; i++)
    {                 /* use XON & XOFF flow control */
    putch(xon);       /* send XON */
    address = i;

    if(chip==0x52)
    {
    asm{
       mov P1,address  /* put address to P1 and P2 */
       mov A,address+1
       ORL A,#$80  // P3.3 must be high
       mov P2,A
       }
    }

    if(chip==0x93)
    {
    asm{
       mov P1,address  /* put address to P1 and P2 */
       mov A,address+1
       mov P2,A
       }
    }

    delay10us();
    P0 = getchr();  /* read raw bytes from console */
    putch(xoff);    /* send XOFF */
    chksum += P0;   /* summing chksum */
    pulseProg();

    delay100us();   // use delay instead of checking RDY/BSY

    }

    clrbit(Vpp)    /* bring Vpp back to 5V */
    delay(100);
    putok();
    }
    }
}

read()    /* read according to number of bytes by 'c' command  */
{  
    unsigned int i;
    if (command=='r')
    {
 /*  temp = getch(); */
  chksum = 0;               /* reset check sum word */

  setbit(prog)
  clrbit(p26)
  clrbit(p27)
  clrbit(p33)
  setbit(p36)
  setbit(p37)

     blank = '1';
  for(i = 0; i < count; i++)
	{
    address = i;
    asm{
       mov P1,address
       mov A,address+1
       mov P2,A
       }

   // delay10us();
       asm" mov P0,#$FF";   /* put FF before read back */
       chksum += P0;
       printA(P0);     /*    read in HEX */


    chkXOFF();
    }
    putok();
    }
}

printA(n)
char n;
{
      ACCU = n;
      ACCU >>= 4;  /* shift right 4 bits */
      putHEX();
      ACCU = n&15;
      putHEX();
   /*   delay(2);     */

}

chkXOFF()     /* use XON and XOFF for controlling flow with host */

{
      if(getconsole() == xoff)
         {
            do
            ;
            while(getconsole() != xon);
         }
}

putHEX()
{
     if (ACCU > 9)
        putch(ACCU+55);
      else putch(ACCU+48);
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
    // lock bit 1
     putstr("\nwarning ! further programming will be disabled...");
     putstr("\nclick yes to continue>");
     temp=getchr();

     if(temp=='y')
     {


     setbit(Vpp)
     delay(100); // wait +12V

     setbit(prog)
     setbit(p26)
     setbit(p27)
     setbit(p33)
     setbit(p36)
     setbit(p37)

	pulseProg();
        delay(100);
  delay(5);
    // lock bit 2
     clrbit(p36)
     clrbit(p37)
  pulseProg();
  delay(100);

    // lock bit 3

     clrbit(p27)
     setbit(p36)
     clrbit(p37)

    pulseProg();
    delay(100);
     putstr("\nlock complete");
     }
     else
     {
      putstr("\nabort");
     }

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

readID()
{
     clrbit(Vpp)
     setbit(prog)

     clrbit(p26)
     clrbit(p27)
     clrbit(p36)
     clrbit(p37)

    address = 0x31; // check SST89E516RD2
    asm{
       mov P1,address
       mov A,address+1
       mov P2,A
       }
    delay10us();
       P0 = 0xFF;  /* put FF before read back */
       chip=P0;

    if(chip != 0x93)
    {
    address = 0x100; // check 89S52 or 89C52
    asm{
       mov P1,address         
       mov A,address+1
       mov P2,A
       }
    delay10us();
       P0 = 0xFF;  /* put FF before read back */
       chip=P0;
    }

}

signature()  /* read signature ATMEL chip 51/52/55 12V or 5V Vpp */

{
       readID();
       printf("%x",chip);
   /*
    address = 0x60; // location stores ATMEL ID for 89C51 and 89C52
    asm{
       mov P1,address
       mov A,address+1
       mov P2,A
       }
    delay10us();
       P0 = 0xFF;  /* put FF before read back */
        temp   = P0;
       printf(" %08b ",temp); // print status

    address = 0x61; // location stores ATMEL ID for 89C51 and 89C52
    asm{
       mov P1,address
       mov A,address+1
       mov P2,A
       }
    delay10us();
       P0 = 0xFF;  /* put FF before read back */
        temp  = P0;
       printf(" %08b ",temp); // print status

// if chip = 0xFF, try with address 0x100 for 89C55WD also

    if(chip ==0xFF)
    {
    address = 0x100;     // stores ATMEL device ID 0x55 for 89C55WD
    asm{
       mov P1,address
       mov A,address+1
       mov P2,A
       }
    delay10us();
       P0 = 0xFF;  /* put FF before read back */
       chip = P0;
 //      setbit(p27)
//       printf("\n %04x = %02x ",address,chip);
     }

     address++;
    asm{
       mov P1,address
       mov A,address+1
       mov P2,A
       }
    delay10us();

      */

       clrbit(Vpp)

       P0 = 0xff;   /* put FF before read back */
       VPP = P0; /* save Vpp 12V (FF) or 5V (5) into VPP */
}


testblank()  /* need to call signature function pior calling testblank */
{
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

  clrbit(Vpp)
  setbit(prog)
  clrbit(p26)
  clrbit(p27)
  setbit(p36)
  setbit(p37)

   blank = '1';
  for(i = 0; i < bytes; i++)
	{
    address = i;
    asm{
       mov P1,address
       mov A,address+1
    //   orl A,#$80
       mov P2,A
    //   clr p27
       }

  //  delay10us();
       P0 = 0xff;   /* put FF before read back */
       chksum += P0;
       blanktest();
  //     setbit(p27)  /* next address */
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
     //   testblank();
        signature();
        if(chip!=0xff)
        {
        if(chip==0x93) putstr("\nfound SST89E51RD");
        if(chip==0x52) putstr("\nfound 89C52 or 89S52");
        printf("\n nonblank %u bytes",nonblank);
        printf("\n bytes counter %u",count);
        }
        else(putstr("\ntarget not found "));
        putok();
    }
}

PGMparameters() /* for simple host interfacing */
{
    if (command=='p')
    {
       readID();
       delay(100);

       nonblank=0;
     //   testblank();
        printf("%x,%u,%u",0x52,nonblank,count);   // test..
        putok();
    }

}



printhelp()
{
    if (command == '?')
    {
        putstr("\n e erase block");
        putstr("\n q read lock bit");
        putstr("\n s set number of byte");
        putstr("\n r read code");
        putstr("\n l lock the chip");
        putstr("\n g get chip info");
        putstr("\n w write byte");
        putstr("\n b set block 0/1");


        sendprompt();
    }
}

pulseProg()

{
        clrbit(prog)
        asm{
        NOP
        NOP
        NOP
        NOP
        }
        setbit(prog)
}

delay10us()
{
    int i;
    for (i=0;i<1;i++);

}

delay100us()
{
   int i;
   for (i=0; i<10;i++);
}
