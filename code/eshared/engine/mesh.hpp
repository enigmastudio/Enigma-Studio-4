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

#ifndef MESH_HPP
#define MESH_HPP

enum eMeshType
{
    eMT_STATIC,
    eMT_DYNAMIC
};

struct eMeshPrimitive
{
    const eMaterial *       mat;
    eU32                    count;
    eU32                    indices[3];
};

class eMesh
{
    friend class eMeshInst;
    friend class eLSys3Op;

private:
    enum ClusterType
    {
        CT_POINT,
        CT_LINE,
        CT_TRIANGLE,
    };

    struct Cluster
    {
        ClusterType         type;
        eArray<eU32>        prims;
        eArray<eFullVtx *>  verts;
        eArray<eU32>        indices;
        const eMaterial *   mat;
        eGeometry *         geo;
    };

public:
    eMesh(eU32 primitiveCount=0, eU32 vertexCount=0);
    ~eMesh();

    void                    addWireCube(const eVector3 verts[8], const eColor &col);

    void                    reserve(eU32 numPrims, eU32 numVerts);
    void                    finishLoading(eMeshType type);
    void                    fromEditMesh(const eEditMesh &em, eMeshType type);
    void                    clear();
    eBool                   isEmpty() const;

	eU32                    addVertex(const eVector3 &pos, const eVector3 &normal, const eVector2 &uv, const eColor &col=eCOL_WHITE);
    eU32                    addVertex(const eVector3 &pos, const eColor &col=eCOL_WHITE);
    eU32                    addTriangle(eU32 vtx0, eU32 vtx1, eU32 vtx2, const eMaterial *mat);
    eU32                    addLine(eU32 startVtx, eU32 endVtx, const eMaterial *mat);
    eU32                    addLine(const eVector3 &pos0, const eVector3 &pos1, const eColor &col, const eMaterial *mat);
    eU32                    addPoint(eU32 vertex, const eMaterial *mat);
    eU32                    addPoint(const eVector3 &pos, const eColor &col, const eMaterial *mat);

    const eFullVtx &        getVertex(eU32 index) const;
    const eMeshPrimitive &  getPrimitive(eU32 index) const;
    eU32                    getVertexCount() const;
    eU32                    getPrimitiveCount() const;
    const eAABB &           getBoundingBox() const;
    eAABB &					getBoundingBox();
	eArray<eFullVtx> &      getVertexArray();

private:
    Cluster &               _findCluster(const eMaterial *mat, ClusterType type);
    eU32                    _addPrimitive(Cluster &cluster, const eU32 *verts, eU32 numVerts);
    void                    _fromTriangleEditMesh(const eEditMesh &em);
    static void             _fillGeoBuffers(eGeometry *geo, ePtr param);

private:
    eArray<eFullVtx>        m_verts;
    eArray<eMeshPrimitive>  m_prims;
    eArray<Cluster>         m_clusters;
    eMeshType               m_type;
    eAABB                   m_bbox;
};

struct eLodMesh
{
    const eMesh *           mesh;
    eF32                    minDist; // minimum distance to camera so that mesh is rendered
};

// instance of a mesh in scene graph
// with LOD support.
class eMeshInst : public eIRenderable
{
public:
    eMeshInst(const eMesh &mesh, eBool castsShadows=eFALSE);
    eMeshInst(const eLodMesh *lodMeshes, eU32 lodLevels, eBool castsShadows=eFALSE);

    virtual void            getRenderJobs(const eTransform &transf, eF32 distToCam, eRenderJobQueue &jobs) const;
    virtual eAABB           getBoundingBox() const;
    virtual eRenderableType getType() const;

    eBool                   getCastsShadows() const;
    eU32                    getLodLevelCount() const;
    const eMesh &           getMesh(eU32 lodLevel) const;

public:
    static const eU32       MAX_LOD_LEVELS = 5;

private:
    eLodMesh                m_lodMeshes[MAX_LOD_LEVELS];
    eU32                    m_lodLevels;
    const eBool             m_castsShadows;
};

#endif // MESH_HPP