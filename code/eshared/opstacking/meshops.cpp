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

#if defined(eEDITOR)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <stdio.h>
#endif

#include <windows.h>
#include "../eshared.hpp"
#include "../math/math.hpp"
#include "../engine/engine.hpp"

eDEF_OPERATOR_SOURCECODE(MESHOPS);

static eTransform getRandomTrans(const eVector3 &randTrans, const eVector3 &randRot, const eVector3 &randScale)
{
    const eVector3 rs = eVector3(1.0f)+randScale.random();
    const eVector3 rt = randTrans.random();
    const eVector3 rr = (randRot*eTWOPI).random();
    return eTransform(eQuat(rr), rt, rs, eTO_STR);
}

void eIMeshOp::processMultiply(eU32 mode, eU32 count, 
                            const eVector3 &trans, const eVector3 &rot, const eVector3 &scale,                                              // Linear
                            eF32 radius, const eVector3 &circularRot,                                                                       // Circular
                            eF32 timeStart, eF32 timeEnd, const eVector3 &pathScale, const eVector3 &upVector, const eIPathOp *pathOp,      // Along path
                            const eVector3 &randTrans, const eVector3 &randRot, const eVector3 &randScale, eU32 seed,                       // Randomization
                            ePtr parent, eConstPtr element, void(*addElement)(ePtr, eConstPtr, const eTransform &))
{
    eRandomize(seed+1);

    switch (mode) 
    {
    case 0: // Linear
        {
            eTransform transStep(eQuat(rot*eTWOPI), trans, scale);
            eTransform tf;

            for (eU32 i=0; i<count+1; i++)
            {
                addElement(parent, element, getRandomTrans(randTrans, randRot, randScale)*tf);
                tf *= transStep;
            }
            break;
        }
    case 1: // Circular
        {
            const eVector3 exRot = circularRot*eTWOPI;
            const eVector3 trans(0.0f, 0.0f, radius);

            eVector3 curRot = exRot;
            eF32 step = eTWOPI/(count+1);

            for (eU32 i=0; i<=count; i++)
            {
                curRot += exRot;
                curRot.y += step;
                eTransform tf = eTransform(eQuat(curRot), trans, eVector3(1.0f), eTO_STR);
                addElement(parent, element, getRandomTrans(randTrans, randRot, randScale)*tf);
            }
            break;
        }
    case 2: // Along path
        {
            if (pathOp)
            {
                const eIPathOp::Result &res = pathOp->getResult();
                const ePath4 &path = res.path;

                eF32 time = timeStart;
                eF32 timeStep = (timeEnd - timeStart) / count;
                while(count--)
                {
                    eVector3 pos = path.evaluate(time).toVec3();
                    eVector3 pos_next = path.evaluate(time + timeStep * 0.1f).toVec3();

                    pos.scale(pathScale);
                    pos_next.scale(pathScale);

                    eVector3 direction = (pos_next - pos).normalized();
                    eTransform tf = eTransform(eQuat(upVector, direction).inverse(), pos, eVector3(1.0f, 1.0f, 1.0f), eTO_SRT);
                    addElement(parent, element, getRandomTrans(randTrans, randRot, randScale)*tf);

                    time += timeStep;
                }
            }
            break;
        }
    }
}

// UV-Mapping (mesh) operator
// ---------------------
// Maps UV coordinates

#if defined(HAVE_OP_MESH_MAP_UV) || defined(eEDITOR)
eOP_DEF_MESH(eUvMapOp, "Map UV", ' ', 1, 1, eOP_INPUTS(eOC_MESH))

public:
    enum MappingMode
    {
        MM_SPHERICAL,
        MM_SPHERICAL_MERCATOR,
        MM_CYLINDER,
        MM_PLANE,
        MM_CUBE
    };

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LABEL(eUvMapOp, label, "General", "General",
        eOP_PAR_ENUM(eUvMapOp, mode, "Method", "Sphere|Sphere mercator|Cylinder|Plane|Cube", 0,
        eOP_PAR_FXYZ(eUvMapOp, up, "Up", -1, 1, 0, 0, 1,
        eOP_PAR_FXYZ(eUvMapOp, projAxis, "ProjectAxis", -1, 1, 0, 1, 0,
        eOP_PAR_FXYZ(eUvMapOp, center, "Center", -eF32_MAX, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXY(eUvMapOp, uvScale, "UVScale", 0, eF32_MAX, 1, 1,
		eOP_PAR_END)))))))
    {
        _copyFirstInputMesh();
        internalExecute(m_mesh, (MappingMode&)mode, up, projAxis, center, uvScale);
    }
	eOP_EXEC2_END

    static void internalExecute(eEditMesh &em, MappingMode method, const eVector3 &up, const eVector3 &projAxis, const eVector3 &center, const eVector2 &uvScale)
    {
        eMatrix4x4 mat;
        mat.lookAt(projAxis, eVector3(0,0,0), up);
        eMatrix4x4 invMat = mat.inverse();

        struct vRecord
        {
            eVector3    pos;
            eVector3    normal;
            eF32        lat;
        };

        eArray<vRecord> vList;
        for(eU32 f = 0; f < em.getFaceCount(); f++)
        {
            eEmFace &face = em.getFace(f);
            vList.clear();
            eU32 defCnt = 0;
            eF32 lastLat;
            for(eU32 e = 0; e<face.count; e++)
            {
                vRecord &rec = vList.append();
                rec.lat = 1.0;
                rec.pos = invMat * (em.getPosition(face.posIdx[e]).pos-center);
                rec.normal = rec.pos.normalized();
                if((rec.normal.x != 0.0f) || (rec.normal.y != 0.0f))
                {
                    rec.lat = eATan2(rec.normal.y, rec.normal.x) + ePI;
                    lastLat = rec.lat;
                    defCnt++;
                }
            }

            for(eU32 e = 0; e < vList.size(); e++)
            {
                vRecord& rec = vList[e];
                if(rec.lat < 0.0f)
                {
                    if(defCnt == 1)
                        rec.lat = lastLat;
                    else
                    {
                        eF32 lat0 = vList[(e+1)%vList.size()].lat;
                        eF32 lat1 = vList[(e+2)%vList.size()].lat;
                        rec.lat = 0.5f * (lat0 + lat1);
                        if(eAbs(lat1 - lat0 >= ePI))
                            rec.lat += ePI;
                    }
                }

                eF32 tu, tv;
                switch(method) {
                    case MM_SPHERICAL: {
                                tu = rec.lat;
                                tv = eACos(rec.normal.z) * 2.0f;
                                tu = eClamp(0.0f, tu, 2.0f * ePI);
                                tv = eClamp(0.0f, tv, 2.0f * ePI);
                            } break;
/*
                    case MM_SPHERICAL_MERCATOR: {
                                tu = rec.lat;
                                eF32 s = rec.normal.z;
                                if(s != 1.0f)
                                    tv = eATanh(s) + ePI;
                                else 
                                    tv = ePI;
                                tu = eClamp(0.0f, tu, 2.0f * ePI);
                                tv = eClamp(0.0f, tv, 2.0f * ePI);
                            } break;
/**/
                    case MM_CYLINDER: {
                                tu = rec.lat;
                                tv = rec.pos.z;
                                tu = eClamp(0.0f, tu, 2.0f * ePI);
                            } break;
/*
                    case MM_PLANE: {
                                tu = rec.pos.x;
                                tv = rec.pos.y;
                            } break;
                    case MM_CUBE: {
                                eVector3 n = invMat * face.normal;
                                eU32 normAxis = 0;
                                if(eAbs(n[1]) > eAbs(n[normAxis]))
                                    normAxis = 1;
                                if(eAbs(n[2]) > eAbs(n[normAxis]))
                                    normAxis = 2;
                                eU32 side0 = 0;
                                while(side0 == normAxis)
                                    side0++;
                                eU32 side1 = 0;
                                while((side1 == normAxis) || (side1 == side0))
                                    side1++;

                                tu = rec.pos[side0];
                                tv = rec.pos[side1];
                            } break;
/**/
                }
                tu *= 0.5f * uvScale.x / ePI;
                tv *= 0.5f * uvScale.y / ePI;

                eEmWedge &wedge = em.getWedge(face.wdgIdx[e]);
                em.getProperty(wedge.propsIdx).uv.set(tu, tv);
            }

            // fix mapping errors
            switch(method) {
                case MM_SPHERICAL:
                case MM_CYLINDER:
/*
                case MM_SPHERICAL_MERCATOR:
				case MM_CUBE:
/**/
					for(eU32 e = 0; e < face.count ; e++)
                    {

                        eEmWedge &wgCur = em.getWedge(face.wdgIdx[e]);
                        eEmWedge &wgNext = em.getWedge(face.wdgIdx[(e+1)%face.count]);
                        eEmWedge &wg2ndNext = em.getWedge(face.wdgIdx[(e+2)%face.count]);

                        eVector2 &wgCurUv = em.getProperty(wgCur.propsIdx).uv;
                        eVector2 &wgNextUv = em.getProperty(wgNext.propsIdx).uv;
                        eVector2 &wg2ndNextUv = em.getProperty(wg2ndNext.propsIdx).uv;

                        if( (eAbs(wgNextUv.x - wgCurUv.x) >= uvScale.x * 0.5f) &&
                            (eAbs(wg2ndNextUv.x - wgCurUv.x) >= uvScale.x * 0.5f)) {
                                if(wgCurUv.x >= uvScale.x * 0.5f) {
									// insert additional wedges
									eVector2 newUv1 = eVector2(wgNextUv.x + uvScale.x, wgNextUv.y);
									eU32 idx1 = em.addWedge(wgNext.posIdx, wgNext.nrmIdx, em.addProperty(newUv1, eCOL_WHITE));
									face.wdgIdx[(e+1)%face.count] = idx1;

									eVector2 newUv2 = eVector2(wg2ndNextUv.x + uvScale.x, wg2ndNextUv.y);
									eU32 idx2 = em.addWedge(wg2ndNext.posIdx, wg2ndNext.nrmIdx, em.addProperty(newUv2, eCOL_WHITE));
									face.wdgIdx[(e+2)%face.count] = idx2;
                                }
                                else {

									// insert additional wedges
									eVector2 newUv0 = eVector2(wgCurUv.x + uvScale.x, wgCurUv.y);
									eU32 idx0 = em.addWedge(wgCur.posIdx, wgCur.nrmIdx, em.addProperty(newUv0, eCOL_WHITE));
									face.wdgIdx[e] = idx0;
								}
                        }
                    }
                    break;
            }
        }
    }
eOP_END(eUvMapOp);
#endif

// Plane (mesh) operator
// ---------------------
// Creates a 2-dimensional, segmented plane.

