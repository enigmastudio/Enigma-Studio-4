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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <gl/glu.h>

#include "../system/system.hpp"
#include "../math/math.hpp"
#include "engine.hpp"

eEditMesh::eEditMesh(eU32 vtxCount, eU32 faceCount) :
    m_triangulated(eTRUE),
    m_closed(eTRUE)
{
    reserve(vtxCount, faceCount);
}

void eEditMesh::merge(const eEditMesh &em, const eTransform &trans)
{
    eASSERT(&em != this);

    const eU32 oldNumPos = m_vtxPos.size();
    const eU32 oldNumProps = m_vtxProps.size();
    const eU32 oldNumNrms = m_vtxNrms.size();
    const eU32 oldNumWdgs = m_wedges.size();

    const eMatrix4x4 mtxPos = trans.getMatrix();
    const eMatrix3x3 mtxNrm = trans.getNormalMatrix();

    for (eU32 i=0; i<em.m_vtxPos.size(); i++)
    {
        eEmVtxPos &vtxPos = m_vtxPos.append(em.m_vtxPos[i]);
        vtxPos.pos *= mtxPos;
        m_bbox.updateExtent(vtxPos.pos);
    }

    m_vtxNrms.append(em.m_vtxNrms);
    m_vtxProps.append(em.m_vtxProps);

    for (eU32 i=0; i<m_vtxNrms.size(); i++)
        m_vtxNrms[i] *= mtxNrm;

    for (eU32 i=0; i<em.m_wedges.size(); i++)
    {
        eEmWedge &wedge = m_wedges.append(em.m_wedges[i]);
        wedge.posIdx += oldNumPos;
        wedge.nrmIdx += oldNumNrms;
        wedge.propsIdx += oldNumProps;
    }

    for (eU32 i=0; i<em.m_faces.size(); i++)
    {
        eEmFace &face = m_faces.append(em.m_faces[i]);
        face.normal *= mtxNrm;

        for (eU32 j=0; j<face.count; j++)
        {
            face.wdgIdx[j] += oldNumWdgs;
            face.posIdx[j] += oldNumPos;
        }
    }

    m_triangulated &= em.m_triangulated;
}

void eEditMesh::reserve(eU32 vtxCount, eU32 faceCount)
{
    m_vtxPos.reserve(vtxCount);
    m_vtxNrms.reserve(vtxCount);
    m_vtxProps.reserve(vtxCount);
    m_wedges.reserve(vtxCount);
    m_faces.reserve(faceCount);
}

void eEditMesh::clear()
{
    m_vtxPos.clear();
    m_vtxNrms.clear();
    m_vtxProps.clear();
    m_wedges.clear();
    m_faces.clear();
    m_bbox.clear();
    m_triangulated = eTRUE;
}

void eEditMesh::free()
{
    m_vtxPos.free();
    m_vtxNrms.free();
    m_vtxProps.free();
    m_wedges.free();
    m_faces.free();
    m_triangulated = eTRUE;
}

void eEditMesh::triangulate()
{
    if (m_triangulated)
        return;

    eU32 triCount = 0;
    for (eU32 i=0; i<m_faces.size(); i++)
        triCount += m_faces[i].count-2;

    eArray<eEmFace> triangles;
    triangles.reserve(triCount);

    for (eU32 i=0; i<m_faces.size(); i++)
    {
        eEmFace &face = m_faces[i];

        if (face.count == 3)
            triangles.append(face);
        else
        {
            for (eU32 j=2; j<face.count; j++)
            {
                eEmFace &newFace = triangles.append(face);
                newFace.count = 3;
                newFace.wdgIdx[1] = newFace.wdgIdx[j-1];
                newFace.wdgIdx[2] = newFace.wdgIdx[j];
                newFace.posIdx[1] = newFace.posIdx[j-1];
                newFace.posIdx[2] = newFace.posIdx[j];
            }

            face.count = 3;
        }
    }

    m_faces = triangles;
    m_triangulated = eTRUE;
}

