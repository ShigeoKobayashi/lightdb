//
//****************************************************************************
//*                                                                          *
//*  Copyright (C) Shigeo Kobayashi, 2016.                                   *
//*                 All rights reserved.                                     *
//*                                                                          *
//****************************************************************************
//
// bt_cacheio: File I/O is done by block unit in this source file.
//             cacheio handles chache (blocks read in) memories.
//
#include "stdafx.h"
#include "cacheio.h"

//
// Binary search class for cashed blocks.
// Each node in binary-tree is a linked CachedBlock.
//
CacheIo::CacheIo()
{
	m_pNodes      =  NULL;
    m_nNode       =  0;
	m_iFree       =  0;
	m_pRoot       = NULL;
	m_pNewest     = NULL;
	m_pOldest     = NULL;
	for(int i=0;i<256;++i) m_pCache8[i] = NULL;
#ifdef _DEBUG
	cHash64 = 0;
	c2tree = 0;
	cTotal = 0;
#endif // _DEBUG
}

CacheIo::~CacheIo()
{
#ifdef _DEBUG
	printf(" @@@@ Cache hit count: Total I/O = %u, Hash64 = %u, 2-tree = %u\n",cTotal,cHash64,c2tree);
#endif // _DEBUG
    CIoFinalize();
}

//
// Returns number of blocks cached in memory.
// Cache count is selected so that the cache size is around 1Mb.
//  sBlock: the size of a block in bytes.
U_INT32  
CacheIo::CIoGetCacheCount(U_INT32 sBlock)
{
	if(sBlock<= 512) return 2048;
	if(sBlock<=1024) return 1024;
	if(sBlock<=2048) return 512;
	return 256;
//	if(sBlock<=3072) return 256;
//	if(sBlock<=4096) return 128;
//	return 64;
}


//
// Allocates and initializes cache.
void 
CacheIo::CIoInitialize(U_INT32   nc,U_INT32   sb)
{
	CHECK(sb>0,ERROR_FILE_BAD_BLOCKSIZE);
	if(nc<=0) nc = CIoGetCacheCount(sb);
    m_nNode   = nc;
	m_iFree   = 0;
	m_pNodes  = (CachedBlock**)BseMemAlloc(sizeof(CachedBlock*)*m_nNode);
	for(int i=0;i<(int)m_nNode;++i)
	{
		m_pNodes[i] = new CachedBlock(this);
		CHECK(m_pNodes[i]!=NULL,ERROR_MEMORY_ALLOC);
		m_pNodes[i]->Index = i;
	}
	m_pRoot       = NULL; // Root node.
	m_pNewest     = NULL; // Most new node.
	m_pOldest     = NULL; // Most old node
}

int 
CacheIo::CIoFlush()
{
	int  f=FALSE;
	if(FIoIsReadOnly()) return f;
	for(int i=0;i<(int)m_nNode;++i) {
		if(m_pNodes[i]->Modified) {
			Unload(m_pNodes[i]);
			f = TRUE;
		}
	}
	return f;
}

void 
CacheIo::CIoFinalize()
{
	if(m_pNodes!=NULL) {
		CIoFlush();
		for(int i=0;i<(int)m_nNode;++i) delete m_pNodes[i];
		BseMemFree((void**)&m_pNodes);
	}
	m_pNodes    = NULL; // Must be NULL before this line.
	m_pRoot     = NULL; // Root node.
	m_pNewest   = NULL; // Most new node.
	m_pOldest   = NULL; // Most old node
}

// read contents of the cache from the database file. The cache contents must not be modified before being read.
void CacheIo::Load(CachedBlock *pCache)
{
	ASSERT((pCache->iBlock & FLAG64_DELETED)==0);
	ASSERT(pCache->Modified == FALSE);
	FIoSeek(pCache->iBlock*BlockHeader.BlockByteSize);
	FIoRead(pCache->Buffer,(size_t)BlockHeader.BlockByteSize);
	pCache->Modified = FALSE;
}

// writes contents of cache back to the database file if the cache is modified.
void CacheIo::Unload(CachedBlock *pCache)
{
	if(!pCache->Modified) return;
	ASSERT((pCache->iBlock & FLAG64_DELETED)==0);
	FIoSeek(pCache->iBlock*BlockHeader.BlockByteSize);
	FIoWrite(pCache->Buffer,(size_t)BlockHeader.BlockByteSize);
	pCache->Modified = FALSE;
}

