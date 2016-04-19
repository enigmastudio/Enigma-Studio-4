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
    float2  posVal:     packoffset(c0.x);
    float   radiusVal:  packoffset(c1.x);
    float   thickness:  packoffset(c2.x);
    float   decay:      packoffset(c3.x);
    float4  col:        packoffset(c4.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint2 xy = int2(threadId.xy);
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    float real_thickness = thickness * w / 256;
    float real_decay = decay * w / 256;

    float2 pos = posVal * wh;
    float radius = radiusVal * w;
    float centerDist = length(xy - pos);
    float borderDist = abs(centerDist - radius);
    float thicknessDist = borderDist - real_thickness;
    float alpha = saturate(thicknessDist / real_decay);

    output[xy] = (1.0f-alpha)*col+alpha*input[xy];
}