void eEditMesh::calcAdjacency()
{
    eArray<eU32> unpaired;
    unpaired.reserve(m_faces.size()*eEmFace::MAX_DEGREE); // very conservative
    m_edges.clear();
    m_edges.reserve(m_faces.size()*3);

    for (eU32 i=0; i<m_faces.size(); i++)
    {
        eEmFace &face = m_faces[i];
        const eU32 oldEdgeCount = m_edges.size();

        for (eU32 j=0; j<face.count; j++)
        {
            face.edges[j] = m_edges.size();
            
            eEmEdge &edge = m_edges.append();
            edge.temp = 0;
            edge.startPos = face.posIdx[j];
            edge.endPos = face.posIdx[(j+1)%face.count];
            edge.startWdg = face.wdgIdx[j];
            edge.endWdg = face.wdgIdx[(j+1)%face.count];
            edge.face = i;
            edge.edge = j;
            edge.next = oldEdgeCount+(j+1)%face.count;
            edge.prev = oldEdgeCount+(j+face.count-1)%face.count;
            edge.twin = -1;

            // try to find twin for current edge
            for (eU32 k=0; k<unpaired.size(); k++) // fast as unpaired's size is usually little
            {
                const eU32 p = unpaired[k];
                eEmEdge &pe = m_edges[p];

                if (pe.startPos == edge.endPos && pe.endPos == edge.startPos)
                {
                    edge.twin = p;
                    pe.twin = m_edges.size()-1;
                    unpaired.removeSwap(k);
                    break;
                }
            }

            if (edge.twin == -1)
                unpaired.append(m_edges.size()-1);
        }
    }

    // is mesh closed or not?
    m_closed = unpaired.isEmpty();
}

void eEditMesh::calcNormals()
{
    for (eU32 i=0; i<m_vtxNrms.size(); i++)
        m_vtxNrms[i].null();

    for (eU32 i=0; i<m_faces.size(); i++)
    {
        eEmFace &face = m_faces[i];
        eASSERT(face.count >= 3);

        const eVector3 &a = m_vtxPos[face.posIdx[0]].pos;
        const eVector3 &b = m_vtxPos[face.posIdx[1]].pos;
        const eVector3 &c = m_vtxPos[face.posIdx[2]].pos;
        face.normal = ((c-b)^(a-b)).normalized();

        for (eU32 j=0; j<face.count; j++)
            m_vtxNrms[m_wedges[face.wdgIdx[j]].nrmIdx] += face.normal;
    }

    for (eU32 i=0; i<m_vtxNrms.size(); i++)
        m_vtxNrms[i].normalize();
}

void eEditMesh::calcAvgNormals()
{
    for (eU32 i=0; i<m_vtxPos.size(); i++)
        m_vtxPos[i].avgNormal.null();

    for (eU32 i=0; i<m_wedges.size(); i++) 
    {
        eEmWedge &wedge = m_wedges[i];
        m_vtxPos[wedge.posIdx].avgNormal += m_vtxNrms[wedge.nrmIdx];
    }

    for (eU32 i=0; i<m_vtxPos.size(); i++)
        m_vtxPos[i].avgNormal.normalize();
}

void eEditMesh::calcBoundingBox()
{
    m_bbox.clear();

    for (eU32 i=0; i<m_vtxPos.size(); i++)
        m_bbox.updateExtent(m_vtxPos[i].pos);
}

void eEditMesh::center()
{
    for (eU32 i=0; i<m_vtxPos.size(); i++)
        m_vtxPos[i].pos -= m_bbox.getCenter();

    getBoundingBox().translate(-m_bbox.getCenter());
}

void eEditMesh::setBoundingBox(const eAABB &bbox)
{
    m_bbox = bbox;
}

// merges duplicate positions
void eEditMesh::unifyPositions()
{
    eArray<eU32> idxMap(m_vtxPos.size()), vtxMap(m_vtxPos.size());
    eU32 numUniqueVerts = 0;

    for (eU32 i=0; i<m_vtxPos.size(); i++)
    {
        eInt foundAt = -1;

        for (eU32 j=0; j<i && foundAt==-1; j++)
            if (m_vtxPos[i].pos == m_vtxPos[j].pos)
                foundAt = j;

        if (foundAt == -1)
        {
            vtxMap[numUniqueVerts] = i;
            idxMap[i] = numUniqueVerts++;
        }
        else
            idxMap[i] = idxMap[foundAt];
    }

    if (numUniqueVerts != m_vtxPos.size())
    {
        for (eU32 i=0; i<m_faces.size(); i++)
            for (eU32 j=0; j<m_faces[i].count; j++)
                m_faces[i].posIdx[j] = idxMap[m_faces[i].posIdx[j]];

        for (eU32 i=0; i<m_wedges.size(); i++)
            m_wedges[i].posIdx = idxMap[m_wedges[i].posIdx];

        for (eU32 i=0; i<numUniqueVerts; i++)
            m_vtxPos[i] = m_vtxPos[vtxMap[i]];

        m_vtxPos.resize(numUniqueVerts);
    }
}

