//
//****************************************************************************
//*                                                                          *
//* Å@Copyright (C) Shigeo Kobayashi, 2016.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
//  All OS dependent I/O routines are in bt_fileio.h and bt_fileio.cpp.
//
#ifndef ___BT_FILEIO_H___
#define ___BT_FILEIO_H___

#include "base.h"

//
// Basic file I/O class.
//
class CFileIo : public CBase
{
public:
	CFileIo()
	{
		m_hFile      = NULL;
		OpenMode     = 0;
//		m_szFile[0]  = 0;
	}

	~CFileIo()
	{
		FIoClose();
	}

	void     FIoOpen(const P_CHAR szPath,char chMode);
	int      FIoExist(const char *pszPath);
	void     FIoClose();
	void     FIoRead(void *pBuf,U_INT32   cb);
	void     FIoWrite(void *pBuf,U_INT32   cb);
	void     FIoSeek(U_INT64  cb);
	U_INT64  FIoSize();
	void     FIoFlush();
	int      FIoIsReadOnly()     {return OpenMode=='R';}
	int      FIoIsNewFile ()     {return OpenMode=='N'||OpenMode=='T';};
	int      FIoIsOpened()       {return m_hFile!=NULL;};

public:
    char     OpenMode;      // Open mode char.

private:
    FILE   *m_hFile;          // file handle
//	char    m_szFile[MAX_PATH_SIZE];
};


#endif //___BT_FILEIO_H___
