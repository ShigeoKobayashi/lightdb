//
//****************************************************************************
//*                                                                          *
//* @Copyright (C) Shigeo Kobayashi,2016.                                   *
//*                 All rights reserved.                                     *
//*                                                                          *
//****************************************************************************
//

#include "stdafx.h"
#include "pageio.h"

static
const char *gstType[] = {
        "T_UBYTE8",    /*  1  ... Unsigned char   ( 8-bit)         */
        "T_SBYTE8",    /*  2  ... Signed char     ( 8-bit)         */
        "T_USHORT16",  /*  3  ... Unsigned short  (16-bit)         */
        "T_SSHORT16",  /*  4  ... Signed short    (16-bit)         */
        "T_UINT32",    /*  5  ... Unsigned integer(32-bit)         */
        "T_SINT32",    /*  6  ... Signed integer  (32-bit)         */
        "T_UINT64 ",   /*  7  ... Unsigned long   (64-bit,__int64) */
        "T_SINT64 ",   /*  8  ... Signed long     (64-bit,__int64) */
        "T_FLOAT32",   /*  9  ... Float           (32-bit)         */
        "T_DOUBLE64",  /* 10  ... Double          (64-bit)         */
        "T_UNDEFINED", /* 11  ... Undefined       ( 8-bit)         */
};


void CPageIo::PIoScanDb(int f)
{

	CIoFlush();

	T_(f==0,"\nLightDB file structure & cache verification started\n");

	T_(f==0," LightDB version    = %lld\n",BlockHeader.Version);
	T_(f==0," Total blocks       = %lld\n",BlockHeader.TotalBocks);
	T_(f==0," Total records      = %lld\n",PageHeader.TotalRecords);
	T_(f==0," Deleted pages      = %lld\n",BlockHeader.DeletedBlocks);
	T_(f==0," Page byte size     = %lld\n",BlockHeader.BlockByteSize);
	T_(f==0," First deleted page = %lld\n",BlockHeader.TopDeleted);

	int ik = PageHeader.KeyType;
	int id = PageHeader.DataType;
	const char *sik = "?";
	const char *sid = "?";
	if(ik>0&&ik<=sizeof(gstType)/sizeof(gstType[0])) sik = (char*)gstType[ik-1]; 
	if(id>0&&id<=sizeof(gstType)/sizeof(gstType[0])) sid = (char*)gstType[id-1];

	// Check for file size
	U_INT64  fileSize = FIoSize();
	if(BlockHeader.TotalBocks*BlockHeader.BlockByteSize!=fileSize) {
		printf(" Warning: The file size is not equal to TotalBocks * BlockByteSize.\n");
	}
	T_(f==0," File size          = %lld\n",fileSize);
	if((fileSize % BlockHeader.BlockByteSize) != 0) printf(" Warning: The file size is not a multiple of page size.\n");

	T_(f==0," Key type           = %ld[%s]\n",PageHeader.KeyType,sik);
	T_(f==0," Key byte size      = %ld\n",PageHeader.KeyByteSize);
	T_(f==0," Data type          = %ld[%s]\n",PageHeader.DataType,sid);
	T_(f==0," Data byte size     = %ld\n",PageHeader.DataByteSize);
	T_(f==0," Root page          = %lld\n",PageHeader.RootPage);
	T_(f==0," Max. items         = %ld\n",PageHeader.MaxItems);
	PIoVerify(f);
}


int CPageIo::PIoGetInfo(LDB_INFO *pInfo)
{
	extern const char *gstType[11];

	int sRec   = PageHeader.KeyByteSize + PageHeader.DataByteSize;
	int nItems = PageHeader.MaxItems;
	int cbAll  = PAGE_SIZE(sRec,nItems);
	int cbSys  = sizeof(PageHeader) + sizeof(BlockHeader);
	int cbRest = cbAll - cbSys;
	int cbUser = cbRest/2;

	ASSERT(BlockHeader.BlockByteSize==cbAll);
	if(pInfo) {
		pInfo->Version          = BlockHeader.Version;
		pInfo->CachedPages      = CIoCacheCount();
		pInfo->DataByteSize     = (U_INT32  )PageHeader.DataByteSize;
		pInfo->DataType         = (LDB_TYPE)PageHeader.DataType;
		pInfo->DeletedPage      = BlockHeader.TopDeleted;
		pInfo->KeyByteSize      = PageHeader.KeyByteSize;
		pInfo->KeyType          = (LDB_TYPE)PageHeader.KeyType;
		pInfo->MaxItems         = PageHeader.MaxItems;
		pInfo->PageByteSize         = cbAll;
		pInfo->RootPage         = PageHeader.RootPage;
		pInfo->TotalPages       = BlockHeader.TotalBocks;
		pInfo->UserAreaByteSize = cbUser;
	} else {
		printf(" LightDb version        = %llu\n",BlockHeader.Version);
		printf(" Total number of caches = %d\n",CIoCacheCount());
		printf(" Data byte size         = %lu\n",PageHeader.DataByteSize);
		printf(" Data type              = %d[%s]\n",PageHeader.DataType,gstType[PageHeader.DataType-1]);
		printf(" Top deleted page       = %llu\n",BlockHeader.TopDeleted);
		printf(" Key byte size          = %u\n",PageHeader.KeyByteSize);
		printf(" Key type               = %d[%s]\n",PageHeader.KeyType,gstType[PageHeader.KeyType-1]);
		printf(" Max. number of items   = %d\n",PageHeader.MaxItems);
		printf(" Page size              = %d\n",cbAll);
		printf(" Root page              = %llu\n",PageHeader.RootPage);
		printf(" Total blocks/Pages     = %llu\n",BlockHeader.TotalBocks);
		printf(" User area byte size    = %d\n",cbUser);
	}
	return 0;
}

int CPageIo::PIoUserHeaderIo(void *pbytes,int cb,char chIo)
{
	int sRec   = PageHeader.KeyByteSize + PageHeader.DataByteSize;
	int nItems = PageHeader.MaxItems;
	int cbAll  = PAGE_SIZE(sRec,nItems);
	int cbSys  = sizeof(PageHeader) + sizeof(BlockHeader);
	int cbRest = cbAll - cbSys;
	int cbUser = cbRest/2;
	int cbIo = cb;
	CachedBlock *pCache =	CIoGetCache(0);
	U_CHAR  *bptr;

	if(cbIo>cbUser) cbIo = cbUser;
	FIoSeek(cbAll-cbUser);
	bptr = (PU_CHAR)(pCache->Buffer)+(cbAll-cbUser);
	if(chIo=='W' || chIo=='w') {
		// 1:write
		CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 
		memcpy(bptr,pbytes,cbIo);
		FIoWrite(pbytes,cbIo);
	} else if(chIo=='R' || chIo=='r') {
		// 2:read
		memcpy(pbytes,bptr,cbIo);
	} else {
		ER_THROW(ERROR_BAD_ARGUMENT);
	}
	return 0;
}


#define KEY_COMP(NAME,K) \
 int NAME(void *pk1,void *pk2,int c,LDB_HANDLE *ph) {\
		K register *p1  = (K *)pk1;\
		K register *p2  = (K *)pk2;\
		for(int register i=0;i<c;++i) {\
				if     (p1[i]>p2[i]) return  1;\
				else if(p1[i]<p2[i]) return -1;\
		}\
		return 0;\
	}

