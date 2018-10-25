//
//****************************************************************************
//*                                                                          *
//* Å@Copyright (C) Shigeo Kobayashi, 2014.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
//
#ifndef ___BT_CACHE_IO_H___
#define ___BT_CACHE_IO_H___

#include "fileio.h"

// Search pointer.
typedef struct _BTREE_SEARCH_PTR {
	U_INT64  iCurrentPage;// Current search page
	U_INT32  iCurrentItem;// Current search item in the current search page.
} BTREE_SEARCH_PTR;


#define Nil      ((U_INT64 )0)
#define IsNil(a) (((U_INT64 )a)==Nil)

#define SZ_ALLOC_COPY(st,so) {int len=strlen(so); st=(char*)MemAlloc(sizeof(char)*(len+2));if(st) strcpy_s(st,len+1,so);}


void T_(int f,char *psz,...);

class CachedBlock;


class CacheIo: public CFileIo
{
public:
	CacheIo();
   ~CacheIo();
	BLOCK_HEADER BlockHeader;   // Keeps total number of blocks,block size etc which are set in upper class. 

    void          CIoInitialize(U_INT32   nc,U_INT32   sb);
	void          CIoFinalize();
	int           CIoFlush();
	int           CIoFlush(CachedBlock *pCache);
	CachedBlock  *CIoGetCache(U_INT64  iBlock);
	U_INT32       CIoCacheCount() {return m_nNode;}
	U_INT32       CIoGetCacheCount(U_INT32   sBlock);

	// Debugging tool
	void          CIoPrintTree();
	void          CIoCheckCache(int f);
	void          Load(CachedBlock *pCache);
	void          Unload(CachedBlock *pCache);

private:
	void	      CheckNode(CachedBlock *i,S_INT32   *pc);
	void          PrintNode(CachedBlock *pNode);

	CachedBlock *Find(U_INT64  vKey);

	U_CHAR   Hash64(U_INT64  u64)
	{
		U_INT32  u32 =  ((U_INT32*)(&u64))[0]+((U_INT32*)(&u64))[1];
		U_INT16  u16 =  ((U_INT16*)(&u32))[0]+((U_INT16*)(&u32))[1];
		return (U_CHAR)(((U_CHAR *)(&u16))[0]+((U_CHAR *)(&u16))[1]);
	};

	U_CHAR   Hash32(U_INT32  u32)
	{
		U_INT16  u16 =  ((U_INT16*)(&u32))[0]+((U_INT16*)(&u32))[1];
		return (U_CHAR)(((U_CHAR *)(&u16))[0]+((U_CHAR *)(&u16))[1]);
	};

	U_CHAR   Hash16(U_INT32  u16)
	{
		return (U_CHAR)(((U_CHAR *)(&u16))[0]+((U_CHAR *)(&u16))[1]);
	};


	U_INT64  Hash(U_INT64  ul)
	{
		PU_CHAR  p = (PU_CHAR)(&ul);
		for(int i=0;i<sizeof(ul);++i) {
			if(p[i]&0x01) p[i] = (p[i]>>1)|0x80;
			else          p[i] = (p[i]>>1);
		}
		// U_CHAR   uch;uch  = p[0];p[0] = p[3];p[3] = uch;	uch  = p[1];p[1] = p[2];p[2] = uch;
		return ul;
	}

	U_INT64  UnHash(U_INT64  ul)
	{
		int i;
		PU_CHAR  p = (PU_CHAR)(&ul);
		// U_CHAR   uch; uch  = p[0];	p[0] = p[3];p[3] = uch;	uch  = p[1];p[1] = p[2];p[2] = uch;
		for(i=0;i<sizeof(ul);++i) {
			if(p[i]&0x80) p[i] = (p[i]<<1)|0x01;
			else  		  p[i] = (p[i]<<1);
		}
		return ul;
	}

