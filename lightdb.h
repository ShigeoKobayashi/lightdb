/*
////////////////////////////////////////////////////////////////////////////////////////
///                                                                                  ///
///  Copyright(C) 2018 Shigeo Kobayashi (shigeo@tinyforest.jp). All Rights Reserved. ///
///                                                                                  /// 
////////////////////////////////////////////////////////////////////////////////////////
*/

/*
 LightDb is a B-tree based data base I/O system.
 The data base file consists of records, and a record consists of key and data.
 A LightDb file is divided into multiple blocks(=pages). Every blocks/pages are the same size.
 A Block is I/O unit. A block is called a page on the B-tree point of view.
 Block and page is the same as disk I/O point of view.
 All blocks are numbered from 0(file top) to n(file end) ( n=(file size)/(block size) ).
 Byte size of page/block is properly decided by the program according to the key and record length 
 and some other informations specified.
 The first block(block 0) contains data base informations(key length,key type,data type,data length,...).
 
 From page 1,keys and corresponding data are stored.
  key    + data           => record (all records in the data base are properly ordered according to the value of key)
  record + [right page #] => item 
    All keys in the [right page #] specified are greater than the key in the record but
    less than the key int next record in the next item in the page.

 One page is structured as follows(all page number consist of 8 bytes):
    Page # of the page itself.
    Page # of parent page. All keys in the parent page are greater than this page(root page's parent is 0 which means no page).
    Page # of left page. All keys in the left page are less than this page. This is 0 when the page has no left page.
    Number of effective items(=ni) in this page(8 byte)
    item 1
    item 2
    ....
    item ni
    Items from ni+1 to the end of the page are empty items.
*/

#ifndef __LIGHT_DB_H__
#define __LIGHT_DB_H__

#ifdef _WIN32
#ifndef WINDOWS
    #define WINDOWS
    #ifdef _WIN64
        /* 64bit Windows */
    #define BIT64
    #else
        /* 32bit Windows */
    #define BIT32
    #endif
#endif
#else
#ifndef LINUX
    #define LINUX
    #ifdef __x86_64__
            /* 64bit Linux */
        #define BIT64
       #else
           /* 32bit Linux */
        #define BIT32
    #endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef WINDOWS /**** WINDOWS ****/
/* WINDOWS specific part (Same define's must be defined for other platforms.) */
 #define LDB_EXPORT(t)  __declspec(dllexport) t __stdcall
/* Note:
__stdcall __cdecl and C#
------------------------
  WIN32 API's use __stdcall(which can't be used for vararg functions) with .def file representation.
  __stdcall without .def file changes function name exported in .lib file.
  __cdecl (c compiler default) never changes function name exported but consumes more memories than __stdcall.
  C# [Dllexport] atrribute uses __stdcall in default.
  To call __cdecl functions from C#, use CallingConvention.Cdecl like "[DllImport("MyDLL.dll", CallingConvention = CallingConvention.Cdecl)]".
*/
#endif /**** WINDOWS ****/

#ifdef LINUX /******** LINUX ********/
/* gcc/g++ specific part ==> compiled with '-fPIC -fvisibility=hidden' option. */
/*
 -fvisibility=hidden option hides everything except explicitly declared as exported API just like
 as Windows' dll.
*/
#define LDB_EXPORT(t) __attribute__((visibility ("default")))  t
#endif /**** LINUX ****/

/* 64 bit integer mainly used for BLOCK number or handles used in LightDB. */
#define S_INT16           short
#define U_INT16  unsigned short
#define S_INT32           int
#define U_INT32  unsigned int
#define S_INT64           long long int
#define U_INT64  unsigned long long int

typedef struct _LDB_HANDLE
{
    U_INT64  LdbID;     /* After successful open,the contentd will be set to "LightDB" */
    U_INT64  LdbObject; /* After successful open,C++ LightDB main class pointer is set. */
    int      LdbStatus; /* LDB API return value. */
    int      c_errno;   /* C/C++ API error code if provided. */
} LDB_HANDLE;