// Key compare function
//           1     ... Unsigned char
//           2     ... Signed char
//           3     ... Unsigned short
//	         4     ... Signed short
//           5     ... Unsigned Integer
//           6     ... Integer
//           7     ... unsigned __int64
//           8     ... __int64
//           9     ... Float
//          10     ... Double
//          11     ... undefined, user provided key compare function.

/*  1 */ KEY_COMP(CompUC,U_CHAR);
/*  2 */ KEY_COMP(CompC,   CHAR);
/*  3 */ KEY_COMP(CompUS,U_SHORT);
/*  4 */ KEY_COMP(CompS, S_SHORT);
/*  5 */ KEY_COMP(CompUI,U_INT32);
/*  6 */ KEY_COMP(CompI, S_INT32);
/*  7 */ KEY_COMP(CompU64, U_INT64 );
/*  8 */ KEY_COMP(CompI64, S_INT64 );
/*  9 */ KEY_COMP(CompF, float);
/* 10 */ KEY_COMP(CompD, double);

////////////////////// CPageIo ///////////////////////////
CPageIo::CPageIo()
{
	pLdbHandle      = NULL;
	m_fFound        = FALSE;
	m_iCurrentPage  = 0;  // Current search page
	m_iCurrentItem  = -1; // Current search item in the current search page.
	memset(&PageHeader,0,sizeof(PAGE_HEADER));
	m_pKeyFuncs[ 0] = CompUC;
	m_pKeyFuncs[ 1] = CompC;
	m_pKeyFuncs[ 2] = CompUS;
	m_pKeyFuncs[ 3] = CompS;
	m_pKeyFuncs[ 4] = CompUI;
	m_pKeyFuncs[ 5] = CompI;
	m_pKeyFuncs[ 6] = CompU64;
	m_pKeyFuncs[ 7] = CompI64;
	m_pKeyFuncs[ 8] = CompF;
	m_pKeyFuncs[ 9] = CompD;
	m_pKeyFuncs[10] = NULL;
}

CPageIo::~CPageIo()
{
	PIoClose();
}

void 
CPageIo::PIoClose()
{
	BIoClose();
	m_BubbleupItem.Close();
}

U_INT32  
CPageIo::GetByteSize(LDB_TYPE t)
{
	switch(t)
    {
	case T_UBYTE8:   return 1;break; /*  1  ... Unsigned char   ( 8-bit)         */
	case T_SBYTE8:   return 1;break; /*  2  ... Signed char     ( 8-bit)         */
	case T_USHORT16: return 2;break; /*  3  ... Unsigned short  (16-bit)         */
	case T_SSHORT16: return 2;break; /*  4  ... Signed short    (16-bit)         */
	case T_UINT32:   return 4;break; /*  5  ... Unsigned integer(32-bit)         */
	case T_SINT32:   return 4;break; /*  6  ... Signed integer  (32-bit)         */
	case T_UINT64 :  return 8;break; /*  7  ... Unsigned long   (64-bit,__int64) */
	case T_SINT64 :  return 8;break; /*  8  ... Signed long     (64-bit,__int64) */
	case T_FLOAT32:  return 4;break; /*  9  ... Float           (32-bit)         */
	case T_DOUBLE64: return 8;break; /* 10  ... Double          (64-bit)         */
	case T_UNDEFINED:return 1;break; /* 11  ... Undefined       ( 8-bit)         */
	default:
		ER_THROW(ERROR_KEY_TYPE);
		break;
    }
	return 1; /* ????? */
}

//
// determines number of items in a page from record length.
// number of items will be adjusted so that a page size will be 4k or 8k bytes.
//
S_INT32   CPageIo::GetItemCount(S_INT32   sRec)
{

	S_INT32  	nItems = 8;

	while (PAGE_SIZE(sRec,nItems)<4096) {
		nItems += 8;
	}
	if(nItems<=32) {
		/* Number of items is too small for 4k page,then extend page size to 8k */
		while (PAGE_SIZE(sRec,nItems)<8192) {
			nItems += 8;
		}
	}
	return nItems;
}

void CPageIo::PIoOpen (P_CHAR szPath,char chMode,LDB_TYPE keyType,S_INT32   sKey,KEY_COMP_FUNCTION *pKeyCompFunc,
					   LDB_TYPE dataType,S_INT32   sData,
					   S_INT32   nItem,S_INT32   nCached)
{
	U_INT32   sRec;
	errno  = 0;
    CHECK(sKey > 0,ERROR_KEY_SIZE);
    CHECK(sData> 0,ERROR_DATA_SIZE);
	CHECK(keyType>=1 && keyType<=11,ERROR_KEY_TYPE);
	CHECK(dataType>=1 && dataType<=11,ERROR_DATA_TYPE);

	// Change to byte size from element size.
	sKey      = sKey*GetByteSize((LDB_TYPE)keyType);
	sData     = sData*GetByteSize((LDB_TYPE)dataType);
	sRec      = sKey + sData;

	if(nItem<=0 || nItem<8 || (nItem%2)!=0) nItem = GetItemCount(sRec);

	BIoOpen(szPath,chMode,PAGE_SIZE(sRec,nItem),nCached);
	if(FIoIsNewFile())
	{
		// New File(including truncation open)
		PageHeader.RootPage	    = 0;
		PageHeader.TotalRecords = 0;
		PageHeader.MaxItems	    = nItem;
		PageHeader.KeyType		= keyType;
		PageHeader.DataType	    = dataType;
		PageHeader.KeyByteSize	= sKey;
		PageHeader.DataByteSize	= sData;
		WritePageHeader();
	} else {
		// Existing file
		ReadPageHeader();
	}
	m_BubbleupItem.Allocate(PageHeader.KeyByteSize,PageHeader.DataByteSize);
	int ixf = PageHeader.KeyType-1;
	CHECK(ixf>=0 && ixf<sizeof(m_pKeyFuncs)/sizeof(m_pKeyFuncs[0]),ERROR_KEY_TYPE);
	if(PageHeader.KeyType==T_UNDEFINED) CHECK(pKeyCompFunc!=NULL,ERROR_KEY_TYPE);
	m_pKeyFuncs[10] = pKeyCompFunc;
	m_pKeyFunc   = m_pKeyFuncs[ixf];
	m_cKeyComp   = PageHeader.KeyByteSize/GetByteSize((LDB_TYPE)PageHeader.KeyType);
	m_cbKeyUnit  = GetByteSize((LDB_TYPE)PageHeader.KeyType);  // byte size for 1 key element
	m_cbDataUnit = GetByteSize((LDB_TYPE)PageHeader.DataType); // byte size for 1 data element
}

S_INT32 CPageIo::GetLocationInParent(U_INT64  iParent,U_INT64  iPage)
{
	CPageKeeper Page(this,iParent);
	S_INT32       n     = Page.GetItemsUsed();
	S_INT32       i;

	if(iPage==Page.GetYoungest()) return -1;

	for(i=0;i<n;++i) {
		if(Page.GetItemNextPage(i)==iPage) return i;
	}
	ASSERT(FALSE);
	ER_THROW(ERROR_BROKEN_FILE);
}

void CPageIo::SetParent(U_INT64  iPage,U_INT64  iParent) 
{
	if(!IsNil(iPage)) {
		CPageKeeper Page(this,iPage);
		Page.SetParent(iParent);
	}
}

