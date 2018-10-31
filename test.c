/* #include "stdafx.h" if you use pre-compiled header,uncomment this line. */

/*
   LightDB simple test program.
*/

#include "lightdb.h"
#include <stdlib.h>
#include <stdio.h>

LDB_HANDLE  h;

/*
   Change Count,(KEY,KeyType),KeyArraySize,(DATA,DataType) and DataArraySize according to your environment. 
*/
#define     Count         1000

#define     KeyType       T_SINT32
#define     KEY           S_INT32
#define     KeyArraySize  2
KEY         Key [KeyArraySize];
KEY         Key2[KeyArraySize];

#define     DataType      T_UINT64
#define     DATA          U_INT64
#define     DataArraySize 4
DATA        Data[DataArraySize];


KEY_COMP_FUNCTION *pKeyFunction = NULL;

#define ABS(a) ((a>=0)?(a):-(a))

#define Assert(f) AssertProc(f,__FILE__,__LINE__)
void AssertProc(int f,char *file,int line)
{
	if(f) return;
	printf("ERROR: Assertion failed: in file %s,at line %d\n",file,line);
	getchar();
}

/*
   Example of user specific key-compare function.
   If you want to use following Key-compare function,
   just set 
      pKeyFunction = MyKeyComp;
	at the beggining of the main().
*/
int MyKeyComp(void *k1,void *k2,int cb,LDB_HANDLE *ph)
{
	int i;
	for(i=0;i<KeyArraySize;++i)
	{
		if( ((KEY*)k1)[i]>((KEY*)k2)[i] ) return  1;
		if( ((KEY*)k1)[i]<((KEY*)k2)[i] ) return -1;
	}
	return 0;
}

/*
  Create database,the same file is truncated if there.
*/
void create_test()
{
	int      i,j,f;
	int      keysize = KeyArraySize;
	LDB_TYPE keytype = KeyType;

	printf("Create database\n"); fflush(stdout);
	if(pKeyFunction!=NULL) {
		keysize = KeyArraySize*sizeof(KEY);
		keytype = T_UNDEFINED;
	}

	Assert(0==LdbOpen(&h,"test.dbf",'T',keytype,keysize,pKeyFunction,DataType,DataArraySize,0,0));

	f = 1;
	for(i=1;i<=Count;++i) {
		f *= -1;
		Key [0] = (KEY)(i*f);
		printf(" %d\r",i); fflush(stdout);
		for(j=1;j<=Count;++j) {
			f *= -1;
			Key [1] = (KEY)(j*f);
			Data[0] = (DATA)j;
			Data[1] = (DATA)i;
			Assert(0==LdbAddRecord(&h,Key,Data));
			Assert(0==LdbGetData(&h,Key,Data));
			Assert(Data[0]==j && Data[1] == i);
			Data[0] = (DATA)i;
			Data[1] = (DATA)j;
			Assert(0==LdbChangeData(&h,Key,Data));
		}
	}
	LdbVerifyContents(&h,1);
	LdbClose(&h);
}

void read_test()
{
	int      i,j,f;
	int      keysize = KeyArraySize;
	LDB_TYPE keytype = KeyType;

	printf("Read database\n");fflush(stdout);

	if(pKeyFunction!=NULL) {
		keysize = KeyArraySize*sizeof(KEY);
		keytype = T_UNDEFINED;
	}

	Assert(0==LdbOpen(&h,"test.dbf",'R',keytype,keysize,pKeyFunction,DataType,DataArraySize,0,0));

	f = 1;
	for(i=1;i<=Count;++i) {
		f *= -1;
		printf(" %d\r",i);fflush(stdout);
		Key[0]=(KEY)(i*f);
		for(j=1;j<=Count;++j) {
			f *= -1;
			Key[1]=(KEY)(j*f);
			Assert(0==LdbGetData(&h,Key,Data));
			Assert(Data[0]==i && Data[1]==j);
		}
	}
	LdbClose(&h);
}

void seq_test()
{
	int      i,j,k,f=1;
	int      keysize = KeyArraySize;
	LDB_TYPE keytype = KeyType;

	printf("Forward sequential test\n");fflush(stdout);

	if(pKeyFunction!=NULL) {
		keysize = KeyArraySize*sizeof(KEY);
		keytype = T_UNDEFINED;
	}

	Assert(0==LdbOpen(&h,"test.dbf",'R',keytype,keysize,pKeyFunction,DataType,DataArraySize,0,0));

	f = 1;
	for(i=1;i<=Count;++i) {
		printf(" %d\r",i);fflush(stdout);
		for(j=1;j<=Count;++j) {
			Assert(0==LdbGetNextMinRecord(&h,Key,Data,f));
			if(!f) {
				Assert(0==LdbCompareKeys(&h,&f,Key,Key2));
				Assert(f>0);
			}
			f=0;
			for(k=0;k<KeyArraySize;++k) Key2[k] = Key[k];
			Assert(Data[0]==ABS(Key[0]) && Data[1]==ABS(Key[1]));
		}
	}

	f = 1;
	printf("Backward sequential test\n");fflush(stdout);
	for(i=1;i<=Count;++i) {
		printf(" %d\r",i);fflush(stdout);
		for(j=1;j<=Count;++j) {
			Assert(0==LdbGetPrevMaxRecord(&h,Key,Data,f));
			if(!f) {
				Assert(0==LdbCompareKeys(&h,&f,Key,Key2));
				Assert(f<0);
			}
			f=0;
			for(k=0;k<KeyArraySize;++k) Key2[k] = Key[k];
			Assert(Data[0]==ABS(Key[0]) && Data[1]==ABS(Key[1]));
		}
	}
	LdbClose(&h);
}


void delete_test()
{
	int      i,j,f;
	int      keysize = KeyArraySize;
	LDB_TYPE keytype = KeyType;

	printf("Delete test\n");fflush(stdout);

	if(pKeyFunction!=NULL) {
		keysize = KeyArraySize*sizeof(KEY);
		keytype = T_UNDEFINED;
	}

	Assert(0==LdbOpen(&h,"test.dbf",'W',keytype,keysize,pKeyFunction,DataType,DataArraySize,0,0));

	f = 1;
	for(i=1;i<=Count;++i) {
		f *= -1;
		Key [0] = (KEY)(i*f);
		printf(" %d\r",i); fflush(stdout);
		for(j=1;j<=Count;++j) {
			f *= -1;
			Key [1] = (KEY)(j*f);
			Assert(0==LdbDeleteRecord(&h,Key));
			Assert(0!=LdbGetData(&h,Key,Data));
		}
	}
	LdbVerifyContents(&h,1);

	printf("Recreate the database\n");fflush(stdout);
	f = 1;
	for(i=1;i<=Count;++i) {
		f *= -1;
		Key [0] = (KEY)(i*f);
		Data[0] = (DATA)i;
		printf(" %d\r",i); fflush(stdout);
		for(j=1;j<=Count;++j) {
			f *= -1;
			Key [1] = (KEY)(j*f);
			Data[1] = (DATA)j;
			Assert(0==LdbAddRecord(&h,Key,Data));
		}
	}
	LdbVerifyContents(&h,1);
	LdbClose(&h);
}

int main(int argc,char* argv[])
{
	create_test();
	read_test();
	seq_test();
	delete_test();
	return 0;
}
