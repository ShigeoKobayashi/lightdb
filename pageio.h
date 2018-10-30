//
//****************************************************************************
//*                                                                          *
//* @Copyright (C) Shigeo Kobayashi, 2016.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
#ifndef ___BT_PAGEIO_H___
#define ___BT_PAGEIO_H___

#include "blockio.h"

#define MIN_PAGES_CACHED 4 // At least 16 pages are to be cached.

// Next page pointer + Record
typedef struct _ITEM {
	U_INT64    iNextPage;
	U_CHAR     Record[1];      // Record = Key + Data
} ITEM;
typedef ITEM *P_ITEM;

/*
//
//@Page data cached
typedef struct _PAGE {
	U_INT64       iPage;         // Page # (==Block #)
	U_INT64       iParentPage;   // Parent page # 
	S_INT64       iYoungestSon;  // Page # to the youngest son.
	S_INT64       nItemsUsed;
	ITEM          Items[1];      // ITEM => Next large page # + Record 
} PAGE;
typedef PAGE *P_PAGE;
*/

//                             (sizeof(iNextPage)+ Record size)
#define ITEM_SIZE(sRec)        (sizeof(U_INT64 )  + sRec       )
//                              sizeof(U_INT64 )*4 ==>  iPage,iParentPage,iYoungestSon,nItemsUsed
#define PAGE_SIZE(sRec,nItem)  (sizeof(U_INT64 )*4+ITEM_SIZE(sRec)*nItem)

#define SEEK_PAGE          m_pCache->Seek(0)
#define SEEK_PARENT        m_pCache->Seek(sizeof(U_INT64 ))
#define SEEK_YOUNGEST      m_pCache->Seek(sizeof(U_INT64 )*2)
#define SEEK_ITEMS         m_pCache->Seek(sizeof(U_INT64 )*3)
#define ITEM_POS(i)        (sizeof(U_INT64 )*4+ITEM_SIZE(m_pPageIo->GetRecordLength())*i)
#define SEEK_ITEM(i)       m_pCache->Seek(ITEM_POS(i))
#define SEEK_ITEM_KEY(i)   m_pCache->Seek(ITEM_POS(i)+sizeof(U_INT64 ))
#define SEEK_ITEM_DATA(i)  m_pCache->Seek(ITEM_POS(i)+sizeof(U_INT64 )+m_pPageIo->GetKeyLength())
#define SEEK_ITEM_DATA_FIELD(i,offset)  m_pCache->Seek(ITEM_POS(i)+sizeof(U_INT64 )+m_pPageIo->GetKeyLength()+offset)

// Working Item for overflow case
class CBubbleupItem : public CBase
{
public:
	CBubbleupItem()
	{
		m_pKey      = NULL;
		m_pData     = NULL;
		m_iPrevPage = 0;
		m_pItem     = NULL;
		m_pTemp     = NULL;
		Inserted    = FALSE;
	};

	~CBubbleupItem()
	{
		Close();
	};

	void Close()
	{
		if(m_pItem) BseMemFree((void **)&m_pItem);
		if(m_pTemp) BseMemFree((void **)&m_pTemp);
		m_pItem          = NULL;
		m_pKey           = NULL;
		m_pTemp          = NULL;
		m_pData          = NULL;
		m_iPrevPage      = Nil;
	};

	ITEM    *GetItemPTR()             {return m_pItem;};
	void    *GetKey()                 {return m_pKey;};
	void    *GetData()                {return m_pData;};
	U_INT64  GetLeftPage()            {return m_iPrevPage;};
	U_INT64  GetRightPage()           {return m_pItem->iNextPage;};
	void     SetLeftPage (U_INT64  l) {m_iPrevPage = l;};
	void     SetRightPage(U_INT64  l) {m_pItem->iNextPage=l;};
	U_INT32  GetItemSize()            {return sizeof(U_INT64)+m_sKey+m_sData;};

	void   CopyITEM(ITEM *i)
	{
		memcpy(m_pItem,i,m_sItem);
	};

	void   SwapITEM(ITEM *i)
	{
		memcpy(m_pTemp,i,m_sItem);
		memcpy(i,m_pItem,m_sItem);
		memcpy(m_pItem,m_pTemp,m_sItem);
	};