void CPageIo::SetRootPage(U_INT64  iPage) 
{
	PageHeader.RootPage=iPage;
	CPageKeeper Root(this,iPage);
	Root.SetParent(0);
	WritePageHeader();
}


int  CPageIo::InsertItem(U_INT64 iPage,S_INT32 L)
//
// called by AddKey() first to insert an item.
//
// In the page iPage,pData->pSearchKey is searched.
// When the iten is not found, go down to lower page recursively.
// L is the stacked route going down and used to come back(going up).
//
// [Input]
//    iPage ... U_LONG, Page # the key inserted. 
//    L     ... Position of the page in the parent page.
//                   -1 if the page is the youngest son of the parent.
// [output]
//    TRUE  ... Overflow or initial insert
//    FALSE ... Inserted in the page having room.
{

	// If iRoutePage<0,searching reached to leaf and not found in the leaf.
	// This causes the item insertion from the bottom(leaf) of btee.
	if(IsNil(iPage)) return TRUE;
	else {
		S_INT32     R;
		CPageKeeper Page(this,iPage);

		if(BinSearch(iPage,m_BubbleupItem.GetKey(),&R)) {
			// Not leaf => Binary search in the page iRoutePage.
			// The same key found
			m_fFound     = TRUE;
			return FALSE;
		}

		// Key not found in the current page,go down to route.
		if(InsertItem(Page.GetItemNextPage(R),R)) {
			// InsertItem() returns TRUE,then the key must be inserted at R-th position.
			if(Page.GetItemsUsed() < GetMaxItems()) {
				// new item can be inserted (at R+1 location)
				InsertBubbleupItemAt(&Page,R);
				return FALSE;
			}
			// No room to insert. ==> look for left or right side pages.
			// L: route position in the parent page.
			if     (LeftShiftAndInsertBubbleupItemAt (iPage,R,L)) return FALSE; // No need to insert more.
			else if(RightShiftAndInsertBubbleupItemAt(iPage,R,L)) return FALSE; // No need to insert more.
			// No other selection ==> Page divided.
			DividePage(iPage,R);
			return TRUE;
		}
	}
	return FALSE;
}

void CPageIo::InsertBubbleupItemAt(CPageKeeper*pPage,S_INT32 R)
//
// Insert work item at R(=-1,0,1,2,...) position.
//
{
	S_INT32   L,N;
	U_INT64  iPage = pPage->GetPageNo();

	N = GetMaxItems()-1;
	R = R + 1;
	ASSERT(R>=0 && R<=N);
	// ensure Insertion place
	if(R<pPage->GetItemsUsed()) {
		for(L=N;L>R;--L) {
			memcpy(pPage->GetItemPTR(L),pPage->GetItemPTR(L-1),GetItemSize());
		}
	}
	if(R<=0) {
		pPage->SetYoungest(m_BubbleupItem.GetLeftPage());
	}
	CopyITEM(pPage->GetItemPTR(R),m_BubbleupItem.GetItemPTR());
	if(!m_BubbleupItem.Inserted) {
		m_BubbleupItem.Inserted = TRUE;
		m_iCurrentPage = pPage->GetPageNo();
		m_iCurrentItem = R;
	}
	pPage->SetItemsUsed(pPage->GetItemsUsed()+1);

	// Update parent-son relation.
	SetParent(m_BubbleupItem.GetLeftPage(),iPage);
	SetParent(m_BubbleupItem.GetRightPage(),iPage);
}

void CPageIo::DividePage(U_INT64 iOldPage,S_INT32 L)
//
// Divide page at R-th position.
//  iOldPage ... the page divided.
//  L        ... the location of iOldPage to divide.
//
{
	CPageKeeper  OldPage(this,iOldPage);
	CPageKeeper  NewPage(this,AddPage());
	
	U_INT64      iNewPage = NewPage.GetPageNo(); // This makes the page updated.
	S_INT32      n        = GetMaxItems()/2;
	S_INT32      i;

	ASSERT(OldPage.GetItemsUsed()==GetMaxItems());
	// Move last half items in the old page to new page.
	//               (new page) > (old page)
	memcpy(NewPage.GetItemPTR(0),OldPage.GetItemPTR(n),GetItemSize()*n);
	for(i=0;i<n;++i) SetParent(NewPage.GetItemNext(i),iNewPage);
	NewPage.SetParent(OldPage.GetParent());
	OldPage.SetItemsUsed(n);
	NewPage.SetItemsUsed(n);
	if(L<0) {
		SetParent(m_BubbleupItem.GetLeftPage(),iOldPage);
		SetParent(m_BubbleupItem.GetRightPage(),iOldPage);
		OldPage.SetYoungest(m_BubbleupItem.GetLeftPage());
		NewPage.SetYoungest(OldPage.GetItemNextPage(n-1));
		SetParent(NewPage.GetYoungest(),NewPage.GetPageNo());
		OldPage.RightShiftItem(0);
		CopyITEM(OldPage.GetItemPTR(0),m_BubbleupItem.GetItemPTR());
		if(!m_BubbleupItem.Inserted) {
			m_BubbleupItem.Inserted = TRUE;
			m_iCurrentPage = iOldPage;
			m_iCurrentItem = 0;
		}
		CopyITEM(m_BubbleupItem.GetItemPTR(),OldPage.GetItemPTR(n));
	} else if(L<(n-1)) {
		SetParent(m_BubbleupItem.GetLeftPage(),iOldPage);
		SetParent(m_BubbleupItem.GetRightPage(),iOldPage);
		L = L+1; // change to index of array.
		OldPage.RightShiftItem(L);
		CopyITEM(OldPage.GetItemPTR(L),m_BubbleupItem.GetItemPTR());
		if(!m_BubbleupItem.Inserted) {
			m_BubbleupItem.Inserted = TRUE;
			m_iCurrentPage = iOldPage;
			m_iCurrentItem = L;
		}
		CopyITEM(m_BubbleupItem.GetItemPTR(),OldPage.GetItemPTR(n));
		NewPage.SetYoungest(m_BubbleupItem.GetRightPage());
		SetParent(NewPage.GetYoungest(),iNewPage);
	} else if(L==(n-1)) {
		SetParent(m_BubbleupItem.GetLeftPage(),iOldPage);
		SetParent(m_BubbleupItem.GetRightPage(),iNewPage);
		NewPage.SetYoungest(m_BubbleupItem.GetRightPage());
		SetParent(NewPage.GetYoungest(),iNewPage);
	} else {
		SetParent(m_BubbleupItem.GetLeftPage(),iNewPage);
		SetParent(m_BubbleupItem.GetRightPage(),iNewPage);
		NewPage.SetYoungest(NewPage.GetItemNextPage(0));
		L -= n;
		m_BubbleupItem.SwapITEM(NewPage.GetItemPTR(0));
		if(L>0) {
			// rotate !
			memcpy(m_BubbleupItem.WorkItem(),NewPage.GetItemPTR(0),GetItemSize());
			memcpy(NewPage.GetItemPTR(0),NewPage.GetItemPTR(1),GetItemSize()*L);
			memcpy(NewPage.GetItemPTR(L),m_BubbleupItem.WorkItem(),GetItemSize());
			if(!m_BubbleupItem.Inserted) {
				m_BubbleupItem.Inserted = TRUE;
				m_iCurrentPage = NewPage.GetPageNo();
				m_iCurrentItem = L;
			}
		} else {
			if(!m_BubbleupItem.Inserted) {
				m_BubbleupItem.Inserted = TRUE;
				m_iCurrentPage = NewPage.GetPageNo();
				m_iCurrentItem = 0;
			}
		}
	}
	m_BubbleupItem.SetRightPage(iNewPage);
	m_BubbleupItem.SetLeftPage(iOldPage);
}