void eEditMesh::tidyUp()
{
    eU32 *mapPos = eALLOC_STACK(eU32, m_vtxPos.size());
    eU32 *mapWdgs = eALLOC_STACK(eU32, m_wedges.size());
    eU32 *mapNrms = eALLOC_STACK(eU32, m_vtxNrms.size());
    eU32 *mapProps = eALLOC_STACK(eU32, m_vtxProps.size());

    eMemSet(mapPos, 0, sizeof(eU32)*m_vtxPos.size());
    eMemSet(mapWdgs, 0, sizeof(eU32)*m_wedges.size());
    eMemSet(mapNrms, 0, sizeof(eU32)*m_vtxNrms.size());
    eMemSet(mapProps, 0, sizeof(eU32)*m_vtxProps.size());

    // which elements are in use?
    for (eU32 i=0; i<m_faces.size(); i++)
    {
        const eEmFace &face = m_faces[i];
        for (eU32 j=0; j<face.count; j++)
        {
            eEmWedge &wdg = m_wedges[face.wdgIdx[j]];
            wdg.temp = 0; // not visited

            eASSERT(wdg.posIdx == face.posIdx[j]);

            mapWdgs[face.wdgIdx[j]] = 1;
            mapPos[wdg.posIdx] = 1;
            mapNrms[wdg.nrmIdx] = 1;
            mapProps[wdg.propsIdx] = 1;
        }
    }

    // push together used elements, discard rest
    eU32 numPos = 0;
    eU32 numWdgs = 0;
    eU32 numNrms = 0;
    eU32 numProps = 0;

    for (eU32 i=0; i<m_vtxPos.size(); i++)
    {
        if (mapPos[i])
        {
            m_vtxPos[numPos] = m_vtxPos[i];
            mapPos[i] = numPos++;
        }
    }

    for (eU32 i=0; i<m_wedges.size(); i++)
    {
        if (mapWdgs[i])
        {
            m_wedges[numWdgs] = m_wedges[i];
            mapWdgs[i] = numWdgs++;
        }
    }

    for (eU32 i=0; i<m_vtxNrms.size(); i++)
    {
        if (mapNrms[i])
        {
            m_vtxNrms[numNrms] = m_vtxNrms[i];
            mapNrms[i] = numNrms++;
        }
    }

    for (eU32 i=0; i<m_vtxProps.size(); i++)
    {
        if (mapProps[i])
        {
            m_vtxProps[numProps] = m_vtxProps[i];
            mapProps[i] = numProps++;
        }
    }

    m_vtxPos.resize(numPos);
    m_wedges.resize(numWdgs);
    m_vtxNrms.resize(numNrms);
    m_vtxProps.resize(numProps);

    // remap elements
    for (eU32 i=0; i<m_faces.size(); i++)
    {
        eEmFace &face = m_faces[i];
        for (eU32 j=0; j<face.count; j++)
        {
            face.wdgIdx[j] = mapWdgs[face.wdgIdx[j]];
            face.posIdx[j] = mapPos[face.posIdx[j]];

            eEmWedge &wdg = m_wedges[face.wdgIdx[j]];
            if (!wdg.temp) // yet visited (faces share wedges)?
            {
                wdg.posIdx = mapPos[wdg.posIdx];
                wdg.nrmIdx = mapNrms[wdg.nrmIdx];
                wdg.propsIdx = mapProps[wdg.propsIdx];
                wdg.temp = 1;
            }
        }
    }
}

void eEditMesh::setMaterial(const eMaterial *mat)
{
    for (eU32 i=0; i<m_faces.size(); i++)
        m_faces[i].mat = mat;
}