#if defined(HAVE_OP_MESH_PLANE) || defined(HAVE_OP_MESH_CUBE) || defined(eEDITOR)
eOP_DEF_MESH(ePlaneOp, "Plane", 'p', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
			  eOP_PAR_IXY(ePlaneOp, segs, "Segments", 1, 512, 1, 1,
		      eOP_PAR_FXY(ePlaneOp, size, "Size", eALMOST_ZERO, eF32_MAX, 1.0f, 1.0f,
			  eOP_PAR_LINK_MATERIAL(ePlaneOp, matOp, "Material", eOC_MAT, eFALSE,
			  eOP_PAR_LABEL(ePlaneOp, label, "label", "label",
			  eOP_PAR_END))))
			  )
        create(m_mesh, segs, size, matOp);
	eOP_EXEC2_END

    static void create(eEditMesh &mesh, const ePoint &segs, const eVector2 &size, const eIMaterialOp *matOp)
    {
        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);
        const eVector2 posStep(size.x/(eF32)segs.x, size.y/(eF32)segs.y);
        const eVector2 uvStep(1.0f/(eF32)segs.x, 1.0f/(eF32)segs.y);

        eVector3 pos(-size.x*0.5f, 0.0f, size.y*0.5f);
        eVector2 uv;

        const eU32 oldNumWdgs = mesh.getWedgeCount();
        mesh.reserve((segs.x+1)*(segs.y+1), segs.x*segs.y);

        for (eInt i=0; i<segs.y+1; i++, pos.z-=posStep.y, uv.v+=uvStep.v)
        {
            pos.x = -size.x*0.5f;
            uv.u = 0.0f;

            for (eInt j=0; j<segs.x+1; j++, pos.x+=posStep.x, uv.u+=uvStep.u)
                mesh.addWedge(pos, eVEC3_YAXIS, uv, eCOL_WHITE);
        }

        for (eInt i=0; i<segs.y; i++)
        {
            for (eInt j=0; j<segs.x; j++)
            {
                const eU32 base0 = oldNumWdgs+i*(segs.x+1)+j;
                const eU32 base1 = oldNumWdgs+(i+1)*(segs.x+1)+j;
                const eU32 index = mesh.addQuad(base0, base0+1, base1+1, base1, mat);

                eEmFace &face = mesh.getFace(index);
                face.normal = eVEC3_YAXIS;
                face.selected = eFALSE;
            }
        }

        mesh.setBoundingBox(eAABB(eVEC3_ORIGIN, size.x, 0.0f, size.y));
    }
eOP_END(ePlaneOp);
#endif

// Cube (mesh) operator
// --------------------
// Creates a one segment cube.

#if defined(HAVE_OP_MESH_CUBE) || defined(eEDITOR)
eOP_DEF_MESH(eCubeOp, "Cube", 'q', 0, 0, eOP_INPUTS())
    void _addSide(const eVector3 &rot, const eVector3 &trans, const eVector2 &size,
                  eU32 segsX, eU32 segsY, const eIMaterialOp *matOp)
    {
        const eTransform transf(eQuat(rot), trans, eVector3(1.0f), eTO_SRT);
        const eMatrix4x4 &mtx = transf.getMatrix();
        const eU32 oldNumPos = m_mesh.getPositionCount();

        ePlaneOp::create(m_mesh, ePoint(segsX, segsY), size, matOp);
        for (eU32 i=oldNumPos; i<m_mesh.getPositionCount(); i++)
            m_mesh.getPosition(i).pos *= mtx;
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_IXYZ(eCubeOp, segs, "Segments", 1, 64, 1, 1, 1,
        eOP_PAR_ENUM(eCubeOp, uvMapping, "UV mapping", "Single faces|Wrap around", 0,
        eOP_PAR_FXYZ(eCubeOp, size, "Size", eALMOST_ZERO, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_LINK_MATERIAL(eCubeOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END)))))
    {
        _addSide(eVector3(-eHALFPI, 0, 0), eVector3( 0, 0,  0.5f*size.z), eVector2(size.x, size.y), segs.x, segs.y, matOp);
        _addSide(eVector3( eHALFPI, 0, 0), eVector3( 0, 0, -0.5f*size.z), eVector2(size.x, size.y), segs.x, segs.y, matOp);
        _addSide(eVector3(0, 0,  eHALFPI), eVector3( 0.5f*size.x, 0, 0),  eVector2(size.y, size.z), segs.y, segs.z, matOp);
        _addSide(eVector3(0, 0, -eHALFPI), eVector3(-0.5f*size.x, 0, 0),  eVector2(size.y, size.z), segs.y, segs.z, matOp);
        _addSide(eVector3(0, 0, 0),        eVector3( 0,  0.5f*size.y, 0), eVector2(size.x, size.z), segs.x, segs.z, matOp);
        _addSide(eVector3(ePI, 0, 0),      eVector3( 0, -0.5f*size.y, 0), eVector2(size.x, size.z), segs.x, segs.z, matOp);
        
        m_mesh.unifyPositions();
        m_mesh.tidyUp();
        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
    }
	eOP_EXEC2_END
eOP_END(eCubeOp);
#endif

// Torus (mesh) operator
// ---------------------
// Creates a torus.

#if defined(HAVE_OP_MESH_TORUS) || defined(eEDITOR)
eOP_DEF_MESH(eTorusOp, "Torus", 'o', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_IXY(eTorusOp, segs, "Segments", 3, 256, 8, 16,
        eOP_PAR_FLOAT(eTorusOp, radius, "Radius", eALMOST_ZERO, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eTorusOp, thickness, "Thickness", eALMOST_ZERO, eF32_MAX, 0.5f,
        eOP_PAR_FLOAT(eTorusOp, phaseVal, "Phase", 0.0f, 1.0f, 0.0f,
        eOP_PAR_FLOAT(eTorusOp, angle, "Angle", 0.0f, 1.0f, 1.0f,
        eOP_PAR_LINK_MATERIAL(eTorusOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END)))))))
    {
        const eF32 phase = phaseVal*eTWOPI;
        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);
        const eInt segs_y_plusang = segs.y + (angle < 1.0f ? 1 : 0);
        const eInt segs_x_plusone = segs.x+1;
        
        m_mesh.reserve((segs.x+1)*segs_y_plusang, segs.x*segs.y);

        // add positions and normals
        for (eInt i=0; i<segs_y_plusang; i++)
        {
            for (eInt j=0; j<segs.x; j++)
            {
                eVector3 pos(thickness, 0.0f, 0.0f);
                pos.rotate(eVector3(0.0f, 0.0f, j*eTWOPI/(eF32)segs.x+phase));
                pos.x += radius;
                pos.rotate(eVector3(0.0f, i*eTWOPI/(eF32)segs.y*angle, 0.0f));
                
                m_mesh.addNormal(eVector3());
                m_mesh.addPosition(pos);
            }
        }

        // add properties and wedges
        for (eInt i=0; i<=segs.y; i++)
        {
            for (eInt j=0; j<=segs.x; j++)
            {
                m_mesh.addProperty(eVector2((eF32)j/segs.x, (eF32)i/segs.y), eCOL_WHITE);
                m_mesh.addWedge((i % segs_y_plusang) * segs.x + (j % segs.x), 
                                (i % segs_y_plusang) * segs.x + (j % segs.x), 
                                 i                   *(segs.x + 1) + j);
            }
        }

        // add faces
        for (eInt i=0; i<segs.y; i++)
            for (eInt j=0; j<segs.x; j++)
                m_mesh.addQuad(  i                       * segs_x_plusone + (j + 1) % segs_x_plusone, 
                                 i                       * segs_x_plusone +  j, 
                                (i + 1) % segs_y_plusang * segs_x_plusone +  j, 
                                (i + 1) % segs_y_plusang * segs_x_plusone + (j + 1) % segs_x_plusone, 
                                mat);

        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eTorusOp);
#endif

// Ring (mesh) operator
// --------------------
// Creates a ring facing upwards or sidewards.

#if defined(HAVE_OP_MESH_RING) || defined(eEDITOR)
eOP_DEF_MESH(eRingOp, "Ring", 'r', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eRingOp, radius, "Radius", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eRingOp, height, "Height", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_INT(eRingOp, segsX, "Segments X", 3, 256, 8,
        eOP_PAR_INT(eRingOp, segsY, "Segments Y", 1, 256, 1,
        eOP_PAR_LINK_MATERIAL(eRingOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END))))))
    {
        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);

        m_mesh.reserve(segsX*(segsY+1), segsX*segsY);

        // add vertices
        for (eU32 i=0; i<=segsY; i++)
        {
            const eF32 r = radius+i*height/segsY;
            for (eU32 j=0; j<=segsX; j++)
            {
                const eVector2 uv((eF32)i/(eF32)segsY, (eF32)j/(eF32)segsX);

                if (j < segsX)
                {
                    eF32 s, c, phi = j*eTWOPI/segsX;
                    eSinCos(phi, s, c);
                    m_mesh.addWedge(eVector3(s*r, c*r, 0.0f), eVector3(), uv, eCOL_WHITE);
                }
                else
                {
                    const eU32 posIdx = i*(segsX);
                    const eU32 nrmIdx = m_mesh.getNormalCount()-1;
                    const eU32 propsIdx = m_mesh.addProperty(uv, eCOL_WHITE);
                    m_mesh.addWedge(posIdx, nrmIdx, propsIdx);
                }
            }
        }

        // add faces
        for (eU32 i=0; i<segsY; i++)
            for (eU32 j=0; j<segsX; j++)
                m_mesh.addQuad(i*(segsX+1)+j, i*(segsX+1)+(j+1), (i+1)*(segsX+1)+(j+1), (i+1)*(segsX+1)+j, mat);

        // finalize mesh
        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eRingOp);
#endif

// Sphere (mesh) operator
// ----------------------
// Generates a sphere.

#if defined(HAVE_OP_MESH_SPHERE) || defined(eEDITOR)
eOP_DEF_MESH(eSphereOp, "Sphere", 'h', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_IXY(eSphereOp, segs, "Segments", 3, 256, 16, 16,
        eOP_PAR_FLOAT(eSphereOp, radius, "Radius", 0.01f, eF32_MAX, 0.5f,
        eOP_PAR_LINK_MATERIAL(eSphereOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END))))
    {
        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);
        const eU32 sxp = segs.x+1;
        const eU32 sym = segs.y-1;

        // add vertices
        m_mesh.reserve((segs.x+1)*(segs.y+1), 2*segs.x+segs.x*(segs.y-2));
        
        for (eInt j=0; j<=segs.y; j++)
        {
            for (eInt i=0; i<=segs.x; i++)
            {
                const eVector2 uv((eF32)j/(eF32)segs.y, (eF32)i/(eF32)segs.x);

                if (i < segs.x) 
                {
                    eF32 sx, cx, sy, cy;
                    eSinCos(uv.y*eTWOPI, sy, cy);
                    eSinCos(uv.x*ePI, sx, cx);
                    const eVector3 pos(sy*sx*radius, cx*radius, cy*sx*radius);
                    m_mesh.addWedge(pos, eVector3(), uv, eCOL_WHITE);
                }
                else 
                {
                    const eEmWedge &wdg = m_mesh.getWedge(j*sxp);
                    const eU32 propsIdx = m_mesh.addProperty(uv, eCOL_WHITE);
                    m_mesh.addWedge(wdg.posIdx, wdg.nrmIdx, propsIdx);
                }
            }
        }

        // add faces
        for (eInt i=0; i<segs.x; i++)
        {
            m_mesh.addTriangle(i, i+sxp, i+1+sxp, mat); // top
            m_mesh.addTriangle(i+1+sym*sxp, i+sym*sxp, i+segs.y*sxp, mat); // bottom

            for (eInt j=1; j<segs.y-1; j++) // middle
                m_mesh.addQuad(i+(j+1)*sxp, i+1+(j+1)*sxp, i+1+j*sxp, i+j*sxp, mat);
        }

        m_mesh.unifyPositions(); // for top and bottom positions
        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eSphereOp);
#endif