//
// Search and delete the key specified by pKey.
//
int CPageIo::DeleteKey(void *pKey)
{
	int e = 0;

	m_pSearchKey   = pKey;
	m_fFound       = FALSE;
	m_iPageDeleted = 0;
	m_iItemDeleted = -1; 

	if(DeleteKeySearch(GetRootPage())) {
		// underflow!
		CPageKeeper Page(this,GetRootPage());
		if(Page.GetItemsUsed()<=0) {
			// root empty -> delete it
			SetRootPage(Page.GetYoungest());
			DeletePage(Page.GetPageNo());
		}
	}

	if(m_fFound) {
		ASSERT(PageHeader.TotalRecords>0);
		PageHeader.TotalRecords--;
		WritePageHeader();
		ResetPtr(); // Reset current search point.
	} else {
		e = KEY_NOT_FOUND;
	}
	return e;
}

//
// Search key (m_pSearchKey) in the page iPage.
// Actual deletion is done here:
//  Deletion is always done at the leaf position.
//  If the key deleted is at non-leaf page/node,
//    then the most nearest key, in the leaf and is greater than the key deleted,
//    will be copied to the key deleted and be deleted. This means the reduction of 
//    keys in the leaf page. 
//    
int  CPageIo::DeleteKeySearch(U_INT64  iPage)
//    returns FALSE ... if not in underflow condition.
//            TRUE  ... underflowed due to deletion.
{
	U_INT64   iTargetPage;
	S_INT32   R;

	if(IsNil(iPage)) {
		/* Specified key is not in the index file */
		m_fFound       = FALSE;
		return FALSE;
	} else {
		CPageKeeper Page(this,iPage);
		if(BinSearch(iPage,m_pSearchKey,&R)) {
			//////////////////////////////////
			///  The specified key found.  ///
			//////////////////////////////////
			m_iPageDeleted = iPage;   // Save the page having the item deleted.
			m_iItemDeleted = R;       // Save the item deleted.

			// The Key found ==>Check if the page having the key is a leaf or not.
			iTargetPage   = Page.GetItemNextPage(R);
			if(IsNil(iTargetPage)) {
				////////////////////////////////////////////////////////////
				// The page having the key is a leaf. ==> just delete it! //
				////////////////////////////////////////////////////////////
				ASSERT(R>=0);
				return Page.RemoveItem(R);
			} else {
				////////////////////////////////////////////////////////////
				// The page having deleted key is not a leaf.             //
				//     ==> go down to leaf.                               //
				////////////////////////////////////////////////////////////
				if(DeleteNonLeafKey(iTargetPage,R)) {
					return UnderFlow(iPage,iTargetPage,R);
				}
			}
		} else {
			// The key not found, so searching continues.
			//   R  is
			//    -1  ... Specified key is less than the minimum in the page
			//     K  ... Specified key is between (Items[K],Items[K+1]) or greater than max.
			iTargetPage = Page.GetItemNextPage(R);
			// Call myself: recursive call
			if(DeleteKeySearch(iTargetPage)) {
				return UnderFlow(iPage,iTargetPage,R);
			}
		}
	}
	return FALSE;
}

int  CPageIo::DeleteNonLeafKey(U_INT64 iRoutePage,S_INT32 R)
//    returns FALSE ... if not in underflow condition.
//            TRUE  ... underflowed due to deletion.
{
	CPageKeeper Page(this,iRoutePage);
	if(!IsNil(Page.GetYoungest())) {
		// Go further to find the closest leaf.
		if(DeleteNonLeafKey(Page.GetYoungest(),R)) {
			return UnderFlow(iRoutePage,Page.GetYoungest(),-1);
		}
	} else {
		// Reached to the leaf which has the item close to
		// the key but greater than the specified key.
		CPageKeeper PageDel(this,m_iPageDeleted);
		///// 2018/9/28 added. Bug fix.
		CopyREC(PageDel.GetItemKey(m_iItemDeleted),Page.GetItemKey(0));
		PageDel.SetModified();
		return Page.RemoveItem(0);
	}
	return FALSE;
}

int  CPageIo::UnderFlow(U_INT64 iParent,U_INT64 iPage,S_INT32 R)
//
// Process 'Underflow' condition.
//    returns FALSE ... if not in underflow condition.
//            TRUE  ... underflowed due to deletion.
// [Input]
//     iParent      ...  Parent page of the 'UnderFlowPage'
//     iPage        ...  'UnderFlowPage'
//     R            ...  Route from(position of) the parent page to UnderFlowPage.
//                       -1,0,1,2,... 
{
	CPageKeeper Parent(this,iParent);
	CPageKeeper Page(this,iPage);
	S_INT32      n       = (S_INT32)Page.GetItemsUsed();

	ASSERT(n<GetMaxItems()/2);

	if(R>=0) {
		// Can borrow from the left ?
		CPageKeeper LeftPage(this,Parent.GetItemNextPage(R-1));

		if(LeftPage.GetItemsUsed()>GetMaxItems()/2) {
			RightShiftThroughParent(&Parent,R);
			return FALSE;
		}
		// Can not borrow, so merge
		while((n--)>=0) LeftShiftThroughParent(&Parent,R);
		// delete right page.
		DeletePage(Page.GetPageNo());
		// delete R-th parent item.
	} else {
		// Can borrow from the right ?
		CPageKeeper RightPage(this,Parent.GetItemNextPage(R+1));
		if(RightPage.GetItemsUsed()>GetMaxItems()/2) {
			LeftShiftThroughParent(&Parent,R+1);
			return FALSE;
		}
		// Can not borrow, so merge
		n = (S_INT32)RightPage.GetItemsUsed();
		while((n--)>=0) LeftShiftThroughParent(&Parent,R+1);
		// delete right page.
		DeletePage(RightPage.GetPageNo());
		// delete (R+1)-th parent item.
		if(R<0) Parent.SetYoungest(iPage);
		R++;
	}
	// Merged case: Item at R position is moved to child node => just delete it !
	return Parent.RemoveItem(R);
}

int  CPageIo::BinSearch(U_INT64 iPage,void *pKey1,S_INT32 *pR)
//
// [return Value]
//   TRUE if specified key found int page.
//  *pR    ... index to the item if found, or bellow
//   -1    ... Specified key is less than the minimum in the page
//    K    ... Specified key is between (Items[K],Items[K+1]) or greater than max.
//
{
	CPageKeeper Page(this,iPage);
	int       C;
	int       iMiddle,iLeft,iRight;
    void     *pKey2;

	iLeft  = 0;
	iRight = Page.GetItemsUsed() - 1;
	// Check edge condition
	if(iRight>0) {
		if((C = (*m_pKeyFunc)(pKey1,Page.GetItemKey(iRight),m_cKeyComp,pLdbHandle))>0) {
			*pR = iRight;
			return FALSE;
		}
		if(C==0) {
			*pR = iRight;
			return TRUE; // Found.
		}
		--iRight;
		if((C = (*m_pKeyFunc)(pKey1,Page.GetItemKey(iLeft),m_cKeyComp,pLdbHandle))<0) {
			*pR = -1;
			return FALSE;
		}
		if(C==0) {
			*pR = 0;
			return TRUE; // Found.
		}
		++iLeft;
	}
	// The key searched is in the range between iLeft to iRight.
	do {
		iMiddle = (iLeft + iRight)/2;
		pKey2= Page.GetItemKey(iMiddle);
		C = (*m_pKeyFunc)(pKey1,pKey2,m_cKeyComp,pLdbHandle);
		if     (C<0) iRight = iMiddle - 1;
		else if(C>0) iLeft  = iMiddle + 1;
		else {
			*pR = iMiddle;
			return TRUE; // Found.
		}
	} while(iRight>=iLeft);
	*pR = iRight;
	return FALSE; // Not found.
}

