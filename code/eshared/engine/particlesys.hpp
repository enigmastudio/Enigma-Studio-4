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

#ifndef PARTICLE_SYS_HPP
#define PARTICLE_SYS_HPP

enum ePsEmitterMode
{
    ePSEM_FACES,
    ePSEM_VERTICES
};

class eParticleSystem
{
    friend class eParticleSysInst;

private:
    struct DynamicEntity
    {
        eVector3            position; // [m]
        eVector3            velocity; // [m/s]
    };

    struct Particle
    {
        DynamicEntity       dynamicEntity;
        eF32                timeToLive;   // (0..1]
        eF32                timeConstant; // 1/ttl_max
        eF32                rotation;
        eF32                mass;
        eF32                size;
    };

public:
    eParticleSystem();
    ~eParticleSystem();

    void                    update(eF32 time);

    void                    setEmitter(const eEditMesh *mesh, ePsEmitterMode mode);
    void                    setStretch(eF32 stretch);
    void                    setRandomization(eF32 randomization);
    void                    setEmission(eF32 emissionFreq, eF32 emissionVel);
    void                    setLifeTime(eF32 lifeTime);
    void                    setMaterial(eBlendMode blendSrc, eBlendMode blendDst, eBlendOp blendOp, eTexture2d *tex);
    void                    setGravity(eF32 gravity, const eVector3 &gravityConst);
    void                    setPaths(const ePath4Sampler *colorPath, const ePath4Sampler *sizePath, const ePath4Sampler *rotPath);

private:
    void                    _moveParticles(Particle *particles, eU32 count, eF32 nowTime, eF32 deltaTime);
    static void             _fillGeoBuffers(eGeometry *geo, ePtr param);

private:
    eFORCEINLINE __m128 _calcAcceleration(eF32 mass, const __m128 pos) const
    {
        return _mm_load_ps(&m_gravityConst->x);
    }

private:
    static const eU32       MAX_PARTICLES = 20000;
    static const eF32       MAX_TIME_STEP;

private:
    ePsEmitterMode          m_emitterMode;
    eF32                    m_emissionFreq;
    eF32                    m_emissionVel;
    eF32                    m_lifeTime;
    eF32                    m_randomization;
    eF32                    m_stretchAmount;
    eF32                    m_gravity;
    eVector3 *              m_gravityConst;

    eGeometry *             m_geo;
    eTexture2d *            m_tex;
    eArray<Particle>        m_particles;
    eAABB                   m_bbox;

    eF32                    m_lastTime;
    eF32                    m_timer;
    eF32                    m_emitTime;
    eU32                    m_count;

    const ePath4Sampler *    m_sizePath;
    const ePath4Sampler *    m_colorPath;
    const ePath4Sampler *    m_rotPath;

    const eEditMesh *       m_emitterMesh;
    eF32                    m_emitterEmitSurfaceArea;
    eArray<eF32>            m_emitterEntities;

    eBlendMode              m_blendSrc;
    eBlendMode              m_blendDst;
    eBlendOp                m_blendOp;
};

// instance of a particle system for scene graph
class eParticleSysInst : public eIRenderable
{ 
public:
    eParticleSysInst(const eParticleSystem &psys);

    virtual void            getRenderJobs(const eTransform &transf, eF32 distToCam, eRenderJobQueue &jobs) const;
    virtual eAABB           getBoundingBox() const;
    virtual eRenderableType getType() const;

private:
    const eParticleSystem & m_psys;
    mutable eMaterial       m_mat;
};

#endif // PARTICLE_SYS_HPP