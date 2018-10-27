///////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2016 Shigeo Kobayashi. All Rights Reserved.
// 
///////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "pageio.h"

/* LightDB identifier. */
const char     LdbID[] = "LightDB";            /* is written at the top of the data base file as ID */
const U_INT64  LdbID64 = 0x004244746867694cLL; /* == "LightDB": Little endian */

// 
class CPageIo;

CPageIo *GetLdbPtr(LDB_HANDLE *p)
{
	CHECK(p!=NULL && p->LdbID==LdbID64 && p->LdbObject!=0,ERROR_BAD_PTR);
	p->c_errno   = 0;
	p->LdbStatus = 0;
	return (CPageIo*)p->LdbObject;
}

LDB_EXPORT(int)  
	LdbOpen(LDB_HANDLE *ph,char *szPath,char chMode,LDB_TYPE keyType,int keySize,KEY_COMP_FUNCTION *pKeyCompFunc,
			LDB_TYPE dataType,int dataSize,
			int nItem,int nCache)
{
	int e = 0;
	CPageIo *p = NULL;
	if(ph==NULL) return ERROR_BAD_PTR;
	try {
		errno = 0;
		memset(ph,0,sizeof(LDB_HANDLE));
		strcpy((char*)ph,LdbID);
		p = new CPageIo();
		CHECK(p!=NULL,ERROR_MEMORY_ALLOC);
		p->PIoOpen (szPath,chMode,keyType,keySize,pKeyCompFunc,dataType,dataSize,nItem,nCache);
		ph->LdbObject = (U_INT64 )p;
		p->pLdbHandle = ph;
	} catch(CException *ex) {
		ASSERT(FALSE);
		if(p) {
	        e = ex->m_erCode;
			ph->LdbStatus = e;
			ph->c_errno = ex->m_errno;
			delete p;
		}
		delete ex;
	}
	return e;
}

#define ENTER(exp) \
	int e = 0;\
	CPageIo *pIo=NULL;\
	if(ph==NULL) return ERROR_BAD_PTR;\
	try {\
	   pIo = GetLdbPtr(ph);\
       exp;\
	   ph->LdbStatus=e;\
    }

#define LEAVE(exp) \
	catch(CException *ex) {\
      e=ex->m_erCode;\
	  ph->LdbStatus=e;\
	  ph->c_errno=ex->m_errno;\
      exp;\
	  delete ex;\
    };\
	return e;

