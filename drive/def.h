#ifndef _DEF_H
#define _DEF_H
//编译器相关的类型定义
typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned int U32;
typedef signed int S32;

//uart寄存器相关定义
#define rUTXH0     (*(volatile U8 *)0x50000020)
#define rURXH0     (*(volatile U8 *)0x50000024)
#define rGPHCON    (*(volatile U32 *)0x56000070)
#define rULCON0    (*(volatile U32 *)0x50000000)
#define rUCON0     (*(volatile U32 *)0x50000004)
#define rUBRDIV0   (*(volatile U32 *)0x50000028)
#define rUTRSTAT0  (*(volatile U32 *)0x50000010)
//中断寄存器相关定义
#define rINTMSK    (*(volatile U32 *)0x4a000008)
#define rINTSUBMSK (*(volatile U32 *)0x4a00001c)
#define rSUBSRCPND (*(volatile U32 *)0x4a000018)
#define rSRCPND    (*(volatile U32 *)0x4a000000)
#define rINTPND    (*(volatile U32 *)0x4a000010)
//GPIO
#define rGPHUP     (*(volatile U32 *)0x56000078)
#define GPBCON     (*(volatile U32*)0x56000010)
#define GPBDAT     (*(volatile U32*)0x56000014)
//nandflash
#define rGPACON		(*(volatile U32 *)0x56000000)
#define rNFCONF		(*(volatile U32 *)0x4E000000)		//NAND Flash configuration
#define rNFCONT		(*(volatile U32 *)0x4E000004)      //NAND Flash control
#define rNFCMD		(*(volatile U32 *)0x4E000008)      //NAND Flash command
#define rNFADDR		(*(volatile U32 *)0x4E00000C)      //NAND Flash address
#define rNFDATA		(*(volatile U32 *)0x4E000010)      //NAND Flash data
#define rNFDATA8	(*(volatile U8 *)0x4E000010)     //NAND Flash data
#define NFDATA		(0x4E000010)      //NAND Flash data address
#define rNFMECCD0	(*(volatile U32 *)0x4E000014)      //NAND Flash ECC for Main Area
#define rNFMECCD1	(*(volatile U32 *)0x4E000018)
#define rNFSECCD	(*(volatile U32 *)0x4E00001C)		//NAND Flash ECC for Spare Area
#define rNFSTAT		(*(volatile U32 *)0x4E000020)		//NAND Flash operation status
#define rNFECC		(*(volatile U32 *)0x4e000014)      //NAND Flash ECC
#define rNFECC0		(*(volatile U8 *)0x4e00002c)
#define rNFECC1		(*(volatile U8 *)0x4e00002d)
#define rNFECC2		(*(volatile U8 *)0x4e00002e)
#define rNFESTAT0	(*(volatile U32 *)0x4E000024)
#define rNFESTAT1	(*(volatile U32 *)0x4E000028)
#define rNFMECC0	(*(volatile U32 *)0x4E00002C)
#define rNFMECC1	(*(volatile U32 *)0x4E000030)
#define rNFSECC		(*(volatile U32 *)0x4E000034)
#define rNFSBLK		(*(volatile U32 *)0x4E000038)		//NAND Flash Start block address
#define rNFEBLK		(*(volatile U32 *)0x4E00003C)		//NAND Flash End block address
//时钟
#define rTCFG0    (*(volatile U32 *)0x51000000)
#define rTCFG1    (*(volatile U32 *)0x51000004)
#define rTCON     (*(volatile U32 *)0x51000008)
#define rTCNTB4   (*(volatile U32 *)0x5100003c)
#define rTCNTO4   (*(volatile U32 *)0x51000040)
#endif
