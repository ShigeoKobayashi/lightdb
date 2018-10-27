///////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2016 Shigeo Kobayashi. All Rights Reserved.
// 
///////////////////////////////////////////////////////////////////////
//
//  All OS dependent I/O routines are in fileio.h and fileio.cpp.
//

#include "stdafx.h"
#include "base.h"

#ifdef _DEBUG
void DbgAssert(int  f,char *file,int line)
{
	if(f) return;
	printf("****** Debug assertion failed %d:%s\n",line,file);
}
#endif // _DEBUG

static int mem_counter = 0;

//
// if f==TRUE then nothing is done,or print arguments after f in printf format.
//
void
CBase::T_(int f,const char *psz,...)
{
	va_list arg;

	if(f) return;
	va_start(arg,psz);
	vprintf(psz,arg);
	va_end(arg);
}

//
// Memory allocation
void *
	CBase::BseMemAlloc(size_t cb)
{
	ASSERT(cb>0);

	void *p = malloc(cb);
	CHECK(p!=NULL, ERROR_MEMORY_ALLOC);
	mem_counter++;
	return p;
}

void *
	CBase::BseMemReAlloc(void *p,size_t n)
{
	ASSERT(p!=NULL);
	void *ps = realloc(p,n);
	CHECK(ps!=NULL, ERROR_MEMORY_ALLOC);
	return ps;
}

void 
	CBase::BseMemFree(void **p)
{
	void *pm;
	if(p ==NULL) return;
	pm = *p;
	if(pm==NULL) return;
	mem_counter--;
	ASSERT(mem_counter>=0);
	free(pm);
	*p = NULL;
}

void 
	CBase::BseMemCheck()
{
	ASSERT(mem_counter==0);
}

char *
	CBase::BseStrAllocCopy(char *sin)
{
	char *st;
	int len=strlen(sin);
	st = (char*)BseMemAlloc(len+2);
	strcpy(st,sin);
	return st;
}

void 
	CBase::BseStrLimitCopy(char *so,int maxo,const char *si)
{
	int  cb = maxo-1;
	char ch;
	
	for(int i=0;i<cb;++i)
	{
		ch = *si++;
		if(ch==0) break;
		*so++ = ch;
	}
	*so = 0;
}
