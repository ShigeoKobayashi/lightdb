//
//****************************************************************************
//*                                                                          *
//* Å@Copyright (C) Shigeo Kobayashi, 2016.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
//
#ifndef ___BT_BASE_H___
#define ___BT_BASE_H___

// #define MAX_PATH_SIZE _MAX_PATH + 1

// 
// BASIC TYPEs
#define PU_INT32   U_INT32   *
#define PS_INT32   S_INT32   *
#define U_SHORT    unsigned short
#define PU_SHORT   U_SHORT *
#define S_SHORT    short
#define PS_SHORT   S_SHORT *
#define CHAR       char
#define P_CHAR     CHAR *
#define U_CHAR     unsigned char
#define PU_CHAR    U_CHAR *
#define P_VOID     void *

#define Nil      ((U_INT64 )0)
#define IsNil(a) (((U_INT64 )a)==Nil)

//
// The first part of the block 0 is the BLOCK_HEADER followed by the PAGE_HEADER.  
//
typedef struct _BLOCK_HEADER {
	U_INT64  ID;
	U_INT64  Version;
	U_INT64  TotalBocks;
	U_INT64  TopDeleted;
	U_INT64  BlockByteSize;
	U_INT64  DeletedBlocks;
} BLOCK_HEADER;

typedef struct _PAGE_HEADER {
	U_INT64   RootPage;     // Root page block
	U_INT64   TotalRecords; // Total number of records in the data base.
	S_INT32   KeyType;      // Key type.
	U_INT32   KeyByteSize;  // Key length in bytes.
	S_INT32   DataType;     // Data type.
	U_INT32   DataByteSize; // Data length in bytes.
	S_INT32   MaxItems;     // Max numer of items in a page.
	U_INT32   Dummy32;      // dummy for future use
} PAGE_HEADER;

//
// LightDB header structure.
typedef struct _LDB_HEADER {
	BLOCK_HEADER block_header;
	PAGE_HEADER  page_header;
} LDB_HEADER;

#define SZ_ALLOC_COPY(st,so) {int len=strlen(so); st=(char*)MemAlloc(sizeof(char)*(len+2));if(st) strcpy_s(st,len+1,so);}

//
// Utility routines in dllmain.cpp
//
#ifdef _DEBUG
#define ASSERT(f)    DbgAssert(f,__FILE__,__LINE__)
void    DbgAssert(int  f,char *file,int line);
#define TRACE(s) printf(s)
#else 
#define ASSERT(f)
#define TRACE(s)
#endif // _DEBUG

class CBase 
{
public:
	CBase()
	{
		m_erCode = 0;
		m_errno  = 0;
	};
	
	void T_(int f,char *psz,...);
	void *BseMemAlloc(size_t cb);
	void *BseMemReAlloc(void *p,size_t n);
	void  BseMemFree(void **p);
	void  BseMemCheck(); // Confirm if every is memory freed.
	char *BseStrAllocCopy(char *sin);
	void  BseStrLimitCopy(char *so,int maxo,const char *si);

private:
	int   m_erCode;
	int   m_errno;
};

// Error handler
#define ER_THROW(e) throw new CException(e,errno,__FILE__,__LINE__)

// CHECK(f,e) macro,if f==TURE,then nothing is done or exception with error code e, __FILE__ and __LINE__ is thrown.  
#define CHECK(f,e) if(!(f)) ER_THROW(e);


// strerror() & errno
// #define ER_THROW_ERRNO(e) {char sz[512];strerror_s(sz,511,errno);throw new CBtreeException(e,sz,__FILE__,__LINE__);}


// Error handler
class CException 
{
public:
	CException(int e,int esys,char *szFile,int l)
	{
		m_erCode = e;
		m_errno  = esys;
#ifdef _DEBUG
   		printf("ERROR Exception: code= %d(%s)  at %d[%s]\n",e,LdbGetMsg(e),l,szFile);
        if(m_errno!=0) printf("                 Sys = %d(%s)\n",esys,strerror(esys)); 
#endif
//		m_szFile[0]        = 0;
//		StrLimitCopy(m_szFile,MAX_PATH_SIZE,szFile);
//		m_iLine = l;
	};

   ~CException()
	{
	};

public:
	int   m_erCode;
	int   m_errno;
//	char  m_szFile[MAX_PATH_SIZE];
//	int   m_iLine;
};
#endif //___BT_BASE_H___