int  CPageIo::LookupKey(U_INT64  iPage)
{
	U_INT64  iNext;
	S_INT32   R;

	if(IsNil(iPage)) return FALSE;

	if(BinSearch(iPage,m_pSearchKey,&R)) {
		// The same key found
		m_iCurrentPage = iPage;
		m_iCurrentItem = R;
		m_fFound     = TRUE;
		return TRUE;
	} else {
		// The key not found in this page.
		CPageKeeper Page(this,iPage);
		iNext = Page.GetItemNextPage(R);
	}
	m_iCurrentPage = iPage;
	m_iCurrentItem = R;
	return LookupKey(iNext);
}  

void CPageIo::LeftShiftThroughParent(CPageKeeper*pParent,S_INT32 R)
//
//  Shift 1 item from right child page to left child page.
//    pParent ... Parent page.
//      R     ... Parent item position to shift(left & right pages of item R).
//
{
	U_INT64    iPage;
	S_INT32    n;
	ASSERT(R>=0);

	CPageKeeper Left (this,pParent->GetItemNextPage(R-1));
	CPageKeeper Right(this,pParent->GetItemNextPage(R));

	// Setup parent-child relations before shift.
	iPage = Right.GetYoungest();
	if(!IsNil(iPage)) {
		CPageKeeper Page(this,iPage);
		Page.SetParent(Left.GetPageNo());
	}
	ASSERT(pParent->GetItemNext(R)==Right.GetPageNo());

	pParent->SetItemNext(R,Right.GetYoungest());
	Right.SetYoungest(Right.GetItemNext(0));
	Right.SetItemNext(0,Right.GetPageNo());

	// shift down 1 item from parent to left page.
	n      = Left.GetItemsUsed();
	ASSERT(n<GetMaxItems());

	CopyITEM(Left.GetItemPTR(n),pParent->GetItemPTR(R));
	Left.SetItemsUsed(n+1);
	if(Right.GetItemsUsed()>0) {
		// shift up 1 item from right to parent.
		CopyITEM(pParent->GetItemPTR(R),Right.GetItemPTR(0));
		// shift right page left 1 item.
		Right.RemoveItem(0);
	}
}

void CPageIo::RightShiftThroughParent(CPageKeeper*pParent,S_INT32 R)
//
//  Shift 1 item from left child page to right child page.
//    pParent ... Parent page.
//      R     ... Parent item position to shift(left & right pages of item R).
//
{
	U_INT64  iPage;
	S_INT32   n;
	ASSERT(R>=0);

	CPageKeeper Left (this,pParent->GetItemNextPage(R-1));
	CPageKeeper Right(this,pParent->GetItemNextPage(R));

	n = Left.GetItemsUsed()-1;	
	// Setup parent-child relations before shift.
	ASSERT(pParent->GetItemNext(R)==Right.GetPageNo());

	iPage = Left.GetItemNext(n);
	if(!IsNil(iPage)) {
		CPageKeeper Page(this,iPage);
		Page.SetParent(Right.GetPageNo());
	}
	pParent->SetItemNext(R,Right.GetYoungest());
	Right.SetYoungest(iPage);
	Left.SetItemNextPage(n,Right.GetPageNo());

	// shift right page right 1 item.
	Right.RightShiftItem(0);
	// shift down 1 item from parent to right page.
	CopyITEM(Right.GetItemPTR(0),pParent->GetItemPTR(R));
	Right.SetItemsUsed(Right.GetItemsUsed()+1);

	if(n>=0) {
		// shift up 1 item from left to parent.
		CopyITEM(pParent->GetItemPTR(R),Left.GetItemPTR(n));
		Left.SetItemsUsed(Left.GetItemsUsed()-1);
	}
}


int  CPageIo::LeftShiftAndInsertBubbleupItemAt(U_INT64 iRoutePage,S_INT32 R,S_INT32 L)
//
// iRoutePage ... This page number.
// R          ... Location to insert free item.
// L          ... Location in the parent page.
//
{
	if(L<0)               return FALSE; // Unable to shift left.
	if(IsNil(iRoutePage)) return FALSE; // No parent
	else {
		S_INT32  i;
		S_INT32  j;
		U_INT64  iParentPage;
		U_INT64  iPage;

		CPageKeeper RoutePage(this,iRoutePage);
		iParentPage=RoutePage.GetParent();
		if(IsNil(iParentPage))  return FALSE; // No parent.
		CPageKeeper ParentPage(this,iParentPage);

		// check for rooms 
		for(i=L-1;i>=-1;--i) {
			CPageKeeper FarPage(this,iPage=ParentPage.GetItemNextPage(i));

			if(FarPage.GetItemsUsed()<GetMaxItems()) {
				// pPage has room to insert. move 1 item from page i to L items in the parent.
				for(j=i+1;j<L;++j) {
					LeftShiftThroughParent(&ParentPage,j);
				}
				// Now left son has 1 room at least.
				if(R<0) {
					// Place BubbleupItem at the parent position.
					CPageKeeper LeftPage(this,ParentPage.GetItemNextPage(L-1));
					int ni = LeftPage.GetItemsUsed();
					CopyITEM(LeftPage.GetItemPTR(ni),ParentPage.GetItemPTR(L));
					LeftPage.SetItemNext(ni, m_BubbleupItem.GetLeftPage());
					LeftPage.SetItemsUsed(LeftPage.GetItemsUsed()+1);
					SetParent(LeftPage.GetItemNext(ni),LeftPage.GetPageNo()); 
					RoutePage.SetYoungest(m_BubbleupItem.GetRightPage());
					SetParent(RoutePage.GetYoungest(),RoutePage.GetPageNo());
					CopyITEM(ParentPage.GetItemPTR(L),m_BubbleupItem.GetItemPTR());
					if(!m_BubbleupItem.Inserted) {
						m_BubbleupItem.Inserted = TRUE;
						m_iCurrentPage = ParentPage.GetPageNo();
						m_iCurrentItem = L;
					}
					ParentPage.SetItemNext(L,RoutePage.GetPageNo());
					SetParent(ParentPage.GetItemNext(L),ParentPage.GetPageNo());
				} else {
					LeftShiftThroughParent(&ParentPage,L);
					InsertBubbleupItemAt(&RoutePage,R-1);
				}
				return TRUE;
			}
		}
	}
	return FALSE; // No room for right pages.
}

