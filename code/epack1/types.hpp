#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <cassert>
#include <string>

typedef float eF32;
typedef unsigned int eU32;
typedef unsigned short eU16;
typedef int eInt;
typedef char eChar;
typedef eChar eBool;
typedef eChar eU8;
typedef void * ePtr;
typedef const void * eConstPtr;
typedef std::vector<eU8> eByteArray;
#define eArray std::vector
#define eFORCEINLINE __forceinline
#define eALIGN16 __declspec(align(16))
#define eFALSE 0
#define eTRUE  (!eFALSE)
#define eASSERT assert
#define eMemEqual(a, b, c) (!memcmp(a, b, c))
#define eMemSet(a, b, c) memset(a, b, c)
#define eMemCopy(a, b, c) memcpy(a, b, c)
#define eMin(a, b) (std::min(a, b))
#define eMax(a, b) (std::max(a, b))
#define nullptr 0
#define eU32_MAX 0xffffffff
#define eNAKED __declspec(naked)
#define eINLINE inline
#define eString std::string
#define eStrCopy strcpy
#define eAbs std::abs

eINLINE eU32 eAlign(eU32 value, eU32 alignment)
{
    return ((value+alignment-1) & ~(alignment-1));
}

#endif