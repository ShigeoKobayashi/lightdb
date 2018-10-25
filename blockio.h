//
//****************************************************************************
//*                                                                          *
//* @Copyright (C) Shigeo Kobayashi, 2016.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
//
#ifndef ___BT_BLOCKIO_H___
#define ___BT_BLOCKIO_H___

#include "cacheio.h"

class CBlockIo : public CacheIo
{
public:
	CBlockIo();
	~CBlockIo();
  
	void          BIoOpen (const P_CHAR szFile,char chMode,U_INT32   sBlock,U_INT32   nCache);
	void          BIoClose();
	CachedBlock *BIoAddBlock();
	void          BIoDeleteBlock(U_INT64  iBlock);

protected:
	// Write BlockHeader backe to the file without changing cache(0) state(Modified flag).
	void          WriteBlockHeader()
				  {
					// Update header
					CachedBlock *pCache = CIoGetCache(0);
					// BlockHeader updated,set it back to the cache buffer.
					memcpy(pCache->Buffer,&BlockHeader,sizeof(BlockHeader));
					// Write BlockHeader back to the db file witout affecting Modified flag. 
					FIoSeek(0);
					FIoWrite(&BlockHeader,sizeof(BlockHeader));
				 };
};
#endif //___BT_BLOCKIO_H___
