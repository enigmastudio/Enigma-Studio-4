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

#ifndef CULLER_HPP
#define CULLER_HPP

class eCuller
{
public:
    eCuller(const eSceneData &sd=eSceneData());

    void                            construct(const eSceneData &sd);
    void                            cull(const eCamera &cam, eRenderJobQueue &jobs);

private:
    void                            _construct(const eSceneData &sd, const eTransform &transf);

public:
	struct Record {
		const eSceneData *		m_uncalculatedSD;
		eTransform              m_transf;
		eAABB                   m_aabb;
		const eIRenderable *    m_renderable;
	};

	eArray<Record>					m_calculatedRecords;
};

#endif // CULLER_HPP