eU32 eEditMesh::addPosition(const eVector3 &pos)
{
    eEmVtxPos &vtx = m_vtxPos.append();
    vtx.pos = pos;
    vtx.selected = eFALSE;
    vtx.tag = 0;
    vtx.temp = 0;
    return m_vtxPos.size()-1;
}

eU32 eEditMesh::addNormal(const eVector3 &normal)
{
    m_vtxNrms.append(normal);
    return m_vtxNrms.size()-1;
}

eU32 eEditMesh::addProperty(const eVector2 &uv, const eColor &col)
{
    eEmVtxProps &vtxProps = m_vtxProps.append();
    vtxProps.uv = uv;
    vtxProps.col = col;
    return m_vtxProps.size()-1;
}

eU32 eEditMesh::addWedge(eU32 posIdx, eU32 nrmIdx, eU32 propsIdx)
{
    eEmWedge &wedge = m_wedges.append();
    wedge.posIdx = posIdx;
    wedge.nrmIdx = nrmIdx;
    wedge.propsIdx = propsIdx;
    return m_wedges.size()-1;
}

eU32 eEditMesh::addWedge(const eVector3 &pos, const eVector3 &normal, const eVector2 &uv, const eColor &col)
{
    return addWedge(addPosition(pos), addNormal(normal), addProperty(uv, col));
}

eU32 eEditMesh::addFace(const eU32 *wedges, eU32 count, const eMaterial *mat)
{
    eASSERT(count >= 3 && count <= eEmFace::MAX_DEGREE);

    eEmFace &face = m_faces.append();
    face.mat = (mat ? mat : eMaterial::getDefault());
    face.temp = 0;
    face.tag = 0;
    face.count = count;
    face.selected = eFALSE;

    for (eU32 i=0; i<count; i++)
    {
        face.wdgIdx[i] = wedges[i];
        face.posIdx[i] = m_wedges[wedges[i]].posIdx;
    }

    m_triangulated = (m_triangulated && count == 3);
    return m_faces.size()-1;
}

eU32 eEditMesh::addQuad(eU32 w0, eU32 w1, eU32 w2, eU32 w3, const eMaterial *mat)
{
    const eU32 wedges[4] = {w0, w1, w2, w3};
    return addFace(wedges, 4, mat);
}

eU32 eEditMesh::addTriangle(eU32 w0, eU32 w1, eU32 w2, const eMaterial *mat)
{
    const eU32 wedges[3] = {w0, w1, w2};
    return addFace(wedges, 3, mat);
}

// assumes indices are sorted in descending order
void eEditMesh::removeFaces(const eArray<eU32> &indices)
{
    for (eU32 i=0; i<indices.size(); i++)
    {
        eASSERT(indices[i] < m_faces.size());
        m_faces.removeSwap(indices[i]);
    }

    tidyUp();
}

void eEditMesh::clearPositions()
{
    m_vtxPos.clear();
}

void eEditMesh::clearNormals()
{
    m_vtxNrms.clear();
}

void eEditMesh::clearProperties()
{
    m_vtxProps.clear();
}

void eEditMesh::clearWedges()
{
    m_wedges.clear();
}

void eEditMesh::clearFaces()
{
    m_faces.clear();
}

eF32 eEditMesh::getMeshArea() const
{
    eF32 area = 0.0f;
    for (eU32 i=0; i<m_faces.size(); i++)
        area += getFaceArea(i);

    return area;
}

// returns the area of a polygon using Gaussian trapezoid
// formula (taken from NVIDIA mesh tools source code)
eF32 eEditMesh::getFaceArea(eU32 faceIndex) const
{
    const eEmFace &face = m_faces[faceIndex];
    eASSERT(face.count >= 3);

    const eVector3 &pos0 = m_vtxPos[face.posIdx[0]].pos;
    eF32 area = 0.0f;

    for (eU32 i=2; i<face.count; i++)
    {
        const eVector3 &v0 = m_vtxPos[face.posIdx[i-1]].pos-pos0;
        const eVector3 &v1 = m_vtxPos[face.posIdx[i]].pos-pos0;;
        area += (v0^v1).length();
    }

    return area*0.5f;
}

