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

#ifndef RENDER_JOB_HPP
#define RENDER_JOB_HPP

enum eRenderJobFlags
{
    eRJF_MATERIALS_OFF      = 1,
};

enum eRenderJobWhat
{
    eRJW_ALPHA_ON           = 1,
    eRJW_ALPHA_OFF          = 2,
    eRJW_ALPHA_BOTH         = eRJW_ALPHA_ON | eRJW_ALPHA_OFF,
    eRJW_LIGHTED_ON         = 4,
    eRJW_LIGHTED_OFF        = 8,
    eRJW_LIGHTED_BOTH       = eRJW_LIGHTED_ON | eRJW_LIGHTED_OFF,
    eRJW_SHADOWS_ON         = 16,
    eRJW_SHADOWS_OFF        = 32,
    eRJW_SHADOWS_BOTH       = eRJW_SHADOWS_ON | eRJW_SHADOWS_OFF,
    eRJW_RENDER_ALL         = eRJW_ALPHA_BOTH | eRJW_LIGHTED_BOTH | eRJW_SHADOWS_BOTH,
};

struct eRenderJob
{
    eGeometry *             geo;
    const eMaterial *       mat;
    eArray<eInstVtx>        insts;
    eBool                   castsShadows;
};

// do not allocate on stack because internal
// memory allocaions should be kept between
// frames for performance reasons
class eRenderJobQueue
{
public:
    eRenderJobQueue();
    ~eRenderJobQueue();

    void                    add(eGeometry *geo, const eMaterial *mat, const eTransform &transf, eBool castsShadows);
    void                    clear();
    void                    render(const eCamera &cam, eInt renderWhat, eInt renderFlags=0);

private:
    void                    _sort();

private:
    eArray<eRenderJob *>    m_jobs;
    eU32                    m_jobsInUse;
    eBool                   m_sorted;
};

#endif // RENDER_JOB_HPP