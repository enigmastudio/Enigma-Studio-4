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

#ifndef PE_STRUCTS_HPP
#define PE_STRUCTS_HPP

#include "../eshared/system/system.hpp"

enum ePeDirectories
{
    ePEDIR_EXPORTS = 0,
    ePEDIR_IMPORT  = 1,
    ePEDIR_TLS     = 9,
    ePEDIR_IAT     = 12,
};

enum ePeSectionFlags
{
    ePESEC_INITED_DATA   = 0x00000040,
    ePESEC_UNINITED_DATA = 0x00000080,
    ePESEC_MEM_EXECUTE   = 0x20000000,
    ePESEC_MEM_READ      = 0x40000000,
    ePESEC_MEM_WRITE     = 0x80000000
};

struct ePeDosHeader
{
    eU8             magic[2];
    eU8             pad[29*2];
    eU32            peHeaderAddr;
};

struct ePeDataDir
{
    eU32            virtAddr;
    eU32            size;
};

struct ePeFileHeader
{
    eU16            Machine;
    eU16            numSecs;
    eU32            TimeDateStamp;
    eU32            PointerToSymbolTable;
    eU32            NumberOfSymbols;
    eU16            optHeaderSize;
    eU16            Characteristics;
};

struct ePeOptHeader
{
    eU16            Magic;
    eU8             MajorLinkerVersion;
    eU8             MinorLinkerVersion;
    eU32            SizeOfCode;
    eU32            SizeOfInitializedData;
    eU32            SizeOfUninitializedData;
    eU32            entryPoint;
    eU32            BaseOfCode;
    eU32            dataBase;

    eU32            imageBase;
    eU32            sectionAlignment;
    eU32            fileAlignment;
    eU16            MajorOperatingSystemVersion;
    eU16            MinorOperatingSystemVersion;
    eU16            MajorImageVersion;
    eU16            MinorImageVersion;
    eU16            MajorSubsystemVersion;
    eU16            MinorSubsystemVersion;
    eU32            Win32VersionValue;
    eU32            imageSize;
    eU32            SizeOfHeaders;
    eU32            checkSum;
    eU16            Subsystem;
    eU16            DllCharacteristics;
    eU32            SizeOfStackReserve;
    eU32            SizeOfStackCommit;
    eU32            SizeOfHeapReserve;
    eU32            SizeOfHeapCommit;
    eU32            LoaderFlags;
    eU32            NumberOfRvaAndSizes;
    ePeDataDir      dataDirs[16];
};

struct ePeHeader
{
    eU8             magic[4];
    ePeFileHeader   fileHeader;
    ePeOptHeader    optHeader;
};

struct ePeSection
{
    eChar           name[8];
    eU32            virtSize;
    eU32            virtAddr;
    eU32            dataSize;
    eU32            dataPtr;
    eU8             pad0[3*4]; // PointerToRelocations, PointerToLinenumbers, NumberOfRelocations, NumberOfLinenumbers
    eU32            flags;
};

struct ePeImportDesc
{
    union
    {
        eU32        characteristics;
        eU32        orgFirstThunk;
    };

    eU32            timeDateStamp;
    eU32            forwarderChain;
    eU32            name;
    eU32            firstThunk;
};

union ePeImportThunk
{
    eU32            forwarderStr;
    eU32            function;
    eU32            ordinal;
    eU32            dataAddr;
};

struct ePeImportByName
{
    eU16            hint;
    eChar           name[1];
};

#endif