	//
	void SwapITEM(ITEM *i,ITEM *j)
	{
		memcpy(m_pTemp,i,m_sItem);
		memcpy(i,j,m_sItem);
		memcpy(j,m_pTemp,m_sItem);
	};


	void Allocate(U_INT32   sKey,U_INT32   sData) {
		m_pItem  = (ITEM*)BseMemAlloc(sizeof(U_INT64 )+sKey+sData);
		m_pTemp  = BseMemAlloc(sizeof(U_INT64 )+sKey+sData);
		m_pKey   = m_pItem->Record;
		m_pData  = &(((U_CHAR*)m_pKey)[sKey]);
		m_sItem  = sizeof(U_INT64)+sKey+sData;
		m_sKey   = sKey;
		m_sData  = sData;
		Inserted = FALSE;
	};

	void Setup(U_INT64  iNext,U_INT64  iPrev,const void *pRecord)
	{
		memcpy(m_pItem->Record, pRecord, m_sKey+m_sData);
		m_pItem->iNextPage = iNext;
		m_iPrevPage        = iPrev;
		Inserted    = FALSE;
	};

	void Setup(U_INT64  iNext,U_INT64  iPrev,const void *pKey,const void *pData)
	{
		memcpy(m_pKey, pKey, m_sKey);
		memcpy(m_pData,pData,m_sData);
		m_pItem->iNextPage = iNext;
		m_iPrevPage        = iPrev;
		Inserted           = FALSE;
	};

	void *WorkItem() {return m_pTemp;}

public:
	int      Inserted;

private:
	U_INT64   m_iPrevPage;
	void     *m_pTemp;
	void     *m_pKey;
	void     *m_pData;
	ITEM     *m_pItem;
	U_INT32   m_sItem;
	U_INT32   m_sKey;  // Total key byte size
	U_INT32   m_sData; // Total data byte size
};

class CPageKeeper;

//
class CPageIo: public CBlockIo
{
public:

	CPageIo();
	~CPageIo();

	PAGE_HEADER   PageHeader;
	LDB_HANDLE   *pLdbHandle;

	void	PIoOpen (P_CHAR szPath,char chMode,LDB_TYPE fKeyType,S_INT32   sKey,KEY_COMP_FUNCTION *pKeyCompFunc,LDB_TYPE fDataType,S_INT32   sData,S_INT32   nItems,S_INT32   nPagesCached);
	void	PIoClose();
	void    PIoScanDb(int info); // info =1: just print db info,=2 print info and verify db.

	int		PIoGetInfo     (LDB_INFO *pInfo);
	int     PIoUserHeaderIo(void *pData,int cb,char chIo);
	int		PIoAddRecord   (void *pKey,void *pData);
	int		PIoGetData     (void *pKey,void *pData);
	int		PIoSetData     (void *pKey,void *pData);
	int     PIoWriteRecord (void *pKey,void *pData);
	int     PIoDeleteRecord(void *pKey);

	int		PIoGetMinimumRecord(void *pKey,void *pData);
	int		PIoGetMaximumRecord(void *pKey,void *pData);
	int     PIoGetNextRecord(void *pKey,void *pData);
	int     PIoGetPrevRecord(void *pKey,void *pData);
	int		PIoGetCurRecord(void *pKey,void *pData);
	int		PIoSetCurData(void *pData);
	S_INT64 	PIoVerify(int detail);

	//
	// Low level methods
	//
	int     PIoGetCurrentPTR(U_INT64 *piBlock,int *piItem)
	{
		if(IsNil(m_iCurrentPage) || m_iCurrentItem<0) return NO_KEY_SELECTED;
		*piBlock = m_iCurrentPage;  // Current search page
		*piItem  = m_iCurrentItem;  // Current search item in the current search page.
		return 0;
	};