int  CPageIo::RightShiftAndInsertBubbleupItemAt(U_INT64 iRoutePage,S_INT32 R,S_INT32 L)
//
// iRoutePage ... This page number.
// R          ... Location to insert free item.
// L          ... Location in the parent page.
//
{
	if(IsNil(iRoutePage)) return FALSE; // No parent
	else {
		S_INT32   i;
		S_INT32   j;
		S_INT32   n;

		CPageKeeper RoutePage(this,iRoutePage);
		U_INT64     iParentPage = RoutePage.GetParent();
		if(IsNil(iParentPage)) return FALSE; // No parent.
		CPageKeeper ParentPage(this,iParentPage);

		n = ParentPage.GetItemsUsed();
		// check for rooms 
		for(i=L+1;i<n;++i) {
			CPageKeeper FarPage(this, ParentPage.GetItemNextPage(i));
			if(FarPage.GetItemsUsed()<GetMaxItems()) {
				// pPage has room to insert. move 1 item from page i to L items in the parent.
				for(j=i;j>L;--j) RightShiftThroughParent(&ParentPage,j);
				if(R>=RoutePage.GetItemsUsed()) {
					L = L + 1;
					CPageKeeper RightPage(this,ParentPage.GetItemNextPage(L));
					SwapITEM(RoutePage.GetItemPTR(R),ParentPage.GetItemPTR(L));
					RoutePage.SetItemNext(R,RightPage.GetYoungest());
					SetParent(RoutePage.GetItemNextPage(R),RoutePage.GetPageNo());
					m_BubbleupItem.SwapITEM(ParentPage.GetItemPTR(L));
					if(!m_BubbleupItem.Inserted) {
						m_BubbleupItem.Inserted = TRUE;
						m_iCurrentPage = ParentPage.GetPageNo();
						m_iCurrentItem = L;
					}
					RightPage.SetYoungest(ParentPage.GetItemNext(L));
					SetParent(RightPage.GetYoungest(),RightPage.GetPageNo());
					ParentPage.SetItemNext(L,RightPage.GetPageNo());
					SetParent(ParentPage.GetItemNext(L),ParentPage.GetPageNo());
					ParentPage.SetItemNext(L,RightPage.GetPageNo());
					RoutePage.SetItemsUsed(RoutePage.GetItemsUsed()+1);
				} else {
					InsertBubbleupItemAt(&RoutePage,R);
				}
				return TRUE;
			}
		}
	}
	return FALSE; // No room for right pages.
}

int CPageIo::PIoAddRecord(void *pKey,void *pData)
//
// returns:
//   negative ... some IO error detected
//    0       ... The key is normally added.
//   positive ... The same key found while duplicated key is not allowed.
//
{
	int       e = 0;

	CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 

	m_fFound     = FALSE;
	
	m_BubbleupItem.Setup(0,0,pKey,pData);

	// InsertItem starts from the Root Record of the BTree(pIndex->RootPage)
	if(InsertItem(PageHeader.RootPage,0)) {
		// Root divided, so create new root and set it's pointer 
		CPageKeeper Page(this,AddPage());
		PageHeader.RootPage = Page.GetPageNo(); // This makes the page updated.
		Page.SetParent(0);     // No more parent
		Page.SetYoungest(m_BubbleupItem.GetLeftPage());
		SetParent(m_BubbleupItem.GetLeftPage(),PageHeader.RootPage);
		SetParent(m_BubbleupItem.GetRightPage(),PageHeader.RootPage);
		Page.SetItemsUsed (1);
		CopyITEM(Page.GetItemPTR(0),m_BubbleupItem.GetItemPTR());
		if(!m_BubbleupItem.Inserted) {
			m_BubbleupItem.Inserted = TRUE;
			m_iCurrentPage = Page.GetPageNo();
			m_iCurrentItem = 0;
		}
	}
	if(m_fFound) e = SAME_KEY_EXISTS;
	else {
		PageHeader.TotalRecords++;
		WritePageHeader();
	}
	return e;
}

int CPageIo::PIoGetData(void *pKey,void *pData)
{
	int f;
	m_pSearchKey   = pKey;
	ResetPtr();
	f = LookupKey(PageHeader.RootPage);
	if(!f) return KEY_NOT_FOUND;
	CPageKeeper Page(this,m_iCurrentPage);
	Page.GetItemData(pData,m_iCurrentItem);
	return 0;
}

int CPageIo::PIoSetData(void *pKey,void *pData)
{
	int  f;
	CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 
	m_pSearchKey   = pKey;
	ResetPtr();
	f = LookupKey(PageHeader.RootPage);
	if(f){
		// Key found,then change data
		CPageKeeper Page(this,m_iCurrentPage);
		Page.SetItemData((void *)pData,m_iCurrentItem);
		return 0;
	}
	return KEY_NOT_FOUND;
}

int CPageIo::PIoWriteRecord(void *pKey,void *pData)
{
	int  f;
	CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 
	m_pSearchKey   = pKey;
	ResetPtr();
	f = LookupKey(PageHeader.RootPage);
	if(f){
		// Key found,then change data
		CPageKeeper Page(this,m_iCurrentPage);
		Page.SetItemData((void*)pData,m_iCurrentItem);
	} else {
		return PIoAddRecord(pKey,pData);
	}
	return 0;
}

int CPageIo::PIoDeleteRecord(void *pKey)
{
	int e = 0;

	CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 

	m_pSearchKey     = pKey;
	m_fFound = TRUE;

	if(DeleteKeySearch(PageHeader.RootPage)) {
		CPageKeeper Page(this,PageHeader.RootPage);
		if(Page.GetItemsUsed()<=0) {
			PageHeader.RootPage = Page.GetYoungest();
			DeletePage(Page.GetPageNo());
			SetParent(PageHeader.RootPage,0);
		}
	}

	if(m_fFound) {
		PageHeader.TotalRecords--;
		WritePageHeader();
		ResetPtr(); // Reset current search point.
	} else e = KEY_NOT_FOUND;

	return e;
}

int CPageIo::PIoGetCurRecord(void *pKey,void *pData)
{
	if(!IsNil(m_iCurrentPage) &&  m_iCurrentItem>=0) {
		CPageKeeper Page(this,m_iCurrentPage);
		Page.GetItemKey (pKey ,m_iCurrentItem);
		Page.GetItemData(pData,m_iCurrentItem);
		return 0;
	}
	return NO_KEY_SELECTED;
}

int CPageIo::PIoGetMinimumRecord(void *pKey,void *pData)
{
	if(IsNil(PageHeader.RootPage)) return NO_KEYS;
	else {
		U_INT64   iPage = PageHeader.RootPage;
		U_INT64   jPage;
Retry:
		{
			CPageKeeper	Page(this,iPage);
			jPage = Page.GetItemNextPage(-1);
			if(IsNil(jPage)) {
				// Get to minimum  page.
				m_iCurrentItem = 0;
				m_iCurrentPage = iPage;
				Page.GetItemKey (pKey ,m_iCurrentItem);
				Page.GetItemData(pData,m_iCurrentItem);
				return 0;
			}
			iPage = jPage;
		}
		goto Retry;
	}
}

int CPageIo::PIoGetMaximumRecord(void *pKey,void *pData)
{
	if(IsNil(PageHeader.RootPage)) return NO_KEYS;
	else {
		U_INT64   iPage = PageHeader.RootPage;
		U_INT64   jPage;
		S_INT32   n;
Retry:
		{
			CPageKeeper	Page(this,iPage);
			n     = Page.GetItemsUsed()-1;
			jPage = Page.GetItemNext(n);
			if(IsNil(jPage)) {
				// Get to maximum page.
				m_iCurrentItem = n;
				m_iCurrentPage = iPage;
				Page.GetItemKey (pKey ,m_iCurrentItem);
				Page.GetItemData(pData,m_iCurrentItem);
				return 0;
			}
			iPage = jPage;
		}
		goto Retry;
	}
}

