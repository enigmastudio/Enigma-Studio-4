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


Texture2D<float4>   inputs[4]: register(t0);
RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    uint    mode;
};

cbuffer OpInfos : register(b1)
{
    uint numAboveOps;
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    float4 col0 = inputs[0][xy];
    float3 res = col0.rgb;

    [unroll]
    for (uint i=1; i<4; i++)
    {
        if (i < numAboveOps)
        {
            float3 col = inputs[i][xy].rgb;

            if (mode == 0) // add
                res += col;
            else if (mode == 1) // sub
                res -= col;
            else if (mode == 2) // mul
                res *= col;
            else if (mode == 3) // difference
                res = abs(res-col);
            else if (mode == 4) // average
                res = (res+col)*0.5f;
            else if (mode == 5) // minimum
                res = min(res, col);
            else if (mode == 6) // maximum
                res = max(res, col);
        }
    }

    output[xy] = float4(res, col0.a);
}