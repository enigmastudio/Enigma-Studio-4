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
#include "../math/math.hpp"
#include "engine.hpp"

// implementation of particle system

const eF32 eParticleSystem::MAX_TIME_STEP = 1.0f/30.0f;

eParticleSystem::eParticleSystem() :
    m_particles(MAX_PARTICLES),
    m_lastTime(0.0f),
    m_timer(-1.0f),
    m_emitTime(0.0f),
    m_count(0),
    m_tex(nullptr),
    m_emissionFreq(100.0f),
    m_emissionVel(1.0f),
    m_lifeTime(10.0f),
    m_stretchAmount(0.0f),
    m_gravity(0.0f),
    m_blendSrc(eBM_SRCALPHA),
    m_blendDst(eBM_ONE),
    m_blendOp(eBO_ADD),
    m_randomization(0),
    m_sizePath(nullptr),
    m_colorPath(nullptr),
    m_rotPath(nullptr)
{
    m_gravityConst = (eVector3*)eAllocAlignedAndZero(4 * sizeof(eF32), 16);
    m_geo = eGfx->addGeometry(eGEO_DYNAMIC, eVTX_PARTICLE, eGPT_QUADLIST, _fillGeoBuffers, this);
}

eParticleSystem::~eParticleSystem()
{
    eGfx->removeGeometry(m_geo);
    eFreeAligned(m_gravityConst);
}

// assumes time in second
void eParticleSystem::update(eF32 time)
{
    ePROFILER_FUNC();

    if (time == m_lastTime)
        return;

    eU32 seed = eRandomSeed();

    const eF32 deltaTime = eMax(0.0f, time-m_lastTime);
    m_lastTime = time;
    time = deltaTime;
    m_bbox.clear();
    eF32 timeLeft = time > 1.0f ? 1.0f : time;
    while (timeLeft > 0.0f) {
        if (timeLeft <= MAX_TIME_STEP) {
            time = timeLeft;
            timeLeft = 0.0f;
        } else {
            timeLeft -= MAX_TIME_STEP;
            time = MAX_TIME_STEP;
        }

        const eF32 timeNow = m_timer+time;

        // Add life and delete unused particles.
        eS32 low = 0;
        eS32 high = m_count - 1;
        bool upwards = true;
        while(high >= low) {
            Particle &p = upwards ? m_particles[low] : m_particles[high];
            p.timeToLive -= time*p.timeConstant;
            if(upwards) {
                if(p.timeToLive >= 0.0f)
                    low++;
                else
                    upwards = false;
            } else {
                if(p.timeToLive >= 0.0f) {
                    m_particles[low++] = p;
                    upwards = true;
                }
                high--;
            }
        }
        m_count = high + 1;

        // calculate movement of particles
        _moveParticles(&m_particles[0], m_count, m_timer, time);

        // emit new particles
        if (m_emissionFreq > 0.0f) {
            eF32 emitDelay = 1.0f/m_emissionFreq;
            eF32 relTime = m_timer;
            m_emitTime += time;
            while (m_emitTime >= emitDelay) {
                m_emitTime -= emitDelay;
                eF32 timeRemaining = m_emitTime;
                // emit new particles
                if (m_count < MAX_PARTICLES) {
                    Particle &p = m_particles[m_count];
                    p.rotation = 0;
                    p.size = 1.0f * (1.0f - eRandomF(seed) * m_randomization);
                    p.mass = 1.0f * (1.0f - eRandomF(seed) * m_randomization);
                    eF32 initVel = m_emissionVel * (1.0f - eRandomF(seed) * m_randomization);
                    if (m_emitterMesh == nullptr || m_emitterEntities.size() == 0) {
                        p.dynamicEntity.position.null();
                        p.dynamicEntity.velocity = eVector3(eRandomF(-1.0f, 1.0f, seed), 1.0f, eRandomF(-1.0f, 1.0f, seed))*initVel;
                    } else {
                        // lookup random entity with binary search
                        eF32 a = eRandomF(seed) * m_emitterEmitSurfaceArea;
                        eU32 l = 0;
                        eU32 r = m_emitterEntities.size();
                        while (l < r) {
                            eU32 m = (l + r) >> 1;
                            if (a < m_emitterEntities[m])   r = m;
                            else                            l = m+1;
                        }
                        eU32 f = eClamp((eU32)0, r , m_emitterEntities.size()-1);
                        eF32 w0 = eRandomF(seed);
                        eF32 w0Inv = 1.0f - w0;

                        switch (m_emitterMode)
                        {
                            case ePSEM_FACES:
								m_emitterMesh->getPointOnFace(f, eRandomF(), eRandomF(), p.dynamicEntity.position, p.dynamicEntity.velocity);
                                break;

                            case ePSEM_VERTICES:
                                const eEmWedge &wedge = m_emitterMesh->getWedge(f);
                                p.dynamicEntity.position = m_emitterMesh->getPosition(wedge.posIdx).pos;
                                p.dynamicEntity.velocity = m_emitterMesh->getNormal(wedge.nrmIdx);
                                break;
                        }

                        p.dynamicEntity.velocity.normalize();
                        p.dynamicEntity.velocity *= initVel;
                    }
                        
                    const eF32 lifeTime = m_lifeTime * (1.0f - eRandomF(seed) * m_randomization);
                    p.timeConstant = (lifeTime <= 0.0f) ? 1.0f : 1.0f / lifeTime;
                    p.timeToLive = 1.0f - timeRemaining * p.timeConstant;
                    _moveParticles(&p, 1, timeNow - timeRemaining, timeRemaining);
                    m_count++;
                }
            }
        }
        m_timer = timeNow;
    }

    // update bounding box
    eVector3 min(eF32_INF);
    eVector3 max(-eF32_INF);

    for (eU32 i=0; i<m_particles.size(); i++)
    {
        const Particle &p = m_particles[i];
        min.minComponents(p.dynamicEntity.position);
        max.maxComponents(p.dynamicEntity.position);
    }

    m_bbox.setMinMax(min, max);
}