eVector3 eEditMesh::getFaceCenter(eU32 faceIndex) const
{
    eASSERT(faceIndex < m_faces.size());
    const eEmFace &face = m_faces[faceIndex];
    
    eVector3 center;
    for (eU32 i=0; i<face.count; i++)
        center += m_vtxPos[face.posIdx[i]].pos;

    return center/(eF32)face.count;
}

void eEditMesh::getPointOnFace(eU32 faceIndex, eF32 u, eF32 v, eVector3 &resPos, eVector3 &resNormal) const
{
	const eEmFace &face = m_faces[faceIndex];
	const eEmVtxPos &v0 = m_vtxPos[face.posIdx[0]];
	const eEmVtxPos &v1 = m_vtxPos[face.posIdx[1]];
	const eEmVtxPos &v2 = m_vtxPos[face.posIdx[2]];

	eF32 w0 = u;
	eF32 w1 = v;

	if (w0+w1 > 1.0f)
    {
		w0 = 1.0f - w0;
		w1 = 1.0f - w1;
	}

	eF32 w2 = 1.0f-w0-w1;
	resPos = (v0.pos*w0)+(v1.pos*w1)+(v2.pos*w2);
//	resNormal = (v0.avgNormal*w0)+(v1.avgNormal*w1)+(v2.avgNormal*w2);
	resNormal = face.normal;
}

eF32 eEditMesh::calculatePartialSurfaceSums(eArray<eF32>& partialSurfaceSums) const {
	eF32 areaSum = 0.0f;
	partialSurfaceSums.resize(getFaceCount());
	for (eU32 i=0; i<getFaceCount(); i++) {
		// Calc face area.
		areaSum += getFaceArea(i);
		// Add to sum.
		partialSurfaceSums[i] = areaSum;
	}

	eF32 invSum = 1.0f / areaSum;
	// normalize
	for (eU32 i=0; i<getFaceCount(); i++)
		partialSurfaceSums[i] *= invSum;
	return areaSum;
}

eU32 eEditMesh::getSurfacePoint(const eArray<eF32>& partialSurfaceSums, eF32 random0, eF32 random1, eF32 random2, eVector3& resultPos, eVector3& resultNormal) const {
	// lookup random entity with binary search
	eU32 l = 0;
	eU32 r = partialSurfaceSums.size();
	while (l < r) {
		eU32 m = (l + r) >> 1;
		if (random0 < partialSurfaceSums[m]) r = m;
		else                    l = m+1;
	}
	eU32 f = eClamp((eU32)0, r , partialSurfaceSums.size()-1);

	getPointOnFace(f, random1, random2, resultPos, resultNormal);
	return f;
}

eBool eEditMesh::isClosed() const
{
    return m_closed;
}

eBool eEditMesh::isTriangulated() const
{
    return m_triangulated;
}

const eAABB & eEditMesh::getBoundingBox() const
{
    return m_bbox;
}

eAABB & eEditMesh::getBoundingBox()
{
    return m_bbox;
}

const eEmVtxPos & eEditMesh::getPosition(eU32 index) const
{
    eASSERT(index < m_vtxPos.size());
    return m_vtxPos[index];
}

eEmVtxPos & eEditMesh::getPosition(eU32 index)
{
    eASSERT(index < m_vtxPos.size());
    return m_vtxPos[index];
}

const eEmVtxProps & eEditMesh::getProperty(eU32 index) const
{
    eASSERT(index < m_vtxProps.size());
    return m_vtxProps[index];
}

eEmVtxProps & eEditMesh::getProperty(eU32 index)
{
    eASSERT(index < m_vtxProps.size());
    return m_vtxProps[index];
}

const eVector3 & eEditMesh::getNormal(eU32 index) const
{
    eASSERT(index < m_vtxNrms.size());
    return m_vtxNrms[index];
}

eVector3 & eEditMesh::getNormal(eU32 index)
{
    eASSERT(index < m_vtxNrms.size());
    return m_vtxNrms[index];
}

const eEmWedge & eEditMesh::getWedge(eU32 index) const
{
    eASSERT(index < m_wedges.size());
    return m_wedges[index];
}

eEmWedge & eEditMesh::getWedge(eU32 index)
{
    eASSERT(index < m_wedges.size());
    return m_wedges[index];
}

