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

#ifndef EDIT_MESH_HPP
#define EDIT_MESH_HPP

// data structure similar to the one described in
// "Efficient Implementation of Progressive Meshes"
// (Hoppe, 1998)

struct eEmVtxPos
{
    eVector3            pos;
    eVector3            avgNormal;
    eBool               selected;
    eU32                tag;
    eU32                temp;
};

struct eEmVtxProps
{
    eVector2            uv;
    eColor              col;
};

struct eEmWedge
{
    eU32                posIdx;
    eU32                nrmIdx;
    eU32                propsIdx;
    eU32                temp; 
};

struct eEmFace
{
    static const eU32   MAX_DEGREE = 8;

    const eMaterial *   mat;
    eVector3            normal;
    eBool               selected;
    eU32                temp;
    eU32                tag;
    eU32                count;
    eU32                wdgIdx[MAX_DEGREE];
    eU32                posIdx[MAX_DEGREE]; // equal to wedge's position index but saves one look-up
    eU32                edges[MAX_DEGREE];  // use only when adjacency is up-to-date
};

struct eEmEdge
{
    eU32                startPos;
    eU32                endPos;
    eU32                startWdg;
    eU32                endWdg;
    eU32                face;
    eU32                edge;
    eU32                next;
    eU32                prev;
    eInt                twin; // -1 for boundary edges
    eU32                temp;
};

class eEditMesh
{
public:
    eEditMesh(eU32 vtxCount=0, eU32 faceCount=0);

    void                merge(const eEditMesh &em, const eTransform &trans=eTransform());
    void                reserve(eU32 vtxCount, eU32 faceCount);
    void                clear();
    void                free();
    void                triangulate();
    void                calcAdjacency();
    void                calcNormals();
    void                calcAvgNormals();
    void                calcBoundingBox();
    void                unifyPositions();
    void                tidyUp();
    void                setBoundingBox(const eAABB &bbox);
    void                setMaterial(const eMaterial *mat);
    void                center();

    eU32                addPosition(const eVector3 &pos);
    eU32                addNormal(const eVector3 &normal);
    eU32                addProperty(const eVector2 &uv, const eColor &col);
    eU32                addWedge(eU32 posIdx, eU32 nrmIdx, eU32 propsIdx);
    eU32                addWedge(const eVector3 &pos, const eVector3 &normal, const eVector2 &uv, const eColor &col);
    eU32                addFace(const eU32 *wedges, eU32 count, const eMaterial *mat);
    eU32                addQuad(eU32 w0, eU32 w1, eU32 w2, eU32 w3, const eMaterial *mat);
    eU32                addTriangle(eU32 w0, eU32 w1, eU32 w2, const eMaterial *mat);
    void                removeFaces(const eArray<eU32> &indices);
    void                clearPositions();
    void                clearNormals();
    void                clearProperties();
    void                clearWedges();
    void                clearFaces();

    eF32                getMeshArea() const;
    eF32                getFaceArea(eU32 faceIndex) const;
    eVector3            getFaceCenter(eU32 faceIndex) const;
	void				getPointOnFace(eU32 faceIndex, eF32 u, eF32 v, eVector3 &resPos, eVector3 &resNormal) const;
	eF32				calculatePartialSurfaceSums(eArray<eF32>& partialSurfaceSums) const; 
	eU32				getSurfacePoint(const eArray<eF32> &partialSurfaceSums, eF32 random0, eF32 random1, eF32 random2, eVector3 &resPos, eVector3 &resNormal) const;

    eBool               isClosed() const;
    eBool               isTriangulated() const;
    const eAABB &       getBoundingBox() const;
    eAABB &             getBoundingBox();
    const eEmVtxPos &   getPosition(eU32 index) const;
    eEmVtxPos &         getPosition(eU32 index);
    const eEmVtxProps & getProperty(eU32 index) const;
    eEmVtxProps &       getProperty(eU32 index);
    const eVector3 &    getNormal(eU32 index) const;
    eVector3 &          getNormal(eU32 index);
    const eEmWedge &    getWedge(eU32 index) const;
    eEmWedge &          getWedge(eU32 index);
    const eEmFace &     getFace(eU32 index) const;
    eEmFace &           getFace(eU32 index);
    const eEmEdge &     getEdge(eU32 index) const;
    eEmEdge &           getEdge(eU32 index);
    eU32                getPositionCount() const;
    eU32                getNormalCount() const;
    eU32                getPropertyCount() const;
    eU32                getWedgeCount() const;
    eU32                getFaceCount() const;
    eU32                getEdgeCount() const;

private:
    eArray<eEmVtxPos>   m_vtxPos;
    eArray<eVector3>    m_vtxNrms;
    eArray<eEmVtxProps> m_vtxProps;
    eArray<eEmWedge>    m_wedges;
    eArray<eEmFace>     m_faces;
    eArray<eEmEdge>     m_edges;  // use only when adjacency is up-to-date
    eBool               m_closed; //          - "" -
    eAABB               m_bbox;
    eBool               m_triangulated;
};

// triangulator for arbitrary polygons (convex, with holes)

class eTriangulator
{
public:
    eTriangulator();
    ~eTriangulator();

    eBool                       triangulate();
    void                        addContour(const eArray<eVector3> &contour);
    void                        clearContours();

    const eArray<eU32> &        getIndices() const;
    const eArray<eVector3> &    getVertices() const;

private:
    static void eCALLBACK       _vertexCallback(eU32 vtxIndex, eTriangulator *trg);
    static void eCALLBACK       _errorCallback(eU32 errNo, eTriangulator *trg);
    static void eCALLBACK       _edgeFlagCallback(eU8 flag, eTriangulator *trg);
    static void eCALLBACK       _combineCallback(eF64 newVert[3], eU32 neighbourVert[4], eF32 neighborWeight[4], eU32 *index, eTriangulator *trg);

private:
    eArray<eVector3>            m_vertices;
    eArray<eArray<eVector3> *>  m_contours;
    eArray<eU32>                m_triIndices;
    eU32                        m_totalVtxCount;
};

#endif // EDIT_MESH_HPP