//
// This method returns cached page associating the key specified by iBlock.
// If all pages cached are locked,then the oldest cache is flushed to the disk 
// if it is dirty(Modified==TRUE), and the oldest cached is returned after 
// being associated with iBlock.
//
CachedBlock *
	CacheIo::CIoGetCache(U_INT64  iBlock)
{
	CachedBlock *pCache;
	int           ixHash;
	
	ASSERT((iBlock&FLAG64_DELETED)==0);

	iBlock &= (~FLAG64_DELETED);
	ixHash  = Hash64(iBlock);

	ASSERT(ixHash>=0 && ixHash<256);

#ifdef _DEBUG
	cTotal++;
#endif // _DEBUG

	if(m_pRoot==NULL) {
		//
		// The binary tree is empty
		m_pRoot         = m_pNodes[0];
		pCache          = m_pRoot;
		m_pNewest       = m_pRoot;
		m_pOldest       = m_pRoot;
		m_iFree         = 1; // Next free cache slot.
		pCache->iBlock  = iBlock;
	} else {
		// 
		// First fined it in 8 bit hash array, if not there then binary tree search performed.  
		//
		if(m_pCache8[ixHash]!=NULL && iBlock==m_pCache8[ixHash]->iBlock) {
#ifdef _DEBUG
			cHash64++;
#endif // _DEBUG
			return m_pCache8[ixHash];
		}
		// Now look for the key in binary-tree.
		pCache = Find(iBlock);
		if(pCache!=NULL) {
			// The key(block) is found in the tree;
			m_pCache8[ixHash] = pCache;
#ifdef _DEBUG
			c2tree++;
#endif // _DEBUG
			return pCache;
		}
		//
		// iBlock is not in the cache list => get free slot(LRU management). 
		pCache = GetFreeNode();
		// If newly obtained cache has dirty content. => save it to the file before used.
		Unload(pCache);
		pCache->iBlock = iBlock;
		AddToLeaf(pCache);
	}
	// New cache slot is obtained => Contents of the cache must be read  
	// (or written to the db file if file is extended).
	if(iBlock>=BlockHeader.TotalBocks) {
		ASSERT(iBlock==BlockHeader.TotalBocks);
		// The block specified is not in the file => Can't read contents. => write and initialize it.
		pCache->Clear(0,(size_t)BlockHeader.BlockByteSize); // This sets ModifiedFlag = TRUE;
		Unload(pCache);
	} else {
		// Contents of iBlock are within the file => read them
		Load(pCache);
	}
	m_pCache8[ixHash] = pCache;
	return pCache;
}

//
// Find vKey in the binary linked tree and returns it's cache
// if vKey is not in the tree,then returns NULL
// 
CachedBlock *
CacheIo::Find(U_INT64  iBlock)
{
	U_INT64        Key;
	CachedBlock *pCur = m_pRoot;         // Search from binary tree root.
	U_INT64        vKey = Hash(iBlock);

Retry:
	if(pCur==NULL) return NULL;
	Key = Hash(pCur->iBlock);
	if(vKey>Key) {
		pCur = pCur->RightSon;
		goto Retry;;
	} else
	if(vKey<Key) {
		pCur = pCur->LeftSon;
		goto Retry;
	}
	return pCur;
}


//
// Add pNode to the tree tracing from the root node to leaf node.
// pNode including Key must not be in the linked list.
void CacheIo::AddToLeaf(CachedBlock *pNode)
{
	CachedBlock *pCur  = m_pRoot;
	U_INT64  Key1 = Hash(pNode->iBlock);
	U_INT64  Key2;

	pNode->RightSon   = NULL;
	pNode->LeftSon    = NULL;
	pNode->Parent     = NULL;
	pNode->ElderLRU   = NULL;
	pNode->NewerLRU   = NULL;

	// start from Root node
Retry:
	Key2 = Hash(pCur->iBlock);
	ASSERT(pCur!=pNode);
	ASSERT(Key1!=Key2);
	if(Key1<Key2)  {
		if(pCur->LeftSon!=NULL) {
			pCur = pCur->LeftSon;
			goto Retry;
		}
		// reached to the leaf,then add pNode at the left side of pCur
		pCur->LeftSon = pNode;
		pNode->Parent = pCur;
	} else {
		if(pCur->RightSon!=NULL) {
			pCur = pCur->RightSon;
			goto Retry;
		}
		// reached to the leaf,then add pNode at the right side of pCur
		pCur->RightSon = pNode;
		pNode->Parent = pCur;
	}

	// Update LRU
	if(m_pNewest!=NULL) {
		m_pNewest->NewerLRU  = pNode;
	}
	pNode->ElderLRU  = m_pNewest;
	pNode->NewerLRU  = NULL;
	m_pNewest = pNode;
}

