
//
//****************************************************************************
//*                                                                          *
//* Å@Copyright (C) Shigeo Kobayashi, 2016.									 *
//*                 All rights reserved.									 *
//*                                                                          *
//****************************************************************************
//
//
// standard c/c++ includes
//
#ifdef _WIN32
	#define WINDOWS
	#define _CRT_SECURE_NO_WARNINGS
    #ifdef _WIN64
        // 64bit Windows
	#define BIT64
    #else
        // 32bit Windows
	#define BIT32
    #endif
#else
	#define LINUX
	#ifdef __x86_64__
        	// 64bit Linux
		#define BIT64
   	#else
       	// 32bit Linux
		#define BIT32
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#ifdef WINDOWS
//
// ====== WINDOWS specific part =========
//
#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#endif // WINDOWS
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// From version 2,deleted blocks are marked the first bit on.
#define FLAG64_DELETED 0x8000000000000000LL

#include "lightdb.h"

/* LightDB identifier. */
extern const char  LdbID[];     /* is written at the top of the data base file as ID */

/* For little-endian machines */
const extern U_INT64  LdbID64; // = 0x004244746867694cLL; /* == "LightDB" */