	CachedBlock  *GetFreeNode();
	CachedBlock  *DetachOldNode();
	void	     DeleteNonLeafKeyFromList(CachedBlock *pDelete);
	void	     AddToLeaf(CachedBlock *pNode);

private:
	U_INT32   	 m_nNode;	    // Total node count.
	U_INT32  	 m_iFree;	    // Index for unused cache(0<=m_iFree<m_nNode). 
	//                             m_pNodes[m_iFree] is the top of the unused caches.
	CachedBlock   *m_pRoot;	// Root's node #
	CachedBlock   *m_pOldest;	// Eldest node #(LRU link,deleted from link)
	CachedBlock   *m_pNewest;	// Youngest node #(LRU link)
	CachedBlock  **m_pNodes;	// Node array.
	// 256 hash (8 bit count)
	CachedBlock   *m_pCache8[256];
#ifdef _DEBUG
	 // Cache hit counter
	U_INT32   cHash64;
	U_INT32   c2tree;
	U_INT32   cTotal;
#endif // _DEBUG
};

//
// CachedBlock is a node(an element) in 2-tree cache list. Also it has LRU list.
// A node consists of one block buffer,2-tree list,LRU list and a referrence counter.
//
class CachedBlock : public CBase
{        // Node/Element of binary tree search definition.
public:
	CachedBlock(CacheIo *pCacheIo)
	{
		m_pCacheIo = pCacheIo;
		m_sBlock   = (U_INT32  )m_pCacheIo->BlockHeader.BlockByteSize;
		Buffer     = BseMemAlloc((size_t)m_sBlock);
		iBlock     = 0;
		Index      = -1;       // Position in the parent node.
		Parent     = NULL;     // Parent node #
		LeftSon    = NULL;     // Left son's node #
		RightSon   = NULL;     // Right son's node #
		NewerLRU   = NULL;     // Younger node #(LRU link)
		ElderLRU   = NULL;     // Elder node #(LRU link)
		Modified   = FALSE;    // Dirty flag
		m_cRef     = 0;        // Reference count.
		m_pos      = 0;
		fFlag      = 0;
	};

	~CachedBlock()
	{
		ASSERT(m_cRef==0);
		ASSERT(Modified==FALSE);
		if(Buffer!=NULL) BseMemFree((void **)&Buffer);
		Buffer = NULL;
	};

	void     Lock()             {++m_cRef;};
	void     Unlock()           {--m_cRef;ASSERT(m_cRef>=0);};
	int      IsLocked()         {return m_cRef!=0;};

	void* SeekP(U_INT32   pos)   {
		ASSERT(pos<m_sBlock);
		m_pos = pos;
		return ((U_CHAR*)Buffer)+m_pos;
	};

	void Seek(U_INT32   pos)   {
		ASSERT(pos<m_sBlock);
		m_pos = pos;
	};

	void  Clear(int v,int cb){memset(Buffer,v,cb);Modified = TRUE;};

	void  Put(void *buf,U_INT32   cb) 
	{
		if(cb>0) {
			ASSERT(m_pos + cb <=m_sBlock);
			memcpy(((U_CHAR*)Buffer)+m_pos,buf,cb);
			Modified = TRUE;
			m_pos += cb;
		}
#ifdef _DEBUG
		else ASSERT(0);
#endif
	};

	void  Get(void *buf,U_INT32   cb) 
	{
		if(cb>0) {
			ASSERT(m_pos + cb <=m_sBlock);
			memcpy(buf,((U_CHAR*)Buffer)+m_pos,cb);
			m_pos += cb;
		}
#ifdef _DEBUG
		else ASSERT(0);
#endif
	};

public:
	CacheIo        *m_pCacheIo;
	int             Modified;
	void           *Buffer;      // Buffer area (==Page)
	U_INT64         iBlock;      // Block # in the data base file (0==header block).
	U_INT32         Index;       // Position(index) at the parent page
	// 2-tree list
	CachedBlock   *Parent;       // Parent node #
	CachedBlock   *LeftSon;      // Left son's node #
	CachedBlock   *RightSon;     // Right son's node #
	// LRU list
	CachedBlock   *NewerLRU;     // Younger node #(LRU link)
	CachedBlock   *ElderLRU;     // Elder node #(LRU link)
	U_INT32          m_sBlock;
//#ifdef _DEBUG
	int	  	         fFlag;
// #endif // _DEBUG

private:
	U_INT32      m_cRef;          // Refference counter.
	U_INT32      m_pos;           // Seek position.
};

#endif //___BT_CACHE_IO_H___
