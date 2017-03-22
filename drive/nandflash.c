#include "nandflash.h"
#define ECC_BUFF_SIZE 6
static U8 ECCBuf[ECC_BUFF_SIZE];

void NF_Init()
{
	rGPACON = (rGPACON & ~(0x3f << 17)) | (0x3f << 17);            //配置芯片引脚
    //TACLS=1、TWRPH0=2、TWRPH1=0，8位IO
	rNFCONF = (TACLS << 12) | (TWRPH0 << 8) | (TWRPH1 << 4) | (0 << 0);
    //非锁定，屏蔽nandflash中断，初始化ECC及锁定main区和spare区ECC，使能nandflash片选及控制器
	rNFCONT = (0 << 13) | (0 << 12) | (0 << 10) | (0 << 9) | (0 << 8) | (1 << 6)
			| (1 << 5) | (1 << 4) | (1 << 1) | (1 << 0);
}

U32 rNF_ReadPage(U32 page_number, U8 *buff)
{
	U32 i, mecc0, secc;
	NF_RSTECC();                   //复位ECC
	NF_MECC_UnLock();          //解锁main区ECC
	NF_nFCE_L();                           //打开nandflash片选
	NF_CLEAR_RB();                      //清RnB信号
	NF_CMD(CMD_READ1);           //页读命令周期1
	//写入5个地址周期
	NF_ADDR(0x00);                                            //列地址A0~A7
	NF_ADDR(0x00);                                            //列地址A8~A11
	NF_ADDR((page_number) & 0xff);                  //行地址A12~A19
	NF_ADDR((page_number >> 8) & 0xff);           //行地址A20~A27
	NF_ADDR((page_number >> 16) & 0xff);         //行地址A28
	NF_CMD(CMD_READ2);           //页读命令周期2
	NF_DETECT_RB();                    //等待RnB信号变高，即不忙
	//读取一页数据内容
	for (i = 0; i < NF_PAGE_SIZE; i++)
	{
		buff[i] = NF_RDDATA8();
	}
	NF_MECC_Lock();                     //锁定main区ECC值
	NF_SECC_UnLock();                  //解锁spare区ECC
	mecc0 = NF_RDDATA();        //读spare区的前4个地址内容，即第2048~2051地址，这4个字节为main区的ECC
	//把读取到的main区的ECC校验码放入NFMECCD0/1的相应位置内
	rNFMECCD0 = ((mecc0 & 0xff00) << 8) | (mecc0 & 0xff);
	rNFMECCD1 = ((mecc0 & 0xff000000) >> 8) | ((mecc0 & 0xff0000) >> 16);
	NF_SECC_Lock();               //锁定spare区的ECC值
	secc = NF_RDDATA();     //继续读spare区的4个地址内容，即第2052~2055地址，其中前2个字节为spare区的ECC值
	//把读取到的spare区的ECC校验码放入NFSECCD的相应位置内
	rNFSECCD = ((secc & 0xff00) << 8) | (secc & 0xff);
	NF_nFCE_H();             //关闭nandflash片选
	//判断所读取到的数据是否正确
	if ((rNFESTAT0 & 0xf) == 0x0)
		return NF_READ_SUCCEED;                  //正确
	else
		return NF_READ_FAILED;                  //错误
}

static U32 rNF_IsBadBlock(U32 block)
{
	if (rNF_RamdomRead(block * NF_PAGE_COUNT_IN_BLOCK, 2054) == BAD_BLOCK_FLAG)
	{
		return NF_IS_BAD_BLOCK;
	}
	else
	{
		return NF_IS_NOT_BAD_BLOCK;
	}
}

static U32 rNF_MarkBadBlock(U32 block)
{
	U8 result;
	result = rNF_RamdomWrite(block * NF_PAGE_COUNT_IN_BLOCK, 2054,
			BAD_BLOCK_FLAG);
	if (result == NF_WRITE_SUCCEED)
		return NF_MARK_BAD_BLOCK_SUCCEED;       //写坏块标注成功
	else
		return NF_MARK_BAD_BLOCK_FAILED;        //写坏块标注失败
}

void rNF_Reset()
{
	NF_CE_L()
	;                               //打开nandflash片选
	NF_CLEAR_RB()
	;                      //清除RnB信号
	NF_CMD(CMD_RESET);           //写入复位命令
	NF_DETECT_RB()
	;                    //等待RnB信号变高，即不忙
	NF_CE_H()
	;                               //关闭nandflash片选
}

