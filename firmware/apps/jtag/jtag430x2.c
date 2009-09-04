/*! \file jtag430x2.c
  \author Travis Goodspeed <travis at radiantmachines.com>
  
  This is an implementation of the MSP430X2 JTAG protocol
  for the GoodFET project at http://goodfet.sf.net/
  
  See the license file for details of proper use.
*/

#include "platform.h"
#include "command.h"
#include "jtag.h"

unsigned char jtagid;

//! Get the JTAG ID
unsigned char jtag430x2_jtagid(){
  jtag430_resettap();
  return jtagid=jtag_ir_shift8(IR_BYPASS);
}
//! Start JTAG, take pins
unsigned char jtag430x2_start(){
  jtagsetup();
  
  //Known-good starting position.
  //Might be unnecessary.
  SETTST;
  SETRST;
  
  delay(0xFFFF);
  
  //Entry sequence from Page 67 of SLAU265A for 4-wire MSP430 JTAG
  CLRRST;
  delay(10);
  CLRTST;

  delay(5);
  SETTST;
  msdelay(5);
  SETRST;
  P5DIR&=~RST;
  
  delay(0xFFFF);
  
  //Perform a reset and disable watchdog.
  return jtag430x2_jtagid();
}

unsigned int jtag430_coreid(){
  jtag_ir_shift8(IR_COREIP_ID);
  return jtag_dr_shift16(0);
}

unsigned long jtag430_deviceid(){
  jtag_ir_shift8(IR_DEVICE_ID);
  return jtag_dr_shift(0);
}

//! Set the program counter.
void jtag430x2_setpc(unsigned long pc){
  unsigned short Mova;
  unsigned short Pc_l;

  Mova  = 0x0080;
  Mova += (unsigned short)((pc>>8) & 0x00000F00);
  Pc_l  = (unsigned short)((pc & 0xFFFF));

  // Check Full-Emulation-State at the beginning                                                                                                                                   
  jtag_ir_shift8(IR_CNTRL_SIG_CAPTURE);
  if(jtag_dr_shift16(0) & 0x0301){
      // MOVA #imm20, PC                                                                                                                                                             
      CLRTCLK;
      // take over bus control during clock LOW phase                                                                                                                                
      jtag_ir_shift8(IR_DATA_16BIT);
      SETTCLK;
      jtag_dr_shift16(Mova);
      jtag_ir_shift8(IR_CNTRL_SIG_16BIT);
      jtag_dr_shift16(0x1400);
      jtag_ir_shift8(IR_DATA_16BIT);
      CLRTCLK;
      SETTCLK;
      jtag_dr_shift16(Pc_l);
      CLRTCLK;
      SETTCLK;
      jtag_dr_shift16(0x4303);
      CLRTCLK;
      jtag_ir_shift8(IR_ADDR_CAPTURE);
      jtag_dr_shift(0x00000);
  }
}

//! Read data from address
unsigned int jtag430x2_readmem(unsigned int adr){
  unsigned int toret;
  
  //SETPC_430Xv2(StartAddr);
  SETTCLK;
  jtag_ir_shift8(IR_CNTRL_SIG_16BIT);
  jtag_dr_shift16(0x0501);
  jtag_ir_shift8(IR_ADDR_CAPTURE);

  jtag_ir_shift8(IR_DATA_QUICK);

  SETTCLK;
  CLRTCLK;
  toret = jtag_dr_shift16(0);//read

  jtag_ir_shift8(IR_CNTRL_SIG_CAPTURE);
  
  return toret;
}


//! Handles classic MSP430 JTAG commands.  Forwards others to JTAG.
void jtag430x2handle(unsigned char app,
		   unsigned char verb,
		   unsigned char len){
  
  switch(verb){
  case START:
    //Enter JTAG mode.
    do cmddata[0]=jtag430x2_start();
    while(cmddata[0]==00 || cmddata[0]==0xFF);
    
    //MSP430 or MSP430X
    if(jtagid==MSP430JTAGID){ 
      jtag430mode=MSP430MODE;
      drwidth=16;
    }else if(jtagid==MSP430X2JTAGID){
      jtag430mode=MSP430X2MODE;
    }else{
      txdata(app,NOK,1);
      return;
    }
    
    //TAP setup, fuse check
    //jtag430_resettap();
    txdata(app,verb,1);
    break;
  case JTAG430_READMEM:
  case PEEK:
    cmddataword[0]=jtag430x2_readmem(cmddataword[0]);
    txdata(app,verb,2);
    break;
  case JTAG430_COREIP_ID:
    cmddataword[0]=jtag430_coreid();
    txdata(app,verb,2);
    break;
  case JTAG430_DEVICE_ID:
    cmddatalong[0]=jtag430_deviceid();
    txdata(app,verb,4);
    break;
  case JTAG430_HALTCPU:
  case JTAG430_RELEASECPU:
  case JTAG430_SETINSTRFETCH:
  case JTAG430_WRITEMEM:
  case POKE:
  case JTAG430_WRITEFLASH:
  case JTAG430_ERASEFLASH:
  case JTAG430_SETPC:
  default:
    jtaghandle(app,verb,len);
  }
  jtag430_resettap();
}