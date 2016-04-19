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

#ifndef SCENE_HPP
#define SCENE_HPP

// spatially organized scene data
class eScene
{
public:
    eScene(const eSceneData &sceneData=eSceneData())
    {
        setSceneData(sceneData);
    }

    void setSceneData(const eSceneData &sceneData)
    {
        m_sceneData.clear();
        m_sceneData.merge(sceneData);
        m_kdTree.construct(sceneData);
    }

    void getRenderJobs(const eCamera &cam, eRenderJobQueue &jobs) const
    {
        m_kdTree.cull(cam, jobs);
    }

    eU32 getLightCount() const
    {
        return m_sceneData.getLightCount();
    }

    const eLight & getLight(eU32 index) const
    {
        return m_sceneData.getLight(index);
    }

    const eSceneData & getSceneData() const
    {
        return m_sceneData;
    }

    const eCuller & getKdTree() const
    {
        return m_kdTree;
    }

private:
    mutable eCuller     m_kdTree;
    eSceneData          m_sceneData;
};

#endif // SCENE_HPP