LDB_EXPORT(int)
	LdbClose(LDB_HANDLE *ph)
{
	ENTER({pIo->PIoClose();delete pIo;memset(ph,0,sizeof(*ph));})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbFlush(LDB_HANDLE *ph)
{
	ENTER({pIo->CIoFlush();})
	LEAVE({ })
}


LDB_EXPORT(int)
	LdbVerifyContents(LDB_HANDLE *ph,int detail)
{
	ENTER({pIo->PIoScanDb(detail);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbAddRecord(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoAddRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetData(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoGetData(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbChangeData(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoSetData(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbWriteRecord(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoWriteRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbDeleteRecord(LDB_HANDLE *ph,P_VOID pKey)
{
	ENTER({e=pIo->PIoDeleteRecord(pKey);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetMinRecord(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoGetMinimumRecord(pKey,pData);})
	LEAVE({ })
}


LDB_EXPORT(int)
	LdbGetMaxRecord(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoGetMaximumRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetNextRecord(LDB_HANDLE *ph,void *pKey,void *pData)
{
	ENTER({e=pIo->PIoGetNextRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetNextMinRecord(LDB_HANDLE *ph,void *pKey,void *pData,int fGetMin)
{
	ENTER({if(fGetMin) e=pIo->PIoGetMinimumRecord(pKey,pData);else e=pIo->PIoGetNextRecord(pKey,pData);})
	LEAVE({ })
}


LDB_EXPORT(int)
	LdbGetPrevRecord(LDB_HANDLE *ph,void *pKey,void *pData)
{
	ENTER({e=pIo->PIoGetPrevRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetPrevMaxRecord(LDB_HANDLE *ph,void *pKey,void *pData,int fGetMax)
{
	ENTER({if(fGetMax) e=pIo->PIoGetMaximumRecord(pKey,pData);else e=pIo->PIoGetPrevRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetCurRecord(LDB_HANDLE *ph,P_VOID pKey,P_VOID pData)
{
	ENTER({e=pIo->PIoGetCurRecord(pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)	
	LdbChangeCurData(LDB_HANDLE *ph,void *pData)
{
	ENTER({e=pIo->PIoSetCurData(pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbUserAreaIO(LDB_HANDLE *ph,void *pbytes,int cb,char chIo)
{
	ENTER({e=pIo->PIoUserHeaderIo(pbytes,cb,chIo);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetInfo(LDB_HANDLE *ph,LDB_INFO *pInfo)
{
	ENTER({e=pIo->PIoGetInfo(pInfo);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetRootPage(LDB_HANDLE *ph,U_INT64 *pRoot)
{
	ENTER({e=pIo->PIoGetRootPage(pRoot);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetCurrentPTR(LDB_HANDLE *ph,U_INT64 *piBlock,int *piItem)
{
	ENTER({e=pIo->PIoGetCurrentPTR(piBlock,piItem);})
	LEAVE({ })
};

LDB_EXPORT(int)
	LdbGetRecord(LDB_HANDLE *ph,U_INT64 iBlock,int iItem,void *pKey,void *pData)
{
	ENTER({e=pIo->PIoGetRecord(iBlock,iItem,pKey,pData);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdgGetRecordCount(LDB_HANDLE *ph,U_INT64 iBlock,int *pc)
{
	ENTER({*pc=pIo->PIoGetItemNumber(iBlock);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbGetChildPage(LDB_HANDLE *ph,U_INT64 iBlock,int iItem,U_INT64 *pNext)
{
	ENTER({e=pIo->PIoGetNextPage(iBlock,iItem,pNext);})
	LEAVE({ })
}

LDB_EXPORT(int)
	LdbSetData(LDB_HANDLE *ph,U_INT64 iBlock,int iItem,void *pData)
{
	ENTER({e=pIo->PIoSetData(iBlock,iItem,pData);})
	LEAVE({ })
}

LDB_EXPORT(int) 
	LdbCompareKeys(LDB_HANDLE *ph,int *pf,void *pKey1,void *pKey2)
{
	ENTER({e=pIo->PIoKeyCompare(pf,pKey1,pKey2);})
	LEAVE({ })
}


LDB_EXPORT(const char *)
	LdbGetMsg(int e)
{
	switch(e)
	{	
	/* ERROR CODES */
	case ERROR_MEMORY_ALLOC:         return "Error: Memory allocation error.";break;
	case ERROR_FILE_DUPLICATED_OPEN: return "Error: Already opened(Close() before Open()).";break;
	case ERROR_FILE_OPEN:    		 return "Error: Failed to open specified file.";break;
	case ERROR_FILE_OPEN_MODE:		 return "Error: Invalid open mode.";break;
	case ERROR_FILE_BAD_BLOCKSIZE:   return "Error: Invalid block size.";break;
	case ERROR_FILE_SEEK:            return "Error: File seek failed(System I/O error).";break;
	case ERROR_FILE_READ:            return "Error: File read failed(System I/O error).";break;
	case ERROR_FILE_WRITE:           return "Error: File write failed(System I/O error).";break;
	case ERROR_FILE_ACCESS_DENIED:   return "Error: I/O error or write on read only file";break;
	case ERROR_CACHE_TOO_SMALL:      return "Error: Cache size specified too small.";break;
	case ERROR_BUFFER_OVERRUN:       return "Error: Buffer overrun.";break;

	case ERROR_KEY_SIZE:             return "Error: Bad key size.";break;
	case ERROR_KEY_TYPE:             return "Error: Bad key type.";break;
	case ERROR_DATA_SIZE:            return "Error: Bad data size.";break;
	case ERROR_DATA_TYPE:            return "Error: Bad data type.";break;
	case ERROR_ITEMS_NOT_EVEN:       return "Error: Number of items in a page must be even.";break;
	case ERROR_BAD_PAGE_NO:          return "Error: Page number must not be zero.";break;
	case ERROR_BAD_ARGUMENT:         return "Error: Bad or unacceptable argument specified.";break;

	/* System error */
	case ERROR_BROKEN_FILE:          return "Error: File is broken or non lightdb file.";break;
	case ERROR_BAD_PTR:              return "Error: Bad pointer(including NULL).";break;
	case ERROR_SYSTEM:               return "Error: System mulfunction.";break;

	/* Warnings(Key info). */
	case SAME_KEY_EXISTS:            return "Warning: Same key alredy in the data base.";break;
	case KEY_NOT_FOUND:              return "Warning: Specified key not in the data base.";break;
	case NO_MORE_KEY:                return "Warning: No more keys specified in the data base.";break;
	case NO_KEYS:                    return "Warning: Specified keys not in the data base.";break;
	case NO_KEY_SELECTED:            return "Warning: No key selected.";break;

	/* Low level function errors */
	case ERROR_BAD_BLOCK_NUMBER:     return "Error: bad block number specified.";break;
	case ERROR_BAD_ITEM_NUMBER:      return "Error: bad item number specified."; break;
	case ERROR_DELETED_BLOCK:        return "Error: deleted block number specified.";break;

	default:
		ASSERT(FALSE);
		return "Undefined message code specified to LdbGetMsg()";
		break;
	}
	return "???";
}
