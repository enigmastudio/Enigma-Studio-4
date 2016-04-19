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


Texture2D s_texTarget : register(t0);
SamplerState s_samTarget : register(s0);

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

sampler s_target: register(s0);

struct FxaaTex { SamplerState smpl; Texture2D tex; };
#define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
#define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)

float4 main(const ShaderInput input) : SV_Target0
{
    float2 texDim;
    s_texTarget.GetDimensions(texDim.x, texDim.y);
    texDim.x = 1.0 / texDim.x;
    texDim.y = 1.0 / texDim.y;
    FxaaTex tex;
    tex.smpl = s_samTarget;
    tex.tex = s_texTarget;

    float2 posM = input.texCoord;
    float4 rgbyM = FxaaTexTop(tex, posM);

    float lumaS = FxaaTexOff(tex, posM, int2( 0, 1), fxaaQualityRcpFrame.xy).y;
    float lumaE = FxaaTexOff(tex, posM, int2( 1, 0), fxaaQualityRcpFrame.xy).y;
    float lumaN = FxaaTexOff(tex, posM, int2( 0,-1), fxaaQualityRcpFrame.xy).y;
    float lumaW = FxaaTexOff(tex, posM, int2(-1, 0), fxaaQualityRcpFrame.xy).y;

/*--------------------------------------------------------------------------*/
    float maxSM = max(lumaS, rgbyM.y);
    float minSM = min(lumaS, rgbyM.y);
    float maxESM = max(lumaE, maxSM);
    float minESM = min(lumaE, minSM);
    float maxWN = max(lumaN, lumaW);
    float minWN = min(lumaN, lumaW);
    float rangeMax = max(maxWN, maxESM);
    float rangeMin = min(minWN, minESM);
    float rangeMaxScaled = rangeMax * 0.166;
    float range = rangeMax - rangeMin;
    float rangeMaxClamped = max(0.0833, rangeMaxScaled);
    bool earlyExit = range < rangeMaxClamped;
/*--------------------------------------------------------------------------*/
    if(earlyExit)
        return rgbyM;

/*--------------------------------------------------------------------------*/

    float lumaNW = FxaaTexOff(tex, posM, int2(-1,-1), fxaaQualityRcpFrame.xy).y;
    float lumaSE = FxaaTexOff(tex, posM, int2( 1, 1), fxaaQualityRcpFrame.xy).y;
    float lumaNE = FxaaTexOff(tex, posM, int2( 1,-1), fxaaQualityRcpFrame.xy).y;
    float lumaSW = FxaaTexOff(tex, posM, int2(-1, 1), fxaaQualityRcpFrame.xy).y;
    
/*--------------------------------------------------------------------------*/
    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float subpixRcpRange = 1.0/range;
    float subpixNSWE = lumaNS + lumaWE;
    float edgeHorz1 = (-2.0 * rgbyM.y) + lumaNS;
    float edgeVert1 = (-2.0 * rgbyM.y) + lumaWE;
/*--------------------------------------------------------------------------*/
    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
    float edgeVert2 = (-2.0 * lumaN) + lumaNWNE;
/*--------------------------------------------------------------------------*/
    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;
    float edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    float edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    float edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
    float edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
    float edgeHorz = abs(edgeHorz3) + edgeHorz4;
    float edgeVert = abs(edgeVert3) + edgeVert4;
/*--------------------------------------------------------------------------*/
    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float lengthSign = texDim.x;
    bool horzSpan = edgeHorz >= edgeVert;
    float subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;
/*--------------------------------------------------------------------------*/
    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    if(horzSpan) lengthSign = texDim.y;
    float subpixB = (subpixA * (1.0/12.0)) - rgbyM.y;
/*--------------------------------------------------------------------------*/
    float gradientN = lumaN - rgbyM.y;
    float gradientS = lumaS - rgbyM.y;
    float lumaNN = lumaN + rgbyM.y;
    float lumaSS = lumaS + rgbyM.y;
    bool pairN = abs(gradientN) >= abs(gradientS);
    float gradient = max(abs(gradientN), abs(gradientS));
    if(pairN) lengthSign = -lengthSign;
    float subpixC = saturate(abs(subpixB) * subpixRcpRange);
/*--------------------------------------------------------------------------*/
    float2 posB;
    posB.x = posM.x;
    posB.y = posM.y;
    float2 offNP;
    offNP.x = (!horzSpan) ? 0.0 : texDim.x;
    offNP.y = ( horzSpan) ? 0.0 : texDim.y;
    if(!horzSpan) posB.x += lengthSign * 0.5;
    if( horzSpan) posB.y += lengthSign * 0.5;
/*--------------------------------------------------------------------------*/
    float2 posN;
    posN.x = posB.x - offNP.x;
    posN.y = posB.y - offNP.y;
    float2 posP;
    posP.x = posB.x + offNP.x;
    posP.y = posB.y + offNP.y;
    float subpixD = ((-2.0)*subpixC) + 3.0;
    float lumaEndN = FxaaTexTop(tex, posN).y;
    float subpixE = subpixC * subpixC;
    float lumaEndP = FxaaTexTop(tex, posP).y;
/*--------------------------------------------------------------------------*/
    if(!pairN) lumaNN = lumaSS;
    float gradientScaled = gradient * 1.0/4.0;
    float lumaMM = rgbyM.y - lumaNN * 0.5;
    float subpixF = subpixD * subpixE;
    bool lumaMLTZero = lumaMM < 0.0;
/*--------------------------------------------------------------------------*/
    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    if(!doneN) posN.x -= offNP.x * 1.5;
    if(!doneN) posN.y -= offNP.y * 1.5;
    bool doneNP = (!doneN) || (!doneP);
    if(!doneP) posP.x += offNP.x * 1.5;
    if(!doneP) posP.y += offNP.y * 1.5;
/*--------------------------------------------------------------------------*/
    if(doneNP) {
        if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).y;
        if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).y;
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * 2.0;
        if(!doneN) posN.y -= offNP.y * 2.0;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * 2.0;
        if(!doneP) posP.y += offNP.y * 2.0;
/*--------------------------------------------------------------------------*/
        if(doneNP) {
            if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).y;
            if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).y;
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * 4.0;
            if(!doneN) posN.y -= offNP.y * 4.0;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * 4.0;
            if(!doneP) posP.y += offNP.y * 4.0;
/*--------------------------------------------------------------------------*/
            if(doneNP) {
                if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).y;
                if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).y;
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                doneN = abs(lumaEndN) >= gradientScaled;
                doneP = abs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * 12.0;
                if(!doneN) posN.y -= offNP.y * 12.0;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * 12.0;
                if(!doneP) posP.y += offNP.y * 12.0;
            }
        }
    }

    float dstN = posM.x - posN.x;
    float dstP = posP.x - posM.x;
    if(!horzSpan) dstN = posM.y - posN.y;
    if(!horzSpan) dstP = posP.y - posM.y;

    bool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    float spanLength = (dstP + dstN);
    bool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    float spanLengthRcp = 1.0/spanLength;

    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float subpixG = subpixF * subpixF;
    float pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
    float subpixH = subpixG * 0.75;

    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    if(!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;

    return float4(FxaaTexTop(tex, posM).xyz, rgbyM.y);
}
