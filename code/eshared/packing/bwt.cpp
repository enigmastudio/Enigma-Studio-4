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

static eInt sortPred(const eByteArray &src, eU32 idx0, eU32 idx1)
{
    for (eU32 i=0; i<src.size(); i++)
    {
        // wrap around compare
        const eInt diff = (eInt)src[(i+idx1)%src.size()]-(eInt)src[(i+idx0)%src.size()];
        if (diff != 0)
            return diff;
    }

    return 0;
}

eBool eBurrowsWheeler::pack(const eByteArray &src, eByteArray &dst)
{
    // initialize
    eArray<eU32> mtx(src.size());
    for (eU32 i=0; i<src.size(); i++)
        mtx[i] = i;

    /*
    // terribly slow O(n^2) bubble sort
    for (eU32 i=0; i<mtx.size(); i++)
    {
        for (eU32 j=i; j<mtx.size()-1; j++)
            if (sortPred(src, mtx[j], mtx[j+1]) < 0)
                eSwap(mtx[j], mtx[j+1]);

        if (i%100==0)
            eWriteToLog(eIntToStr(i));
    }
    */

    for (eU32 j=1; j<mtx.size(); j++)
    {
        const eU32 temp = mtx[j];
        eInt i = (eInt)j-1;

        while (i >= 0 && sortPred(src, mtx[i], temp) < 0)
        {
            mtx[i+1] = mtx[i];
            i--;
        }

        mtx[i+1] = temp;

        if (j%100==0)
            eWriteToLog(eIntToStr(i));
    }

    // output
    dst.resize(src.size()+sizeof(eU32)); // space for original index
    eU32 origIdx = 0;

    for (eU32 i=0; i<src.size(); i++)
    {
        eInt idx = (mtx[i]-1);
        idx = (idx+(eInt)src.size())%(eInt)src.size();
        dst[i] = src[idx];
        origIdx = (!mtx[i] ? i : origIdx);
    }

    // append original index
    (*(eU32 *)&dst[src.size()]) = origIdx;
    return eTRUE;
}

eBool eBurrowsWheeler::unpack(const eByteArray &src, eByteArray &dst)
{
    const eU32 srcLen = src.size()-sizeof(eU32); // original index appended
    eArray<eU32> pred(srcLen);
    eU32 count[256];

    eMemSet(count, 0, sizeof(count));
    dst.resize(srcLen);

    for (eU32 i=0; i<srcLen; i++)
    {
        const eU8 c = src[i];
        pred[i] = count[c];
        count[c]++;
    }

    for (eU32 i=0, sum=0; i<256; i++)
    {
        const eU32 temp = count[i];
        count[i] = sum;
        sum += temp;
    }

    const eU32 origIdx = *(eU32 *)&src[srcLen]; // read original index
    for (eInt i=srcLen-1, j=origIdx; i>=0; i--)
    {
        dst[i] = src[j];
        j = pred[j]+count[src[j]];
    }

    return eTRUE;
}