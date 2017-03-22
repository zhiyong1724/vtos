#ifndef _NANDFLASH_H
#define _NANDFLASH_H
#include "def.h"
enum NF_STATUS
{
	NF_READ_FAILED,
	NF_READ_SUCCEED,
	NF_IS_BAD_BLOCK,
	NF_IS_NOT_BAD_BLOCK,
	NF_MARK_BAD_BLOCK_FAILED,
	NF_MARK_BAD_BLOCK_SUCCEED,
	NF_WRITE_FAILED,
	NF_WRITE_SUCCEED,
	NF_ERASE_FAILED,
	NF_ERASE_SUCCEED,
};
#define BAD_BLOCK_FLAG 0xaa
#define NF_PAGE_SIZE 2048
#define NF_PAGE_COUNT_IN_BLOCK 64
#define TACLS 1
#define TWRPH0 2
#define TWRPH1 0
#define CMD_READ1                 0x00              //页读命令周期1
#define CMD_READ2                 0x30              //页读命令周期2
#define CMD_READID               0x90              //读ID命令
#define CMD_WRITE1               0x80              //页写命令周期1
#define CMD_WRITE2               0x10              //页写命令周期2
#define CMD_ERASE1               0x60              //块擦除命令周期1
#define CMD_ERASE2               0xd0              //块擦除命令周期2
#define CMD_STATUS                0x70              //读状态命令
#define CMD_RESET                 0xff               //复位
#define CMD_RANDOMREAD1         0x05       //随意读命令周期1
#define CMD_RANDOMREAD2         0xE0       //随意读命令周期2
#define CMD_RANDOMWRITE         0x85       //随意写命令
//宏函数定义
#define NF_CMD(data)               {rNFCMD  = (data); }        //传输命令
#define NF_ADDR(addr)             {rNFADDR = (addr); }         //传输地址
#define NF_RDDATA()               (rNFDATA)                    //读32位数据
#define NF_RDDATA8()              (rNFDATA8)                   //读8位数据
#define NF_WRDATA(data)         {rNFDATA = (data); }           //写32位数据
#define NF_WRDATA8(data)        {rNFDATA8 = (data); }          //写8位数据
#define NF_nFCE_L()                        {rNFCONT &= ~(1<<1); }
#define NF_CE_L()                            NF_nFCE_L()                 //打开nandflash片选
#define NF_nFCE_H()                       {rNFCONT |= (1<<1); }
#define NF_CE_H()                           NF_nFCE_H()                  //关闭nandflash片选
#define NF_RSTECC()                       {rNFCONT |= (1<<4); }          //复位ECC
#define NF_MECC_UnLock()             {rNFCONT &= ~(1<<5); }              //解锁main区ECC
#define NF_MECC_Lock()                 {rNFCONT |= (1<<5); }             //锁定main区ECC
#define NF_SECC_UnLock()                     {rNFCONT &= ~(1<<6); }      //解锁spare区ECC
#define NF_SECC_Lock()                  {rNFCONT |= (1<<6); }            //锁定spare区ECC
#define NF_WAITRB()                {while(!(rNFSTAT&(1<<0)));}           //等待nandflash不忙
#define NF_CLEAR_RB()           {rNFSTAT |= (1<<2); }                    //清除RnB信号
#define NF_DETECT_RB()         {while(!(rNFSTAT&(1<<2)));}           //等待RnB信号变高，即不忙

void NF_Init();
void rNF_Reset();
U8 rNF_ReadID();
U32 rNF_ReadPage(U32 page_number, U8 *buff);
U32 rNF_WritePage(U32 page_number, U8 *buff);
U32 rNF_EraseBlock(U32 block_number);
U8 rNF_RamdomRead(U32 page_number, U32 add);
U32 rNF_RamdomWrite(U32 page_number, U32 add, U8 dat);
#endif