// Text3D (mesh) operator
// ----------------------
// Creates a 3D text.

#if defined(HAVE_OP_MESH_TEXT_3D) || defined(eEDITOR)
eOP_DEF_MESH(eText3dOp, "Text 3D", ' ', 0, 0, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_STRING(eText3dOp, fontName, "Font", "",
        eOP_PAR_TEXT(eText3dOp, text, "Text", "Brain Control",
        eOP_PAR_FLOAT(eText3dOp, size, "Size", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eText3dOp, depth, "Depth", 0.0f, eF32_MAX, 0.25f,
        eOP_PAR_FLAGS(eText3dOp, style, "Style", "Relief|Bold|Italic|Underline", 0,
        eOP_PAR_ENUM(eText3dOp, align, "Align", "Left|Right|Center", 0,
        eOP_PAR_LINK_MATERIAL(eText3dOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END))))))))
    {
        if (eStrLength(text) == 0)
            return;

        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);
        const eF32 scale = size*0.0025f;
        const eBool relief = eGetBit(style, 0);
        const eBool bold = eGetBit(style, 1);
        const eBool italic = eGetBit(style, 2);
        const eBool underline = eGetBit(style, 3);
        const eBool extrude = (depth > 0.0f);
        const eU32 alignFlag = (align == 1 ? DT_RIGHT : (align == 2 ? DT_CENTER : DT_LEFT));

        HDC dc = CreateCompatibleDC(NULL);
        HFONT font = CreateFont(500, 0, 0, 0, (bold ? FW_BOLD : FW_NORMAL), italic, underline, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, fontName);
        SelectObject(dc, font);
        SetBkMode(dc, (relief ? OPAQUE : TRANSPARENT));
        BeginPath(dc);
        RECT r;
        DrawText(dc, text, eStrLength(text), &r, DT_NOCLIP|DT_NOPREFIX|alignFlag);
        EndPath(dc);
        FlattenPath(dc);
    
        const eU32 numPts = GetPath(dc, NULL, NULL, 0);
        eASSERT(numPts > 0);
        eArray<POINT> pts(numPts);
        eByteArray types(numPts);
        GetPath(dc, &pts[0], &types[0], numPts);

        // triangulate generated contours
        eTriangulator trg;
        eArray<eVector3> contour;
        eInt firstPt = 0;

        for (eU32 i=0; i<numPts; i++)
        {
            // make sure that first and last contour points aren't equal
            if (i == firstPt || pts[i].x != pts[firstPt].x || pts[i].y != pts[firstPt].y)
                contour.append(eVector3(pts[i].x*scale, -pts[i].y*scale, -depth*0.5f));

            if (types[i]&PT_CLOSEFIGURE)
            {
                trg.addContour(contour);
                contour.clear();
                firstPt = i;
            }
        }

        eASSERT(contour.isEmpty());
        trg.triangulate();

        // add front faces
        for (eU32 i=0; i<trg.getVertices().size(); i++)
            m_mesh.addWedge(trg.getVertices()[i], eVector3(), eVector2(), eCOL_WHITE);
        for (eU32 i=0; i<trg.getIndices().size(); i+=3)
            m_mesh.addFace(&trg.getIndices()[0]+i, 3, mat);

        if (extrude)
        {
            // add back faces
            const eU32 oldNumPos = m_mesh.getPositionCount();
            const eU32 oldNumFaces = m_mesh.getFaceCount();

            for (eU32 i=0; i<oldNumPos; i++)
            {
                eVector3 pos = m_mesh.getPosition(i).pos;
                pos.z = -pos.z;
                m_mesh.addWedge(pos, eVector3(), eVector2(), eCOL_WHITE);
            }

            for (eU32 i=0; i<oldNumFaces; i++)
            {
                const eU32 *indices = m_mesh.getFace(i).wdgIdx;
                m_mesh.addTriangle(indices[2]+oldNumPos, indices[1]+oldNumPos, indices[0]+oldNumPos, mat);
            }

            m_mesh.calcAdjacency();

            // add side faces
            for (eU32 i=0; i<oldNumFaces; i++)
            {
                for (eU32 j=0; j<3; j++)
                {
                    eEmEdge &edgeFront = m_mesh.getEdge(m_mesh.getFace(i).edges[j]);
                    if (edgeFront.twin == -1)
                    {
                        eU32 mi[3] = {1,0,2};

                        eEmEdge &edgeBack = m_mesh.getEdge(m_mesh.getFace(i+oldNumFaces).edges[mi[j]]);
                        eASSERT(edgeBack.twin == -1);

                        eU32 nrmIdx[]={m_mesh.addNormal(eVector3()), m_mesh.addNormal(eVector3()), m_mesh.addNormal(eVector3()),m_mesh.addNormal(eVector3())};
                        eU32 propsIdx[]={m_mesh.addProperty(eVector2(),eCOL_WHITE), m_mesh.addProperty(eVector2(),eCOL_WHITE), m_mesh.addProperty(eVector2(),eCOL_WHITE), m_mesh.addProperty(eVector2(),eCOL_WHITE)};

                        eU32 i0=m_mesh.addWedge(edgeBack.startPos, nrmIdx[0], propsIdx[0]);
                        eU32 i1=m_mesh.addWedge(edgeFront.endWdg, nrmIdx[1], propsIdx[1]);
                        eU32 i2=m_mesh.addWedge(edgeFront.startWdg, nrmIdx[2], propsIdx[2]);
                        eU32 i3=m_mesh.addWedge(edgeBack.endWdg, nrmIdx[3], propsIdx[3]);

                        m_mesh.addQuad(i0, i1, i2, i3, mat);
                    }
                }
            }
        }

        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
        m_mesh.center();

        eUvMapOp::internalExecute(m_mesh, eUvMapOp::MM_CUBE, eVEC3_XAXIS, eVEC3_YAXIS, eVEC3_ORIGIN, eVector2(4, 4));

        // delete the GDI objects
        DeleteDC(dc);
        DeleteObject(font);
    }
	eOP_EXEC2_END
eOP_END(eText3dOp);
#endif

// Cylinder (mesh) operator
// ---------------------
// Creates a cylinder.

#if defined(HAVE_OP_MESH_CYLINDER) || defined(eEDITOR)
eOP_DEF_MESH(eCylinderOp, "Cylinder", 'c', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eCylinderOp, radius, "Radius", eALMOST_ZERO, eF32_MAX, 0.5f,
        eOP_PAR_FLOAT(eCylinderOp, height, "Height", eALMOST_ZERO, eF32_MAX, 1.0f,
        eOP_PAR_INT(eCylinderOp, segs, "Segments", 1, 256, 4,
        eOP_PAR_INT(eCylinderOp, edges, "Edges", 3, 256, 8,
        eOP_PAR_FLOAT(eCylinderOp, constriction, "Constriction", eALMOST_ZERO, eF32_MAX, 1.0f,
        eOP_PAR_FLAGS(eCylinderOp, closeness, "Closeness", "Top closed|Bottom closed", (1 | 2),
        eOP_PAR_LINK_MATERIAL(eCylinderOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END))))))))
    {
        const eBool topClosed = eGetBit(closeness, 0);
        const eBool botClosed = eGetBit(closeness, 1);
        const eF32 phiStep = eTWOPI/(eF32)edges;
        const eF32 heightStep = height/(eF32)segs;
        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);

        for (eU32 i=0; i<segs+1; i++)
        {
            const eF32 r = eLerp(radius, radius*constriction, (eF32)i/(eF32)segs);
            for (eU32 j=0; j<edges; j++)
            {
                const eF32 phi = (eF32)j*phiStep;
                m_mesh.addNormal(eVector3());
                m_mesh.addPosition(eVector3(eCos(phi)*r, -0.5f*height+(eF32)i*heightStep, eSin(phi)*r));
            }
        }

        for (eU32 i=0; i<segs+1; i++)
            for (eU32 j=0; j<=edges; j++)
                m_mesh.addProperty(eVector2((eF32)i/(eF32)segs, (eF32)j/(eF32)edges), eCOL_WHITE);

        for (eU32 i=0; i<segs+1; i++)
            for (eU32 j=0; j<=edges; j++)
                m_mesh.addWedge(i*edges+(j%edges), i*edges+(j%edges), i*(edges+1)+j);

        for (eU32 i=0; i<segs; i++)
            for (eU32 j=0; j<edges; j++)
                m_mesh.addQuad(i*(edges+1)+j, (i+1)*(edges+1)+j, (i+1)*(edges+1)+(j+1), i*(edges+1)+(j+1), mat);

        if (botClosed)
        {
            const eU32 botPos = m_mesh.getWedgeCount();
            m_mesh.addWedge(eVector3(0.0f, -height*0.5f, 0.0f), eVector3(), eVector2(0.5f, 0.5f), eCOL_WHITE);

            for (eU32 j=0; j<edges; j++)
                m_mesh.addWedge(j, m_mesh.addNormal(eVector3()), m_mesh.addProperty(eVector2((m_mesh.getPosition(j).pos.x+radius)*0.5f/radius, (m_mesh.getPosition(j).pos.z+radius)*0.5f/radius), eCOL_WHITE));
            for (eU32 j=0; j<edges; j++)
                m_mesh.addTriangle(botPos, botPos+1+j, botPos+1+((j+1)%edges), mat);
        }

        if (topClosed)
        {
            const eU32 topPos = m_mesh.getWedgeCount();
            m_mesh.addWedge(eVector3(0.0f, height*0.5f, 0.0f), eVector3(), eVector2(0.5f, 0.5f), eCOL_WHITE);

            for (eU32 j=0; j<edges; j++)
                m_mesh.addWedge((segs)*(edges)+j, m_mesh.addNormal(eVector3()), m_mesh.addProperty(eVector2((m_mesh.getPosition(j).pos.x+radius)*0.5f/radius, (m_mesh.getPosition(j).pos.z+radius)*0.5f/radius), eCOL_WHITE));
            for (eU32 j=0; j<edges; j++)
                m_mesh.addTriangle(topPos+1+((j+1)%edges), topPos+1+j, topPos, mat);
        }

        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
    }
	eOP_EXEC2_END
eOP_END(eCylinderOp);
#endif

// Center (mesh) operator
// ----------------------
// Centers the mesh around the origin.

#if defined(HAVE_OP_MESH_CENTER) || defined(eEDITOR)
eOP_DEF_MESH(eMeshCenterOp, "Center", 'e', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        _copyFirstInputMesh();
        m_mesh.center();
    }
	eOP_EXEC2_END
eOP_END(eMeshCenterOp);
#endif

// Transform (mesh) operator
// -------------------------
// Transforms the specified selection of a mesh.

