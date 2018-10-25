//
//****************************************************************************
//*                                                                          *
//* Å@Copyright (C) Shigeo Kobayashi, 2016.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
// blockio: File I/O is done by block unit in this source file.
//          blockio is also handles chache (blocks read in) memories.
//
#include "stdafx.h"
#include "blockio.h"

CBlockIo::CBlockIo()
{
	memset(&BlockHeader,0,sizeof(BLOCK_HEADER));
}

CBlockIo::~CBlockIo()
{
	BIoClose();
}

void 
CBlockIo::BIoClose()
{
	if(!FIoIsReadOnly()) CIoFlush();
}

//
// Open specified file for block I/O
//   sBlock ... block size in bytes
//   nCache ... number of blocks cached,if nCache <=0 then nCache = GetCacheCount(sBlock) 
void CBlockIo::BIoOpen(const P_CHAR szPath,char chMode,U_INT32 sBlock,U_INT32 nCache)
{
	//
	// Open file according to thw specified open mode.
	FIoOpen(szPath,chMode);
	FIoSeek(0);
	// Allocate cache for BLOCK I/O
	if(FIoIsNewFile()) {
		//Å@initialize
		memcpy(&BlockHeader,LdbID,strlen(LdbID));
		BlockHeader.Version       = 2; // Version # (from C dll 2018/3/9).
		BlockHeader.TotalBocks    = 0;
		BlockHeader.TopDeleted    = 0;
		BlockHeader.BlockByteSize = sBlock;
		// Setup cache
		CIoInitialize(nCache,(U_INT32)BlockHeader.BlockByteSize);
		CachedBlock *pCache = CIoGetCache(0);
		BlockHeader.TotalBocks    = 1;
		WriteBlockHeader();
	} else { // OLD file => info in the file have priorities
		// read block header directly to BlockHeader.
		FIoRead(&BlockHeader,sizeof(BlockHeader));
		//
		// Header check
		CHECK(memcmp(&BlockHeader,LdbID,strlen(LdbID))==0,ERROR_BROKEN_FILE);
		CHECK(BlockHeader.TotalBocks>0,ERROR_BROKEN_FILE);
		CHECK(BlockHeader.BlockByteSize>0,ERROR_BROKEN_FILE);
		CHECK(BlockHeader.TopDeleted<BlockHeader.TotalBocks,ERROR_BROKEN_FILE);
		// Setup cache
		CIoInitialize(nCache,(U_INT32)BlockHeader.BlockByteSize);
	}
}

CachedBlock* CBlockIo::BIoAddBlock()
{
	U_INT64     iBlock;
	CachedBlock *pCache;

	if(BlockHeader.TopDeleted!=0) {
		// reuse deleted block in the deleted chain.
		iBlock = BlockHeader.TopDeleted;
		pCache = CIoGetCache(iBlock);
		// read next deleted block (Top U_INT64  is used for linked list for deleted blocks.
		pCache->Seek(0);
		pCache->Get(&BlockHeader.TopDeleted,sizeof(U_INT64));
		ASSERT((BlockHeader.TopDeleted & FLAG64_DELETED)==FLAG64_DELETED);
		BlockHeader.TopDeleted &= (~FLAG64_DELETED);
		ASSERT(BlockHeader.DeletedBlocks>0);
		BlockHeader.DeletedBlocks--;
	} else {
		// No deleted block => extend file size.
		iBlock = BlockHeader.TotalBocks;
		pCache = CIoGetCache(BlockHeader.TotalBocks);
		BlockHeader.TotalBocks++;
	}
	WriteBlockHeader();
	return CIoGetCache(iBlock);
}

void CBlockIo::BIoDeleteBlock(U_INT64 iBlock)
{
	CachedBlock *pCache;
	ASSERT(iBlock!=0);
	ASSERT(iBlock<BlockHeader.TotalBocks);
	CHECK(iBlock!=0 && iBlock<BlockHeader.TotalBocks,ERROR_SYSTEM);

	pCache = CIoGetCache(iBlock);
	pCache->Seek(0);
	{
		// Set the first bit of deleted block 1.
		U_INT64 iDeleted = BlockHeader.TopDeleted | FLAG64_DELETED;
		pCache->Put(&iDeleted,sizeof(U_INT64));
	}
	BlockHeader.TopDeleted = iBlock;
	BlockHeader.DeletedBlocks++;
	WriteBlockHeader();
}