	int     PIoGetRootPage(U_INT64 *pRoot)
	{
		*pRoot = PageHeader.RootPage;
		return 0;
	};
	int     PIoGetRecord(U_INT64 iBlock,int iItem,void *pKey,void *pData);
	int     PIoGetItemNumber(U_INT64 iBlock);
	int		PIoGetNextPage(U_INT64 iBlock,int iItem,U_INT64 *pNext);
	int     PIoSetData(U_INT64 iBlock,int iItem,void *pData);
	int		PIoKeyCompare(int *pf,void *pKey1,void *pKey2)
	{
		// 2018:10:30 bug fixed.
		*pf = (*m_pKeyFunc)(pKey1,pKey2,m_cKeyComp,pLdbHandle);
		return 0;
	};

private:
	U_INT32      GetKeyLength()      {return PageHeader.KeyByteSize;};
    U_INT32      GetDataLength()     {return PageHeader.DataByteSize;};
    U_INT32      GetRecordLength()   {return PageHeader.KeyByteSize+PageHeader.DataByteSize;};
    S_INT32      GetMaxItems()       {return PageHeader.MaxItems;};
	U_INT32      GetItemSize()       {return (U_INT32  )ITEM_SIZE(GetRecordLength());};
	U_INT32      GetByteSize(LDB_TYPE t);
	U_INT32      GetKeyElementSize() {return m_cbKeyUnit;};
	U_INT32      GetDataElementSize(){return m_cbDataUnit;};
	void         Flush();
	int          IsRecordReady()     {return m_iCurrentPage != Nil;};

private:
	friend class CPageKeeper;

	S_INT32  GetItemCount(S_INT32   sData);

	void     ReadPageHeader()	{
				CachedBlock *pCache = CIoGetCache(0);
				pCache->Seek(sizeof(BLOCK_HEADER));
				pCache->Get(&PageHeader,sizeof(PAGE_HEADER));
			 };

	// write PageHeader back to the db file without affecting Modified flag.
	void     WritePageHeader()
			 {
				CachedBlock *pCache = CIoGetCache(0);
				PAGE_HEADER *ph = (PAGE_HEADER *)pCache->SeekP(sizeof(BLOCK_HEADER));
				FIoSeek(sizeof(BLOCK_HEADER));
				FIoWrite(&PageHeader,sizeof(PageHeader));
				memcpy(ph,&PageHeader,sizeof(PageHeader));
			 };

	U_INT64  AddPage()
	{
		CachedBlock *pCache = BIoAddBlock();
		pCache->Seek(0);
		pCache->Put(&pCache->iBlock,sizeof(U_INT64 ));
		return pCache->iBlock; // Block number.
	};

	void DeletePage(U_INT64  iBlock)
	{
		ASSERT(iBlock!=0);
		BIoDeleteBlock(iBlock);
		ResetPtr();
		if(iBlock==PageHeader.RootPage) {
			// delete root page
			PageHeader.RootPage = 0;
			WritePageHeader();
		}
	};

	void	SetParent(U_INT64  iPage,U_INT64  iParent);
	void    SetRootPage(U_INT64  iPage);

	void    ResetPtr() {m_iCurrentPage = Nil;m_iCurrentItem = -1;};
	void    GetSearchPTR(BTREE_SEARCH_PTR *ptr) {ptr->iCurrentPage=m_iCurrentPage;ptr->iCurrentItem=m_iCurrentItem;};
	void    SetSearchPTR(BTREE_SEARCH_PTR *ptr) {m_iCurrentPage=ptr->iCurrentPage;m_iCurrentItem=ptr->iCurrentItem;};
	U_INT64 GetRootPage() {return PageHeader.RootPage;};

	void SwapITEM(ITEM *i,ITEM *j)
	{
		m_BubbleupItem.SwapITEM(i,j);
	};

	void CopyITEM(ITEM *dest,ITEM *src)
	{
		memcpy(dest,src,GetItemSize());
	};

	void CopyREC(void *dest,void *src)
	{
			memcpy(dest,src,GetRecordLength());
	}

	int     InsertItem(U_INT64  iRoutePage,S_INT32   L);
	void	InsertBubbleupItemAt(CPageKeeper*pPage,S_INT32   R);
	void    DividePage(U_INT64  iOldPage,S_INT32   L);
	S_INT32      GetLocationInParent(U_INT64  iParent,U_INT64  iPage);
	void	LeftShiftThroughParent(CPageKeeper*pParent,S_INT32   R);
	void	RightShiftThroughParent(CPageKeeper*pParent,S_INT32   R);

	U_INT64  GetNextMinimum(U_INT64  iPage);
	U_INT64  GetPrevMaximum(U_INT64  iPage);