/*
     Key & data types,used by the internal function for key-compare. 
*/
typedef enum 
{
        T_UBYTE8   =  1, /*  1  ... Unsigned char   (8-bit)          */
        T_SBYTE8   =  2, /*  2  ... Signed char     (8-bit)          */
        T_USHORT16 =  3, /*  3  ... Unsigned short  (16-bit)         */
        T_SSHORT16 =  4, /*  4  ... Signed short    (16-bit)         */
        T_UINT32   =  5, /*  5  ... Unsigned int    (32-bit)         */
        T_SINT32   =  6, /*  6  ... Signed int      (32-bit)         */
        T_UINT64   =  7, /*  7  ... Unsigned int    (64-bit,__int64) */
        T_SINT64   =  8, /*  8  ... Signed int      (64-bit,__int64) */
        T_FLOAT32  =  9, /*  9  ... Float           (32-bit)         */
        T_DOUBLE64 = 10, /* 10  ... Double          (64-bit)         */
        T_UNDEFINED= 11, /* 11  ... Undefined.
                                    The user must provide key compare function
                                    if this is specified for key type,
                                    the size of T_UNDEFINED must be specified in bytes.
                         */
} LDB_TYPE;

/* Key compare function called in the B-tree search process. */
typedef int (KEY_COMP_FUNCTION)(void *pk1,void *pk2,int cb,LDB_HANDLE *ph);
/*
   LDB_HANDLE *ph is the one specified at LdbOpen(). If you need some more info in 
   the user defined key-compare function you can wrap LDB_HANDLE and use it like
      typedef struct _MY_HANDLE {
         LDB_HANDLE ldb_handle;
         ..........
      } MY_HANDLE;
      .....
      int MyKeyCompFunc(void *p1,void *p2,int ck,LDB_HANDLE ph)
      {
        MY_HANDLE *mh = (MY_HANDLE*)ph;
        .......
      }

      MY_HANDLE   mh;
      LDB_HANDLE *ph = (LDB_HANDLE *)&mh;
      int e = LdbOpen(ph,"db-file path",'W',
                       T_UNDEFINED,keyByteSize,(KEY_COMP_FUNCTION *)MyKeyCompFunc,
                       dataType,dataArraySize,0,0);
*/

typedef struct _LDB_INFO {
    U_INT64       Version;          /* Version number */
    U_INT64       TotalPages;       /* Total pages in the data base. */
    U_INT64       RootPage;         /* Root page's block number */
    U_INT64       DeletedPage;      /* Top of the deleted page chain */
    unsigned int  PageByteSize;     /* Page size in bytes */
    unsigned int  KeyByteSize;      /* Key size in bytes */
    unsigned int  DataByteSize;     /* Data size in bytes */
    unsigned int  UserAreaByteSize; /* User area size in bytes */
    unsigned int  MaxItems;         /* Max numer of items in a page. */
    unsigned int  CachedPages;      /* Number of pages(blocks) cached */
    LDB_TYPE      KeyType;          /* Key type. */
    LDB_TYPE      DataType;         /* Data type. */
} LDB_INFO;
/* 
 To obtain above info,use following LdbGetInfo() function.
 pInfo can be NULL,then infomations are printed to console.
*/
LDB_EXPORT(int) LdbGetInfo(LDB_HANDLE *ph,LDB_INFO *pInfo);

/*
 *   ***** APIs *****
 */

/*
 LdbOpen() opens data base file and set it's handle to user specified area.
  ph    : Pointer to the handle used for the subsequent I/O for the data base.
  szPath: Path name for the data base file opened.
  chMode: Open mode
    'R' ... opens existing file for read only mode.
    'W' ... opens exisitng file for read and write mode.
    'N' ... opens/creates non-existing file for read and write mode.
	        If the file is there,then the open fails. 
    'T' ... opens non-existing or existing file for read and write mode.
            existing file is truncated.
        r,w,n,t ... Lower case character is accepted.
  keyType: type of key which is used for key compare function.
  keyArraySize: Key array size (not byte size). 
  pKeyCompFunc: Address of the key compare function provided by the user.
                When keyType==T_UNDEFINED, then the pKeyCompFunc must be provided,otherwise this should be NULL(not used).
                When keyType==T_UNDEFINED, then keySize must be in bytes.
  dataType: type of data. Not important for LightDb(just for user comvenience).
  dataArraySize: Data array size(not byte size except when dataType==T_UNDEFINED).
  nItem: Number of items in a page. nItem should be even and >=8 otherwise it is computed and used internally.
         So 0 can be specified if you do not mind it.
  nCache: Number of Pages(=Blocks) cached in memory. 
          If zero is specified then the value is computed and used internally. 
Note: keyType,keyArraySize,dataType,dataArraySize,and nItem are ignored when existing file is opened(values in the file are used instead).
*/
LDB_EXPORT(int) LdbOpen(LDB_HANDLE *ph,char *szPath,char chMode,
                        LDB_TYPE keyType,int keyArraySize,KEY_COMP_FUNCTION *pKeyCompFunc,
                        LDB_TYPE dataType,int dataArraySize,
                        int nItem,int nCache);

