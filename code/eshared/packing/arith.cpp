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

struct Range
{
    eU32 low;
    eU32 high;
};

enum : eU32
{
    MAX_HIGH = 0x7fffffff,
    HALF     = MAX_HIGH/2+1,
    QUARTER1 = HALF/2,
    QUARTER3 = 3*QUARTER1
};

eBool eArithmeticPacker::pack(const eByteArray &src, eByteArray &dst)
{
    eDataStream ds;

    // count symbols
    const eU32 symCount = src.size();
    eU32 symCounts[256];
    eMemSet(symCounts, 0, sizeof(symCounts));
    for (eU32 i=0; i<src.size(); i++)
        symCounts[src[i]]++;

    // create ranges
    Range symRanges[256];
    symRanges[0].low = 0;
    symRanges[0].high = symCounts[0];
    for (eU32 i=1; i<256; i++)
    {
        symRanges[i].low = symRanges[i-1].high;
        symRanges[i].high = symRanges[i].low+symCounts[i];
    }

    // store size and counts for decoding
    ds.writeU32(src.size());
    for (eU32 i=0; i<256; i++)
        ds.writeU32(symCounts[i]);

    // encode
    eU32 low = 0;
    eU32 high = MAX_HIGH; // last 31 bits set
    eU32 underflow = 0;

    for (eU32 i=0; i<src.size(); i++)
    {
        const eU8 s = src[i];
        const eU32 symLow = symRanges[s].low;
        const eU32 symHigh = symRanges[s].high;
        const eU32 range = high-low+1;
        
        high = low+(range/symCount)*symHigh-1;
        low = low+(range/symCount)*symLow;

        while (eTRUE)
        {
            if (high < HALF) // E1 scaling: [0,0.5)->[0,1), x->2x
            {
                ds.writeBit(0);
                ds.writeBit(1, underflow);
                underflow = 0;
            }
            else if (low > HALF) // E2 scaling: [0.5,1)->[0,1), x->2(x-0.5)
            {
                ds.writeBit(1);
                ds.writeBit(0, underflow);
                underflow = 0;
                low -= HALF;
                high -= HALF;
            }
            else if (low > QUARTER1 && high < QUARTER3) // E3 scaling: [0.25,0.75)->[0,1): x->2(x-0.25)
            {
                underflow++;
                low -= QUARTER1;
                high -= QUARTER1;
            }
            else
                break;

            low = 2*low;
            high = 2*high+1;
        }

        eASSERT(high-low >= QUARTER1);
    }

    // flush
    if (low < QUARTER1)
    {
        ds.writeBit(0);
        ds.writeBit(1, underflow+1);
    }
    else
    {
        ds.writeBit(1);
        ds.writeBit(0, underflow+1);
    }

    dst = ds.getData();
    return eTRUE;
}

eBool eArithmeticPacker::unpack(const eByteArray &src, eByteArray &dst)
{
    // read length
    eDataStream ds(src);
    eU32 srcSize = ds.readU32();

    // read counts
    eU32 symCounts[256];
    for (eU32 i=0; i<256; i++)
        symCounts[i] = ds.readU32();

    // create ranges
    Range symRanges[256];
    symRanges[0].low = 0;
    symRanges[0].high = symCounts[0];
    for (eU32 i=1; i<256; i++)
    {
        symRanges[i].low = symRanges[i-1].high;
        symRanges[i].high = symRanges[i].low+symCounts[i];
    }

    // decode
    eU32 low = 0;
    eU32 high = MAX_HIGH;
    eU32 val = 0;

    for (eU32 i=0; i<31; i++)
        val |= (ds.readBitOrZero()<<(30-i));

    for (eU32 i=0; i<srcSize; i++)
    {
        const eU32 range = high-low+1;        
        const eU32 count = (val-low)/(range/srcSize);

        eInt j = -1;
        for (j=0; j<256; j++)
            if (count >= symRanges[j].low && count < symRanges[j].high)
                break;

        eASSERT(j != -1);
        const Range &r = symRanges[j];
        const eU8 s = (eU8)j;
        const eU32 symLow = symRanges[s].low;
        const eU32 symHigh = symRanges[s].high;

        dst.append(s);
        high = low+(range/srcSize)*symHigh-1;
        low = low+(range/srcSize)*symLow;

        while (eTRUE)
        {
            if (high < HALF) // E1 scaling: [0,0.5)->[0,1), x->2x
            {
            }
            else if (low >= HALF) // E2 scaling: [0.5,1)->[0,1), x->2(x-0.5)
            {
                low -= HALF;
                high -= HALF;
                val -= HALF;
            }
            else if (low >= QUARTER1 && high < QUARTER3) // E3 scaling: [0.25,0.75)->[0,1): x->2(x-0.25)
            {
                low -= QUARTER1;
                high -= QUARTER1;
                val -= QUARTER1;
            }
            else
                break;

            low = 2*low;
            high = 2*high+1;
            val = (val<<1)|ds.readBitOrZero();
        }
    }

    return eTRUE;
}