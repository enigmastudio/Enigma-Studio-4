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

Texture2D s_texColor : register(t0);
Texture2D s_texPosG : register(t2);

SamplerState s_samColor : register(s0);
SamplerState s_samPosG : register(s2);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float4x4        c_invViewMatrix;
	float4          c_camFov;
    float4          c_sunPosition;

    float4          c_shallowColor;
    float4          c_lowColor;
    float4          c_hiColor;
    float4          c_grassColor;
    float4          c_dirtColor;

    float           c_planetRadius;
    float           c_planetHillTop;
    float           c_planetSeed;
    float           c_planetFreq;

    float           c_time;
    float           c_waterLevel;
    float           c_waterAmplitude;
    float           c_waterFrequency;
    float           c_waterTransparency;

    float           c_cloudScale;
    float           c_cloudTreshold;

    float           c_waterExtinctionRate;
    float           c_snowFrequency;

    float           c_textureNoise;
    float           c_scatterStrength;
    float           c_fogAmount;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

static const float MIN_RAYMARCH_DELTA = 0.1;

static const float m_fRayleighScaleDepth = 0.25;

//earth 0.0157
static const float g_atmospherePercentage = 0.05;
static const float g_atmosphereWidth = c_planetRadius*g_atmospherePercentage;
static const float g_atmosphereHeight = c_planetRadius + g_atmosphereWidth;

static const float g_maxHillTop = lerp(1, g_atmosphereWidth, c_planetHillTop);
static const float RAYMARCH_RANGE = c_planetRadius + g_maxHillTop;

static const float g_heightAdjust = 1/c_planetFreq;

// Colour of the water depth
static const float3 BIGDEPTHCOLOUR = {0.0039, 0.00196, 0.145};

float4 hash4 (float4 x)
{
    float4 z = fmod(fmod(x+c_planetSeed, 5612), eTWOPI);
    return frac((z*z) * 56812.5453/(1+c_planetSeed));
}

float3 asnoise(float2 i, float2 f)
{
	float2 f2 = f*f, f3 = f2*f;
	float2 df = 30*(f3*(f-2)+f2);
	f = f3*(f*(f*6-15)+10);

    i=abs(i);

    float2 p0 = i * float2(1,57);
    float2 p1 = p0 + float2(1,57);
    float4 h = hash4(float4(p0.x+p0.y, p1.x+p0.y, p0.x+p1.y, p1.x+p1.y));

//  float n = i.x + i.y*57;
//  float4 h = hash4(float4(n,n+1,n+57,n+58));

    float k0 = h.x;
    float k1 = h.y - h.x;
    float k2 = h.z - h.x;
    float k3 = h.x - h.y - h.z + h.w;

	float3 t;

    t.y = k0 + k1*f.x + k2*f.y + k3*f.x*f.y;
    t.xz = (float2(k1,k2) + k3*f.yx)*df;

	return t;
}

float3 Get( float3 p )
{
    p*=g_heightAdjust;

    float3 i = floor(p);
    float3 f = p-i;

    i.xy += float2(13,57)*i.z;

    float3 v0 = asnoise(i.xy, f.xy);
    float3 v1 = asnoise(i.xy + float2(13, 57), f.xy);

    return lerp(v0, v1, f.z*f.z*f.z*(f.z*(f.z*6-15)+10));
}

float Fbm3( float3 p)
{
    float s = 0;
    float b = g_maxHillTop*0.5;
	float2 d = 0;

    for (int ii=0; ii<5; ii++)
    {
        float3 a = Get(p);
		d += a.xz;
	    s += b*a.y/(1+dot(d,d));
        b *= 0.5;
        p *= 2;
    }
    return s;
}

float Fbm32( float3 p)
{
    float s = 0;
    float b = g_maxHillTop*0.5;
	float2 d = 0;

    for (int ii=0; ii<9; ii++)
    {
        float3 a = Get(p);
		d += a.xz;
	    s += b*a.y/(1+dot(d,d));
        b *= 0.5;
        p *= 2;
    }
    return s;
}