const eEmFace & eEditMesh::getFace(eU32 index) const
{
    eASSERT(index < m_faces.size());
    return m_faces[index];
}

eEmFace & eEditMesh::getFace(eU32 index)
{
    eASSERT(index < m_faces.size());
    return m_faces[index];
}

const eEmEdge & eEditMesh::getEdge(eU32 index) const
{
    eASSERT(index < m_edges.size());
    return m_edges[index];
}

eEmEdge & eEditMesh::getEdge(eU32 index)
{
    eASSERT(index < m_edges.size());
    return m_edges[index];
}

eU32 eEditMesh::getPositionCount() const
{
    return m_vtxPos.size();
}

eU32 eEditMesh::getNormalCount() const
{
    return m_vtxNrms.size();
}

eU32 eEditMesh:: getPropertyCount() const
{
    return m_vtxProps.size();
}

eU32 eEditMesh::getWedgeCount() const
{
    return m_wedges.size();
}

eU32 eEditMesh::getFaceCount() const
{
    return m_faces.size();
}

eU32 eEditMesh::getEdgeCount() const
{
    return m_edges.size();
}

// triangulator implementation

eTriangulator::eTriangulator() : m_totalVtxCount(0)
{
}

eTriangulator::~eTriangulator()
{
    clearContours();
}

eBool eTriangulator::triangulate()
{
    m_triIndices.clear();
    m_triIndices.reserve(m_totalVtxCount*3);

    GLUtesselator *tess = gluNewTess();
    gluTessCallback(tess, GLU_TESS_EDGE_FLAG_DATA, (void (eCALLBACK *)())_edgeFlagCallback);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (eCALLBACK *)())_vertexCallback);
    gluTessCallback(tess, GLU_TESS_ERROR_DATA, (void (eCALLBACK *)())_errorCallback);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void (eCALLBACK *)())_combineCallback);
    gluTessBeginPolygon(tess, this);

    for (eU32 i=0, vtxNum=0; i<m_contours.size(); i++)
    {
        const eArray<eVector3> &contour = *m_contours[i];
        if(contour.isEmpty())
            continue;

        gluTessBeginContour(tess);
        {
            for (eU32 j=0; j<contour.size(); j++)
            {
                eF64 coords[] = {contour[j].x, contour[j].y, contour[j].z};
                gluTessVertex(tess, coords, (ePtr)vtxNum);
                vtxNum++;
            }
        }
        gluTessEndContour(tess);
    }

    gluTessEndPolygon(tess);
    eASSERT(m_triIndices.size()%3 == 0);
    return eTRUE;
}

void eTriangulator::addContour(const eArray<eVector3> &contour)
{
    eASSERT(contour.size() >= 3);
    m_vertices.append(contour);
    m_contours.append(new eArray<eVector3>(contour));
    m_totalVtxCount += contour.size();
}

void eTriangulator::clearContours()
{
    for (eU32 i=0; i<m_contours.size(); i++)
        eDelete(m_contours[i]);

    m_contours.clear();
    m_totalVtxCount = 0;
}

const eArray<eU32> & eTriangulator::getIndices() const
{
    return m_triIndices;
}

const eArray<eVector3> & eTriangulator::getVertices() const
{
    return m_vertices;
}

void eCALLBACK eTriangulator::_vertexCallback(eU32 vtxIndex, eTriangulator *trg)
{
    trg->m_triIndices.append(vtxIndex);
}

void eCALLBACK eTriangulator::_errorCallback(eU32 errNo, eTriangulator *trg)
{
    eASSERT(eFALSE);
}

// has to be defined even that it doesn't do anything,
// in order to make the GLU tesseleator outputting
// only simple triangles (and no triangle fans or strips).
void eCALLBACK eTriangulator::_edgeFlagCallback(eU8 flag, eTriangulator *trg)
{
}

void eCALLBACK eTriangulator::_combineCallback(eF64 newVert[3], eU32 neighbourVert[4], eF32 neighborWeight[4], eU32 *index, eTriangulator *trg)
{
    trg->m_vertices.append(eVector3((eF32)newVert[0], (eF32)newVert[1], (eF32)newVert[2]));
    *index = trg->m_vertices.size()-1;
}