#if defined(HAVE_OP_MESH_TRANSFORM) || defined(eEDITOR)
eOP_DEF_MESH(eMeshTransformOp, "Transform", 't', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZ(eMeshTransformOp, trans, "Translate", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eMeshTransformOp, rotVal, "Rotate", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eMeshTransformOp, scale, "Scale", eF32_MIN, eF32_MAX, 1, 1, 1,
        eOP_PAR_ENUM(eMeshTransformOp, selection, "Selection", "All|Selected|Unselected", 0,
        eOP_PAR_INT(eMeshTransformOp, tag, "Tag", 0, 255, 0,
		eOP_PAR_END))))))
    {
        _copyFirstInputMesh();

        const eTransform transf(eQuat(rotVal*eTWOPI), trans, scale, eTO_SRT);
        const eMatrix4x4 mtxPos = transf.getMatrix();

        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
        {
            eEmVtxPos &vtxPos = m_mesh.getPosition(i);
            if (vtxPos.tag == tag || !tag)
                if (!selection || (selection == 1 && vtxPos.selected) || (selection == 2 && !vtxPos.selected))
                    vtxPos.pos *= mtxPos;
        }

        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END

    eOP_INTERACT(eSceneData &sd, eOpInteractionInfos &oii)
    {
        eVector3 &transl = (eVector3 &)getParameter(0).getAnimValue().fxyz;
        eVector3 &rot = (eVector3 &)getParameter(1).getAnimValue().fxyz;
        eVector3 &scale = (eVector3 &)getParameter(2).getAnimValue().fxyz;

        if (oii.srtCtrl.interact(oii.input, oii.cam, sd, scale, rot, transl) == eTWR_NOCHANGES)
        {
            oii.srtCtrl.setPosition(m_mesh.getBoundingBox().getCenter());
            return eFALSE;
        }
        else
        {
            setChanged();
            return eTRUE;
        }
    }
    eOP_END(eMeshTransformOp);
#endif

// Merge (mesh) operator
// ---------------------
// Merges multiple mesh operators.

#if defined(HAVE_OP_MESH_MERGE) || defined(eEDITOR)
eOP_DEF_MESH(eMeshMergeOp, "Merge", 'm', 1, 32, eOP_INPUTS(eOP_32INPUTS(eOC_MESH)))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        for (eU32 i=0; i<getAboveOpCount(); i++)
        {
            const eEditMesh &mesh = ((eIMeshOp *)getAboveOp(i))->getResult().mesh;
            m_mesh.merge(mesh);
        }
    }
	eOP_EXEC2_END
eOP_END(eMeshMergeOp);
#endif

// Select cube (mesh) operator
// ---------------------------
// Selects primitives (vertices or
// polygons) that lie inside a cube. 

#if defined(HAVE_OP_MESH_SELECT_CUBE) || defined(eEDITOR)
eOP_DEF_MESH(eSelectCubeOp, "Select cube", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eSelectCubeOp, primType, "Primitive", "Vertex|Face (contains)|Face (intersects)", 0,
        eOP_PAR_ENUM(eSelectCubeOp, mode, "Mode", "Set|Add|Remove|Toggle", 0,
        eOP_PAR_INT(eSelectCubeOp, tag, "Tag", 0, 255, 0,
        eOP_PAR_LABEL(eSelectCubeOp, label, "Cube", "Cube",
        eOP_PAR_FXYZ(eSelectCubeOp, trans, "Translate", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZ(eSelectCubeOp, rot, "Rotate", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZ(eSelectCubeOp, scale, "Scale", eF32_MIN, eF32_MAX, 1.25f, 1.25f, 1.25f,
		eOP_PAR_END))))))))
    {
        _copyFirstInputMesh();

        // vertices and side planes of selection cube
        m_transf = eTransform(eQuat(rot*eTWOPI), trans, scale);
        const eMatrix4x4 &mtx = m_transf.getMatrix();

        const eVector3 cubeVerts[] =
        {
            eVector3(-0.5f, -0.5f, -0.5f)*mtx, // ulh
            eVector3(-0.5f, -0.5f,  0.5f)*mtx, // ulv
            eVector3( 0.5f, -0.5f,  0.5f)*mtx, // urv
            eVector3( 0.5f, -0.5f, -0.5f)*mtx, // urh
            eVector3(-0.5f,  0.5f, -0.5f)*mtx, // olh
            eVector3(-0.5f,  0.5f,  0.5f)*mtx, // olv
            eVector3( 0.5f,  0.5f,  0.5f)*mtx, // orv
            eVector3( 0.5f,  0.5f, -0.5f)*mtx, // orh
        };

        const ePlane cubePlanes[6] =
        {
            ePlane(cubeVerts[2], cubeVerts[0], cubeVerts[1]), // bottom
            ePlane(cubeVerts[5], cubeVerts[4], cubeVerts[6]), // top
            ePlane(cubeVerts[3], cubeVerts[2], cubeVerts[6]), // right 
            ePlane(cubeVerts[4], cubeVerts[1], cubeVerts[0]), // left
            ePlane(cubeVerts[5], cubeVerts[2], cubeVerts[1]), // front
            ePlane(cubeVerts[0], cubeVerts[3], cubeVerts[4]), // back
        };

        // select desired primitves
        if (primType == 0) // vertex
        {
            for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
            {
                eEmVtxPos &vp = m_mesh.getPosition(i);
                
                if (vp.pos.isInsideCube(cubePlanes))
                {
                    _selectPrimitive(vp.selected, mode);
                    vp.tag = tag;
                }
                else if (mode == 0) // set
                {
                    vp.selected = eFALSE;
                    vp.tag = 0;
                }
             }
        }
        else // face
        {
            for (eU32 i=0; i<m_mesh.getFaceCount(); i++)
            {
                eEmFace &face = m_mesh.getFace(i);
                eU32 numSelVerts = 0;

                for (eU32 j=0; j<face.count; j++)
                    if (m_mesh.getPosition(face.posIdx[j]).pos.isInsideCube(cubePlanes))
                        numSelVerts++;

                if ((primType == 1 && numSelVerts == face.count) || // face contained
                    (primType == 2 && numSelVerts > 0))             // face intersected
                {
                    _selectPrimitive(face.selected, mode);
                    face.tag = tag;
                }
                else
                {
                    if (mode == 0) // set
                    {
                        face.selected = eFALSE;
                        face.tag = 0;
                    }
                }
            }
        }
    }
	eOP_EXEC2_END

    eOP_INTERACT(eSceneData &sd, eOpInteractionInfos &oii)
    {
        oii.infoTransf = m_transf;
        oii.infoMesh.addWireCube(nullptr, eCOL_ORANGE);
        oii.infoMesh.finishLoading(eMT_DYNAMIC);
        return eFALSE;
    }

    eOP_VAR(eTransform  m_transf);
eOP_END(eSelectCubeOp);
#endif

// Select random (mesh) operator
// -----------------------------
// Randomly selects primitives in the input mesh.

#if defined(HAVE_OP_MESH_SELECT_RANDOM) || defined(eEDITOR)
eOP_DEF_MESH(eSelectRandOp, "Select random", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eSelectRandOp, primitive, "Primitive", "Vertex|Edge|Face", 0,
        eOP_PAR_ENUM(eSelectRandOp, mode, "Mode", "Set|Add|Remove|Toggle", 0,
        eOP_PAR_INT(eSelectRandOp, tag, "Tag", 0, 255, 0,
        eOP_PAR_FLOAT(eSelectRandOp, amount, "Amount", 0.0f, 1.0f, 0.25f,
        eOP_PAR_INT(eSelectRandOp, seed, "Seed", 0, 65535, 0,
		eOP_PAR_END))))))
    {
        _copyFirstInputMesh();
        eRandomize(seed+1);
         
        // if selection mode is "set" => unselect first
        if (mode == 0) // set
        {
            if (primitive == 0) // vertex
            {
                for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
                {
                    m_mesh.getPosition(i).selected = eFALSE;
                    m_mesh.getPosition(i).tag = 0;
                }
            }
            else // face
            {
                for (eU32 i=0; i<m_mesh.getFaceCount(); i++)
                {
                    m_mesh.getFace(i).selected = eFALSE;
                    m_mesh.getFace(i).tag = 0;
                }
            }
        }

        // select given amount of primtives
        if (primitive == 0) // vertex
        {
            for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
            {
                if (eRandomF() <= amount)
                {
                    _selectPrimitive(m_mesh.getPosition(i).selected, mode);
                    m_mesh.getPosition(i).tag = tag;
                }
            }
        }
        else // face
        {
            for (eU32 i=0; i<m_mesh.getFaceCount(); i++)
            {
                if (eRandomF() <= amount)
                {
                    _selectPrimitive(m_mesh.getFace(i).selected, mode);
                    m_mesh.getFace(i).tag = tag;
                }
            }
        }
    }
	eOP_EXEC2_END
eOP_END(eSelectRandOp);
#endif

// Multiply (mesh) operator
// ------------------------
// Duplicates the mesh multiple times.

#if defined(HAVE_OP_MESH_MULTIPLY) || defined(eEDITOR)
eOP_DEF_MESH(eMeshMultiplyOp, "Multiply", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
    eOP_INIT()
    {
    }

    static void addElement(ePtr parent, eConstPtr element, const eTransform &transf)
    {
        ((eEditMesh *)parent)->merge(*(const eEditMesh *)element, transf);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eMeshMultiplyOp, mode, "Mode", "Linear|Circular|Along path", 0,
        eOP_PAR_INT(eMeshMultiplyOp, count, "Count", 1, 65536, 1,
        eOP_PAR_LABEL(eMeshMultiplyOp, label0, "Linear", "Linear",
        eOP_PAR_FXYZ(eMeshMultiplyOp, trans, "Translate", eF32_MIN, eF32_MAX, 1, 0, 0,
        eOP_PAR_FXYZ(eMeshMultiplyOp, rot, "Rotate", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eMeshMultiplyOp, scale, "Lin.Scale", eF32_MIN, eF32_MAX, 1, 1, 1,
        eOP_PAR_LABEL(eMeshMultiplyOp, label1, "Circular", "Circular",
        eOP_PAR_FLOAT(eMeshMultiplyOp, radius, "Radius", eF32_MIN, eF32_MAX, 5.0f,
        eOP_PAR_FXYZ(eMeshMultiplyOp, circularRot, "Rotation", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_LABEL(eMeshMultiplyOp, label2, "Along path", "Along path",
        eOP_PAR_FLOAT(eMeshMultiplyOp, timeStart, "Time start", eF32_MIN, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eMeshMultiplyOp, timeEnd, "Time end", eF32_MIN, eF32_MAX, 1.0f,
        eOP_PAR_FXYZ(eMeshMultiplyOp, pathScale, "Scale", eF32_MIN, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_FXYZ(eMeshMultiplyOp, upVector, "Up vector", eF32_MIN, eF32_MAX, 0.0f, 1.0f, 0.0f,
        eOP_PAR_LINK_PATH(eMeshMultiplyOp, pathOp, "Path", eOC_PATH, eFALSE,
        eOP_PAR_LABEL(eMeshMultiplyOp, label3, "Randomize", "Randomize",
        eOP_PAR_FXYZ(eMeshMultiplyOp, randTrans, "Rand. trans.", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eMeshMultiplyOp, randRot, "Rand. rotation", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eMeshMultiplyOp, randScale, "Rand. scale", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_INT(eMeshMultiplyOp, seed, "Seed", 0, 65535, 0,
		eOP_PAR_END)))))))))))))))))))))
    {
        const eEditMesh &em = ((eIMeshOp *)getAboveOp(0))->getResult().mesh;
        eIMeshOp::processMultiply(mode, count, trans, rot, scale, radius, circularRot, timeStart, timeEnd, pathScale,
                                  upVector, pathOp, randTrans, randRot, randScale, seed, &m_mesh, &em, addElement);

		m_mesh.calcNormals();
    }
	eOP_EXEC2_END
eOP_END(eMeshMultiplyOp);
#endif

// Set material (mesh) operator
// ----------------------------
// Sets the material given in the second input
// operator to the mesh given in first input.

