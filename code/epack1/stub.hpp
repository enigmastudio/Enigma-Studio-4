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

#ifndef STUB_HPP
#define STUB_HPP

#include "pestructs.hpp"

const eU32 eSTUB_DATA_BEGIN_MAGIC = 0xdeadc0de;
const eU32 eSTUB_IMAGE_BASE = 0x00400000;

struct eStubData
{
    eU32 imageBase;
    eU32 orgEntryPoint;
    eU32 orgImpTabAddr;
    eU32 virtImgBegin;
    eU32 virtImgSize;
    eU32 virtImgSecRva;
};

extern "C"
{
    void eStubBegin();
    void eStubEnd();

    void iatBegin();
    void iatEnd();

    void impTabBegin();
    void impTabEnd();
}

/*
#define eSTUB_EMIT_BYTE(b)      __asm _emit (b)
#define eSTUB_EMIT_WORD(w)      __asm _emit ((w)&0xff) __asm _emit ((w>>8)&0xff)
#define eSTUB_EMIT_DWORD(dw)    __asm _emit ((dw)&0xff) __asm _emit ((dw>>8)&0xff) __asm _emit ((dw>>16)&0xff) __asm _emit ((dw>>24)&0xff)
#define eSTUB_DATA_UNINITED     0xbaadf00d

static eNAKED void eStub()
{
    __asm
    {
        pushad
        call    ni
ni:     pop     ebp
        sub     ebp, offset ni
        mov     eax, dword ptr [ebp+imageBase]
        add     eax, dword ptr [ebp+orgEntryPoint]
        jmp     eax

        eSTUB_EMIT_DWORD(eSTUB_DATA_BEGIN_MAGIC)
imageBase:
        eSTUB_EMIT_DWORD(0xffffffff)
orgEntryPoint:
        eSTUB_EMIT_DWORD(0xffffffff)
    }
}

static void eStubEnd()
{
}
*/

#endif