	int 	MoveNext();
	int 	MovePrev();

	int 	LookupKey(U_INT64  iPage);
	int		DeleteKey(void *pKey);
	int 	DeleteKeySearch(U_INT64  iPage);
	int 	DeleteNonLeafKey(U_INT64  iRoutePage,S_INT32 R);
	int 	UnderFlow(U_INT64  iParent,U_INT64  iPage,S_INT32   R);
	int 	BinSearch(U_INT64  iPage,void *pKey1,S_INT32   *pR);
	int 	LeftShiftAndInsertBubbleupItemAt(U_INT64  iRoutePage,S_INT32   R,S_INT32   L);
	int 	RightShiftAndInsertBubbleupItemAt(U_INT64  iRoutePage,S_INT32   R,S_INT32   L);
//	int		KeyComp(const void *pKey1,const void *pKey2,INT32   sKey);

//#ifdef _DEBUG
private:
	U_INT64 CheckDeletedChain(int f);
	void	CheckPage(U_INT64 iPage);
	// only used in PIoVerify()
	U_INT64  m_cPage; 
	U_INT64  m_cDeleted;
	S_INT64  m_cKeys;
	S_INT32  m_nHeight;
	S_INT32  m_cbKeyUnit;  // byte size for 1 key element
	S_INT32  m_cbDataUnit; // byte size for 1 data element
//#endif  // _DEBUG

protected:
	CBubbleupItem m_BubbleupItem;

	int			       m_cKeyComp;
	int   		       m_fFound;
	void	          *m_pSearchKey;    // Temporally pointer to search key.

	// Internal record position
	U_INT64 		   m_iCurrentPage;  // Current search page
	S_INT32  		   m_iCurrentItem;  // Current search item in the current search page.

	U_INT64 		   m_iPageDeleted;  // Page having the item deleted.
	S_INT32            m_iItemDeleted;  // item deleted.

	KEY_COMP_FUNCTION *m_pKeyFuncs[11];
	KEY_COMP_FUNCTION *m_pKeyFunc;
};

//
// chache management class used by CPageIo main Page I/O class.
//   CPageIo << CBloakIo (Controls caches and actual I/O in blocks)
//   CPageKeeper keeps specified page(on the serch path) in cache memory and locks/unlocks it.   
// This CPageKeeper class is declared at the top of each functions and 
// kept in stack(not declared as "new CPageKeeper"). Cache in the memories(in CPageKeeper) is locked.
// When returning from functions(methods),C++ pops stack and deletes all class in the stack kept for the functions.
// CPageKeeper class unlocks it's cache when it's destructor is called.
// So CPageKeeper handles only one page, whereas CPageIo handles all pages handled by CPageKeeper.
// The access to any page data must be done through CPageKeeper class which assures the data on memories. 
// 
class CPageKeeper {
public:
	CPageKeeper(CPageIo *pPageIo,U_INT64  iBlock)
	{
		m_pPageIo       = pPageIo;
		m_pCache        = m_pPageIo->CIoGetCache(iBlock);
		m_pCache->Lock();
//#ifdef _DEBUG
		MX = m_pPageIo->GetMaxItems();
//#endif
	}

	~CPageKeeper()
	{
		if(m_pPageIo==NULL) return;
		m_pCache->Unlock();
		m_pPageIo        = NULL;
		m_pCache         = NULL;
	}

	U_INT64  GetPageNo()                  {U_INT64  v;SEEK_PAGE;		 m_pCache->Get(&v,sizeof(U_INT64 ));return v;};
	void     SetPageNo(U_INT64  v)        {           SEEK_PAGE;         m_pCache->Put(&v,sizeof(U_INT64 ));};

	U_INT64  GetParent()                  {U_INT64  v;SEEK_PARENT;       m_pCache->Get(&v,sizeof(U_INT64 ));return v;};
	void     SetParent(U_INT64  v)        {           SEEK_PARENT;       m_pCache->Put(&v,sizeof(U_INT64 ));};

	U_INT64  GetYoungest()                {U_INT64  v;SEEK_YOUNGEST;     m_pCache->Get(&v,sizeof(U_INT64 ));return v;};
	void     SetYoungest(U_INT64  v)      {           SEEK_YOUNGEST;     m_pCache->Put(&v,sizeof(U_INT64 ));};