//
// returns free node to add linked list.
// find unused node,detachh oldest node and returns it otherwise.
//
CachedBlock *
	CacheIo::GetFreeNode()
{
	CachedBlock *pCache = NULL;

	// find unused node first
	if(m_iFree>=m_nNode) {
		// no unused node,then detach oldest linked node
		pCache = DetachOldNode(); 
		if(pCache!=NULL) return pCache;
		// can't delete because all node locked => create new node and add it
		ASSERT(m_iFree==m_nNode);
		TRACE("Cache size extended\n");
		{
			int nc = m_nNode + m_nNode/2+1; // increase 50%
			m_pNodes = (CachedBlock**)BseMemReAlloc(m_pNodes,sizeof(CachedBlock*)*nc);
			for(int i=(int)m_iFree;i<nc;++i) {
				m_pNodes[i] = new CachedBlock(this);
				CHECK(m_pNodes[i]!=NULL, ERROR_MEMORY_ALLOC);
				m_pNodes[i]->Index = i;
			}
			m_nNode = nc;
		}
	}
	return m_pNodes[m_iFree++];
}

//
// detach oldest node from LRU & binary-tree list and return it
CachedBlock *
	CacheIo::DetachOldNode()
{
	CachedBlock *pDelete = m_pOldest;
	CachedBlock *pStart  = m_pOldest;
	ASSERT(pDelete!=NULL);

Retry:
	if(pDelete->IsLocked()) {
		CachedBlock *pNodes = NULL;
		// delete target node is locked,then add it to LRU list as newest and look for next old node for deletion.
		m_pOldest = m_pOldest->NewerLRU; // set next old node as the oldest.
		m_pOldest->ElderLRU = NULL;      // because no older node for the oldest
		pDelete->ElderLRU = m_pNewest; 
		pDelete->NewerLRU = NULL;
		m_pNewest->NewerLRU = pDelete;   // set the delete target to the newest
		m_pNewest = pDelete;            
		pDelete   = m_pOldest;           // set oldest node to delete target
		if(pDelete!=pStart)	goto Retry;  // retry if the new delete target is locked or not
		TRACE("Every nodes locked\n");
		return NULL;
	}
	// return pDelete after deleting it from link list
	ASSERT(m_pOldest==pDelete);
	DeleteNonLeafKeyFromList(pDelete);
	return pDelete;
}