#if defined(HAVE_OP_MESH_SET_MATERIAL) || defined(eEDITOR)
eOP_DEF_MESH(eSetMaterialOp, "Set material", 's', 2, 2, eOP_INPUTS(eOC_MESH, eOC_MAT))

eOP_EXEC2(ENABLE_STATIC_PARAMS,
    eOP_PAR_ENUM(eSetMaterialOp, selection, "Apply on", "All faces|Selected faces|Unselected faces", 0,
    eOP_PAR_INT(eSetMaterialOp, tag, "Tag", 0, 255, 0,
	eOP_PAR_END)))
{
    _copyFirstInputMesh();
    const eMaterial *mat = &((eIMaterialOp *)getAboveOp(1))->getResult().mat;

    for (eU32 i=0; i<m_mesh.getFaceCount(); i++)
    {
        eEmFace &face = m_mesh.getFace(i);
        const eBool tagging = (face.tag == tag || !tag);

        switch (selection)
        {
        case 0: // all
            if (tagging)
                face.mat = mat;
            break;

        case 1: // selected
            if (face.selected && tagging)
                face.mat = mat;
            break;

        case 2: // unselected
            if (!face.selected && tagging)
                face.mat = mat;
            break;
        }
    }
}
eOP_EXEC2_END
eOP_END(eSetMaterialOp);
#endif

// Bend (mesh) operator
// --------------------
// Bends the input mesh along a bezier spline.

#if defined(HAVE_OP_MESH_BEND) || defined(eEDITOR)
eOP_DEF_MESH(eBendOp, "Bend", 'b', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LABEL(eBendOp, label0, "General", "General",
        eOP_PAR_FXYZ(eBendOp, upVal, "Up", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 1.0f,
        eOP_PAR_BOOL(eBendOp, keepAlignment, "Keep Alignment", false,
        eOP_PAR_LABEL(eBendOp, label1, "Bend Region", "Bend Region",
        eOP_PAR_FXYZ(eBendOp, axis0, "Axis0", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZ(eBendOp, axis1, "Axis1", eF32_MIN, eF32_MAX, 0.0f, 1.0f, 0.0f,
        eOP_PAR_LABEL(eBendOp, label2, "Control Points", "Control Points",
        eOP_PAR_FXYZ(eBendOp, control0, "Control0", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZ(eBendOp, control1, "Control1", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.333f,
        eOP_PAR_FXYZ(eBendOp, control2, "Control2", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.666f,
        eOP_PAR_FXYZ(eBendOp, control3, "Control3", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 1.0f,
		eOP_PAR_END))))))))))))
    {
        _copyFirstInputMesh();

        const eVector3 &up = upVal.normalized();
        const eVector3 axis = axis1-axis0;
        const eF32 axisLen = axis.length();

        eMatrix4x4 mtx;
        mtx.lookAt(eVEC3_ORIGIN, axis.normalized(), up);
        const eMatrix4x4 &mtxInv = mtx.inverse();

        eVector3 bezPos;
        eVector3 bezTangent;

        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
        {
            eVector3 pos = m_mesh.getPosition(i).pos-axis0;
            eVector3 posRelAxis = mtxInv*pos;
            eF32 t = posRelAxis.z/axisLen;
            eF32 tclamp = eClamp(0.0f, t, 1.0f);

            eVector3::cubicBezier(tclamp, control0, control1, control2, control3, bezPos, bezTangent);
            bezPos.z *= axisLen;
            bezPos = mtx*bezPos+axis0;
            bezTangent.z *= axisLen;
            bezTangent = mtx*bezTangent.normalized();

            eMatrix4x4 bezMtx;
            bezMtx.lookAt(eVEC3_ORIGIN, bezTangent, up);
            posRelAxis.z = (t-tclamp)*axisLen;

            if (keepAlignment)
                m_mesh.getPosition(i).pos = mtx*posRelAxis+bezPos;
            else
                m_mesh.getPosition(i).pos = bezMtx*posRelAxis+bezPos;
        }
    }
	eOP_EXEC2_END

    eOP_INTERACT(eSceneData &sd, eOpInteractionInfos &oii)
    {
        const eVector3 up = eVector3(getParameter(1).getAnimValue().fxyz).normalized();
        const eVector3 axis0 = getParameter(4).getAnimValue().fxyz;
        const eVector3 axis1 = getParameter(5).getAnimValue().fxyz;

        const eVector3 control[] =
        {
            getParameter(7).getAnimValue().fxyz,
            getParameter(8).getAnimValue().fxyz,
            getParameter(9).getAnimValue().fxyz,
            getParameter(10).getAnimValue().fxyz
        };

        eMatrix4x4 mtx;
        mtx.lookAt(eVector3(0,0,0), (axis1 - axis0).normalized(), up);
        eMatrix3x3 subMtx = mtx.getUpper3x3();

        const eF32 axisLen = (axis1 - axis0).length();

        eU32 segs = 20;
        eF32 size = 0.5f;
        eF32 size2 = 0.1f;
        const eMaterial *mat = eMaterial::getWireframe();

        eMesh &bezierMesh = oii.infoMesh;
        {
            eInt vcnt = 0;
            eInt icnt = 0;

            for(eU32 d = 0; d < 2; d++)
            {
                eVector3 pos = (d == 0) ? axis0 : axis1;

                eVector3 v0 = pos + subMtx.getColumn(0) * size;
                eVector3 v1 = pos - subMtx.getColumn(0) * size;
                eVector3 v2 = pos + subMtx.getColumn(1) * size;
                eVector3 v3 = pos - subMtx.getColumn(1) * size;

                bezierMesh.addLine(v0, v1, eCOL_YELLOW, mat);
                bezierMesh.addLine(v2, v3, eCOL_YELLOW, mat);
            }

            for(eU32 c = 0; c < 4; c++)
            {
                eVector3 pos = control[c];
                pos.z *= axisLen;
                pos = mtx * pos + axis0;

                for(eU32 d = 0; d < 3; d++)
                {
                    eVector3 v0 = pos + subMtx.getColumn(d) * size2;
                    eVector3 v1 = pos - subMtx.getColumn(d) * size2;

                    bezierMesh.addLine(v0, v1, eCOL_CYAN, mat);
                }
            }

            for(eU32 s = 0; s < segs; s++)
            {
                eF32 t0 = (eF32)s / (eF32)segs;
                eF32 t1 = (eF32)(s+1) / (eF32)segs;
                eVector3 tangent0;
                eVector3 tangent1;
                eVector3 pos0;
                eVector3 pos1;
                eVector3::cubicBezier(t0, control[0], control[1], control[2], control[3], pos0, tangent0);
                eVector3::cubicBezier(t1, control[0], control[1], control[2], control[3], pos1, tangent1);
                pos0.z *= axisLen;
                pos1.z *= axisLen;
                pos0 = mtx * pos0 + axis0;
                pos1 = mtx * pos1 + axis0;
                bezierMesh.addLine(pos0, pos1, eCOL_PINK, mat);
            }
        }
        bezierMesh.finishLoading(eMT_DYNAMIC);

        return eFALSE;
    }

eOP_END(eBendOp);
#endif

#if defined(HAVE_OP_MESH_BC_LOGO) || defined(eEDITOR)
eOP_DEF_MESH(eBcLogoOp, "BC Logo", ' ', 0, 0, eOP_INPUTS())

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_MATERIAL(eBcLogoOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END))
    {
        const eVector3 vertices[] =
        {
            eVector3(-0.250824f, 0.0f,  0.073242f),
            eVector3(-0.409866f, 0.0f,  0.134430f),
            eVector3(-0.350372f, 0.0f, -0.026154f),
            eVector3( 0.091481f, 0.0f, -0.087286f),
            eVector3( 0.165737f, 0.0f, -0.013035f),
            eVector3( 0.140244f, 0.0f,  0.012466f),
            eVector3( 0.093686f, 0.0f, -0.034098f),
            eVector3( 0.053761f, 0.0f,  0.005840f),
            eVector3( 0.139267f, 0.0f,  0.091324f),
            eVector3( 0.244686f, 0.0f, -0.014107f),
            eVector3( 0.092587f, 0.0f, -0.166208f),
            eVector3(-0.076981f, 0.0f,  0.003366f),
            eVector3( 0.137962f, 0.0f,  0.218330f),
            eVector3( 0.369087f, 0.0f, -0.012810f),
            eVector3( 0.093437f, 0.0f, -0.288425f),
            eVector3(-0.201508f, 0.0f,  0.006516f),
            eVector3( 0.139473f, 0.0f,  0.347427f),
            eVector3( 0.494225f, 0.0f, -0.007294f),
            eVector3( 0.094559f, 0.0f, -0.407234f),
            eVector3(-0.280838f, 0.0f, -0.031807f),
            eVector3(-0.374870f, 0.0f, -0.125992f),
            eVector3(-0.489258f, 0.0f,  0.213760f),
            eVector3(-0.148537f, 0.0f,  0.100511f),
            eVector3(-0.242554f, 0.0f,  0.006470f),
            eVector3( 0.095924f, 0.0f, -0.331944f),
            eVector3( 0.419647f, 0.0f, -0.008087f),
            eVector3( 0.142059f, 0.0f,  0.269440f),
            eVector3(-0.122963f, 0.0f,  0.004423f),
            eVector3( 0.093929f, 0.0f, -0.212473f),
            eVector3( 0.293671f, 0.0f, -0.012695f),
            eVector3( 0.138882f, 0.0f,  0.142113f),
            eVector3( 0.000492f, 0.0f,  0.003702f),
        };

        const eU32 indices[] =
        {
            22,  2, 21,
             2,  3, 21,
            22, 23,  2,
             2, 23,  1,
            21,  3, 20,
            23, 24,  1,
             3,  1, 20,
             1, 24, 20,
             7,  8,  6,
             8,  9,  6,
             9, 10,  6,
             6, 10,  5,
            10, 11,  5,
             5, 11,  4,
            11, 12,  4, 
             4, 12, 32,
            12, 13, 32,
            32, 13, 31,
            13, 14, 31,
            31, 14, 30,
            14, 15, 30,
            30, 15, 29,
            15, 16, 29,
            29, 16, 28,
            16, 17, 28,
            28, 17, 27,
            17, 18, 27,
            27, 18, 26,
            18, 19, 26,
            26, 19, 25,
            19, 20, 25,
            20, 24, 25,
        };

        const eVector2 texCoords[] =
        {
            0.307515f, 0.433466f,
            0.369618f, 0.433466f,
            0.365292f, 0.572047f,
            0.199846f, 0.554811f,
            0.211639f, 0.291257f,
            0.474845f, 0.322239f,
            0.460749f, 0.687600f,
            0.099505f, 0.645803f,
            0.116740f, 0.169842f,
            0.577559f, 0.237387f,
            0.557812f, 0.801828f,
            0.000000f, 0.728002f,
            0.022050f, 0.061754f,
            0.632056f, 0.146466f,
            0.662271f, 0.000000f,
            1.000000f, 0.242132f,
            0.609518f, 0.367595f,
            0.619008f, 0.205847f,
            0.082199f, 0.144372f,
            0.055614f, 0.687461f,
            0.494174f, 0.724792f,
            0.510432f, 0.290140f,
            0.171028f, 0.251902f,
            0.164259f, 0.586910f,
            0.400809f, 0.618310f,
            0.403182f, 0.376038f,
            0.272905f, 0.362152f,
            0.270184f, 0.499407f,
            0.304026f, 0.501151f,
            0.676764f, 0.286006f,
            0.704420f, 0.093293f,
            0.881475f, 0.220232f,
        };

        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);

        for (eU32 i=0; i<sizeof(vertices)/sizeof(eVector3); i++)
            m_mesh.addWedge(vertices[i], eVector3(), texCoords[i], eCOL_WHITE);

        for (eU32 i=0; i<sizeof(indices)/sizeof(eU32); i+=3)
            m_mesh.addTriangle(indices[i]-1, indices[i+1]-1, indices[i+2]-1, mat); // -1 cause indices start with 1

        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eBcLogoOp);