U_INT64  CPageIo::GetNextMinimum(U_INT64  iPage)
{
	if(IsNil(iPage)) return Nil;
	else {
		CPageKeeper Page(this,iPage);
		U_INT64       jPage = Page.GetYoungest();
		if(!IsNil(jPage)) return GetNextMinimum(jPage);
		return Page.GetPageNo();
	}
}

U_INT64  CPageIo::GetPrevMaximum(U_INT64 iPage)
{
	if(IsNil(iPage)) return Nil;
	else {
		CPageKeeper  Page(this,iPage);
		S_INT32        ix = Page.GetItemsUsed() - 1;
		U_INT64        jPage = Page.GetItemNext(ix);
		if(!IsNil(jPage)) return GetPrevMaximum(jPage);
		return Page.GetPageNo();
	}
}

int  CPageIo::MoveNext()
{
	if(IsNil(m_iCurrentPage)) {
		m_iCurrentItem = -1;
		return FALSE;
	} else if(m_iCurrentItem<0) {
		// This means the previous search was failed,and the last seach point must be at leaf page.
		// So no need to go down.
		m_iCurrentItem = 0;
		return TRUE;
	} else {
		CPageKeeper Page(this,m_iCurrentPage);
		U_INT64     iNextPage;
		S_INT32     n         = Page.GetItemsUsed() - 1;

		iNextPage = Page.GetItemNext(m_iCurrentItem);

		if(!IsNil(iNextPage)) {
			// can go down
			m_iCurrentPage = GetNextMinimum(iNextPage);
			m_iCurrentItem = 0;
			ASSERT(!IsNil(m_iCurrentPage));
			return TRUE;
		}

		// The page is already a leaf. => go up !
		if(m_iCurrentItem<n) {
			m_iCurrentItem++;
			return TRUE;
		} else {
			// Get to page end,then go up.
			U_INT64  iParent = Page.GetParent();
			while(!IsNil(iParent)) {
				m_iCurrentItem = GetLocationInParent(iParent,m_iCurrentPage) + 1;
				m_iCurrentPage = iParent;
				{
					CPageKeeper CurPage(this,m_iCurrentPage);
					n     = CurPage.GetItemsUsed() - 1;
					if(m_iCurrentItem<=n) return TRUE;
					iParent = CurPage.GetParent();
				}
			}
			m_iCurrentItem = -1;
			m_iCurrentPage = Nil;
			return FALSE;
		}
	}
}

int  CPageIo::MovePrev()
{
	if(IsNil(m_iCurrentPage)) {
		m_iCurrentItem = -1;
		return FALSE;
	} else {
		CPageKeeper Page(this,m_iCurrentPage);
		S_INT32       n         = Page.GetItemsUsed() - 1;
		U_INT64       iNextPage;

		// ASSERT(m_iCurrentItem>=0);
		if(m_iCurrentItem<=0) {
			iNextPage = Page.GetYoungest();
		} else {
			iNextPage = Page.GetItemNext(m_iCurrentItem-1);
		}
		if(!IsNil(iNextPage)) {
			// can go down
			m_iCurrentPage = GetPrevMaximum(iNextPage);
			ASSERT(!IsNil(m_iCurrentPage));
			{
				CPageKeeper PrevPage(this,m_iCurrentPage);
				m_iCurrentItem = PrevPage.GetItemsUsed() - 1;
			}
			return TRUE;
		}

		// The page is already a leaf. => go up !
		if(m_iCurrentItem>0) {
			m_iCurrentItem--;
			return TRUE;
		} else {
			// Get to page top,then go up.
			U_INT64  iParent = Page.GetParent();
			while(!IsNil(iParent)) {
				m_iCurrentItem = GetLocationInParent(iParent,m_iCurrentPage);
				m_iCurrentPage = iParent;
				if(m_iCurrentItem>=0) return TRUE;
				CPageKeeper CurPage(this,iParent);
				iParent = CurPage.GetParent();
			}
			m_iCurrentItem = -1;
			m_iCurrentPage = Nil;
			return FALSE;
		}
	}
}

int CPageIo::PIoGetNextRecord(void *pKey,void *pData)
{
	if(MoveNext()) return PIoGetCurRecord(pKey,pData);
	return NO_MORE_KEY;
}

int CPageIo::PIoGetPrevRecord(void *pKey,void *pData)
{
	if(MovePrev()) return PIoGetCurRecord(pKey,pData);
	return NO_MORE_KEY;
}

int CPageIo::PIoSetCurData(void *pData)
{
	CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 
	if(IsNil(m_iCurrentPage) || m_iCurrentItem<0) return NO_KEY_SELECTED;
	else {
		CPageKeeper Page(this,m_iCurrentPage);
		Page.SetItemData((void *)pData,m_iCurrentItem);
	}
	return 0;
}

int CPageIo::PIoGetRecord(U_INT64 iBlock,int iItem,void *pKey,void *pData)
{
	if(iBlock==0 || iBlock>=BlockHeader.TotalBocks) ER_THROW(ERROR_BAD_BLOCK_NUMBER);
	CPageKeeper Page(this,iBlock);
	if((Page.GetPageNo()&FLAG64_DELETED)==FLAG64_DELETED) ER_THROW(ERROR_DELETED_BLOCK);
	if(iItem<-1 || iItem>=Page.GetItemsUsed())  ER_THROW(ERROR_BAD_ITEM_NUMBER);
	Page.GetItemData(pData,iItem);
	Page.GetItemKey (pKey,iItem);
	return 0;
}

int CPageIo::PIoGetItemNumber(U_INT64 iBlock)
{
	if(iBlock==0 || iBlock>=BlockHeader.TotalBocks) ER_THROW(ERROR_BAD_BLOCK_NUMBER);
	CPageKeeper Page(this,iBlock);
	if((Page.GetPageNo()&FLAG64_DELETED)==FLAG64_DELETED) ER_THROW(ERROR_DELETED_BLOCK);
	return Page.GetItemsUsed();
}

int	CPageIo::PIoGetNextPage(U_INT64 iBlock,int iItem,U_INT64 *pNext)
{
	if(iBlock==0 || iBlock>=BlockHeader.TotalBocks) ER_THROW(ERROR_BAD_BLOCK_NUMBER);
	CPageKeeper Page(this,iBlock);
	if((Page.GetPageNo()&FLAG64_DELETED)==FLAG64_DELETED) ER_THROW(ERROR_DELETED_BLOCK);
	if(iItem<-1 || iItem>=Page.GetItemsUsed())  ER_THROW(ERROR_BAD_ITEM_NUMBER);
	*pNext =  Page.GetItemNextPage(iItem);
	return 0;
}

int CPageIo::PIoSetData(U_INT64 iBlock,int iItem,void *pData)
{
	CHECK(!FIoIsReadOnly(),ERROR_FILE_ACCESS_DENIED); 
	if(iBlock==0 || iBlock>=BlockHeader.TotalBocks) ER_THROW(ERROR_BAD_BLOCK_NUMBER);
	CPageKeeper Page(this,iBlock);
	if((Page.GetPageNo()&FLAG64_DELETED)==FLAG64_DELETED) ER_THROW(ERROR_DELETED_BLOCK);
	if(iItem<-1 || iItem>=Page.GetItemsUsed())  ER_THROW(ERROR_BAD_ITEM_NUMBER);
	Page.SetItemData(pData,iItem);
	return 0;
}