//
// reconstruct the binary tree and LRU list after deleting pDelete
void CacheIo::DeleteNonLeafKeyFromList(CachedBlock *pDelete)
{
	CachedBlock *p,*pNow,*pPrev,*pSon;

	pPrev = pDelete;

	// delete it from binary tree structure
	if((pNow=pDelete->LeftSon)!=NULL) {
		// Find the biggest but less than or equal to iNode.
		// go to left subtree
		while((p=pNow->RightSon)!=NULL) {  // ***
			// and down to left until leach to leaf
			pPrev = pNow;
			pNow  = p;
        }
        // place pNow on the place of pDelete. 
		if(pPrev!=pDelete) {
			pSon                   = pNow->LeftSon;
			if(pSon!=NULL) pSon->Parent = pPrev;
			pPrev->RightSon = pNow->LeftSon;
			pNow->LeftSon  = pDelete->LeftSon;
		}
		pNow->RightSon = pDelete->RightSon;
		pNow->Parent   = pDelete->Parent;
	} else if((pNow=pDelete->RightSon)!=NULL) {
		// find bigger than iNode,but minimum node.
		while((p=pNow->LeftSon)!=NULL) { // ***
			pPrev = pNow;
			pNow  = p;
		}
        // delete iDelete
		if(pPrev!=pDelete) {
			pSon                    = pNow->RightSon;
			if(pSon!=NULL) pSon->Parent = pPrev;
			pPrev->LeftSon  = pNow->RightSon;
			pNow->RightSon  = pDelete->RightSon;
		}
		pNow->LeftSon  = pDelete->LeftSon;
		pNow->Parent   = pDelete->Parent;
    } else {
		// iDelete has no sons ==> delete itself.
		pPrev = pDelete->Parent;       // ***
		if(pPrev!=NULL) {
			if(pPrev->LeftSon==pDelete) pPrev->LeftSon = NULL;
			else {
				ASSERT(pPrev->RightSon==pDelete);
				pPrev->RightSon= NULL;
			}
		}
		pNow = NULL;
	}

	if(pDelete==m_pRoot) m_pRoot = pNow;
	if(pNow!=NULL) { // reset parent-child relation.
		p =  pNow->Parent;     // ***
		if(p!=NULL) {
			if(p->RightSon==pDelete) p->RightSon = pNow;
			else {
				ASSERT(p->LeftSon==pDelete);
				p->LeftSon = pNow;
			}
		}
		p = pDelete->RightSon;
		if((p!=pNow)&&(p!=NULL)) p->Parent = pNow;
		p = pDelete->LeftSon;
		if((p!=pNow)&&(p!=NULL)) p->Parent = pNow;
	}

	//
	// Update LRU list
	if(m_pOldest==pDelete) {
		m_pOldest = pDelete->NewerLRU;  // ****
		if(m_pOldest!=NULL) {
			ASSERT(m_pOldest->ElderLRU==pDelete);
			m_pOldest->ElderLRU = NULL;
		}
	} else if(m_pNewest==pDelete) {
		m_pNewest = pDelete->ElderLRU;
		if(m_pNewest!=NULL) {
			ASSERT(m_pNewest->NewerLRU==pDelete);
			m_pNewest->NewerLRU = NULL;
		}
	} else {
		p = pDelete->ElderLRU;
		if(p!=NULL) {
			ASSERT(p->NewerLRU==pDelete);
			p->NewerLRU = pDelete->NewerLRU;
		}
		p = pDelete->NewerLRU;
		if(p!=NULL) {
			ASSERT(p->ElderLRU==pDelete);
			p->ElderLRU = pDelete->ElderLRU;
		}
	}
	pDelete->Parent   = NULL;
	pDelete->LeftSon  = NULL;
	pDelete->RightSon = NULL;
	pDelete->ElderLRU = NULL;
	pDelete->NewerLRU = NULL;
}

void
CacheIo::PrintNode(CachedBlock *pNode)
{
	if(pNode==NULL) {
		printf("   NULL\n");
		return;
	}
	printf("   K =%lld ",pNode->iBlock);
	if(pNode->Parent==NULL) printf("   P =NULL ");
	else printf("   P =%lld ",pNode->Parent->iBlock);
	if(pNode->LeftSon==NULL) printf("   L =NULL ");
	else printf("   L =%lld ",pNode->LeftSon->iBlock);
	if(pNode->RightSon==NULL) printf("   R =NULL ");
	else printf("   R =%lld ",pNode->RightSon->iBlock);
	if(pNode->NewerLRU==NULL) printf("   N =NULL ");
	else printf("   N =%lld ",pNode->NewerLRU->iBlock);
	if(pNode->ElderLRU==NULL) printf("   E =NULL \n");
	else printf("   E =%lld",pNode->ElderLRU->iBlock);
	printf("   H =%lld\n",Hash(pNode->iBlock));
}

void
CacheIo::CIoPrintTree()
{
	S_INT32   i;
	printf("\n*** TREE INFO ***\n","");
	if(m_pRoot==NULL)   printf(" Root    = NULL\n") ; 
	else printf(" Root    = %lld\n",m_pRoot->iBlock) ; 
	if(m_pOldest==NULL) printf(" Oldest  = NULL\n") ; 
	else printf(" Oldest  = %lld\n",m_pOldest->iBlock);
	if(m_pNewest==NULL) printf(" Newest  = NULL\n") ; 
	else printf(" Newest  = %lld\n",m_pNewest->iBlock);
	printf(" Total     = %d\n",m_nNode);
	printf(" Free index= %d\n",m_iFree);

	double d = 0.0;
	for(i=0;i<(int)m_nNode;++i) {
		printf(" (%d)",i);
		PrintNode(m_pNodes[i]);
		if(m_pNodes[i]==NULL) continue;
		if(m_pNodes[i]->LeftSon!=NULL) d += 0.5;
		if(m_pNodes[i]->RightSon!=NULL) d += 0.5;
	}
	printf(" Child %%=%g\n",d/m_iFree);
}