float densityTerra(float3 pos)
{
    float hight = length(pos);
    return c_planetRadius - hight + Fbm3(pos/hight * c_planetRadius) - c_waterLevel;
}

float densityTerra2(float3 pos)
{
    float hight = length(pos);
    return c_planetRadius - hight + Fbm32(pos/hight * c_planetRadius) - c_waterLevel;   
}

float densityWater(float3 pos)
{
    float hight = length(pos);
    float density = c_planetRadius - hight;

    // wave amplitude scale factors
    const float3 scale = float3(0.05, 0.04, 0.001);

    const float3 spx = float3(1.0,  1.6,  6.6);
    const float3 stx = float3(0.4, -0.4, -1.0);
    const float3 spz = float3(1.0,  1.7,  2.7);
    const float3 stz = float3(0.6, -0.6,  1.176);

    pos/=hight;
    float2 uv = (float2(atan2(pos.z, pos.x), asin(pos.y))/eTWOPI + float2(0.5, 0.5)) * c_waterFrequency;

    float3 x = spx * uv.x + stx * c_time;
    float3 z = spz * uv.y + stz * c_time;

    // wave amplitudes
    float3 amplitude = float3(snoise(float2(x.x, z.x)), snoise(float2(x.y, z.y)), snoise(float2(x.z, z.z)));

    return density + dot(amplitude, scale * c_waterAmplitude);
}

float fbm(float3 pos)
{
    const float4 scale1 = float4(0.5, 0.25, 0.125, 0.0625);
    return dot((float4(snoise(pos),snoise(pos*2),snoise(pos*4),snoise(pos*8))+1)*.5, scale1);
}

float3 getTerrainNormal( const float3 p, const float t )
{
    float2 e = float2(0.01*t,0);
    return normalize(float3(densityTerra2(p - e.xyy), densityTerra2(p - e.yxy), densityTerra2(p - e.yyx)) - densityTerra2(p));
}

float3 getWaterNormal( const float3 p, const float t )
{
    float2 e = float2(0.01*t,0);
    return normalize(float3(densityWater(p - e.xyy), densityWater(p - e.yxy), densityWater(p - e.yyx)) - densityWater(p));
}

// intersect sphere in the origin
bool iSphere( float3 ro, float3 rd, float rad, out float i1, out float i2 )
{
    float b = 2*dot( ro, rd );
    float c = dot(ro, ro) - rad*rad;
    float h = b*b - 4*c;
    if (h<0)
		return false;

    h=sqrt(h);
    i1 = (-b - h) * 0.5;
    i2 = (-b + h) * 0.5;
    
    // are both intersections behind us?
    float m = max(i1,i2);
    if (m<0)
        return false;

    return true;
}

float raymarch(float3 ro, float3 rd)
{
    float from, range, h, lh = 0;
	float t = 0, delt = 0;

    // march only where necessary
    if (!iSphere(ro, rd, RAYMARCH_RANGE, from, range))
        return 0;

    if(length(ro) > RAYMARCH_RANGE)
        t = from;

    for( int i=0; i<100; i++ )
    {
		if (t > range)
			return 0;

        float3 p = ro + rd * t;

		h = densityTerra(p);

        if( h > 0 )
            return t - delt + delt*abs(lh)/(h-lh);

        delt = max(-h, MIN_RAYMARCH_DELTA);

        t += delt;

        lh = h;
    }

    return 0;
}