/* close LightDb file  */
LDB_EXPORT(int) LdbClose(LDB_HANDLE *ph);

/* Flush every modified blocks back to the LightDb file opened */
LDB_EXPORT(int)    LdbFlush(LDB_HANDLE *ph);

/*
 Check the validity of the LightDb file.
   detail ... 0: No output except for Warning or Error messages.
              1: Header info.
             >1: More info. 
*/
LDB_EXPORT(int) LdbVerifyContents(LDB_HANDLE *ph,int detail);

/*
 Add new record to the data base.
 If the same key is in the data base,then this function fails(use LdbUpdateRecord() instead).
 Array size of pKey & pData must be greater than or equal to the size specified at Open().
 (This array size assumption applies to other I/O functions except stated otherwise.)
*/
LDB_EXPORT(int) LdbAddRecord(LDB_HANDLE *ph,void *pKey,void *pData);

/*
 Find the record having the key(pKey) and copy it's data to pData.
*/
LDB_EXPORT(int) LdbGetData(LDB_HANDLE *ph,void *pKey,void *pData);

/*
 Find the record having the key(pKey) and change the data to pData.
*/
LDB_EXPORT(int) LdbChangeData(LDB_HANDLE *ph,void *pKey,void *pData);

/*
 Compares 2 keys and returns result to *pf.
*/
LDB_EXPORT(int) LdbCompareKeys(LDB_HANDLE *ph,int *pf,void *pKey1,void *pKey2);

/*
 Find the record having the key(pKey) and change the data to pData.
 If the record is not found,then the record specified is added to the data base.
*/
LDB_EXPORT(int) LdbWriteRecord(LDB_HANDLE *ph,void *pKey,void *pData);

/*
 Delete the record having the key(pKey) from the data base.
*/
LDB_EXPORT(int) LdbDeleteRecord(LDB_HANDLE *ph,void *pKey);

/* Retrieve minimum key record */
LDB_EXPORT(int) LdbGetMinRecord(LDB_HANDLE *ph,void *pKey,void *pData);

/* Retrieve maximum key record */
LDB_EXPORT(int) LdbGetMaxRecord(LDB_HANDLE *ph,void *pKey,void *pData);

/* Retrieve next record from current record position */
LDB_EXPORT(int)    LdbGetNextRecord(LDB_HANDLE *ph,void *pKey,void *pData);

/* Retrieve minimum key record if fGetMin != 0,otherwise the same as LdbGetNextRecord() */ 
LDB_EXPORT(int)    LdbGetNextMinRecord(LDB_HANDLE *ph, void *pKey, void *pData,int fGetMin);

/* Retrieve previous record from current record position */
LDB_EXPORT(int)    LdbGetPrevRecord(LDB_HANDLE *ph, void * pKey, void *pData);

/* Retrieve maximum key record if fGetMax != 0,otherwise the same as LdbGetPrevRecord() */ 
LDB_EXPORT(int)    LdbGetPrevMaxRecord(LDB_HANDLE *ph,void *pKey,void *pData,int fGetMax);

/* Get current record position record */
LDB_EXPORT(int)    LdbGetCurRecord(LDB_HANDLE *ph,void *pKey,void *pData);

/* Change current data */
LDB_EXPORT(int)    LdbChangeCurData(LDB_HANDLE *ph,void *pData);

/*
 User control area I/O function.
   chIo ... 'W' or 'w' : write cb bytes of *pData to user area.
            'R' or 'r' : read cb byte to *pData from user area.
    The max value of cb available can be obtained by calling LdbGetInfo().
*/
LDB_EXPORT(int) LdbUserAreaIO(LDB_HANDLE *ph,void *pData,int cb,char chIo);

