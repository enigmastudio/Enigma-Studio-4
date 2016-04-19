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

eCuller::eCuller(const eSceneData &sd)
{
    construct(sd);
}

void eCuller::construct(const eSceneData &sd)
{
    ePROFILER_FUNC();
	m_calculatedRecords.clear();
	m_calculatedRecords.reserve(sd.getRenderableTotal() + 1);
	Record& r = m_calculatedRecords.append();
	r.m_uncalculatedSD = &sd;
	r.m_aabb = sd.getBoundingBox();
	r.m_transf = eTransform();
}

void eCuller::cull(const eCamera &cam, eRenderJobQueue &jobs)
{
    ePROFILER_FUNC();
    jobs.clear();
	eU32 pos = 0;
	while(pos < m_calculatedRecords.size()) {
		const Record& r = m_calculatedRecords[pos];
		if (cam.intersectsAabb(r.m_aabb)) {
			if(r.m_uncalculatedSD != nullptr) {
				_construct(*r.m_uncalculatedSD, r.m_transf);
				// we need to evaluate the children
				m_calculatedRecords[pos] = m_calculatedRecords[m_calculatedRecords.size() - 1];
				m_calculatedRecords.removeLast();
				pos--;
			} else {
				// a renderable
				const eF32 distToCam = cam.getWorldPos().distance(r.m_aabb.getCenter());
				r.m_renderable->getRenderJobs(r.m_transf, distToCam, jobs);
			}
		};
		pos++;
	}
}

void eCuller::_construct(const eSceneData &sd, const eTransform &transf)
{
    for (eU32 i=0; i<sd.getEntryCount(); i++)
    {
        const eSceneData::Entry &entry = sd.getEntry(i);

		Record& r = m_calculatedRecords.append();
		r.m_transf = entry.transf*transf;
        if (entry.renderable)
        {
			r.m_renderable = entry.renderable;
			r.m_aabb = entry.renderable->getBoundingBox();
			r.m_uncalculatedSD = nullptr;
        } else {
			r.m_uncalculatedSD = entry.sceneData;
			r.m_aabb = entry.sceneData->getBoundingBox();
		}
		r.m_aabb.transform(r.m_transf.getMatrix());
    }

}