U8 rNF_ReadID()
{
	U8 pMID;
	U8 pDID;
	U8 cyc3, cyc4, cyc5;
	NF_nFCE_L()
	;                           //打开nandflash片选
	NF_CLEAR_RB()
	;                      //清RnB信号
	NF_CMD(CMD_READID);         //读ID命令
	NF_ADDR(0x0);                        //写0x00地址
	//读五个周期的ID
	pMID = NF_RDDATA8();                   //厂商ID：0xEC
	pDID = NF_RDDATA8();                   //设备ID：0xDA
	cyc3 = NF_RDDATA8();                     //0x10
	cyc4 = NF_RDDATA8();                     //0x95
	cyc5 = NF_RDDATA8();                     //0x44
	NF_nFCE_H()
	;                    //关闭nandflash片选
	return (pDID);
}

U32 rNF_WritePage(U32 page_number, U8 *buff)
{
	U32 i, mecc0, secc;
	U8 stat;
	if (rNF_IsBadBlock(page_number >> 6) == NF_IS_BAD_BLOCK)
		return NF_IS_BAD_BLOCK;           //是坏块，返回
	NF_RSTECC()
	;                   //复位ECC
	NF_MECC_UnLock()
	;          //解锁main区的ECC
	NF_nFCE_L()
	;             //打开nandflash片选
	NF_CLEAR_RB()
	;        //清RnB信号
	NF_CMD(CMD_WRITE1);                //页写命令周期1
	//写入5个地址周期
	NF_ADDR(0x00);                                     //列地址A0~A7
	NF_ADDR(0x00);                                     //列地址A8~A11
	NF_ADDR((page_number) & 0xff);           //行地址A12~A19
	NF_ADDR((page_number >> 8) & 0xff);    //行地址A20~A27
	NF_ADDR((page_number >> 16) & 0xff);  //行地址A28
	//写入一页数据
	for (i = 0; i < NF_PAGE_SIZE; i++)
	{
		NF_WRDATA8(buff[i]);
	}
	NF_MECC_Lock()
	;                     //锁定main区的ECC值
	mecc0 = rNFMECC0;                    //读取main区的ECC校验码
	//把ECC校验码由字型转换为字节型，并保存到全局变量数组ECCBuf中
	ECCBuf[0] = (U8) (mecc0 & 0xff);
	ECCBuf[1] = (U8) ((mecc0 >> 8) & 0xff);
	ECCBuf[2] = (U8) ((mecc0 >> 16) & 0xff);
	ECCBuf[3] = (U8) ((mecc0 >> 24) & 0xff);
	NF_SECC_UnLock()
	;                  //解锁spare区的ECC
	//把main区的ECC值写入到spare区的前4个字节地址内，即第2048~2051地址
	for (i = 0; i < 4; i++)
	{
		NF_WRDATA8(ECCBuf[i]);
	}
	NF_SECC_Lock()
	;                      //锁定spare区的ECC值
	secc = rNFSECC;                   //读取spare区的ECC校验码
	//把ECC校验码保存到全局变量数组ECCBuf中
	ECCBuf[4] = (U8) (secc & 0xff);
	ECCBuf[5] = (U8) ((secc >> 8) & 0xff);
	//把spare区的ECC值继续写入到spare区的第2052~2053地址内
	for (i = 4; i < 6; i++)
	{
		NF_WRDATA8(ECCBuf[i]);
	}
	NF_CMD(CMD_WRITE2);                //页写命令周期2
	for (i = 0; i < 1000; i++)
		;        //延时一段时间，以等待写操作完成
	NF_CMD(CMD_STATUS);                 //读状态命令
//判断状态值的第6位是否为1，即是否在忙，该语句的作用与NF_DETECT_RB();相同
	do
	{
		stat = NF_RDDATA8();
	} while (!(stat & 0x40));
	NF_nFCE_H()
	;                    //关闭nandflash片选
//判断状态值的第0位是否为0，为0则写操作正确，否则错误
	if (stat & 0x1)
	{
		if (rNF_MarkBadBlock(page_number >> 6) == NF_MARK_BAD_BLOCK_FAILED)
			return NF_MARK_BAD_BLOCK_FAILED;            //标注坏块失败
		else
			return NF_WRITE_FAILED;            //写操作失败
	}
	else
		return NF_WRITE_SUCCEED;                  //写操作成功
}