LDB_EXPORT(const char *)    LdbGetMsg(int e); /* e must be the code listed bellow */
/* ERROR CODES: 
   Negative values mean fatal errors.
   Positive values are not always fatal.
   Most LightDB API returns value 0 on success(normal case), or returns one of value
   listed bellow. The returned value is also stored in LDB_HANDLE member(LdbStatus).
   c_error member of LDB_HANDLE struct may have more information(through strerror(c_error))
  */
#define  ERROR_MEMORY_ALLOC          -10 /* Error: Memory allocation error. */
#define  ERROR_FILE_DUPLICATED_OPEN  -20 /* Error: Already opened(Close() before Open()). */
#define  ERROR_FILE_OPEN             -30 /* Error: Failed to open specified file. */
#define  ERROR_FILE_OPEN_MODE        -40 /* Error: Invalid open mode. */
#define  ERROR_FILE_BAD_BLOCKSIZE    -50 /* Error: Invalid block size. */
#define  ERROR_FILE_SEEK             -60 /* Error: File seek failed(System I/O error). */
#define  ERROR_FILE_READ             -70 /* Error: File read failed(System I/O error). */
#define  ERROR_FILE_WRITE            -80 /* Error: File write failed(System I/O error). */
#define  ERROR_FILE_ACCESS_DENIED    -90 /* Error: I/O error or write on read only file. */
#define  ERROR_CACHE_TOO_SMALL      -100 /* Error: Cache size specified too small. */
#define  ERROR_BUFFER_OVERRUN       -110 /* Error: Buffer overrun. */
#define  ERROR_KEY_SIZE             -120 /* Error: Bad key size. */
#define  ERROR_KEY_TYPE             -130 /* Error: Bad key type. */
#define  ERROR_DATA_SIZE            -140 /* Error: Bad data size. */
#define  ERROR_DATA_TYPE            -150 /* Error: Bad data type. */
#define  ERROR_ITEMS_NOT_EVEN       -160 /* Error: Number of items in a page must be even. */
#define  ERROR_BAD_PAGE_NO          -170 /* Error: Page number must not be zero. */
#define  ERROR_BAD_ARGUMENT         -180 /* Error: Bad or unacceptable argument specified. */

#define  ERROR_BROKEN_FILE          -200 /* Error: File is broken or non lightdb file. */
#define  ERROR_BAD_PTR              -210 /* Error: Bad pointer(including NULL). */
#define  ERROR_SYSTEM               -220 /* Error: System mulfunction. */

/* Warnings */
#define  SAME_KEY_EXISTS            10 /* Warning: Same key alredy in the data base. */
#define  KEY_NOT_FOUND              20 /* Warning: Specified key not in the data base. */
#define  NO_MORE_KEY                30 /* Warning: No more keys specified in the data base. */
#define  NO_KEYS                    40 /* Warning: Specified keys not in the data base. */
#define  NO_KEY_SELECTED            50 /* Warning: No key selected. */

/************************* Low level I/O functions ******************/
/*
  Care must be paid for using following low level functions.
  After successful addition or deletion of any record may change the position of other records in the pages they involved.
 */

#define  ERROR_BAD_BLOCK_NUMBER       -1 /* Error: bad page number specified. */
#define  ERROR_BAD_ITEM_NUMBER        -2 /* Error: bad record(item) number specified. */
#define  ERROR_DELETED_BLOCK          -3 /* Error: deleted block(page) number specified. */

LDB_EXPORT(int) LdbGetRootPage(LDB_HANDLE *ph,U_INT64 *piPage);
/*
  *piBlock==0 means that the current record pointer is undefined. 
*/
LDB_EXPORT(int) LdbGetCurrentPTR(LDB_HANDLE *ph,U_INT64 *piPage,int *piRecord);
LDB_EXPORT(int) LdbGetRecord(LDB_HANDLE *ph,U_INT64 iPage,int iRecord,void *pKey,void *pData);
LDB_EXPORT(int) LdbGetRecordCount(LDB_HANDLE *ph,U_INT64 iPage,int *pc);
LDB_EXPORT(int) LdbGetChildPage(LDB_HANDLE *ph,U_INT64 iPage,int iRecord,U_INT64 *pChildPage);
LDB_EXPORT(int) LdbSetData(LDB_HANDLE *ph,U_INT64 iPage,int iRecord,void *pData);

/************ end of header ***************/
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __LIGHT_DB_H__ */
