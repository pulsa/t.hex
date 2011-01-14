#pragma once

#ifndef _DEFS_H_
#define _DEFS_H_

#ifndef int32
typedef int int32;
#endif

#ifndef uint32
typedef unsigned long int uint32;
#endif

#ifndef int64
typedef __int64 int64;
#endif

#ifndef uint64
typedef unsigned __int64 uint64;
#endif

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef LITTLEENDIAN_MODE
#define LITTLEENDIAN_MODE   0
#define BIGENDIAN_MODE      1
#define NATIVE_ENDIAN_MODE  LITTLEENDIAN_MODE
#endif

#define MAX_ADDRESS 0x7FFFFFFFFFFFFFFF // ((1 << 63) - 1)
//#define THSIZE uint64
typedef uint64 THSIZE;

typedef enum {BASE_DEC, BASE_HEX, BASE_BOTH, BASE_OCT, BASE_ALL} THBASE;

const uint32 MEGA = 1024 * 1024;


#endif // _DEFS_H_