#endif

// Vertex noise (mesh) operator
// ----------------------------
// Randomly translates vertices.

#if defined(HAVE_OP_MESH_VERTEX_NOISE) || defined(eEDITOR)
eOP_DEF_MESH(eVertexNoiseOp, "Vertex noise", 'n', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZ(eVertexNoiseOp, amount, "Amount", eF32_MIN, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_INT(eVertexNoiseOp, seed, "Seed", 0, 65535, 0,
        eOP_PAR_ENUM(eVertexNoiseOp, mode, "Mode", "All directions|By normal", 0,
        eOP_PAR_ENUM(eVertexNoiseOp, selMode, "Selection", "All|Selected|Unselected", 0,
        eOP_PAR_INT(eVertexNoiseOp, tag, "Tag", 0, 255, 0,
		eOP_PAR_END))))))
    {
        _copyFirstInputMesh();
        eRandomize(seed);

        if (mode == 1) // by normal
            m_mesh.calcAvgNormals();

        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
        {
            eEmVtxPos &vp = m_mesh.getPosition(i);

            if (vp.tag == tag || !tag)
            {
                if (!selMode || (selMode == 1 && vp.selected) || (selMode == 2 && !vp.selected))
                {
                    if (mode == 0) // all directions
                    {
                        vp.pos.x += amount.x*(eRandomF()-0.5f);
                        vp.pos.y += amount.y*(eRandomF()-0.5f);
                        vp.pos.z += amount.z*(eRandomF()-0.5f);
                    }
                    else if (mode == 1) // by normal
                    {
                
                        vp.pos.x += vp.avgNormal.x*(amount.x*(eRandomF()-0.5f));
                        vp.pos.y += vp.avgNormal.y*(amount.y*(eRandomF()-0.5f));
                        vp.pos.z += vp.avgNormal.z*(amount.z*(eRandomF()-0.5f));
                    }
                }
            }
        }

        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
    }
	eOP_EXEC2_END
eOP_END(eVertexNoiseOp);
#endif

// Options UV (mesh) operator
// --------------------------
// Scrolls and scales texture coordinates
// of input mesh.

#if defined(HAVE_OP_MESH_OPTIONS_UV) || defined(eEDITOR)
eOP_DEF_MESH(eOptionsUvOp, "Options UV", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eOptionsUvOp, scale, "Scale UV", eF32_MIN, eF32_MAX, 1.0f, 1.0f,
        eOP_PAR_FXY(eOptionsUvOp, scroll, "Scroll UV", eF32_MIN, eF32_MAX, 0.0f, 0.0f,
        eOP_PAR_ENUM(eOptionsUvOp, selection, "Selection", "All|Selected|Unselected", 0,
		eOP_PAR_END))))
    {
        _copyFirstInputMesh();

        for (eU32 i=0; i<m_mesh.getWedgeCount(); i++)
            m_mesh.getWedge(i).temp = 0;

        for (eU32 i=0; i<m_mesh.getFaceCount(); i++)
        {
            eEmFace &face = m_mesh.getFace(i);
            
            if (selection == 0 || (selection == 1 && face.selected) || (selection == 2 && !face.selected))
                for (eU32 j=0; j<face.count; j++)
                    m_mesh.getWedge(face.wdgIdx[j]).temp = 1;
        }

        for (eU32 i=0; i<m_mesh.getWedgeCount(); i++)
        {
            eEmWedge &wedge = m_mesh.getWedge(i);
            if (wedge.temp == 1)
            {
                eVector2 &uv = m_mesh.getProperty(wedge.propsIdx).uv;
                uv.scale(scale);
                uv += scroll;
            }
        }
    }
	eOP_EXEC2_END
eOP_END(eOptionsUvOp);
#endif

// Inside out (mesh) operator
// --------------------------
// Negates the position of all vertices of the mesh.

#if defined(HAVE_OP_MESH_INSIDEOUT) || defined(eEDITOR)
eOP_DEF_MESH(eInsideOutOp, "Insideout", 'i', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        _copyFirstInputMesh();

        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
            m_mesh.getPosition(i).pos.negate();

        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
    }
	eOP_EXEC2_END
eOP_END(eInsideOutOp);
#endif

// Wave transform (mesh) operator
// ------------------------------
// Transforms vertices with sine waves.

#if defined(HAVE_OP_MESH_WAVE) || defined(eEDITOR)
eOP_DEF_MESH(eWaveOp, "Wave", 'w', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eWaveOp, target, "Target", "Transform|Rotate|Scale", 0,
        eOP_PAR_FLOAT(eWaveOp, speed, "Speed", 0.1f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eWaveOp, amount, "Amount", -eF32_MAX, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eWaveOp, radius, "Radius", 0.1f, eF32_MAX, 100.0f,
        eOP_PAR_FLOAT(eWaveOp, time, "Time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLAGS(eWaveOp, affect, "Affect", "X-axis|Y-axis|Z-axis", 1 | 2 | 4,
		eOP_PAR_END)))))))
    {
        _copyFirstInputMesh();

        const eBool affectX = eGetBit(affect, 0);
        const eBool affectY = eGetBit(affect, 1);
        const eBool affectZ = eGetBit(affect, 2);
        const eF32 sqrRadius = radius*radius;

        eF32 val;
        eVector3 res;

        for (eU32 i=0; i< m_mesh.getPositionCount(); i++)
        {
            eVector3 &pos = m_mesh.getPosition(i).pos;
            const eF32 dist = pos.sqrLength()/sqrRadius;
            const eF32 sine = eSin(time*speed+dist)*amount;

            switch (target)
            {
            case 0: // translate
                res.set(affectX ? sine : 0.0f, affectY ? sine : 0.0f, affectZ ? sine : 0.0f);
                pos += res;
                break;


            case 1: // rotate
                val = eDegToRad(sine);
                res.set(affectX ? val : 0.0f, affectY ? val : 0.0f, affectZ ? val : 0.0f);
                pos.rotate(res);
                break;


            case 2: // scale
                val = sine+(1.0f-amount);
                res.set(affectX ? val : 1.0f, affectY ? val : 1.0f, affectZ ? val : 1.0f);
                pos.scale(res);
            }
        }

        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
    }
	eOP_EXEC2_END
eOP_END(eWaveOp);
#endif

// Color (mesh) operator
// ---------------------
// Sets the vertex colors of a mesh to a given color.

#if defined(HAVE_OP_MESH_COLOR) || defined(eEDITOR)
eOP_DEF_MESH(eMeshColorOp, "Color", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_RGB(eMeshColorOp, col, "Color", 255, 255, 255,
		eOP_PAR_END))
    {
        _copyFirstInputMesh();

        for (eU32 i=0; i<m_mesh.getPropertyCount(); i++)
            m_mesh.getProperty(i).col = col;
    }
	eOP_EXEC2_END
eOP_END(eMeshColorOp);
#endif

// Displace (mesh) operator
// ------------------------
// Displaces a mesh (first input operator) using
// pixel information from a bitmap (second operator).

#if defined(HAVE_OP_MESH_DISPLACE) || defined(eEDITOR)
eOP_DEF_MESH(eDisplaceOp, "Displace", 'd', 2, 2, eOP_INPUTS(eOC_MESH, eOC_BMP))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eDisplaceOp, mode, "Mode", "All directions|Normal direction", 0,
        eOP_PAR_FLOAT(eDisplaceOp, range, "Range", eF32_MIN, eF32_MAX, 1.0f,
        eOP_PAR_ENUM(eDisplaceOp, selMode, "Selection", "All|Selected|Unselected", 0,
        eOP_PAR_INT(eDisplaceOp, tag, "Tag", 0, 255, 0,
		eOP_PAR_END)))))
    {
        _copyFirstInputMesh();

        const eIBitmapOp *bmpOp = (eIBitmapOp *)getAboveOp(1);
        const eIBitmapOp::Result &bmpRes = bmpOp->getResult();

        if (bmpOp->getChanged() || m_bmpData.size() == 0)
            eGfx->readTexture2d(bmpRes.uav->tex, m_bmpData);

        // for accessing properties from positions
        for (eU32 i=0; i<m_mesh.getWedgeCount(); i++) 
        {
            eEmWedge &wedge = m_mesh.getWedge(i);
            m_mesh.getPosition(wedge.posIdx).temp = i;
        }

        // perform the displacements
		m_mesh.calcNormals();
        m_mesh.calcAvgNormals();

        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
        {
            eEmVtxPos &vp = m_mesh.getPosition(i);

            const eVector2 &uv = m_mesh.getProperty(m_mesh.getWedge(vp.temp).propsIdx).uv;
            const eInt tx = eFtoL(eClamp(0.0f, uv.u, 1.0f)*(bmpRes.width-1));
            const eInt ty = eFtoL(eClamp(0.0f, uv.v, 1.0f)*(bmpRes.height-1));
            const eColor pixel = m_bmpData[ty*bmpRes.width+tx];
            const eF32 displace = (eF32)pixel.grayScale()/255.0f*range;

            if (vp.tag == tag || !tag)
            {
                if (!selMode || (selMode == 1 && vp.selected) || (selMode == 2 && !vp.selected))
                {
                    if (!mode) // all directions
                        vp.pos *= displace;
                    else // normal direction
                        vp.pos += vp.avgNormal*displace;
                }
            }
        }

        m_mesh.calcNormals();
    }
	eOP_EXEC2_END

    eOP_VAR(eArray<eColor> m_bmpData);
eOP_END(eDisplaceOp);
#endif

// Attractor (mesh) operator
// -------------------------
// Attracts vertices towards a point.

#if defined(HAVE_OP_MESH_ATTRACTOR) || defined(eEDITOR)
eOP_DEF_MESH(eMeshAttractorOp, "Attractor", 'a', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZ(eMeshAttractorOp, pos, "Position", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FLOAT(eMeshAttractorOp, range, "Range", 0.0f, eF32_MAX, 10.0f,
        eOP_PAR_FLOAT(eMeshAttractorOp, power, "Power", -20.0f, 20.0f, 2.0f,
		eOP_PAR_END))))
    {
        _copyFirstInputMesh();

        const eF32 rr = range*range;

        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
        {
            eEmVtxPos &vp = m_mesh.getPosition(i);
            const eVector3 dv = pos-vp.pos;
            const eF32 dist = dv.sqrLength();

            if (dist <= rr)
            {
                const eF32 att = 1.0f-ePow(dist/rr, eIsFloatZero(power) ? 0.01f : power);
                vp.pos += dv*att;
            }
        }

        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eMeshAttractorOp);
#endif

// Triangulate (mesh) operator
// ---------------------------
// Triangulates the input mesh.

