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

#include "../system/system.hpp"
#include "packing.hpp"

eBool eMoveToFront::pack(const eByteArray &src, eByteArray &dst)
{
    dst.resize(src.size());

    eU8 map[256];
    for (eU32 i=0; i<256; i++)
        map[i] = i;

    for (eU32 i=0; i<src.size(); i++)
    {
        eU32 j;
        for (j=0; src[i]!=map[j]; j++);

        for (eU32 k=j; k>0; k--)
            map[k] = map[k-1];

        map[0] = src[i];
        dst[i] = j;
    }

    return eTRUE;
}

eBool eMoveToFront::unpack(const eByteArray &src, eByteArray &dst)
{
    dst.resize(src.size());

    eU8 map[256];
    for (eU32 i=0; i<256; i++)
        map[i] = i;

    for (eU32 i=0; i<src.size(); i++)
    {
        const eU32 pos = map[src[i]];

        for (eU32 j=src[i]; j>0; j--)
            map[j] = map[j-1];

        map[0] = pos;
        dst[i] = pos;
        
    }

    return eTRUE;
}