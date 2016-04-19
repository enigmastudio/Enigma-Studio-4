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
    float   strength:   packoffset(c0.x);
    float   intensity:  packoffset(c1.x);
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

    // calculate normal vector by applying
    // a 2-dimensional sobel filter
    float v0 = grayScale(input[int2(x-1, y-1)].rgb);
    float v1 = grayScale(input[int2(x,   y-1)].rgb);
    float v2 = grayScale(input[int2(x+1, y-1)].rgb);
    float v3 = grayScale(input[int2(x-1, y)].rgb);
    float v5 = grayScale(input[int2(x+1, y)].rgb);
    float v6 = grayScale(input[int2(x-1, y+1)].rgb);
    float v7 = grayScale(input[int2(x  , y+1)].rgb);
    float v8 = grayScale(input[int2(x+1, y+1)].rgb);

    float3 normal = {v0-v2+2.0f*(v3-v5)+v6-v8, v0+2.0f*(v1-v7)+v2-v6-v8, 1.001f-strength};
    normal = normalize(normal);
    normal = lerp(float3(0.0f, 0.0f, 1.0f), normal, intensity);

    // convert floating point to byte values:
    // range [-1.0,1.0] to [0.0,1.0]
    normal += 1.0f;
    normal *= 0.5f;

    output[xy] = float4(normal, 1.0f);
}