void
CacheIo::CheckNode(CachedBlock *i,S_INT32 *pc)
{
	CachedBlock *il,*ir,*ip;
	U_INT64       iBlock;

	if(i==NULL) return;
	
	++(*pc);
	T_(i->fFlag==0,"ERROR:circular reference of linked cache list.");
	i->fFlag = 1;
	iBlock = i->iBlock;
	il = i->LeftSon;
	ir = i->RightSon;
	ip = i->Parent;
	if(ip!=NULL) { 
		// Parent exists.
		T_(i->fFlag==1,"ERROR:ERROR:circular reference of linked cache list(child)");
		T_((ip->LeftSon==i && ip->RightSon!=i) ||
			    (ip->LeftSon!=i && ip->RightSon==i),"ERROR:badly linked cache");
	} else {
		T_(i==m_pRoot,"ERROR:bad parent-child relation");
	}

	if(il!=NULL) {
		// Left son exists.
		T_(il->fFlag==0,"ERROR:circular reference of linked cache list(left child)");
		T_(Hash(il->iBlock)<Hash(iBlock),"ERROR:bad parent-child value relation(left child)");
		T_(il->Parent == i,"ERROR:parent-child relation(left son)");
		CheckNode(il,pc);
	}
	if(ir!=NULL) {
		// Right son exists.
		T_(ir->fFlag==0,"ERROR:circular reference of linked cache list(right child)");
		T_(Hash(ir->iBlock)>Hash(iBlock),"ERROR:bad parent-child value relation(right child)");
		T_(ir->Parent == i,"ERROR:parent-child relation(right son)");
		CheckNode(ir,pc);
	}
}

//
//     Check the validity of the cache tree
//
void
CacheIo::CIoCheckCache(int f)
{
	CachedBlock  *i;
	S_INT32      n;
	S_INT32      c;
	
//	if(f) printf("\n Check on cache.\n");
	i = m_pRoot;
	if(i==NULL) {
		// Tree not yet constructed.
		if(f) printf(" The 2-tree cache is empty.\n");
		T_(m_iFree==0,"ERROR:Chache counter!=0,for empty cache.");
		for(int j=0;j<(int)m_nNode;++j) {
			T_(m_pNodes[j]->ElderLRU==NULL,"ERROR:Link to old node[%d]!=NULL,for empty cache.",j);
			T_(m_pNodes[j]->NewerLRU==NULL,"ERROR:Link to new node[%d]!=NULL,for empty cache.",j);
			T_(m_pNodes[j]->RightSon==NULL,"ERROR:Link to right node[%d]!=NULL,for empty cache.",j);
			T_(m_pNodes[j]->LeftSon==NULL, "ERROR:Link to left node[%d]!=NULL,for empty cache.",j);
			T_(m_pNodes[j]->Parent==NULL,  "ERROR:Link to parent node[%d],for empty cache.",j);
			T_(!m_pNodes[j]->IsLocked(),   "ERROR:The cache node[%d] is locked,for empty cache.",j);
		}
		T_(m_pNewest==NULL,"ERROR:Link to the newest node != NULL,for empty cache.");
		T_(m_pOldest==NULL,"ERROR:Link to the oldest node != NULL,for empty cache.");
		return;
	}
	
	// Check LRU list.
	for(int j=0;j<(int)m_nNode;++j) m_pNodes[j]->fFlag = 0;

	n = m_iFree; // m_iFree is the index for next free node,m_iFree==m_nFree if no empty cache.
	i = m_pNewest;
	c = 0;

	//
	// LRU check
    while(i!=NULL) {
		T_(i->fFlag==0,"ERROR:circular reference of linked cache list(LRU)");
		i->fFlag = 1;
		++c;
		T_(c<=n,"ERROR:circular reference of linked cache list(counter)");
		if(i->NewerLRU!=NULL) {
			T_(i->NewerLRU->ElderLRU==i,"ERROR:bad link list(old and new)");
		} else {
			T_(i==m_pNewest,"ERROR:The cache is initial state(newest node)");
		}
		if(i->ElderLRU!=NULL) {
			T_(i->ElderLRU->NewerLRU==i,"ERROR:bad link list(new and old)");
		} else {
			T_(i==m_pOldest,"ERROR:The cache is initial state(oldest node)");
		}
		i = i->ElderLRU;
	}

	//
	// tree structure check
	c = 0;
	// Check for binary tree structure.
	for(int j=0;j<(int)m_nNode;++j) m_pNodes[j]->fFlag = 0;
	CheckNode(m_pRoot,&c);
	T_(c==n,"ERROR:total cache count");
	for(int j=0;j<(int)m_nNode;++j) m_pNodes[j]->fFlag = 0;
}
