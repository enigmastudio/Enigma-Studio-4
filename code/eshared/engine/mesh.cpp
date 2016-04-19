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

// implementation of mesh

eMesh::eMesh(eU32 primitiveCount, eU32 numVerts)
{
    reserve(primitiveCount, numVerts);
}

eMesh::~eMesh()
{
    clear();
}

// expects vertices in the following order:
// ulh, ulv, urv, urh, olh, olv, orv, orh.
// if vertices is null default vertices are
// used (which is a cube centered at origin
// with size 1/1/1.
void eMesh::addWireCube(const eVector3 verts[8], const eColor &col)
{
    const eF32 defVerts[24] = {-1, -1, -1, -1, -1, 1, 1, -1, 1, 1, -1, -1, -1, 1, -1, -1, 1, 1, 1, 1, 1, 1, 1, -1};
    const eU32 indices[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
    const eU32 oldVtxCount = m_verts.size();

    reserve(12, 8);

    for (eU32 i=0; i<8; i++)
    {
        const eVector3 v = (verts ? verts[i] : eVector3(defVerts[i*3], defVerts[i*3+1], defVerts[i*3+2])*0.5f);
        addVertex(v, col);
    }

    for (eU32 i=0; i<24; i+=2)
        addLine(oldVtxCount+indices[i], oldVtxCount+indices[i+1], eMaterial::getWireframe());
}

void eMesh::reserve(eU32 numPrims, eU32 numVerts)
{
    m_verts.reserve(numVerts);
    m_prims.reserve(numPrims);
}

// has to be called after adding geometry
void eMesh::finishLoading(eMeshType type)
{
    ePROFILER_FUNC();

    m_type = type;
    eArray<eU32> posMap(m_verts.size());

    for (eU32 i=0; i<m_clusters.size(); i++)
    {
        Cluster &cluster = m_clusters[i];
        const eU32 primSize = cluster.type+1; // type+1 is equal to size
        eMemSet(&posMap[0], 0xff, posMap.size()*sizeof(eU32));
		cluster.indices.reserve(cluster.prims.size()*primSize);
		cluster.verts.reserve(cluster.prims.size()*primSize);

        // create data for index and vertex buffer
        for (eU32 j=0, numVerts=0; j<cluster.prims.size(); j++)
        {
            for (eU32 k=0; k<primSize; k++)
            {
                const eU32 vtxIndex = m_prims[cluster.prims[j]].indices[k];
                if (posMap[vtxIndex] == 0xffffffff)
                {
                    posMap[vtxIndex] = numVerts++;
                    cluster.verts.append(&m_verts[vtxIndex]);
                }

                cluster.indices.append(posMap[vtxIndex]);
            }
        }

        eASSERT(cluster.indices.size() == cluster.prims.size()*primSize);

        // create geometry
        const eGeoPrimitiveType primTypes[3] = {eGPT_QUADLIST, eGPT_LINELIST, eGPT_TRILIST};
        eInt flags = (m_type == eMT_DYNAMIC || cluster.type == CT_POINT ? eGEO_DYNAMIC : eGEO_STATIC);
        flags |= (cluster.type != CT_POINT ? eGEO_IB32 : 0);
        eGfx->removeGeometry(cluster.geo);
        cluster.geo = eGfx->addGeometry(flags, eVTX_FULL, primTypes[cluster.type], _fillGeoBuffers, &cluster);
    }

    // upload already if mesh is static
    if (m_type == eMT_STATIC)
        for (eU32 i=0; i<m_clusters.size(); i++)
            _fillGeoBuffers(m_clusters[i].geo, (ePtr)&m_clusters[i]);
}

void eMesh::fromEditMesh(const eEditMesh &em, eMeshType type)
{
    if (em.isTriangulated())
        _fromTriangleEditMesh(em);
    else
    {
        eEditMesh triMesh(em);
        triMesh.triangulate();
        _fromTriangleEditMesh(triMesh);
    }

    finishLoading(type);
}

void eMesh::clear()
{
    m_bbox.clear();
    m_verts.clear();
    m_prims.clear();

    for (eU32 i=0; i<m_clusters.size(); i++)
    {
        Cluster &cluster = m_clusters[i];
        cluster.prims.free();
        cluster.verts.free();
        cluster.indices.free();
        eGfx->removeGeometry(cluster.geo);
    }
    
    m_clusters.clear();
}

eBool eMesh::isEmpty() const
{
    return m_prims.isEmpty();
}

eU32 eMesh::addVertex(const eVector3 &pos, const eVector3 &normal, const eVector2 &uv, const eColor &col)
{
    eFullVtx &vtx = m_verts.append();
    vtx.set(pos, normal, uv, col);
	m_bbox.updateExtent(pos);
    return m_verts.size()-1;
}

eU32 eMesh::addVertex(const eVector3 &pos, const eColor &col)
{
    return addVertex(pos, eVector3(), eVector2(), col);
}

eU32 eMesh::addTriangle(eU32 vtx0, eU32 vtx1, eU32 vtx2, const eMaterial *mat)
{
    const eU32 vertices[3] = { vtx0, vtx1, vtx2 };
    return _addPrimitive(_findCluster(mat, CT_TRIANGLE), vertices, 3);
}

eU32 eMesh::addLine(eU32 startVtx, eU32 endVtx, const eMaterial *mat)
{
    const eU32 vertices[2] = { startVtx, endVtx };
    return _addPrimitive(_findCluster(mat, CT_LINE), vertices, 2);
}

eU32 eMesh::addLine(const eVector3 &pos0, const eVector3 &pos1, const eColor &col, const eMaterial *mat)
{
    const eU32 vtx0 = addVertex(pos0, col);
    const eU32 vtx1 = addVertex(pos1, col);
    return addLine(vtx0, vtx1, mat);
}

eU32 eMesh::addPoint(eU32 vertex, const eMaterial *mat)
{
    return _addPrimitive(_findCluster(mat, CT_POINT), &vertex, 1);
}

eU32 eMesh::addPoint(const eVector3 &pos, const eColor &col, const eMaterial *mat)
{
    return addPoint(addVertex(pos, col), mat);
}

const eFullVtx & eMesh::getVertex(eU32 index) const
{
    eASSERT(index < m_verts.size());
    return m_verts[index];
}

const eMeshPrimitive & eMesh::getPrimitive(eU32 index) const
{
    eASSERT(index < m_prims.size());
    return m_prims[index];
}

eU32 eMesh::getVertexCount() const
{
    return m_verts.size();
}

eU32 eMesh::getPrimitiveCount() const
{
    return m_prims.size();
}

const eAABB & eMesh::getBoundingBox() const
{
    return m_bbox;
}

eAABB & eMesh::getBoundingBox()
{
    return m_bbox;
}

eMesh::Cluster & eMesh::_findCluster(const eMaterial *mat, ClusterType type)
{
    for (eU32 i=0; i<m_clusters.size(); i++)
        if (m_clusters[i].mat == mat && m_clusters[i].type == type)
            return m_clusters[i];

    Cluster &cluster = m_clusters.append();
    cluster.type = type;
    cluster.mat = mat;
    cluster.geo = nullptr;
    return cluster;
}

eU32 eMesh::_addPrimitive(Cluster &cluster, const eU32 *verts, eU32 numVerts)
{
    eASSERT(numVerts <= 3);

    eMeshPrimitive &prim = m_prims.append();
    prim.mat = cluster.mat;
    prim.count = numVerts;

    for (eU32 i=0; i<numVerts; i++)
    {
        eASSERT(verts[i] < m_verts.size());
        prim.indices[i] = verts[i];
    }

    const eU32 index = m_prims.size()-1;
    cluster.prims.append(index);
    return index;
}

void eMesh::_fromTriangleEditMesh(const eEditMesh &em)
{
    ePROFILER_FUNC();
    eASSERT(em.isTriangulated());

    clear();
    reserve(em.getFaceCount(), em.getFaceCount()*3); 
    m_bbox = em.getBoundingBox();

    for (eU32 i=0; i<em.getWedgeCount(); i++)
    {
        const eEmWedge &wedge = em.getWedge(i);
        const eVector3 &pos = em.getPosition(wedge.posIdx).pos;
        const eVector3 &nrm = em.getNormal(wedge.nrmIdx);
        const eEmVtxProps &props = em.getProperty(wedge.propsIdx);

        addVertex(pos, nrm, props.uv, props.col);
    }

    for (eU32 i=0; i<em.getFaceCount(); i++)
    {
        const eEmFace &face = em.getFace(i);
        eASSERT(face.count == 3);
        addTriangle(face.wdgIdx[0], face.wdgIdx[1], face.wdgIdx[2], face.mat);
    }
}

void eMesh::_fillGeoBuffers(eGeometry *geo, ePtr param)
{
    ePROFILER_FUNC();

    Cluster *cluster = (Cluster *)param;
    eFullVtx *vp = nullptr;
    eU32 *ip = nullptr;

    if (cluster->type == CT_POINT)
    {
        eVector3 s, t;
        eGfx->getBillboardVectors(s, t);
        const eVector3 n = s^t;
        const eVector3 r = s*cluster->mat->pointSize;
        const eVector3 u = t*cluster->mat->pointSize;

        eGfx->beginLoadGeometry(geo, cluster->verts.size()*4, (ePtr *)&vp);
        {
            for (eU32 i=0; i<cluster->verts.size(); i++, vp+=4)
            {
                const eVector3 &pos = cluster->verts[i]->pos;
                const eColor &col = cluster->verts[i]->col;

                vp[0].set(pos-r-u, n, eVector2(0.0f, 0.0f), col);
                vp[1].set(pos-r+u, n, eVector2(0.0f, 1.0f), col);
                vp[2].set(pos+r+u, n, eVector2(1.0f, 1.0f), col);
                vp[3].set(pos+r-u, n, eVector2(1.0f, 0.0f), col);
            }
        }
        eGfx->endLoadGeometry(geo);
    }
    else
    {
        eGfx->beginLoadGeometry(geo, cluster->verts.size(), (ePtr *)&vp, cluster->indices.size(), (ePtr *)&ip);
        {
            for (eU32 j=0; j<cluster->verts.size(); j++)
                vp[j] = *cluster->verts[j];

            eMemCopy(ip, &cluster->indices[0], cluster->indices.size()*sizeof(eU32));
        }
        eGfx->endLoadGeometry(geo);
    }
}

eArray<eFullVtx> & eMesh::getVertexArray()
{
	return m_verts;
}

// implementation of mesh instance

eMeshInst::eMeshInst(const eMesh &mesh, eBool castsShadows) :
    m_castsShadows(castsShadows),
    m_lodLevels(1)
{
    m_lodMeshes[0].mesh = &mesh;
    m_lodMeshes[0].minDist = 0.0f;
}

eMeshInst::eMeshInst(const eLodMesh *lodMeshes, eU32 lodLevels, eBool castsShadows) :
    m_castsShadows(castsShadows),
    m_lodLevels(lodLevels)
{
    eASSERT(lodLevels >= 1 && lodLevels <= MAX_LOD_LEVELS);
    eMemCopy(m_lodMeshes, lodMeshes, lodLevels*sizeof(eLodMesh));
}

void eMeshInst::getRenderJobs(const eTransform &transf, eF32 distToCam, eRenderJobQueue &jobs) const
{
    // find correct LOD level
    eF32 maxDist = 0.0f;
    eU32 lodIndex = 0;

    for (eU32 i=0; i<m_lodLevels; i++)
    {
        if (distToCam >= maxDist && distToCam >= m_lodMeshes[i].minDist)
        {
            maxDist = m_lodMeshes[i].minDist;
            lodIndex = i;
        }
    }

    // add render jobs of LOD level
    const eMesh *mesh = m_lodMeshes[lodIndex].mesh;

    for (eU32 i=0; i<mesh->m_clusters.size(); i++)
    {
        const eMesh::Cluster &cluster = mesh->m_clusters[i];
        jobs.add(cluster.geo, cluster.mat, transf, m_castsShadows);
    }
}

eAABB eMeshInst::getBoundingBox() const
{
    eAABB allAabb;
    for (eU32 i=0; i<m_lodLevels; i++)
        allAabb.merge(m_lodMeshes[i].mesh->getBoundingBox());

    return allAabb;
}

eRenderableType eMeshInst::getType() const
{
    return eRT_MESH;
}

eBool eMeshInst::getCastsShadows() const
{
    return m_castsShadows;
}

eU32 eMeshInst::getLodLevelCount() const
{
    return m_lodLevels;
}

const eMesh & eMeshInst::getMesh(eU32 lodLevel) const
{
    eASSERT(lodLevel < m_lodLevels);
    return *m_lodMeshes[lodLevel].mesh;
}