#if defined(HAVE_OP_MESH_TRIANGULATE) || defined(eEDITOR)
eOP_DEF_MESH(eTriangulateOp, "Triangulate", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        _copyFirstInputMesh();
        m_mesh.triangulate();
    }
	eOP_EXEC2_END
eOP_END(eTriangulateOp);
#endif

// Delete faces (mesh) operator
// ----------------------------
// Deletes selected primitives from a mesh.

#if defined(HAVE_OP_MESH_DELETE_FACES) || defined(eEDITOR)
eOP_DEF_MESH(eMeshDeleteFaceOp, "Delete faces", 'l', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eMeshDeleteFaceOp, tag, "Tag", 0, 255, 0,
		eOP_PAR_END))
    {
        _copyFirstInputMesh();

        eArray<eU32> indices;
        for (eInt i=m_mesh.getFaceCount()-1; i>=0; i--)
            if (m_mesh.getFace(i).selected && (!m_mesh.getFace(i).tag || m_mesh.getFace(i).tag == tag))
                indices.append(i);

        m_mesh.removeFaces(indices);
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eMeshDeleteFaceOp);
#endif

// Flatten (mesh) operator
// -----------------------
// Duplicates wedges so that each face has its own
// four wedges with its own normals which are equal
// to the corresponding face normal => flat shading.

#if defined(HAVE_OP_MESH_FLAT_SHADE) || defined(eEDITOR)
eOP_DEF_MESH(eFlatShadeOp, "Flat shade", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eFlatShadeOp, smoothness, "Smoothness", 0.0f, 1.0f, 0.0f,
		eOP_PAR_END))
    {
        _copyFirstInputMesh();

        eEditMesh smoothMesh = m_mesh;

        smoothMesh.clearNormals();
        for (eU32 i=0; i<m_mesh.getPositionCount(); i++)
            smoothMesh.addNormal(eVector3());
        for (eU32 i=0; i<smoothMesh.getFaceCount(); i++)
        {
            eEmFace &face = smoothMesh.getFace(i);
            for (eU32 j=0; j<smoothMesh.getFace(i).count; j++)
            {
                eEmWedge &wdg = smoothMesh.getWedge(face.wdgIdx[j]);
                wdg.nrmIdx = wdg.posIdx;
            }
        }

        smoothMesh.calcNormals();

        if (eAreFloatsEqual(smoothness, 1.0f))
            m_mesh = smoothMesh;
        else
        {
            eArray<eEmWedge> oldWdgs(m_mesh.getWedgeCount());
            for (eU32 i=0; i<m_mesh.getWedgeCount(); i++)
                oldWdgs[i] = m_mesh.getWedge(i);

            m_mesh.clearNormals();
            m_mesh.clearWedges();

            for (eU32 i=0; i<m_mesh.getFaceCount(); i++)
            {
                eEmFace &face = m_mesh.getFace(i);
                for (eU32 j=0; j<face.count; j++)
                {
                    eEmWedge wdg = oldWdgs[face.wdgIdx[j]];
                    wdg.nrmIdx = m_mesh.addNormal(eLerp(face.normal, smoothMesh.getNormal(wdg.posIdx), smoothness));
                    face.wdgIdx[j] = m_mesh.addWedge(wdg.posIdx, wdg.nrmIdx, wdg.propsIdx);
                }
            }
        }
    }
	eOP_EXEC2_END
eOP_END(eFlatShadeOp);
#endif

// Subdivide (mesh) operator
// -------------------------
// Refinement operation, which subdivides and
// smoothes input mesh.

#if defined(HAVE_OP_MESH_SUBDIVIDE) || defined(eEDITOR)
eOP_DEF_MESH(eSubdivideOp, "Subdivide", 'u', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eSubdivideOp, iterations, "Iterations", "1|2|3|4|5|6", 0,
        eOP_PAR_FLOAT(eSubdivideOp, smoothness, "Smoothness", -5.0f, 5.0f, 1.0f,
		eOP_PAR_END)))
    {
        _copyFirstInputMesh();

        eEditMesh temp;
        eEditMesh *dst = &temp;
        eEditMesh *src = &m_mesh;

        for (eInt i=0; i<=iterations; i++)
        {
            dst->clear();
            src->calcAdjacency();
            _subdivideOnce(*dst, *src, smoothness);
            eSwap(src, dst);
        }

        if (src != &m_mesh) // use source because of previous swap
            m_mesh = *src;

        m_mesh.calcBoundingBox();
        m_mesh.calcNormals();
    }
	eOP_EXEC2_END

    void _subdivideOnce(eEditMesh &dst, const eEditMesh &src, eF32 smoothness) const
    {
        // add vertices in the middle of the faces (P vertices)
        eArray<eU32> face2midWedge(src.getFaceCount());
        for (eU32 i=0; i<src.getFaceCount(); i++)
        {
            const eEmFace &face = src.getFace(i);
            eVector2 uv;
            eVector3 normal;
            eColor col;
            for (eU32 j=0; j<face.count; j++)
            {
                const eEmWedge &wdg = src.getWedge(face.wdgIdx[j]);
                uv += src.getProperty(wdg.propsIdx).uv;
                col += src.getProperty(wdg.propsIdx).col;
                normal += src.getNormal(wdg.nrmIdx);
            }
            uv /= (eF32)face.count;
            normal /= (eF32)face.count;

            eU32 posIdx = dst.addPosition(src.getFaceCenter(i));
            eU32 nrmIdx = dst.addNormal(eVector3());
            eU32 propsIdx = dst.addProperty(uv, eCOL_WHITE);
            face2midWedge[i] = dst.addWedge(posIdx, nrmIdx, propsIdx);
        }

        // add vertices in the middle of the edges (E vertices)
        eArray<eU32> he2midEdgeWedge(src.getEdgeCount());
        eMemSet(&he2midEdgeWedge[0], 0xff, sizeof(eU32)*he2midEdgeWedge.size());
        eArray<eU32> vtxEx(src.getEdgeCount());
        eMemSet(&vtxEx[0], 0xff, vtxEx.size()*sizeof(eU32));
        for (eU32 i=0; i<src.getEdgeCount(); i++)
        {
            if (he2midEdgeWedge[i] != 0xffffffff)
                continue;

            const eEmEdge &edge = src.getEdge(i);
            const eEmVtxPos *endVtx0 = &src.getPosition(edge.startPos);
            const eEmVtxPos *endVtx1 = &src.getPosition(edge.endPos);
            const eEmVtxPos *faceVtx0 = &dst.getPosition(dst.getWedge(face2midWedge[edge.face]).posIdx);
            const eEmVtxPos *faceVtx1 = &dst.getPosition(dst.getWedge(face2midWedge[src.getEdge(edge.twin).face]).posIdx);
            eVector3 vtx = (endVtx0->pos+endVtx1->pos)*0.5f;
            eVector3 vtx2 = (endVtx0->pos+endVtx1->pos+faceVtx0->pos+faceVtx1->pos)*0.25f;
            vtx = vtx.lerp(smoothness, vtx2);

            const eVector2 uv = (src.getProperty(src.getWedge(edge.startWdg).propsIdx).uv+src.getProperty(src.getWedge(edge.endWdg).propsIdx).uv)*0.5f;
            const eVector3 normal = (src.getNormal(src.getWedge(edge.startWdg).nrmIdx)+src.getNormal(src.getWedge(edge.endWdg).nrmIdx))*0.5f;

            eU32 &newVtx = vtxEx[i];
            if (newVtx == 0xffffffff)
            {
                newVtx = dst.addPosition(vtx);
                vtxEx[edge.twin] = newVtx;
            }

            eU32 wedge = dst.addWedge(newVtx, dst.addNormal(normal), dst.addProperty(uv, eCOL_WHITE));
            he2midEdgeWedge[i] = wedge;

            //
            const eEmEdge &twin = src.getEdge(edge.twin);
            if (twin.startWdg == edge.endWdg && twin.endWdg == edge.startWdg)
                he2midEdgeWedge[edge.twin] = wedge;
        }

        // add original vertices (V vertices)
        eArray<eU32> wdgEdges(src.getWedgeCount()*9);
        eMemSet(&wdgEdges[0], 0, sizeof(eU32)*wdgEdges.size());

        eArray<eU32> posEdges(src.getPositionCount()*9);
        eMemSet(&posEdges[0], 0, sizeof(eU32)*posEdges.size());

        for (eU32 i=0; i<src.getFaceCount(); i++)
        {
            const eEmFace &face = src.getFace(i);
            for (eU32 j=0; j<src.getFace(i).count; j++)
            {
                {
                    eU32 wdg = face.wdgIdx[j];
                    eU32 &count = wdgEdges[wdg*9+0];
                    wdgEdges[wdg*9+1+count] = face.edges[j];
                    count++;
                }

                {
                    eU32 pos = face.posIdx[j];
                    eU32 &count = posEdges[pos*9+0];
                    posEdges[pos*9+1+count] = face.edges[j];
                    count++;
                }
            }
        }

        eArray<eU32> he2HeStartWedge(src.getEdgeCount());
        eArray<eU32> vtx2newVtx(src.getPositionCount());
        eMemSet(&vtx2newVtx[0], 0xff, vtx2newVtx.size()*sizeof(eU32));
        for (eU32 i=0; i<src.getWedgeCount(); i++)
        {
            const eEmWedge &wedge = src.getWedge(i);

            // accumulate mid vertices of edges and faces
            eU32 edges[eEmFace::MAX_DEGREE];
            eU32 wdgEdgeCount = wdgEdges[i*9+0];
            for (eU32 j=0; j<wdgEdgeCount; j++)
                edges[j] = wdgEdges[i*9+1+j];

            eU32 &newVtx = vtx2newVtx[wedge.posIdx];
            if (newVtx == 0xffffffff)
            {
                eVector3 mv;
                eU32 posEdgeCount = posEdges[wedge.posIdx*9+0];
                for (eU32 j=0; j<posEdgeCount; j++)
                {
                    eU32 he = posEdges[wedge.posIdx*9+1+j];
                    mv += dst.getPosition(dst.getWedge(he2midEdgeWedge[he]).posIdx).pos;
                    mv += dst.getPosition(dst.getWedge(face2midWedge[src.getEdge(he).face]).posIdx).pos;
                }

                // do affine combination between vertices
                const eVector3 vtxPos = src.getPosition(wedge.posIdx).pos;
                eVector3 sv = mv/(eF32)(posEdgeCount*posEdgeCount)+vtxPos*(eF32)(posEdgeCount-2)/(eF32)posEdgeCount;
                eVector3 rv = vtxPos.lerp(smoothness, sv);

                newVtx = dst.addPosition(rv);
            }

            eU32 newWedge = dst.addWedge(newVtx, dst.addNormal(src.getNormal(wedge.nrmIdx)), dst.addProperty(src.getProperty(wedge.propsIdx).uv, src.getProperty(wedge.propsIdx).col));

            for (eU32 j=0; j<wdgEdgeCount; j++)
                he2HeStartWedge[edges[j]] = newWedge;
        }

        // add faces
        eArray<eU32> aa;
        for (eU32 i=0; i<src.getFaceCount(); i++)
        {
            const eEmFace &face = src.getFace(i);

            for (eU32 j=0; j<face.count; j++)
            {
                eU32 w0 = he2HeStartWedge[face.edges[j]];
                eU32 w1 = he2midEdgeWedge[face.edges[j]];
                eU32 w2 = face2midWedge[src.getEdge(face.edges[j]).face];
                eU32 w3 = he2midEdgeWedge[src.getEdge(face.edges[j]).prev];
                dst.addQuad(w0, w1, w2, w3, face.mat);
            }
        }
    }
