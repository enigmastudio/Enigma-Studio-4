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

eSceneData::eSceneData()
    : m_renderableTotal(0)
{
}

void eSceneData::merge(const eSceneData &sd, const eTransform &transf)
{
    for (eU32 i=0; i<sd.getLightCount(); i++)
        m_lights.append(sd.m_lights[i]);

    if (!sd.getBoundingBox().isEmpty())
    {
        Entry &ne = m_entries.append();
        ne.transf = transf;
        ne.sceneData = &sd;
        ne.renderable = nullptr;
        m_renderableTotal += sd.getRenderableTotal();

        eAABB aabb = sd.getBoundingBox();
        aabb.transform(transf);
        m_aabb.merge(aabb);
    }
}

void eSceneData::transform(const eTransform &transf)
{
    m_aabb.clear();

    for (eU32 i=0; i<m_entries.size(); i++)
        m_entries[i].transf *= transf;
}

void eSceneData::clear()
{
    m_renderableTotal = 0;
    m_aabb.clear();
    m_entries.clear();
    m_lights.clear();
}

void eSceneData::addLight(const eLight *light)
{
    m_lights.append(light);
}

void eSceneData::addRenderable(const eIRenderable *ra, const eTransform &transf)
{
    if (!ra->getBoundingBox().isEmpty())
    {
        Entry &ne = m_entries.append();
        ne.transf = transf;
        ne.sceneData = nullptr;
        ne.renderable = ra;
        m_renderableTotal++;

        eAABB aabb = ra->getBoundingBox();
        aabb.transform(transf);
        m_aabb.merge(aabb);
    }
}

eU32 eSceneData::getEntryCount() const
{
    return m_entries.size();
}

const eSceneData::Entry & eSceneData::getEntry(eU32 index) const
{
    eASSERT(index < m_entries.size());
    return m_entries[index];
}

eSceneData::Entry & eSceneData::getEntry(eU32 index)
{
    eASSERT(index < m_entries.size());
    return m_entries[index];
}

const eAABB & eSceneData::getBoundingBox() const
{
    return m_aabb;
}

eAABB & eSceneData::getBoundingBox()
{
    return m_aabb;
}

eU32 eSceneData::getLightCount() const
{
    return m_lights.size();
}

const eLight & eSceneData::getLight(eU32 index) const
{
    eASSERT(index < m_lights.size());
    return *m_lights[index];
}

eU32 eSceneData::getRenderableTotal() const 
{
    return m_renderableTotal;
}