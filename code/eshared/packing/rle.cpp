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

eTextRle::eTextRle(eU8 escapeCode) :
    m_escapeCode(escapeCode)
{
}

eBool eTextRle::pack(const eByteArray &src, eByteArray &dst)
{
    dst.clear();

    for (eU32 i=0; i<src.size(); )
    {
        const eU8 c0 = src[i];
        eASSERT(c0 != m_escapeCode);
        eU32 j = i+1;
        eU32 count = 0;

        while (j < src.size() && count < 0xffff)
        {
            const eU8 c1 = src[j++];

            if (c0 == c1 && count < 0xff)
                count++;
            else
                break;
        }

        if (count <= 4)
        {
            for (eU32 k=0; k<count+1; k++)
                dst.append(c0);
        }
        else
        {
            eASSERT(count <= 0xff);
            dst.append(m_escapeCode);
            dst.append(count+1);
            dst.append(c0);
        }

        i += count+1;
    }

    return eTRUE;
}

eBool eTextRle::unpack(const eByteArray &src, eByteArray &dst)
{
    dst.clear();

    for (eU32 i=0; i<src.size(); )
    {
        const eU8 c = src[i++];

        if (c == m_escapeCode)
        {
            const eU32 count = src[i++];
            const eU8 c1 = src[i++];
            eASSERT(c1 != m_escapeCode);

            for (eU32 j=0; j<count; j++)
                dst.append(c1);
        }
        else
            dst.append(c);
    }

    return eTRUE;
}