U32 rNF_EraseBlock(U32 block_number)
{
	U32 i = 0;
	U8 stat;
	if (rNF_IsBadBlock(block_number) == NF_IS_BAD_BLOCK)
		return NF_IS_BAD_BLOCK;           //是坏块，返回
	NF_nFCE_L()
	;             //打开片选
	NF_CLEAR_RB()
	;        //清RnB信号
	NF_CMD(CMD_ERASE1);         //擦除命令周期1
	//写入3个地址周期，从A18开始写起
	NF_ADDR((block_number << 6) & 0xff);         //行地址A18~A19
	NF_ADDR((block_number >> 2) & 0xff);         //行地址A20~A27
	NF_ADDR((block_number >> 10) & 0xff);        //行地址A28
	NF_CMD(CMD_ERASE2);         //擦除命令周期2
	for (i = 0; i < 1000; i++)
		;         //延时一段时间
	NF_CMD(CMD_STATUS);          //读状态命令
//判断状态值的第6位是否为1，即是否在忙，该语句的作用与NF_DETECT_RB();相同
	do
	{
		stat = NF_RDDATA8();
	} while (!(stat & 0x40));
	NF_nFCE_H()
	;             //关闭nandflash片选
//判断状态值的第0位是否为0，为0则擦除操作正确，否则错误
	if (stat & 0x1)
	{
		if (rNF_MarkBadBlock(block_number) == NF_MARK_BAD_BLOCK_FAILED)
			return NF_MARK_BAD_BLOCK_FAILED;            //标注坏块失败
		else
			return NF_ERASE_FAILED;            //擦除操作失败
	}
	else
		return NF_ERASE_SUCCEED;                  //擦除操作成功
}

U8 rNF_RamdomRead(U32 page_number, U32 add)
{
	NF_nFCE_L()
	;                    //打开nandflash片选
	NF_CLEAR_RB()
	;               //清RnB信号
	NF_CMD(CMD_READ1);           //页读命令周期1
	//写入5个地址周期
	NF_ADDR(0x00);                                            //列地址A0~A7
	NF_ADDR(0x00);                                            //列地址A8~A11
	NF_ADDR((page_number) & 0xff);                  //行地址A12~A19
	NF_ADDR((page_number >> 8) & 0xff);           //行地址A20~A27
	NF_ADDR((page_number >> 16) & 0xff);         //行地址A28
	NF_CMD(CMD_READ2);          //页读命令周期2
	NF_DETECT_RB()
	;                    //等待RnB信号变高，即不忙
	NF_CMD(CMD_RANDOMREAD1);                 //随意读命令周期1
	//页内地址
	NF_ADDR((U8 )(add & 0xff));                          //列地址A0~A7
	NF_ADDR((U8 )((add >> 8) & 0x0f));                 //列地址A8~A11
	NF_CMD(CMD_RANDOMREAD2);                //随意读命令周期2
	return NF_RDDATA8();               //读取数据
}

U32 rNF_RamdomWrite(U32 page_number, U32 add, U8 dat)
{
	U32 i;
	U8 stat;
	NF_nFCE_L()
	;                    //打开nandflash片选
	NF_CLEAR_RB()
	;               //清RnB信号
	NF_CMD(CMD_WRITE1);                //页写命令周期1
	//写入5个地址周期
	NF_ADDR(0x00);                                     //列地址A0~A7
	NF_ADDR(0x00);                                     //列地址A8~A11
	NF_ADDR((page_number) & 0xff);           //行地址A12~A19
	NF_ADDR((page_number >> 8) & 0xff);    //行地址A20~A27
	NF_ADDR((page_number >> 16) & 0xff);  //行地址A28
	NF_CMD(CMD_RANDOMWRITE);                 //随意写命令
	//页内地址
	NF_ADDR((U8 )(add & 0xff));                   //列地址A0~A7
	NF_ADDR((U8 )((add >> 8) & 0x0f));          //列地址A8~A11
	NF_WRDATA8(dat);                          //写入数据
	NF_CMD(CMD_WRITE2);                //页写命令周期2
	for (i = 0; i < 1000; i++)
		;         //延时一段时间
	NF_CMD(CMD_STATUS);                        //读状态命令
	//判断状态值的第6位是否为1，即是否在忙，该语句的作用与NF_DETECT_RB();相同

	do
	{
		stat = NF_RDDATA8();
	} while (!(stat & 0x40));
	NF_nFCE_H()
	;                    //关闭nandflash片选
//判断状态值的第0位是否为0，为0则写操作正确，否则错误
	if (stat & 0x1)
		return NF_WRITE_FAILED;                  //失败
	else
		return NF_WRITE_SUCCEED;                  //成功
}