float scalef(float fCos)
{
     float x = 1.0 - fCos;
     return m_fRayleighScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

float3 traceScattering( float3 rgb, float3 ro, float3 rd, float3 nor, float ei1, float ai1, float ai2 )
{     
    float m_Kr = 0.0025 * c_scatterStrength;    // Rayleigh scattering constant
    float fKr4PI = m_Kr*4.0*3.1415;
    float m_Km = 0.0010 * c_scatterStrength;    // Mie scattering constant
    float fKm4PI = m_Km*4.0*3.1415;
    float m_ESun = 20.0;                        // Sun brightness constant
    float fKrESun = m_Kr * m_ESun;
    float fKmESun = m_Km * m_ESun;
    float g = -0.990;
    float g2_ren = g*g;
	
    const float3 v3InvWavelength = float3(5.6020, 9.4732, 19.6438);
    const float3 light = normalize(c_sunPosition);

    float fSamples = 16;
    const int nSamples = 16;
    float scale = 1.0 / g_atmosphereWidth;
    float fScaleOverScaleDepth = scale / m_fRayleighScaleDepth;

    float3 posa;

    float fSampleLength;
    float fStartAngle, fStartDepth;

    // we are inside the atmosphere
    if (ai1<0)
    {
        posa = ro;
        if (ei1>0)
            fSampleLength = ei1 / fSamples;
        else
            fSampleLength = ai2 / fSamples;

        fStartAngle = dot(rd, posa) / length(posa);
        fStartDepth = exp(fScaleOverScaleDepth * (c_planetRadius - length(posa)));
    }
    else
    {
        posa = ro + rd * ai1;
        if (ei1>0)
            fSampleLength = (ei1-ai1) / fSamples;
        else
            fSampleLength = (ai2-ai1) / fSamples;

        fStartAngle = dot(rd, posa) / g_atmosphereHeight;
        fStartDepth = exp(-1.0 / m_fRayleighScaleDepth);
    }

    // Initialize the scattering loop variables
    float fScaledLength = fSampleLength * scale;
    float3 v3SampleRay = rd * fSampleLength;

    // is earth hit ?
    if ( ei1>0 )	 
    {
        float fDepth = exp((-g_atmosphereWidth) / m_fRayleighScaleDepth);
        float fCameraAngle = dot(-rd, nor);
        float fLightAngle = dot(light, nor);

        float fCameraScale = scalef(fCameraAngle);
        float fLightScale = scalef(fLightAngle);
        float fCameraOffset = fDepth*fCameraScale;
        float fTemp = (fLightScale + fCameraScale);
             
        // Now loop through the sample rays
        float3 v3FrontColor = 0;
        float3 v3Attenuate;
        for (int i=0; i<nSamples; i++)
        {
            float fHeight = length(posa);
            float fDepth = exp(fScaleOverScaleDepth * (c_planetRadius - fHeight));
            float fScatter = fDepth*fTemp - fCameraOffset;

            v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
            v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
            posa += v3SampleRay;
        }
 
        // Finally, scale the Mie and Rayleigh colors and set up
        float3 rgb2 = v3Attenuate;
        float3 rgb1 = v3FrontColor * (v3InvWavelength * fKrESun + fKmESun);

        // are we in space?
        if (ai1>0)
            ei1-=ai1;

        return lerp(rgb*rgb2, rgb1*0.5 + 0.25*rgb2, saturate((exp(-(200-ei1)*0.08*c_fogAmount))-0.1));
    }

	// let's do the sky     
    float fStartOffset = fStartDepth * scalef(fStartAngle);     
	     
    float3 zz = posa;

    // Now loop through the sample rays
    float3 v3FrontColor = 0;
    for (int i=0; i<nSamples; i++)
    {
        float fHeight = length(posa);
        float fDepth = exp(fScaleOverScaleDepth * (c_planetRadius - fHeight));
        float fLightAngle = dot(light, posa) / fHeight;
        float fCameraAngle = dot(rd, posa) / fHeight;
        float fScatter = (fStartOffset + fDepth*(scalef(fLightAngle) - scalef(fCameraAngle)));

        float3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
        posa += v3SampleRay;
    }

	float3 col2 = v3FrontColor * fKmESun;
	float3 col1 = v3FrontColor * (v3InvWavelength * fKrESun);

    float fCos = dot(light, -rd);
    float fMiePhase = 1.5 * ((1.0 - g2_ren) / (2.0 + g2_ren)) * (1.0 + fCos*fCos) / pow(1.0 + g2_ren - 2.0*g*fCos, 1.5);

    return lerp(0,rgb,(length(zz)-c_planetRadius)/g_atmosphereWidth) + col1 + fMiePhase * col2;
}

float3 getSky(float3 col, float3 ro, float3 rd, float dis)
{
    const float3 cloudCol2 = float3(0.976, 0.956, 0.882);
    const float3 cloudCol1 = float3(0.121, 0.239, 0.349);

	float skyTop = c_planetRadius+g_atmosphereWidth*0.8;

    float d1,d2;
    if (iSphere(ro, rd, skyTop, d1, d2))
    {
		float blend=saturate((length(ro)-skyTop)/20);

        // are we below the clouds
        if (d1<0)
        {
            // if ground is hit no clouds should be rendered
            if(dis>0)
                return col;

            float3 pos = ro + rd * d2;

            float de = saturate(fbm(pos/200) * c_cloudTreshold + c_cloudScale);

            float diffuse = dot(normalize(pos), normalize(c_sunPosition));

            return lerp(lerp(cloudCol2, cloudCol1, de) * diffuse, col, de);

        }
        else if (dis==0)
        {
            float3 pos = ro + rd * d2;

            float de = saturate(fbm(pos/200) * c_cloudTreshold + c_cloudScale);

            float diffuse = dot(normalize(pos), normalize(c_sunPosition));

            col = lerp(lerp(cloudCol2, cloudCol1, de) * diffuse, col, de);

			blend*=saturate((d2-d1)/1000);
        }

        float3 pos = ro + rd * d1;

        float de = saturate(fbm(pos/200) * c_cloudTreshold + c_cloudScale);

        float diffuse = dot(normalize(pos), normalize(c_sunPosition));

        return lerp(col, lerp(cloudCol2, cloudCol1, de) * diffuse, de*blend*de);
    }

    return col;
}

float shadowmarch(float3 ro, float3 rd)
{
    float h,r=1,t=MIN_RAYMARCH_DELTA;

    for( int i=0; i<50; i++ )
    {
        float3 p = ro + rd * t;

        h = densityTerra( p );

        if( h > 0 )
            return 0;

        r = min( r, /*5=shadowamount*/5*abs(h)/t );

        t += max(-h, MIN_RAYMARCH_DELTA);
    }

    return saturate(r);
}

float3 getTerrain( const float3 pos, const float3 nor)
{
    float ph = length(pos);
    float3 col, pnor = pos/ph, ldir = normalize(c_sunPosition.xyz-pos);
    float3 diff = saturate(dot(ldir, nor));
    float angle = saturate(dot(nor, pnor));
    float fh = Fbm3(pnor * c_planetRadius);
    float h = fh/g_maxHillTop;

// shadow
    float ha = c_planetRadius - ph + fh - c_waterLevel;
    float3 p=pos;
    if(ha>0) p+=pnor*ha;
    float sh = shadowmarch(p, ldir);
    diff *= float3( sh, sh*sh*0.5+0.5*sh, sh*sh );

// texture
    float r = abs(snoise(c_textureNoise*pos)+1)*0.5;
    //float r = abs(snoise(c_textureNoise*pos));
    col = (r*0.25 + 0.75) * 0.9 * lerp( c_lowColor, c_hiColor, h );
    col = lerp(col, 0.17*c_dirtColor*(0.50 + 0.50*r), smoothstep(0.70, 0.9, angle));
    col = lerp(col, 0.10*c_grassColor*(0.25 + 0.75*r), smoothstep(0.95, 1.0, angle));
    col *= 0.75;

// snow
    h = smoothstep(55, 80, h*100 + 25*fbm(c_snowFrequency*pos));
    float e = smoothstep(1.0-0.5*h, 1.0-0.1*h, angle);
    float o = 0.3 + 0.7*smoothstep(0.0, 0.1, nor.x+h*h);
    float s = h*e*o;
    s = smoothstep( 0.1, 0.9, s );
    col = lerp( col, 0.4*float3(0.6, 0.65, 0.7), s );

// brdf
    float3 brdf  = 2.0*float3(0.17,0.19,0.20)*angle;
           brdf += 6.0*float3(1.00,0.95,0.80)*diff;
//         brdf += 2.0*float3(0.2,0.2,0.2);

    return saturate(sqrt(col * brdf));
}

float Schlick( float3 vNormal, float3 vView, float fR0, float fSmoothFactor)
{
    float fDot = dot(vNormal, -vView);
    fDot = clamp((1.0 - fDot), 0.0, 1.0);
    float fDotPow = pow(fDot, 5.0);
    return fR0 + (1.0 - fR0) * fDotPow * fSmoothFactor;
}

float3 ApplyFresnel(float3 diffuse, float3 specular, float3 normal, float3 view)
{
    float fresnel = Schlick(normal, view, 0.01, 1);
    return lerp(diffuse, specular, fresnel);
}

float3 getWater( float3 pos, float3 normal, float3 rd)
{
	float3 reflection, refraction, color;

    // Direction of the light
    float3 ld = normalize(c_sunPosition.xyz - pos);

    // get reflection
	float3 red = reflect(rd, normal);

    // The rate at which colors die away in the water
    const float3 extinction = {7, 30, 40};

    float t, depth = saturate(-densityTerra(pos)/(extinction/(c_waterExtinctionRate+1)));

    color = lerp(c_shallowColor, BIGDEPTHCOLOUR, depth);

	if (t = raymarch(pos, red))
	{
		float3 p = pos + red * t;

		reflection = getTerrain(p, getTerrainNormal(p,t));
	}
    else
    {
        float ai1, ai2;

        iSphere( pos, red, g_atmosphereHeight, ai1, ai2);

        reflection = getSky(traceScattering(0, pos, red, 0, 0, ai1, ai2), pos, red, 0);
    }

    // Water transparency along eye vector.
    const float visibility = 0.2f;

    float3 rfd = refract(rd, normal, 0);//1.33

    // check if our refraction vector is a null vector
    if (any(rfd))
    {
        if (t = raymarch(pos, rfd))
        {
			float3 p = pos + rfd * t;

            refraction = getTerrain(p, getTerrainNormal(p,t));

		    float3 depthN = t * c_waterTransparency;

            color = lerp(lerp(refraction, c_shallowColor, saturate(depthN / visibility)),
						    BIGDEPTHCOLOUR, depth);
        }
    }

    return ApplyFresnel(color, reflection, normal, rd);
}


//  float sunAmount = max( dot( rd, normalize(c_sunPosition.xyz) ), 0.0 );
//  float3 spaceColor = float3(0,0,0);
//  float sunPerc = pow(sunAmount, 190);
//  float3 suncolor = lerp(c_sunColor.xyz, float3(1.0, 1.0, 1.0) * 4.0, 0.1);
//	col = lerp(spaceColor, suncolor, sunPerc);
    

/*
static const int _VolumeSteps = 64;
static const float _Density = 1.;

static const float _SphereRadius = 100.2;
static const float _NoiseFreq = .1;
static const float _NoiseAmp = 1.;

static const float4 innerColor = float4(0.7, 0.7, 0.7, _Density);
static const float4 outerColor = float4(1.0, 1.0, 1.0, 0.0);

float fbmz(float3 p)
{
    float f;
    f  = 0.5000*snoise( p ); p = p*2.02;
    f += 0.2500*snoise( p ); p = p*2.03;
    f += 0.1250*snoise( p ); p = p*2.01;
    f += 0.0625*snoise( p ); p = p*2.04;
    f += 0.0312*snoise( p ); p = p*2.02;
    f += 0.0156*snoise( p ); p = p*2.01;
    f += 0.0078*snoise( p ); p = p*2.03;
    f += 0.0039*snoise( p );
    return f;
}

float4 volumeFunc(float3 p)
{
    float d = length(p) - _SphereRadius + fbmz(p*c_cloudTreshold) * c_cloudScale;
	
    return lerp(innerColor, outerColor, smoothstep(0.5, 1.0, d));
}

float3 sky(float3 col, float3 ro, float3 rd, float dis)
{
    float t1,t2,t3,t4,r1,r2;

    // we are outside
    if(length(ro)>c_planetRadius+6)
    {
        r1 = c_planetRadius+6;
        r2 = c_planetRadius+5;
    }
    else
    {
        if (dis>0)
            return col;

        r1 = c_planetRadius+6;
        r2 = c_planetRadius+5;
    }

    if (iSphere(ro,rd,r1,t1,t2))
    {
	    float tnear = t1;
	    float tfar;
	
        if (iSphere(ro,rd,r2,t3,t4))
	        tfar = t3;
	    else
	        tfar = t2;
    
        tnear = max(tnear, 0);
        tfar = max(tfar, 0);

        float3 pnear = ro+rd*tnear;
        float3 pfar = ro+rd*tfar;
	
        ro = pnear;
        float3 rayStep = rd*(length(pfar-pnear)/float(_VolumeSteps));	
	
        float4 rgb = 0;
	
        for (int i=0; i<_VolumeSteps; i++)
        {
            float4 c = volumeFunc(ro);
		
            c.a *= sin(smoothstep(r1,r2,length(ro))*3.1415);
	    	    
            c.rgb *= c.a;		// pre-multiply alpha
            rgb += c*(1.0 - rgb.a);	// over operator for front-to-back
		
            ro += rayStep;
        }

        return lerp(col, rgb.xyz, rgb.w);
    }

    return col;
}
*/
float3 ro()
{
	return c_invViewMatrix[3].xyz;
}


float4 main(const ShaderInput input) : SV_Target0
{
	float3 rd = normalize(mul(float3(2*input.texCoord-1, 1) * c_camFov,(float3x3)c_invViewMatrix));
	float4 rgb = 0;
    float depth2, depth1 = 0;
    float ai1, ai2;

    // intersect atmosphere
    if (iSphere( ro(), rd, g_atmosphereHeight, ai1, ai2))
    {
		float3 nor;
        // intersect planet
        float outt = raymarch(ro(), rd);

        float wi1, wi2;

        // intersect water
		if (iSphere(ro(), rd, c_planetRadius, wi1, wi2) && ( outt==0 || wi1<outt))
        {
            depth1 = wi1;

            float3 pos = ro() + rd * depth1;

            nor = getWaterNormal(pos, depth1);
			rgb.xyz = getWater(pos, nor, rd);
        }
        else if (outt)
	    {
            depth1 = outt;

            float3 pos = ro() + rd * depth1;

            nor = getTerrainNormal(pos, depth1);
    	    rgb.xyz = getTerrain(pos, nor);
	    }
        else
        {
            rgb.xyz = s_texColor.SampleLevel(s_samColor, input.texCoord, 0.0).xyz;
        }

        rgb.xyz = traceScattering(rgb.xyz, ro().xyz, rd, nor, depth1, ai1, ai2);

        rgb.xyz = getSky(rgb.xyz, ro(), rd, depth1);

        if (depth1 == 0)
        {
            // are we in space?
            if (ai1>0)
                depth1 = ai1;
            else
                depth1 = ai2;
        }
    }
 
    // clip against posG
    depth2 = length(mul(float4(s_texPosG.Sample(s_samPosG, input.texCoord).xyz, 1.0), c_invViewMatrix) - ro());

    if (depth1>0 && depth2>0)
	{
		if(depth1 > depth2)
			rgb.xyz = s_texColor.SampleLevel(s_samColor, input.texCoord, 0.0).xyz;

		return float4(rgb.xyz, 1.0f);
	}
	
	if (depth2>0)
		rgb.xyz = s_texColor.SampleLevel(s_samColor, input.texCoord, 0.0).xyz;

    return float4(rgb.xyz, 1.0f);
}