void eParticleSystem::setEmitter(const eEditMesh *mesh, ePsEmitterMode mode)
{
    m_emitterMode = mode;

    if (mesh)
    {
        m_emitterMesh = mesh;
        m_emitterEmitSurfaceArea = 0.0f;

        switch (mode)
        {
        case ePSEM_FACES:
            m_emitterEntities.resize(m_emitterMesh->getFaceCount());
            for (eU32 i=0; i<m_emitterMesh->getFaceCount(); i++)
            {
                m_emitterEmitSurfaceArea += m_emitterMesh->getFaceArea(i);
                m_emitterEntities[i] = m_emitterEmitSurfaceArea;
            }
            break;

        case ePSEM_VERTICES:
            m_emitterEntities.resize(m_emitterMesh->getWedgeCount());
            for (eU32 i=0; i<m_emitterMesh->getWedgeCount(); i++)
            {
                m_emitterEmitSurfaceArea += 1.0f;
                m_emitterEntities[i] = m_emitterEmitSurfaceArea;
            }
            break;
        }
    }
}

void eParticleSystem::setStretch(eF32 stretch)
{
    m_stretchAmount = stretch;
}

void eParticleSystem::setRandomization(eF32 randomization)
{
    m_randomization = randomization;
}

void eParticleSystem::setEmission(eF32 emissionFreq, eF32 emissionVel)
{
    m_emissionFreq = emissionFreq;
    m_emissionVel = emissionVel;
}

void eParticleSystem::setLifeTime(eF32 lifeTime)
{
    m_lifeTime = lifeTime;
}

void eParticleSystem::setMaterial(eBlendMode blendSrc, eBlendMode blendDst, eBlendOp blendOp, eTexture2d *tex)
{
    m_blendSrc = blendSrc;
    m_blendDst = blendDst;
    m_blendOp = blendOp;
    m_tex = tex;
}

void eParticleSystem::setGravity(eF32 gravity, const eVector3 &gravityConst)
{
    m_gravity = gravity;
    *m_gravityConst = gravityConst;
}

void eParticleSystem::setPaths(const ePath4Sampler *colorPath, const ePath4Sampler *sizePath, const ePath4Sampler *rotPath)
{
    m_colorPath = colorPath;
    m_sizePath = sizePath;
    m_rotPath = rotPath;
}

void eParticleSystem::_moveParticles(Particle *particles, eU32 count, eF32 nowTime, eF32 deltaTime)
{
    const eF32 dtHalf = 0.5f * deltaTime;
    const eF32 dtdtHalf = dtHalf * deltaTime;

    for (eU32 i=0; i<count; i++)
    {
        // time step of verlet integration
        Particle &p = particles[i];

        __m128 pos = _mm_loadu_ps(p.dynamicEntity.position);
        const __m128 oldAccel = _calcAcceleration(p.mass, pos);

        __m128 vel = _mm_loadu_ps(p.dynamicEntity.velocity);
        pos = _mm_add_ps(pos, _mm_mul_ps(vel, _mm_set1_ps(deltaTime)));
        pos = _mm_add_ps(pos, _mm_mul_ps(oldAccel, _mm_set1_ps(dtdtHalf)));

        const __m128 newAccel = _calcAcceleration(p.mass, pos);
        vel = _mm_add_ps(vel, _mm_mul_ps(_mm_set1_ps(dtHalf), _mm_add_ps(newAccel, oldAccel)));

        _mm_storeu_ps(&p.dynamicEntity.position.x, pos);
        _mm_storeu_ps(&p.dynamicEntity.velocity.x, vel);
    }
}

