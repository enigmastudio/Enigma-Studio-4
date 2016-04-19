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

eRenderJobQueue::eRenderJobQueue() :
    m_jobsInUse(0),
    m_sorted(eFALSE)
{
}

eRenderJobQueue::~eRenderJobQueue()
{
    for (eU32 i=0; i<m_jobs.size(); i++)
        eDelete(m_jobs[i]);
}

void eRenderJobQueue::clear()
{
    for (eU32 i=0; i<m_jobs.size(); i++)
        m_jobs[i]->insts.clear();

    m_jobsInUse = 0;
    m_sorted = eFALSE;
}

void eRenderJobQueue::add(eGeometry *geo, const eMaterial *mat, const eTransform &transf, eBool castsShadows)
{
    ePROFILER_FUNC();

    eU32 i;
    for (i=0; i<m_jobsInUse; i++)
        if (m_jobs[i]->geo == geo && m_jobs[i]->mat == mat)
            break;

    eInstVtx instVtx;
    instVtx.modelMtx = transf.getMatrix();
    instVtx.normalMtx = transf.getNormalMatrix();
    
    if (i < m_jobsInUse) // job exists already => add instance
    {
        m_jobs[i]->insts.append(instVtx);
    }
    else // job does not exists
    {
        if (m_jobsInUse == m_jobs.size()) // new job needs to be allocated?
            m_jobs.append(new eRenderJob);

        // fill render job
        eRenderJob *job = m_jobs[m_jobsInUse];
        job->geo = geo;
        job->mat = mat;
        job->castsShadows = castsShadows;
        job->insts.append(instVtx);

        m_jobsInUse++;
        m_sorted = eFALSE;
    }
}

void eRenderJobQueue::render(const eCamera &cam, eInt renderWhat, eInt renderFlags)
{
    if (!m_sorted)
    {
        _sort();
        m_sorted = eTRUE;
    }

    for (eU32 i=0; i<m_jobsInUse; i++)
    {
        eRenderJob *job = m_jobs[i];
        eASSERT(job->insts.size() > 0);

        // check if job has to be rendered
        if (job->mat->blending && !(renderWhat & eRJW_ALPHA_ON))
            continue;
        else if (!job->mat->blending && !(renderWhat & eRJW_ALPHA_OFF))
            continue;

        if (job->mat->lighted && !(renderWhat & eRJW_LIGHTED_ON))
            continue;
        else if (!job->mat->lighted && !(renderWhat & eRJW_LIGHTED_OFF))
            continue;

        if (job->castsShadows && !(renderWhat & eRJW_SHADOWS_ON))
            continue;
        else if (!job->castsShadows && !(renderWhat & eRJW_SHADOWS_OFF))
            continue;

        // render geometry
        const eBool useMats = !(renderFlags & eRJF_MATERIALS_OFF);
        if (useMats)
            job->mat->activate();

        cam.activate(job->insts[0].modelMtx);
        eGfx->renderGeometry(job->geo, job->insts);
    }
}

void eRenderJobQueue::_sort()
{
    ePROFILER_FUNC();

    static eArray<eRenderJob *> partition[256];

    for (eU32 i=0; i<32; i+=8)
    {
        // partition phase
        for (eU32 j=0; j<m_jobsInUse; j++)
        {
            const eU32 key = m_jobs[j]->mat->getSortKey();
            const eU32 slot = (key>>i)&0x000000ff;

            partition[slot].append(m_jobs[j]);
        }

        // collection phase
        for (eU32 j=0, l=0; j<256; j++)
        {
            for (eU32 k=0; k<partition[j].size(); k++)
                m_jobs[l++] = partition[j][k];

            partition[j].clear();
        }
    }
}