	S_INT32  GetItemsUsed()               {U_INT64  v;SEEK_ITEMS;        m_pCache->Get(&v,sizeof(U_INT64 ));return (S_INT32)v;};
	void     SetItemsUsed(U_INT64  v)     {          SEEK_ITEMS;         m_pCache->Put(&v,sizeof(U_INT64 ));};

	U_INT64  GetItemNext(int i)           {U_INT64  v;ASSERT(i>=0 && i<MX);SEEK_ITEM(i);     m_pCache->Get(&v,sizeof(U_INT64 ));return v;};
	void     SetItemNext(int i,U_INT64  v){           ASSERT(i>=0 && i<MX);SEEK_ITEM(i);     m_pCache->Put(&v,sizeof(U_INT64 ));};

	void     GetItemKey(void *p,int i)    {ASSERT(i>=0 && i<MX);          SEEK_ITEM_KEY(i);  m_pCache->Get(p,m_pPageIo->GetKeyLength());};
	void     SetItemKey(void *p,int i)    {ASSERT(i>=0 && i<MX);          SEEK_ITEM_KEY(i);  m_pCache->Put(p,m_pPageIo->GetKeyLength());};

	void     GetItemData(void *p,int i)   {ASSERT(i>=0 && i<MX);          SEEK_ITEM_DATA(i); m_pCache->Get(p,m_pPageIo->GetDataLength());};
	void     SetItemData(void *p,int i)   {ASSERT(i>=0 && i<MX);          SEEK_ITEM_DATA(i); m_pCache->Put(p,m_pPageIo->GetDataLength());};
	ITEM    *GetItemPTR(int i)            {ASSERT(i>=0 && i<MX);return (ITEM *)(((U_CHAR*)m_pCache->Buffer)+ITEM_POS(i));};
	void    *GetItemKey(int i)			  {ASSERT(i>=0 && i<MX);return (void *)(((U_CHAR*)GetItemPTR(i))+sizeof(U_INT64 ));};

	U_INT64   GetItemNextPage(S_INT32 ix)
	{
		ASSERT(ix>=-1 && ix<MX);
		if(ix<0) return GetYoungest();
		return GetItemNext(ix);
	};

	void  SetItemNextPage(S_INT32 ix,U_INT64  l)
	{
		ASSERT(ix>=-1 && ix<MX);
		if(ix<0) SetYoungest(l);
		else     SetItemNext(ix,l);
	};

	// from ixS position,shift 1 Item to right and get space at Item[ixS]
	void RightShiftItem(S_INT32 ixS)
	{
		ASSERT(ixS>=0 && ixS<MX);
		S_INT32   ixE = GetItemsUsed();
		U_INT32   cb = m_pPageIo->GetItemSize();
		ASSERT(ixS>=0 && ixE>=0);
		while(ixE>ixS) {
			// 2018:10:30 memcpy -> memmove
			memmove(GetItemPTR(ixE),GetItemPTR(ixE-1),cb);
			ixE--;
		}
	}

	// from ixS position,shift 1 Item to left and get space at Item[ixS]
	// returns number of items after deletion
	S_INT32   LeftShiftItem(S_INT32 ixS)
	{
		ASSERT(ixS>=0 && ixS<MX);
		S_INT32   ixE = GetItemsUsed() - 1;
		U_INT32   cb = m_pPageIo->GetItemSize()*(ixE-ixS);
		// 2018:10:30 memcpy -> memmove
		if(cb>0) memmove(GetItemPTR(ixS),GetItemPTR(ixS+1),cb);
		return ixE;
	}

	// remove item at the ix position and
	// returns TRUE if underflow 
	int  RemoveItem(S_INT32 ix)
	{
		ASSERT(ix>=0 && ix<MX);
		S_INT32   n = LeftShiftItem(ix);
		SetItemsUsed(n);
		return n<m_pPageIo->GetMaxItems()/2;
	}

	void SetModified()
	{
		m_pCache->Modified = TRUE;
	};

public:
    // Page caches
	CPageIo    *m_pPageIo;
	CachedBlock *m_pCache;
//#ifdef _DEBUG
	S_INT32        MX; // Max. number of items
//#endif
};
#endif // ___BT_PAGEIO_H___
