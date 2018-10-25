// #include "stdafx.h" if you use pre-compiled header,uncomment this line.
#include "lightdb.h"
#include <stdlib.h>
#include <stdio.h>

LDB_HANDLE  h;

#define     Count    1000

#define     KeyType  T_SINT32
#define     KEY      S_INT32
#define     KeySize  2
KEY         Key [KeySize];

#define     DataType T_UINT64
#define     DATA     U_INT64
#define     DataSize 4
DATA        Data[DataSize];

#define Assert(f) AssertProc(f,__FILE__,__LINE__)

void AssertProc(int f,char *file,int line)
{
	if(f) return;
	printf("ERROR: Assertion failed: in file %s,at line %d\n",file,line);
}


int MyKeyComp(int *k1,int *k2,int cb,LDB_HANDLE *ph)
{
	int i;
	for(i=0;i<KeySize;++i)
	{
		if( ((KEY*)k1)[i]>((KEY*)k2)[i] ) return  1;
		if( ((KEY*)k1)[i]<((KEY*)k2)[i] ) return -1;
	}
	return 0;
}

void make()
{
	int i,j;
	printf("Write data\n");
	Assert(0==LdbOpen(&h,"test.dbf",'T',KeyType,KeySize,NULL,DataType,DataSize,0,0));

	for(i=1;i<=Count;++i) {
		Key[0]  = i;
		Data[0] = i;
		printf(" %d\r",i);
		for(j=1;j<=Count;++j) {
			Key[1]  = j;
			Data[1] = j;
			Assert(0==LdbAddRecord(&h,Key,Data));
			Assert(0==LdbGetData(&h,Key,Data));
			Assert(Key[0]==i && Data[0]==i && Key[1]==j && Data[1]==j);
		}
	}
	LdbVerifyContents(&h,1);
	LdbClose(&h);
}

void seq_test()
{
	int i,j;
	int f=1;
	printf("Sequential test\n");
	Assert(0==LdbOpen(&h,"test.dbf",'R',KeyType,KeySize,NULL,DataType,DataSize,0,0));
	for(i=1;i<=Count;++i) {
		for(j=1;j<=Count;++j) {
			Assert(0==LdbGetNextMinRecord(&h,Key,Data,f)); f=0;
			Assert(Key[0]==i && Data[0]==i && Key[1]==j && Data[1]==j);
		}
	}
	LdbClose(&h);
}

void rev_test()
{
	int i,j;
	int f=1;
	printf("Reverse sequential test\n");
	Assert(0==LdbOpen(&h,"test.dbf",'R',KeyType,KeySize,NULL,DataType,DataSize,0,0));

	for(i=Count;i>=1;--i) {
		printf(" %d\r",i);
		for(j=Count;j>=1;--j) {
			Assert(0==LdbGetPrevMaxRecord(&h,Key,Data,f)); f=0;
			Assert(Key[0]==i && Data[0]==i && Key[1]==j && Data[1]==j);
		}
	}
	LdbClose(&h);
}

void read_test()
{
	int i,j;
	printf("Read test\n");
	Assert(0==LdbOpen(&h,"test.dbf",'R',KeyType,KeySize,NULL,DataType,DataSize,0,0));

	for(i=1;i<=Count;++i) {
		printf(" %d\r",i);
		Key[0]=i;
		for(j=1;j<=Count;++j) {
			Key[1]=j;
			Assert(0==LdbGetData(&h,Key,Data));
			Assert(Key[0]==i && Data[0]==i && Key[1]==j && Data[1]==j);
		}
	}
	LdbClose(&h);
}

void seq_test2()
{
	int i,j;
	int f=1;
	printf("Sequential test\n");

	Assert(0==LdbOpen(&h,"test.dbf",'W',KeyType,KeySize,NULL,DataType,DataSize,0,0));
	for(i=1;i<=Count;++i) {
		printf(" %d\r",i);
		for(j=1;j<=Count;++j) {
			Assert(0==LdbGetNextMinRecord(&h,Key,Data,f)); f=0;
			Assert(Key[0]==i && Data[0]==i && Key[1]==j && Data[1]==j);
			Data[0] += 1;			
			Data[1] += 1;			
			Assert(0==LdbChangeCurData(&h,Data));
		}
	}
	LdbClose(&h);

	printf(" Reset data \n");fflush(stdout);
	Assert(0==LdbOpen(&h,"test.dbf",'W',KeyType,KeySize,NULL,DataType,DataSize,0,0));
	f = 1;
	for(i=1;i<=Count;++i) {
		printf(" %d\r",i);
		for(j=1;j<=Count;++j) {
			Assert(0==LdbGetNextMinRecord(&h,Key,Data,f)); f=0;
			Assert(Key[0]==i && Data[0]==i+1 && Key[1]==j && Data[1]==j+1);
			Data[0] = i;			
			Data[1] = j;			
			Assert(0==LdbChangeCurData(&h,Data));
		}
	}
	LdbClose(&h);
}

void delete_test()
{
	int i,j,k;
	printf("Create test\n");
	Assert(0==LdbOpen(&h,"test.dbf",'T',
				T_UNDEFINED,sizeof(KEY)*KeySize,(KEY_COMP_FUNCTION*)MyKeyComp,
				DataType,DataSize,
				0,0) /* Number of items and caches are decided by LightDB  */
		  );

	for(i=1;i<=Count;++i) {
		for(k=0;k<KeySize;++k)  Key [k]  = i+k;
		for(k=0;k<DataSize;++k) Data[k]  = i+k;
		printf(" %d\r",i);fflush(stdout);
		for(j=1;j<=Count;++j) {
			Key[0] += j;
			Assert(0==LdbAddRecord(&h,Key,Data));
		}
	}
	LdbVerifyContents(&h,1);
	LdbClose(&h);

	printf("Read test\n");
	/* Some argument can be ignored for opening old file */
	Assert(0==LdbOpen(&h,"test.dbf",'R',
				T_UNDEFINED,sizeof(KEY)*KeySize,(KEY_COMP_FUNCTION*)MyKeyComp,
				DataType,DataSize,
				0,0) /* Number of items and caches are decided by LightDB  */
		  );
	for(i=1;i<=Count;++i) {
		for(k=0;k<KeySize;++k)  Key [k]  = i+k;
		printf(" %d\r",i);fflush(stdout);
		for(j=1;j<=Count;++j) {
			Key[0] += j;
			Assert(0==LdbGetData(&h,Key,Data));
			for(k=0;k<DataSize;++k) Assert(Data[k]==i+k);
		}
	}
	LdbClose(&h);

	printf("Delete test\n");
	Assert(0==LdbOpen(&h,"test.dbf",'W',
				T_UNDEFINED,sizeof(KEY)*KeySize,(KEY_COMP_FUNCTION*)MyKeyComp,
				DataType,DataSize,
				0,0) /* Number of items and caches are decided by LightDB  */
		  );
	for(i=1;i<=Count;++i) {
		for(k=0;k<KeySize;++k)  Key [k]  = i+k;
		printf(" %d\r",i);fflush(stdout);
		for(j=1;j<=Count;++j) {
			Key[0] += j;
			Assert(0==LdbGetData(&h,Key,Data));
			Assert(0==LdbDeleteRecord(&h,Key));
			Assert(0!=LdbGetData(&h,Key,Data));
		}
	}
	LdbVerifyContents(&h,1);
	LdbClose(&h);
}

int main(int argc,char* argv[])
{
	make();
	read_test();
	seq_test();
	rev_test();
	seq_test2();
	delete_test();
	return 0;
}