eOP_END(eSubdivideOp);
#endif

// Extrude (mesh) operator
// -----------------------
// Extrudes faces of a mesh.

#if defined(HAVE_OP_MESH_EXTRUDE) || defined(eEDITOR)
eOP_DEF_MESH(eExtrudeOp, "Extrude", 'x', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eExtrudeOp, length, "Length", eF32_MIN, eF32_MAX, 1.0f,
        eOP_PAR_INT(eExtrudeOp, segments, "Segments", 1, 64, 1,
        eOP_PAR_ENUM(eExtrudeOp, dir, "Direction", "Face normal|Vertex normal|Average vertex normal", 0,
        eOP_PAR_FXYZ(eExtrudeOp, scale, "Scale", eF32_MIN, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_FXYZ(eExtrudeOp, rot, "Rotate", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
		eOP_PAR_END))))))
    {
        const eTransform transf(rot*eTWOPI, eVector3(), scale);

        _copyFirstInputMesh();

        for (eU32 i=0; i<segments; i++)
        {
            m_mesh.calcAdjacency();
            m_mesh.calcAvgNormals();
            const eU32 oldFaceCount = m_mesh.getFaceCount();

            // find contour of selection (edges of selected faces
            // that have one neighbouring face selected one not)
            eArray<eEmEdge *> contour;
            eArray<eEmFace *> selFaces;
            for (eU32 j=0; j<m_mesh.getFaceCount(); j++)
            {
                eEmFace &face = m_mesh.getFace(j);
                for (eU32 k=0; k<face.count; k++)
                {
                    eEmEdge &edge = m_mesh.getEdge(face.edges[k]);
                    if (face.selected)
                    {
                        selFaces.append(&face);
                        if (edge.twin == -1 || !m_mesh.getFace(m_mesh.getEdge(edge.twin).face).selected)
                            contour.append(&edge);
                    }
                }
            }

            //
            for (eU32 i=0; i<selFaces.size(); i++)
                for (eU32 j=0; j<selFaces[i]->count; j++)
                    m_mesh.getWedge(selFaces[i]->wdgIdx[j]).temp = 0; // not visited

            for (eU32 i=0; i<selFaces.size(); i++)
            {
                eEmFace *face = selFaces[i];

                for (eU32 j=0; j<face->count; j++)
                {
                    eEmWedge &wdg = m_mesh.getWedge(face->wdgIdx[j]);

                    if (!wdg.temp) // already visited?
                    {

                    }
                }
            }
        }

        // unifyNormals???
        m_mesh.unifyPositions();
        m_mesh.tidyUp();
        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eExtrudeOp);
#endif

// Ribbon (mesh) operator
// ----------------------
// Generates a ribbon mesh.

#if defined(HAVE_OP_MESH_RIBBON) || defined(eEDITOR)
eOP_DEF_MESH(eRibbonOp, "Ribbon", ' ', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_PATH(eRibbonOp, posOp, "Position path", eOC_PATH, eTRUE,
        eOP_PAR_FXYZ(eRibbonOp, scale, "Scale", eF32_MIN, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_FLOAT(eRibbonOp, length, "Length", 0.0f, 1.0f, 1.0f,
        eOP_PAR_INT(eRibbonOp, totalSegs, "Segments", 1, 128, 16,
        eOP_PAR_INT(eRibbonOp, edges, "Edges", 3, 32, 8,
        eOP_PAR_LINK_MATERIAL(eRibbonOp, matOp, "Material", eOC_MAT, eFALSE,
		eOP_PAR_END)))))))
    {
        if (eIsFloatZero(length))
            return;

        const ePath4 &path = posOp->getResult().path;
        const eF32 endTime = path.getEndTime()*length;
        const eU32 segments = eFtoL((eF32)totalSegs*length);
        const eF32 timeStep = path.getDuration()/(eF32)totalSegs;
        const eMaterial *mat = (matOp ? &matOp->getResult().mat : nullptr);

        m_mesh.reserve((segments+1)*edges, segments*edges);

        eVector3 dir;

        for (eU32 i=0; i<=segments; i++)
        {
            // sample path twice
            const eF32 time0 = path.getStartTime()+(eF32)i*timeStep;
            const eF32 time1 = eMin(time0+timeStep, endTime);
            const eVector3 pos0 = path.evaluate(time1).toVec3();
            const eVector3 pos1 = path.evaluate(time0).toVec3();

            // check if length of direction is zero and if it is
            // use old direction (can happen in the last segment)
            const eVector3 tempDir = (pos1-pos0).normalized();
            if (!eIsFloatZero(tempDir.sqrLength()))
                dir = tempDir;

            // create two vectors perpendicular to direction
            const eVector3 axis0 = dir;
            const eVector3 axis1 = eVector3(axis0.z, axis0.y*axis0.z/(axis0.x-1.0f),  1.0f+(axis0.z*axis0.z)/(axis0.x-1.0f));
            const eVector3 axis2 = axis0^axis1;

            // add a ring of vertices
            const eF32 step = eTWOPI/(eF32)edges;

            for (eU32 j=0; j<edges; j++)
            {
                eVector3 pos = axis2*eQuat(dir, (eF32)j*step)+pos0;
                pos.scale(scale);
                const eVector2 uv((eF32)i/(eF32)(totalSegs+1), (eF32)j/(eF32)edges);
                m_mesh.addWedge(pos, eVector3(), uv, eCOL_WHITE);
            }
        }

        // generate faces
        for (eU32 i=0; i<segments; i++)
            for (eU32 j=0; j<edges; j++)
                m_mesh.addQuad(i*edges+j, i*edges+(j+1)%edges, (i+1)*edges+(j+1)%edges, (i+1)*edges+j, nullptr);

        m_mesh.calcNormals();
        m_mesh.calcBoundingBox();
    }
	eOP_EXEC2_END
eOP_END(eRibbonOp);
#endif

// Wavefront OBJ import (mesh) operator
// ------------------------------------
// Imports a wavefront OBJ file.

#if defined(HAVE_OP_MESH_IMPORT_OBJ) || defined(eEDITOR)
eOP_DEF_MESH(eImportObjOp, "Import OBJ", 'c', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_STRING(eImportObjOp, fileName, "Filename", "",
		eOP_PAR_END))
    {
		FILE *F;
		fopen_s(&F, fileName,"rt");
		if(F != 0)
		{
			eVector3 pos, normal;
			eU32 f0, f1, f2, f3, dummy;
			char line[1000];
//			while(0 != fscanf_s(F,"%s\n",line)) {
			while(!feof(F)) {
				fgets(line, sizeof(line), F);
		        if(line[strlen(line)-1] == '\n') 
					line[strlen(line)-1] = 0;
				if(line[0] == '#')
					continue; // comment
				if(line[0] == 'v') {
					if(line[1] == 'n') {
						// normal
						sscanf_s(line,"vn %f %f %f", &normal.x, &normal.y, &normal.z);
					} else {
						// vertice
						sscanf_s(line,"v %f %f %f", &pos.x, &pos.y, &pos.z);
						m_mesh.addWedge(pos, normal, eVector2(), eCOL_WHITE);
					}
				} else {
					// face
					if(4 == sscanf_s(line,"f %i %i %i %i", &f0, &f1, &f2, &f3)) {
						m_mesh.addQuad(f0 - 1, f1 - 1, f2 - 1, f3 - 1, nullptr);
					}
					if(8 == sscanf_s(line,"f %i/%i %i/%i %i/%i %i/%i", &f0, &dummy, &f1, &dummy, &f2, &dummy, &f3, &dummy)) {
						m_mesh.addQuad(f0 - 1, f1 - 1, f2 - 1, f3 - 1, nullptr);
					}
					if(6 == sscanf_s(line,"f %i//%i %i//%i %i//%i", &f0, &dummy, &f1, &dummy, &f2, &dummy)) {
						m_mesh.addTriangle(f0 - 1, f1 - 1, f2 - 1, nullptr);
					}
//					m_mesh.addTriangle(f0 - 1, f2 - 1, f1 - 1, nullptr);
				}
			};
//			eShowError("File opened");
			fclose(F);

            eUvMapOp::internalExecute(m_mesh, eUvMapOp::MM_CUBE, eVEC3_ZAXIS, eVEC3_YAXIS, eVector3(), eVector2(5));
	        m_mesh.calcNormals();
            m_mesh.center();
			m_mesh.calcBoundingBox();
		}		
    }
	eOP_EXEC2_END
eOP_END(eImportObjOp);
#endif

// Mesh selector (mesh) operator
// ------------------------------------
// 
#if defined(HAVE_OP_MESH_SELECT_MESH) || defined(eEDITOR)
eOP_DEF_MESH(eMeshSelectMeshOp, "Select Mesh", 'l', 1, 16, eOP_INPUTS(eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH, eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH,eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eMeshSelectMeshOp, countF, "Input", 0, 15, 0,
		eOP_PAR_END))
    {
        _copyFirstInputMesh();
		eU32 nr = eClamp((eU32)0, (eU32)eFtoL(countF), getAboveOpCount() - 1);
		m_mesh = eEditMesh(((eIMeshOp*)getAboveOp(nr))->getResult().mesh);
    }
	eOP_EXEC2_END
eOP_END(eMeshSelectMeshOp);
#endif

// Bitmap to Mesh (mesh) operator
// ------------------------------------
// 
#if defined(HAVE_OP_MESH_BITMAP2MESH) || defined(eEDITOR)
eOP_DEF_MESH(eMeshBitmap2MeshOp, "Bitmap2Mesh", 'l', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
		eOP_PAR_END)
    {
		const eIBitmapOp::Result& res = ((eIBitmapOp*)getAboveOp(0))->getResult(); 
		eU32 size = res.width * res.height;
        eArray<eColor> inp;
		inp.reserve(size);
		m_mesh.reserve(size * 4, size * 2);
        eGfx->readTexture2d(res.uav->tex, inp);
		eF32 stepX = 1.0f / ((eF32)res.width - 1);
		eF32 stepY = -1.0f / ((eF32)res.height - 1);
		eU32 idx = 0;
		for(eU32 y = 0; y < res.height - 1; y++) {
			for(eU32 x = 0; x < res.width - 1; x++) {
				if(inp[y * res.width + x].r != 0) {
					eF32 px = (eF32)x * stepX;
					eF32 py = (eF32)y * stepY;
					eVector3 normal(0,0,1);
					m_mesh.addWedge(eVector3(px, py, 0), normal, eVector2(), eCOL_WHITE);
					m_mesh.addWedge(eVector3(px+stepX, py, 0), normal, eVector2(), eCOL_WHITE);
					m_mesh.addWedge(eVector3(px+stepX, py+stepY, 0), normal, eVector2(), eCOL_WHITE);
					m_mesh.addWedge(eVector3(px, py+stepY, 0), normal, eVector2(), eCOL_WHITE);
					m_mesh.addQuad(idx, idx + 1, idx + 2, idx + 3, nullptr);
					idx += 4;
				}
			}
		}

    }
	eOP_EXEC2_END
eOP_END(eMeshBitmap2MeshOp);
#endif
