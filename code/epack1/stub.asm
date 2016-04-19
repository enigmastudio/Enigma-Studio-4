; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;
;   This file is part of
;       ______        _                             __ __
;      / ____/____   (_)____ _ ____ ___   ____ _   / // /
;     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
;    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
;   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
;                    /____/                              
;
;   Copyright © 2003-2012 Brain Control, all rights reserved.
;
; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

.686
.model flat, stdcall
option casemap:none

include stub.inc

STUB_DATA_BEGIN_MAGIC equ 0deadc0deh
STUB_IMAGE_BASE       equ 040000000h

public eStubBegin
public eStubEnd
public iatBegin
public iatEnd
public impTabBegin
public impTabEnd

.code
eStubBegin:
eStub:          pushad
                call    nextInstr
nextInstr:      pop     ebp
                sub     ebp, offset nextInstr  
	  
copyImage:      mov     ecx, dword ptr [ebp+virtImgSize]
                mov     esi, dword ptr [ebp+imageBase]
                add     esi, dword ptr [ebp+virtImgBegin] ; src
                mov     edi, dword ptr [ebp+imageBase]
                add     edi, dword ptr [ebp+virtImgSecRva] ; dst
                rep movsb

processImports: mov     esi, dword ptr [ebp+imageBase]
                add     esi, dword ptr [ebp+orgImpTabAddr]        ; esi=IMAGE_IMPORT_DESCRIPTOR*
impNextDll:     cmp     dword ptr [esi], 0
                je      importsDone                               ; yes => importing done

                mov     eax, dword ptr [esi+12]                   ; eax=IMAGE_IMPORT_DESCRIPTOR->name
                add     eax, dword ptr [ebp+imageBase]
                push    eax
                call    dword ptr [ebp+llIat]                     ; call LoadLibrary
                mov     edx, eax                                  ; edx=DLL handle returned by LoadLibrary
    
                mov     edi, dword ptr [esi]                      ; eax=IMAGE_IMPORT_DESCRIPTOR->orgFirstThunk
                add     edi, dword ptr [ebp+imageBase]
impNextFunc:    cmp     dword ptr [edi], 0
                jz      impFuncDone

                mov     ebx, dword ptr [edi]                      ; ebx=IMAGE_IMPORT_BY_NAME->name
                add     ebx, dword ptr [ebp+imageBase]
                add     ebx, 2
                push    ebx                                       ; push function name
                push    edx                                       ; push DLL handle pointer
                call    dword ptr [ebp+gpaIat]                    ; call GetProcAddress

                mov     ebx, dword ptr [esi+16]                   ; ebx=IMAGE_IMPORT_DESCRIPTOR->firstThunk
                add     ebx, dword ptr [ebp+imageBase]
                mov     dword ptr [ebx], eax

                add     edi, 4
                jmp     impNextFunc
    
impFuncDone:    add     esi, sizeof(ePeImportDesc)                ; advance to next import descriptor
                jmp     impNextDll

importsDone:    mov     eax, dword ptr [ebp+imageBase]
                add     eax, dword ptr [ebp+orgEntryPoint]
                jmp     eax

; ------------------------------- data starts here -------------------------------
	  
                dd      STUB_DATA_BEGIN_MAGIC
imageBase       dd      0
orgEntryPoint   dd      0
orgImpTabAddr   dd      0
virtImgBegin    dd      0
virtImgSize     dd      0
virtImgSecRva   dd      0

iatBegin:
gpaIat          ePeIatThunk<02000h+(gpaName-eStubBegin)>
llIat           ePeIatThunk<02000h+(llName-eStubBegin)>
                ePeIatThunk<0>
iatEnd:
              
gpaIlt          ePeIltThunk<02000h+(gpaName-eStubBegin)>
llIlt           ePeIltThunk<02000h+(llName-eStubBegin)>
                ePeIltThunk<0> ; terminator

impTabBegin:
krnl32ImpDesc   ePeImportDesc<02000h+(gpaIlt-eStubBegin), 0, 0, 02000h+(krnl32DllName-eStubBegin), 02000h+(gpaIat-eStubBegin)>
impDescNull     ePeImportDesc<0, 0, 0, 0, 0> ; terminator
impTabEnd:

krnl32DllName   db      "kernel32.dll", 0

gpaName         dw      0 ; hint
                db      "GetProcAddress", 0
llName          dw      0 ; hint
                db      "LoadLibraryA", 0

eStubEnd:
end eStub