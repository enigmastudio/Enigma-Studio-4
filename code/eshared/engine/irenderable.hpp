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

#ifndef IRENDERABLE_HPP
#define IRENDERABLE_HPP

enum eRenderableType
{
    eRT_MESH,
    eRT_PSYS,
};

class eIRenderable
{
public:
    virtual ~eIRenderable()
    {
    }

    virtual void            getRenderJobs(const eTransform &transf, eF32 distToCam, eRenderJobQueue &jobs) const = 0;
    virtual eAABB           getBoundingBox() const = 0;
    virtual eRenderableType getType() const = 0;
};

#endif // IRENDERABLE_HPP