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

#include "globals.hlsli"

Texture2D<float4>   input:  register(t0);
RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    uint    passes:     packoffset(c0.x);
    float2  amount:     packoffset(c1.x);
    float   amplify:    packoffset(c2.x);
};

static const uint THREAD_COUNT = 128;
groupshared float4 temp[THREAD_COUNT];

#if defined (HBLUR)
[numthreads(THREAD_COUNT, 1, 1)]
void main(uint3 tid: SV_DispatchThreadID, uint3 gtId : SV_GroupThreadID, uint3 gid : SV_GroupID, uint gi : SV_GroupIndex)
{
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    int kernelHalf = amount.x*w;

    int2 coord = int2(gi-kernelHalf+(THREAD_COUNT-kernelHalf*2)*gid.x, gid.y);
    coord = clamp(coord, int2(0, 0), int2(wh.x-1, wh.y-1));
    temp[gi] = input[coord];

    GroupMemoryBarrierWithGroupSync();

    if ((int)gi >= kernelHalf && (int)gi < (THREAD_COUNT-kernelHalf) && ((gid.x*(THREAD_COUNT-2*kernelHalf)+gi-kernelHalf) < wh.x))
    {
        float4 vOut = 0;
        for (int i = -kernelHalf; i <= kernelHalf; ++i)
            vOut += temp[gi+i];

        uint2 dstCoords = uint2(gi-kernelHalf+(THREAD_COUNT-kernelHalf*2)*gid.x, gid.y);
        output[dstCoords] = float4(vOut.rgb/(float)(kernelHalf*2+1)*amplify, temp[gi].a);
    }
}
#elif defined (VBLUR)
[numthreads(1, THREAD_COUNT, 1)]
void main(uint3 tid: SV_DispatchThreadID, uint3 gtId : SV_GroupThreadID, uint3 gid : SV_GroupID, uint gi : SV_GroupIndex)
{
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    int kernelHalf = amount.y*w;

    int2 coord = int2(gid.x, gi-kernelHalf+(THREAD_COUNT-kernelHalf*2)*gid.y);
    coord = clamp(coord, int2(0, 0), int2(wh.x-1, wh.y-1));
    temp[gi] = input[coord];

    GroupMemoryBarrierWithGroupSync();

    if ((int)gi >= kernelHalf && (int)gi < (THREAD_COUNT-kernelHalf) && ((gid.y*(THREAD_COUNT-2*kernelHalf)+gi-kernelHalf) < wh.y))
    {
        float4 vOut = 0;
        for (int i = -kernelHalf; i <= kernelHalf; ++i)
            vOut += temp[gi+i];

        uint2 dstCoords = uint2(gid.x, gi-kernelHalf+(THREAD_COUNT-kernelHalf*2)*gid.y);
        output[dstCoords] = float4(vOut.rgb/(float)(kernelHalf*2+1)*amplify, temp[gi].a);
    }
}
//[
#endif
//]