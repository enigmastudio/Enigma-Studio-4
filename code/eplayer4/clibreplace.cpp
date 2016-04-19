/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright © 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../eshared/system/system.hpp"

#if defined(eRELEASE)
// call ctors and dtors of static and global variables
typedef void (eCDECL *ePVFV)();

#pragma data_seg(".CRT$XCA")
ePVFV __xc_a[] = {nullptr};
#pragma data_seg(".CRT$XCZ")
ePVFV __xc_z[] = {nullptr};
#pragma data_seg() // reset data segment

static const eU32 eMAX_ATEXITS = 32;
static ePVFV g_atExitList[eMAX_ATEXITS];

static void initTerm(ePVFV *pfbegin, ePVFV *pfend)
{
    while (pfbegin < pfend)
    {
        if (*pfbegin)
            (**pfbegin)();
        
        pfbegin++;
    }
}

static void initAtExit()
{
    eMemSet(g_atExitList, 0, sizeof(g_atExitList));
}

static void doAtExit()
{
    initTerm(g_atExitList, g_atExitList+eMAX_ATEXITS);
}

eInt eCDECL atexit(ePVFV func)
{
    // get next free entry in atexist list
    eU32 index = 0;
    while (g_atExitList[index++]);
    eASSERT(index < eMAX_ATEXITS);

    // put function pointer to destructor there
    if (index < eMAX_ATEXITS)
    {
        g_atExitList[index] = func;
        return 0;
    }

    return -1;
}

eInt eCDECL _purecall()
{
    eASSERT(eFALSE);
    return 0;
}

// fixed unresolved externals to c-lib symbols
extern "C"
{
    eInt    _fltused = 0;
    #define _PAGESIZE_ 0x1000

    eNAKED void eCDECL _chkstk(void)
    {
        __asm
        {
            push    ecx
            lea     ecx, [esp]+8-4
            sub     ecx, eax
            sbb     eax, eax
            not     eax
            and     ecx, eax
            mov     eax, esp
            and     eax, ~(_PAGESIZE_-1)
cs10:       cmp     ecx, eax
            jb      short cs20
            mov     eax, ecx
            pop     ecx
            xchg    esp, eax
            mov     eax, dword ptr [eax]
            mov     dword ptr [esp], eax
            ret
cs20:       
            sub     eax, _PAGESIZE_
            test    dword ptr [eax], eax
            jmp     short cs10
            ret
        }
    }

    eNAKED void eCDECL _alloca_probe_16()
    {
        __asm
        {
            push    ecx
            lea     ecx, [esp+8]
            sub     ecx, eax
            and     ecx, 15
            add     eax, ecx
            sbb     ecx, ecx
            or      eax, ecx
            pop     ecx
            jmp    _chkstk
        }
    }
};
#endif

void eGlobalsStaticsInit()
{
#ifdef eRELEASE
    initAtExit();
    initTerm(__xc_a, __xc_z); 
#endif
}

void eGlobalsStaticsFree()
{
#ifdef eRELEASE
    doAtExit();
#endif
}