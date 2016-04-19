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

#ifndef SCENE_DATA_HPP
#define SCENE_DATA_HPP

// list of renderables and lights
class eSceneData
{
public:
    eALIGN16 struct Entry
    {
        const eIRenderable *    renderable;
        const eSceneData   *    sceneData;
        eTransform              transf;
    };

public:
    eSceneData();

    void                        merge(const eSceneData &sd, const eTransform &transf=eTransform());
    void                        transform(const eTransform &transf);
    void                        clear();

    void                        addLight(const eLight *light);
    void                        addRenderable(const eIRenderable *ra, const eTransform &transf=eTransform());

    eU32                        getEntryCount() const;
    const Entry &               getEntry(eU32 index) const;
    Entry &                     getEntry(eU32 index);
    const eAABB &               getBoundingBox() const;
    eAABB &                     getBoundingBox();
    eU32                        getLightCount() const;
    const eLight &              getLight(eU32 index) const;
    eU32                        getRenderableTotal() const;

private:
    eAABB                       m_aabb;
    eArray<Entry>               m_entries;
    eArray<const eLight *>      m_lights;
    eU32                        m_renderableTotal;
};

#endif // SCENE_DATA_HPP