void eParticleSystem::_fillGeoBuffers(eGeometry *geo, ePtr param)
{
    const eParticleSystem *psys = (eParticleSystem *)param;

    if (psys->m_count == 0)
        return;

    eVector3 right, up, view;
    eGfx->getBillboardVectors(right, up, &view);
    eMatrix4x4 mat = eGfx->getModelMatrix();
    view = view*mat;
    right = right*mat;
    up = up*mat;

    view.normalize();
    right.normalize();
    up.normalize();

    eParticleVtx *vertices = nullptr;
    eU32 vtxCount=0;

    eGfx->beginLoadGeometry(geo, MAX_PARTICLES*4, (ePtr *)&vertices);
    {
        for (eU32 i=0; i<psys->m_count; i++)
        {
            const Particle &p = psys->m_particles[i];

            eF32 ptime = 1.0f-p.timeToLive;
            eColor col = eCOL_WHITE;
            eF32 scale = (!psys->m_sizePath ? eSin(p.timeToLive*ePI) : psys->m_sizePath->evaluate(ptime).x);

            if (psys->m_colorPath)
            {
                const eVector4 &res = psys->m_colorPath->evaluate(ptime);
                col.r = eFtoL(res.x);
                col.g = eFtoL(res.y);
                col.b = eFtoL(res.z);
                col.a = eFtoL(res.w);
            }
            else 
                col.a = eFtoL(eClamp(1.0f, eSin(p.timeToLive*ePI), 1.0f)*255.0f);

            scale *= p.size;
            eF32 rot = (!psys->m_rotPath ? 0.0f : psys->m_rotPath->evaluate(ptime).x);

            eVector3 r = right * scale;
            eVector3 u = up * scale;
            eVector3 pos2 = p.dynamicEntity.position;

            if(psys->m_stretchAmount != 0.0f) 
            {
                eVector3 flyDir = p.dynamicEntity.velocity;
                const eVector3 velNorm = p.dynamicEntity.velocity.normalized();
                const eF32 rightCos = view*velNorm;
                const eQuat qr(view, rot);
                const eQuat qr90(view, -eHALFPI);
                r = -((velNorm * qr90)*qr) * scale;
                u = (velNorm * qr) * scale;
                pos2 = p.dynamicEntity.position+p.dynamicEntity.velocity*psys->m_stretchAmount;
            }
            else if(rot != 0)
            {
                r = (r * eQuat(view, rot));
                u = (u * eQuat(view, rot));
            }

            const eVector3 mid = (p.dynamicEntity.position + pos2) * 0.5f;

            vertices[vtxCount+0].set(pos2                     + u, eVector2(0.0f, 0.0f), col);
            vertices[vtxCount+1].set(mid                      + r, eVector2(1.0f, 0.0f), col);
            vertices[vtxCount+2].set(p.dynamicEntity.position - u, eVector2(1.0f, 1.0f), col);
            vertices[vtxCount+3].set(mid                      - r, eVector2(0.0f, 1.0f), col);

            vtxCount += 4;
        }
    }
    eGfx->endLoadGeometry(geo, vtxCount);
}

// implementation of particle system instance

eParticleSysInst::eParticleSysInst(const eParticleSystem &psys) :
    m_psys(psys)
{
    m_mat.depthWrite = eFALSE;
    m_mat.blending = eTRUE;
    m_mat.ps = eGfx->loadPixelShader(eSHADER(ps_particles));
    m_mat.vs = eGfx->loadVertexShader(eSHADER(vs_particles));
}

void eParticleSysInst::getRenderJobs(const eTransform &transf, eF32 distToCam, eRenderJobQueue &jobs) const
{
    m_mat.blendSrc = m_psys.m_blendSrc;
    m_mat.blendDst = m_psys.m_blendDst;
    m_mat.blendOp = m_psys.m_blendOp;
    m_mat.textures[eMTU_DIFFUSE] = m_psys.m_tex;

    jobs.add(m_psys.m_geo, &m_mat, transf, eFALSE);
}

eAABB eParticleSysInst::getBoundingBox() const
{
    return m_psys.m_bbox;
}

eRenderableType eParticleSysInst::getType() const
{
    return eRT_PSYS;
}