// Check deleted block chain
U_INT64 CPageIo::CheckDeletedChain(int f)
{
	U_INT64  iDeleted = BlockHeader.TopDeleted;
	U_INT64  jDeleted;
	U_INT64  cDeleted = 0;

	// Check on deleted block link.
	while(iDeleted!=0) {
		++cDeleted;
		if(iDeleted>=BlockHeader.TotalBocks) {
			printf("ERROR: Bad deleted block #(=%lld, Total blocks=%lld !\n",iDeleted,BlockHeader.TotalBocks);
			break;
		}
		if(++m_cDeleted>BlockHeader.TotalBocks) {
			printf("ERROR: Bad deleted block chain(Total=%lld)!\n",BlockHeader.TotalBocks);
			break;
		}
		CachedBlock *pCache = CIoGetCache(iDeleted);
		pCache->Seek(0);
		pCache->Get(&jDeleted,sizeof(iDeleted));
		iDeleted = jDeleted & (~FLAG64_DELETED);
		T_((jDeleted&FLAG64_DELETED)==FLAG64_DELETED," Deleted page(%lld) not flaged as DELETED\n",iDeleted);
	}
//	T_(f==0," Deleted pages      = %lld\n",cDeleted);
	return cDeleted;
}

//
// Verify cache and database file.
//    f >1 for detailed printout.
S_INT64  CPageIo::PIoVerify(int f)
{
	U_INT64     iBlock;
	U_INT32     H = 0;

	m_cPage    = 0;
	m_cKeys    = 0;
	m_cDeleted = 0;
	m_nHeight  = 0;
	
#ifdef _DEBUG
//	CIoPrintTree();
#endif // _DEBUG

	CIoCheckCache(f);

	m_cDeleted = CheckDeletedChain(f);
	T_(m_cDeleted==BlockHeader.DeletedBlocks,"ERROR:Inconsistent deleted blocks number(counted=%lld,header=%lld).\n",m_cDeleted,BlockHeader.BlockByteSize);
	T_(f==0," Deleted pages      = %lld\n",m_cDeleted);

	if(IsNil(PageHeader.RootPage)) {
		T_(f==0," No data !\n");
		T_((BlockHeader.TotalBocks-1)==m_cDeleted,"ERROR: Bad sum of effective and deleted pages!\n");
		return m_cKeys;
	}

	// Scan all pages(blocks)
	for(iBlock=1;iBlock<BlockHeader.TotalBocks;++iBlock) {
		CheckPage(iBlock);
	}

	// Height ?
	iBlock = PageHeader.RootPage;
	while(iBlock!=0) {
		++m_nHeight;
		{
			CPageKeeper Page(this,iBlock);
			iBlock = Page.GetYoungest();
		}
	}

	T_((BlockHeader.TotalBocks-1)==m_cPage+m_cDeleted,"ERROR: Bad sum of effective and deleted pages!\n");
	T_(PageHeader.TotalRecords==m_cKeys,"ERROR: Inconsistent record count(counted=%lld,header=%lld)\n",m_cKeys,PageHeader.TotalRecords);
	T_(f==0," Counted records    = %lld\n",m_cKeys);
	T_(f==0," Counted pages      = %lld\n",m_cPage);
	T_(f==0," Deleted pages      = %lld\n",m_cDeleted);
	T_(f==0," B-Tree height      = %ld\n",m_nHeight);
	T_(f==0," Record occupancy   = %f%%\n",(((double)m_cKeys)/(m_cPage*GetMaxItems()))*100.0);
	return m_cKeys;
}

void CPageIo::CheckPage(U_INT64 iPage)
{
	U_INT64      iBlock;
	U_INT64		 iChild;
	CPageKeeper  Page(this,iPage);
	S_INT32      nItem;
	S_INT32      ix;

	iBlock = Page.GetPageNo();
	if(iBlock&FLAG64_DELETED) {
		// Deleted block
		++m_cDeleted;
		return;
	}

	T_(iBlock==iPage,"ERROR: Bad page number(%lld should be %lld)\n",iBlock,iPage);
	nItem = Page.GetItemsUsed();
	T_(nItem>0 && nItem<=GetMaxItems(),"ERROR: Page (%lld) has illegal number of items(%d)\n",iPage,nItem);

	m_cPage++;
	m_cKeys += nItem;

	U_INT64 iParent = Page.GetParent();
	if(iParent==0) {
		// Root page
		T_(iPage==PageHeader.RootPage,"ERROR: Page(%lld) marked as root,but not actual root(%lld)\n",iPage,PageHeader.RootPage);
	} else {
		T_(nItem>=GetMaxItems()/2,"ERROR: Page(%lld) has %ld items(underflow!)\n",iPage,nItem);
	}

	// Order check
	for(int i=0;i<nItem-1;++i) {
		T_((*m_pKeyFunc)(Page.GetItemKey(i),Page.GetItemKey(i+1),m_cKeyComp,pLdbHandle)<0,"ERROR: Page(%lld) key (%d) not ordered properly.\n",iPage,i);
	}

	// Check parent - child relation.
	iChild = Page.GetYoungest();
	if(iChild) {
		CPageKeeper Child(this,iChild);
		U_INT64  jChild = Child.GetPageNo();
		T_((jChild&FLAG64_DELETED)==0,"ERROR: Page(%lld)'s child %lld is deleted",iPage,iChild&(~FLAG64_DELETED));
		T_(jChild==iChild,"ERROR: Page(%lld)'s child( %lld)'s parent is not the same.",iPage,iChild);
		T_((*m_pKeyFunc)(Page.GetItemKey(0),Child.GetItemKey(0),m_cKeyComp,pLdbHandle)>0,"ERROR: Page(%lld) key (0) not ordered properly.\n",iPage);
		ix = Child.GetItemsUsed()-1;
		ASSERT(ix>0);
		T_((*m_pKeyFunc)(Page.GetItemKey(0),Child.GetItemKey(ix),m_cKeyComp,pLdbHandle)>0,"ERROR: Page(%lld) key (%d) not ordered properly.\n",iPage,ix);
	}
	for(int i=0;i<nItem;++i) {
		iChild = Page.GetItemNextPage(i);
		if(iChild) {
			CPageKeeper Child(this,iChild);
			U_INT64  jChild = Child.GetPageNo();
			T_((jChild&FLAG64_DELETED)==0,"ERROR: Page(%lld)'s child %lld is deleted.\n",iPage,iChild&(~FLAG64_DELETED));
			T_(jChild==iChild,"ERROR: Page(%lld)'s child( %lld)'s parent is not the same.\n",iPage,iChild);
			T_((*m_pKeyFunc)(Page.GetItemKey(i),Child.GetItemKey(0),m_cKeyComp,pLdbHandle)<0,"ERROR: Page(%lld) key (%d) not ordered properly.\n",iPage,i);
			if(i<nItem-1) {
				ix = Child.GetItemsUsed()-1;
				ASSERT(ix>0);
				T_((*m_pKeyFunc)(Page.GetItemKey(i+1),Child.GetItemKey(ix),m_cKeyComp,pLdbHandle)>0,"ERROR: Page(%lld) key (%d) not ordered properly.\n",iPage,i);
			}
		}
	}
}
