/////////////////////////////////////////////////////////////
//! \file CsTypes.h
//!    \brief Gage Application Programming Interface (API)
//!
/////////////////////////////////////////////////////////////

#ifndef _CS_TYPES_H_
#define _CS_TYPES_H_

#if defined(_WIN32)
#include "CsCvi.h"
#else
#ifndef __KERNEL__
#include <stdint.h>
#endif
#endif

#ifndef _VARTYPES
#define _VARTYPES
typedef unsigned char  uInt8;
typedef unsigned short uInt16;
typedef unsigned long  uInt32;

typedef char            int8;
typedef short           int16;
typedef long            int32;
#endif

#ifndef __cplusplus
#ifndef bool	
typedef char bool;
#endif
#endif

typedef char cschar; //for now concider _tchar and _wchar

typedef uInt32    CSHANDLE;
typedef CSHANDLE *PCSHANDLE;

typedef uInt32     RMHANDLE;
typedef RMHANDLE *PRMHANDLE;

typedef uInt32      DRVHANDLE;
typedef DRVHANDLE *PDRVHANDLE;


typedef enum _CSFILETYPES
{
	cftNone = 0,
	cftBinaryData = cftNone + 1,
	cftSigFile = cftBinaryData + 1,
	cftAscii = cftSigFile + 1,
}CsFileType;

typedef enum _CSFILEMODES
{
	cfmRead = 1,
	cfmWrite = 2,
	cfmReadWrite = cfmRead | cfmWrite,
}CsFileMode;

typedef enum _CSFILEATTACH
{
	fmNew = 0,
}CsFileAttach;

#if defined(_WIN32)

#if defined ( CSDRV_KERNEL )
typedef BOOLEAN    BOOL;

#if defined (int64)
#undef int64
#endif
typedef	LARGE_INTEGER	int64;
typedef	long real32;
typedef	LARGE_INTEGER	real64;

typedef int32	CSDRVSTATUS;
#else
#if defined ( CSDRV_WDFKERNEL )
typedef BOOLEAN    BOOL;
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
typedef __int64 int64;
#else
#if defined(__BORLANDC__)
typedef __int64 int64;
#else
#ifdef Int64
typedef Int64 int64;
#else
#if defined(_CVI_) && (_CVI_ >= 700)
typedef __int64 int64;
#else
#ifdef __s64
typedef __s64 int64;
#else
typedef double int64;
#ifdef _WIN32
#pragma message ("===  Warning: int64 defined as double!")
#endif
#endif
#endif
#endif
#endif
#endif

typedef float  real32;
typedef double real64;

#endif

typedef void *EVENT_HANDLE;
typedef void *SEM_HANDLE;
typedef void *MUTEX_HANDLE;
typedef void *FILE_HANDLE;
#endif


#ifdef __linux__

#ifndef GAGE_LINUX_PORT_H_

#ifndef BOOL
typedef int	BOOL;
#endif

#ifndef BOOLEAN
typedef BOOL BOOLEAN;
#endif

//#include <Threads/CWinEventHandle.h>
typedef int64_t	int64;
typedef void* PVOID;
typedef char* LPSTR;

//#ifndef FALSE
//#define FALSE 0
//#endif

//#ifndef TRUE
//#define TRUE 1
//#endif

#endif // GAGE_LINUX_PORT_H

#define SIGNAL_TRIGGER		0x01
#define SIGNAL_ENDOFBUSY	0x02

#if defined (__KERNEL__)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
#ifndef __cplusplus
typedef int bool;
#endif  // __cplusplus
#endif



#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)

#define READ_REGISTER_UCHAR                     readb
#define READ_REGISTER_USHORT                    readw
#define READ_REGISTER_ULONG                     readl
#define WRITE_REGISTER_UCHAR(register,  value)  writeb ((value), (register))       // Linux is the opposite order than Windows
#define WRITE_REGISTER_USHORT(register, value)  writew ((value), (register))       // Linux is the opposite order than Windows
#define WRITE_REGISTER_ULONG(register,  value)  writel ((value), (register))       // Linux is the opposite order than Windows

#else

#define READ_REGISTER_UCHAR                     ioread8
#define READ_REGISTER_USHORT                    ioread16
#define READ_REGISTER_ULONG                     ioread32
#define WRITE_REGISTER_UCHAR(register,  value)  iowrite8  ((value), (register))    // Linux is the opposite order than Windows
#define WRITE_REGISTER_USHORT(register, value)  iowrite16 ((value), (register))    // Linux is the opposite order than Windows
#define WRITE_REGISTER_ULONG(register,  value)  iowrite32 ((value), (register))    // Linux is the opposite order than Windows

#define READ_REGISTER_BUFFER_UCHAR              ioread8_rep
#define READ_REGISTER_BUFFER_USHORT             ioread16_rep
#define READ_REGISTER_BUFFER_ULONG              ioread32_rep
#define WRITE_REGISTER_BUFFER_UCHAR             iowrite8_rep
#define WRITE_REGISTER_BUFFER_USHORT            iowrite16_rep
#define WRITE_REGISTER_BUFFER_ULONG             iowrite32_rep

#endif

typedef void                        VOID;
typedef int                         INT;
typedef char                        CHAR;
typedef short                       SHORT;
typedef long                        LONG;
typedef CHAR                       *PCHAR, *CHAR_PTR;
typedef SHORT                      *PSHORT, *SHORT_PTR;
typedef LONG                       *PLONG, *LONG_PTR;

typedef unsigned int                UINT;
typedef unsigned char               UCHAR;
typedef unsigned short              USHORT;
typedef unsigned long               ULONG;

typedef UCHAR                      *PUCHAR;
typedef USHORT                     *PUSHORT;
typedef ULONG                      *PULONG;
typedef long                        NTSTATUS;
typedef long long                   LONGLONG;
typedef unsigned long long          ULONGLONG;
#define WCHAR                       unsigned short
typedef WCHAR                      *PWSTR, *LPWSTR;

typedef uInt32                      HANDLE;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif


typedef union _LARGE_INTEGER {
	struct {
		ULONG LowPart;
		LONG HighPart;
	};
	struct {
		ULONG LowPart;
		LONG HighPart;
	} u;
	LONGLONG QuadPart;
} LARGE_INTEGER;

typedef LARGE_INTEGER *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
	struct {
		ULONG LowPart;
		ULONG HighPart;
	};
	struct {
		ULONG LowPart;
		ULONG HighPart;
	} u;
	ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef ULARGE_INTEGER *PULARGE_INTEGER;


typedef long                        real32;
typedef LARGE_INTEGER               real64;

typedef LARGE_INTEGER              PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

// Calculate the byte offset of a field in a structure of type type.
//

#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))

#define CONTAINING_RECORD container_of

#endif  // __KERNEL__

#endif // __linux__


#endif	// _CS_TYPES_H_
