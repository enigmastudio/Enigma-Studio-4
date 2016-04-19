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

#include "../../eshared.hpp"
#include "../../math/math.hpp"
#include "../../engine/engine.hpp"

#define X 0
#define Y 1
#define Z 2
#define SQR(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

// Slice (mesh) operator
// ---------------------
// Cuts a mesh along random planes into multiple
// pieces. Each piece has a different tag number
// for later sub-mesh extraction by model operators.

#if defined(HAVE_OP_MESH_SLICE) || defined(eEDITOR)
eOP_DEF_MESH(eSliceOp, "Slice", ' ', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eSliceOp, numCuts, "Number of cuts", 1, eU16_MAX, 10,
        eOP_PAR_FLOAT(eSliceOp, minArea, "Minimum area", eALMOST_ZERO, eF32_MAX, 0.1f,
        eOP_PAR_FLOAT(eSliceOp, maxArea, "Maximum area", eALMOST_ZERO, eF32_MAX, 0.5f,
        eOP_PAR_INT(eSliceOp, maxTries, "Maximum tries", 1, eU16_MAX, 10,
        eOP_PAR_INT(eSliceOp, seed, "Seed", 0, 65535, 0,
        eOP_PAR_BOOL(eSliceOp, closeHoles, "Close holes", eTRUE,
		eOP_PAR_END)))))))
    {
        eRandomize(seed);
        _copyFirstInputMesh();

        eArray<eEditMesh *> piecesSrc, piecesDst;
        eArray<eEditMesh *> *psrc = &piecesSrc;
        eArray<eEditMesh *> *pdst = &piecesDst;
        eEditMesh *firstMesh = new eEditMesh;
        firstMesh->merge(m_mesh);
        psrc->append(firstMesh);

        for (eU32 i=0; i<numCuts; i++)
        {
            for (eInt j=(eInt)psrc->size()-1; j>=0; j--)
            {
                const eEditMesh *mesh = (*psrc)[j];
                eEditMesh *leftMesh = new eEditMesh;
                eEditMesh *rightMesh = new eEditMesh;
                eU32 tries = 0;

                while (tries++ < maxTries)
                {
                    const eAABB &aabb = mesh->getBoundingBox();
                    const eVector3 point = aabb.getSize().random()+aabb.getCenter();
                    const ePlane plane(point, eVector3(1.0f).random());

                    _cutMesh(*mesh, plane, *leftMesh, *rightMesh, closeHoles);

                    const eF32 leftArea = leftMesh->getMeshArea();
                    const eF32 rightArea = rightMesh->getMeshArea();

                    if (leftArea >= minArea && leftArea <= maxArea &&
                        rightArea >= minArea && rightArea <= maxArea)
                    {
                        pdst->append(leftMesh);
                        pdst->append(rightMesh);
                        break;
                    }
                }

                if (tries > maxTries) // no appropriate splitting plane could be found
                {
                    eDelete(leftMesh);
                    eDelete(rightMesh);
                    pdst->append(new eEditMesh(*mesh));
                }
            }

            for (eU32 j=0; j<psrc->size(); j++)
                eDelete((*psrc)[j]);
            psrc->clear();

            eSwap(pdst, psrc);
        }

        if (pdst->isEmpty())
            pdst = psrc;

        m_mesh.clear();
        for (eU32 i=0; i<pdst->size(); i++)
        {
            for (eU32 j=0; j<(*pdst)[i]->getFaceCount(); j++)
                (*pdst)[i]->getFace(j).tag = i+1;

            m_mesh.merge(*(*pdst)[i]);
        }

        for (eU32 j=0; j<pdst->size(); j++)
            eDelete((*pdst)[j]);
    }
	eOP_EXEC2_END

    void _cutMesh(const eEditMesh &mesh, const ePlane &plane, eEditMesh &leftMesh, eEditMesh &rightMesh, eBool closeHoles) const
    {
        leftMesh.clear();
        rightMesh.clear();

        // convert to super simple mesh data structure
        eArray<Face> faces(mesh.getFaceCount());
        for (eU32 i=0; i<mesh.getFaceCount(); i++)
        {
            const eEmFace &face = mesh.getFace(i);
            Face &f = faces[i];
            f.count = face.count;
            for (eU32 j=0; j<face.count; j++)
                f.verts[j] = mesh.getPosition(face.posIdx[j]).pos;
        }

        // perform splitting
        eArray<Face> frontMesh, backMesh;
        frontMesh.reserve(mesh.getFaceCount());
        backMesh.reserve(mesh.getFaceCount());

        for (eInt i=(eInt)faces.size()-1; i>=0; i--)
        {
            Face frontFace, backFace;
            if (_splitFace(faces[i], plane, frontFace, backFace))
            {
                if (frontFace.count > 0)
                    frontMesh.append(frontFace);
                if (backFace.count > 0)
                    backMesh.append(backFace);
            }
            else if (frontFace.count > 0)
                frontMesh.append(frontFace);
            else if (backFace.count > 0)
                backMesh.append(backFace);
            else
                eASSERT(eFALSE);
        }

        // merge meshes
        for (eU32 i=0; i<frontMesh.size(); i++)
        {
            Face &f=frontMesh[i];
            if (f.count < 3 || f.count > 8) continue;
            eU32 wedges[8];
            for (eU32 j=0; j<f.count; j++)
            {
                wedges[j] = leftMesh.getWedgeCount();
                leftMesh.addWedge(f.verts[j], eVector3(), eVector2(), eCOL_WHITE);
            }
            leftMesh.addFace(wedges, f.count, nullptr);
        }
        leftMesh.unifyPositions();

        for (eU32 i=0; i<backMesh.size(); i++)
        {
            Face &f=backMesh[i];
            if (f.count < 3 || f.count > 8) continue;
            eU32 wedges[8];
            for (eU32 j=0; j<f.count; j++)
            {
                wedges[j] = rightMesh.getWedgeCount();
                rightMesh.addWedge(f.verts[j], eVector3(), eVector2(), eCOL_WHITE);
            }
            rightMesh.addFace(wedges, f.count, nullptr);
        }
        rightMesh.unifyPositions();

        // finalize mesh
        if (closeHoles)
        {
            _closeHoles(leftMesh);
            _closeHoles(rightMesh);
        }

        leftMesh.calcNormals();
        leftMesh.calcBoundingBox();

        rightMesh.calcNormals();
        rightMesh.calcBoundingBox();
    }

    void _closeHoles(eEditMesh &mesh) const
    {
        // close holes
        mesh.calcAdjacency();

        eArray<eEmEdge> boundaryEdges;
        for (eU32 i=0; i<mesh.getEdgeCount(); i++)
            if (mesh.getEdge(i).twin == -1)
                boundaryEdges.append(mesh.getEdge(i));

        while (!boundaryEdges.isEmpty())
        {
            eArray<eEmEdge> holeContour;
            holeContour.append(boundaryEdges.first());
            boundaryEdges.removeAt(0);
            
            while (eTRUE)
            {
                eEmEdge &findNextFor = holeContour.last();
                eBool found=eFALSE;
                for (eU32 j=0; j<boundaryEdges.size(); j++)
                {
                    if (boundaryEdges[j].endPos == findNextFor.startPos)
                    {
                        holeContour.append(boundaryEdges[j]);
                        boundaryEdges.removeAt(j);
                        found = eTRUE;
                        break;
                    }
                }
                if (!found)
                    break;
            }

            eU32 *wdgs = eALLOC_STACK(eU32, holeContour.size());
            for (eU32 j=0; j<holeContour.size(); j++)
                wdgs[j] = holeContour[j].startWdg;

            if (holeContour.size() > 8) // triangulate on the fly
            {
                for (eU32 j=2; j<holeContour.size(); j++)
                {
                    eU32 triWdgs[3];
                    
                    triWdgs[0] = wdgs[0];
                    triWdgs[1] = wdgs[j-1];
                    triWdgs[2] = wdgs[j];


                    triWdgs[0] = mesh.addWedge(mesh.getWedge(triWdgs[0]).posIdx, mesh.addNormal(eVector3()), mesh.addProperty(eVector2(), eCOL_WHITE));
                    triWdgs[1] = mesh.addWedge(mesh.getWedge(triWdgs[1]).posIdx, mesh.addNormal(eVector3()), mesh.addProperty(eVector2(), eCOL_WHITE));
                    triWdgs[2] = mesh.addWedge(mesh.getWedge(triWdgs[2]).posIdx, mesh.addNormal(eVector3()), mesh.addProperty(eVector2(), eCOL_WHITE));

                    mesh.addFace(triWdgs, 3, nullptr);
                }
            }
            else if (holeContour.size() > 2)
            {
                for (eU32 j=0; j<holeContour.size(); j++)
                {
                    wdgs[j] = mesh.addWedge(mesh.getWedge(wdgs[j]).posIdx, mesh.addNormal(eVector3()), mesh.addProperty(eVector2(), eCOL_WHITE));
                }

                mesh.addFace(wdgs, holeContour.size(), nullptr);
            }
        }

        int b=0;
    }

    struct Face
    {
        eVector3 verts[32];
        eU32     count;
    };

    eBool _splitFace(const Face &face, const ePlane &p, Face &frontFace,Face &backFace) const
    {
        eBool crossing = eFALSE;
        frontFace.count = 0;
        backFace.count = 0;

        eVector3 vl = face.verts[face.count-1];
        ePlaneSide cls, lastCls = p.getSide(vl);
        eVector3 vi, vn, splitPoint;

        for (eU32 j=face.count-1, k=0; k<face.count; j=k, lastCls=cls, vl=vi, k++)
        {
            vi = face.verts[k];
            cls = p.getSide(vi);

            if (lastCls == ePLS_BACK && cls == ePLS_BACK)
            {
                backFace.verts[backFace.count++] = vi;
            }
            else if (lastCls == ePLS_BACK && cls == ePLS_ON)
            {
                backFace.verts[backFace.count++]  = vi;
                frontFace.verts[frontFace.count++] = vi;
            }
            else if (lastCls == ePLS_BACK && cls == ePLS_FRONT)
            {
                p.intersects(vi, vl, &vn);
                backFace.verts[backFace.count++] = vn;
                frontFace.verts[frontFace.count++] = vn;
                frontFace.verts[frontFace.count++] = vi;
            }
            else if (lastCls == ePLS_ON && cls == ePLS_BACK)
            {
                backFace.verts[backFace.count++] = vl;
                backFace.verts[backFace.count++] = vi;
            }
            else if ((lastCls == ePLS_ON && cls == ePLS_ON) || (lastCls == ePLS_ON && cls == ePLS_FRONT))
            {
                frontFace.verts[frontFace.count++] = vi;
            }
            else if (lastCls == ePLS_FRONT && cls == ePLS_BACK)
            {
                p.intersects(vl, vi, &vn);
                crossing = eTRUE;
                frontFace.verts[frontFace.count++] = vn;
                backFace.verts[backFace.count++] = vn;
                backFace.verts[backFace.count++] = vi;
            }
            else if ((lastCls == ePLS_FRONT && cls == ePLS_ON) || (lastCls == ePLS_FRONT && cls == ePLS_FRONT))
            {
                frontFace.verts[frontFace.count++] = vi;
            }
        } 

        return crossing;
    }
eOP_END(eSliceOp);
#endif

// Explode (model) operator
// ------------------------
// 

#if defined(HAVE_OP_MODEL_EXPLODE) || defined(eEDITOR)
eOP_DEF_MODEL(eExplodeOp, "Explode", ' ', 1, 1, eOP_INPUTS(eOC_MESH|eOC_MODEL))
    eOP_INIT()
    {
        m_lastTime = -1.0f;
    }

    eOP_DEINIT()
    {
        _clear();
    }

    eVector3 m_explPos;
    eVector3 m_explSpeed;
    eVector3 m_gravity;
    eF32     m_ignitionTime;
    eF32    m_angVel;

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eExplodeOp, time, "Time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eExplodeOp, ignitionTime, "Ignition time", 0.0f, eF32_MAX, 2.0f,
        eOP_PAR_FXYZ(eExplodeOp, explPos, "Explosion position", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZ(eExplodeOp, explSpeed, "Explosion speed", 0.0f, eF32_MAX, 0.1f, 0.1f, 0.1f,
        eOP_PAR_FLOAT(eExplodeOp, angVel, "Angular speed", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_FXYZ(eExplodeOp, gravity, "Gravity", eF32_MIN, eF32_MAX, 0.0f, -9.81f, 0.0f,
        eOP_PAR_FLOAT(eExplodeOp, delay, "Delay", 0.0f, eF32_MAX, 1.0f,
		eOP_PAR_END))))))))
    {
        // initialize
        if (getAboveOp(0)->getChanged())
        {
            _clear();

            if (getAboveOp(0)->getResultClass() == eOC_MESH)
            {
                const eEditMesh &mesh = ((eIMeshOp *)getAboveOp(0))->getResult().mesh;

                eArray<eU32> tags;
                for (eU32 i=0; i<mesh.getFaceCount(); i++)
                    tags.appendUnique(mesh.getFace(i).tag);

                m_rbs.resize(tags.size());

                for (eU32 i=0; i<tags.size(); i++)
                {
                    m_rbs[i].em = new eEditMesh;
                    _extractMeshByTag(mesh, tags[i], *m_rbs[i].em);
                    m_rbs[i].mesh = new eMesh;
                    m_rbs[i].mesh->fromEditMesh(*m_rbs[i].em, (getAboveOp(0)->isAnimated() ? eMT_DYNAMIC : eMT_STATIC));
                    m_rbs[i].mi = new eMeshInst(*m_rbs[i].mesh, eFALSE);
                }
            }
            else if (getAboveOp(0)->getResultClass() == eOC_MODEL)
            {
                // duplicate renderables from parent
            }
        }

                //eF32 dt = time;//-m_lastTime;

        if (time <= ignitionTime || getAboveOp(0)->getChanged() || m_explPos != explPos || m_explSpeed != explSpeed || m_gravity != gravity || m_angVel != angVel || m_ignitionTime != ignitionTime)
        {
            m_explPos = explPos;
            m_explSpeed = explSpeed;
            m_gravity = gravity;
            m_ignitionTime = ignitionTime;
            m_angVel = angVel;
            m_lastTime = time;

            for (eU32 i=0; i<m_rbs.size(); i++)
            {
                m_rbs[i].rot.set(0,0,0,1);
                m_rbs[i].pos.null();
                _initRigidBody(m_rbs[i], explPos, explSpeed);
            }
        }

        // simulate

        for (eU32 i=0; i<m_rbs.size(); i++)
        {
            RigidBody &pm = m_rbs[i];

            // update physical properties
            if (time > ignitionTime)
            {
                eF32 t = time-ignitionTime;

                const eF32 objDensity = 1.0f;
                const eF32 mass = pm.vol*objDensity;

                pm.rot = eQuat(pm.rotAxis, t*pm.angVel*angVel);

                eVector3 force = gravity;
                eVector3 accel = force/mass;

                pm.pos = 0.5f*accel*t*t + pm.linearVel*t;
            }

            eTransform t0(eQuat(), -pm.massCenter, eVector3(1.0f));
            eTransform t1(pm.rot, eVector3(0.0f), eVector3(1.0f));
            eTransform t2(eQuat(), pm.massCenter, eVector3(1.0f));
            eTransform t3(eQuat(), pm.pos, eVector3(1.0f));

            eTransform tges = t0*t1*t2*t3;

            m_sceneData.addRenderable(m_rbs[i].mi, tges);
        }

        m_lastTime = time;
    }
	eOP_EXEC2_END

    void _clear()
    {
        for (eU32 i=0; i<m_rbs.size(); i++)
        {
            eDelete(m_rbs[i].em);
            eDelete(m_rbs[i].mesh);
            eDelete(m_rbs[i].mi);
        }

        m_rbs.clear();
    }

    // physikalische Groessen:
    // -----------------------------------
    //     F: force
    //     r: Hebelarm an der Drehachse
    //     L: Drehimpuls ("angular momentum")
    //     I: Traegheitstensor
    // tau/M: Drehmoment
    // omega: Winkelgeschwindigkeit
    //
    // Zusammenhaenge:
    // -----------------------------------
    // L=I*omega <=> I^-1*L=omega
    struct RigidBody
    {
        eEditMesh * em;
        eMesh *     mesh;
        eMeshInst * mi;
        eVector3    pos;        // current position
        eQuat       rot;        // current rotation

        eF32        vol;        // volume of mesh (for homogenous density: mass=density*volume)
        eVector3    massCenter; // center of mass
        eMatrix3x3  inertia;    // untransformed inertia tensor ("Traegheitstensor"), unit I
        eVector3    rotAxis;    // axis of rotation the torque acts on
        eF32        angVel;     // magnitude of rotation axis (speed of rotation around this axis)
        eVector3    linearVel;  // translational velocity

        /*
        eVector3    torque;     // "Drehmoment", unit tau or M, tau=rxF
        eVector3    vel;        // linear velocity
        */
    };

    void _initRigidBody(RigidBody &rb, const eVector3 &exploPos, const eVector3 &exploSpeed)
    {
        _calcPhysicalProperties(rb);

        eVector3 dir = rb.massCenter-exploPos;
        eF32 dist = dir.length();
        dir.normalize();

        eVector3 force = exploSpeed*dist;
        force.scale(dir);

        const eVector3 hitPoint = rb.massCenter-exploPos;
        const eVector3 torque = hitPoint^force; // equal to angular momentum
        const eVector3 angVel = 0.1f;//rb.inertia.inverse()*torque;

        rb.angVel = angVel.length();
        rb.rotAxis.set(1.0f,1.0f,1.0f);// = angVel.normalized();
        rb.rotAxis = rb.rotAxis.random();
        rb.rotAxis.normalize();
        rb.linearVel = force;
    }

    eMatrix3x3 _transformInertiaTensor(const eMatrix3x3 &inertiaTensor, const eQuat &rot) const
    {
        const eMatrix3x3 mtxRot(rot);
        return mtxRot*inertiaTensor*mtxRot.transposed();
    }

int A;   /* alpha */
int B;   /* beta */
int C;   /* gamma */
double P1, Pa, Pb, Paa, Pab, Pbb, Paaa, Paab, Pabb, Pbbb; /* projection integrals */
double Fa, Fb, Fc, Faa, Fbb, Fcc, Faaa, Fbbb, Fccc, Faab, Fbbc, Fcca; /* face integrals */
double T0, T1[3], T2[3], TP[3]; /* volume integrals */

    void _calcPhysicalProperties(RigidBody &rb)
    {
        compVolumeIntegrals(*rb.em);

        rb.vol = (eF32)T0;
        rb.massCenter.null();
        rb.inertia.identity();

        if (!rb.mesh->getVertexCount())
            return;

        for (eU32 i=0; i<rb.mesh->getVertexCount(); i++)
            rb.massCenter += rb.mesh->getVertex(i).pos;
        rb.massCenter /= (eF32)rb.mesh->getVertexCount();
    }

    void _extractMeshByTag(const eEditMesh &mesh, eU32 tag, eEditMesh &res)
    {
        eU32 *remapWdg = eALLOC_STACK(eU32, mesh.getWedgeCount());
        eMemSet(remapWdg, 0xff, sizeof(eU32)*mesh.getWedgeCount());
        eU32 *remapPos = eALLOC_STACK(eU32, mesh.getPositionCount());
        eMemSet(remapPos, 0xff, sizeof(eU32)*mesh.getPositionCount());
        eU32 *remapNrm = eALLOC_STACK(eU32, mesh.getNormalCount());
        eMemSet(remapNrm, 0xff, sizeof(eU32)*mesh.getNormalCount());
        eU32 *remapProp = eALLOC_STACK(eU32, mesh.getPropertyCount());
        eMemSet(remapProp, 0xff, sizeof(eU32)*mesh.getPropertyCount());

        for (eU32 i=0; i<mesh.getFaceCount(); i++)
        {
            const eEmFace &face = mesh.getFace(i);
            if (face.tag == tag)
            {
                eU32 newWdgs[8];

                for (eU32 j=0; j<face.count; j++)
                {
                    const eEmWedge &wedge = mesh.getWedge(face.wdgIdx[j]);

                    if (remapPos[wedge.posIdx] == 0xffffffff)
                    {
                        remapPos[wedge.posIdx] = res.getPositionCount();
                        res.addPosition(mesh.getPosition(wedge.posIdx).pos);
                    }

                    if (remapNrm[wedge.nrmIdx] == 0xffffffff)
                    {
                        remapNrm[wedge.nrmIdx] = res.getNormalCount();
                        res.addNormal(mesh.getNormal(wedge.nrmIdx));
                    }

                    if (remapProp[wedge.propsIdx] == 0xffffffff)
                    {
                        remapProp[wedge.propsIdx] = res.getPropertyCount();
                        const eEmVtxProps &p = mesh.getProperty(wedge.propsIdx);
                        res.addProperty(p.uv, p.col);
                    }

                    if (remapWdg[face.wdgIdx[j]] == 0xffffffff)
                    {
                        remapWdg[face.wdgIdx[j]] = res.getWedgeCount();
                        res.addWedge(remapPos[wedge.posIdx], remapNrm[wedge.nrmIdx], remapProp[wedge.propsIdx]);
                    }

                    newWdgs[j] = remapWdg[face.wdgIdx[j]];
                }

                res.addFace(newWdgs, face.count, face.mat);
            }

            res.calcNormals();
            res.calcBoundingBox();
        }
    }

    // compute various integrations over projection of face
    void compProjectionIntegrals(eEditMesh &m, eEmFace &f)
    {
        double a0, a1, da;
        double b0, b1, db;
        double a0_2, a0_3, a0_4, b0_2, b0_3, b0_4;
        double a1_2, a1_3, b1_2, b1_3;
        double C1, Ca, Caa, Caaa, Cb, Cbb, Cbbb;
        double Cab, Kab, Caab, Kaab, Cabb, Kabb;
        eU32 i;

        P1 = Pa = Pb = Paa = Pab = Pbb = Paaa = Paab = Pabb = Pbbb = 0.0;

        for (i = 0; i < f.count; i++) {

            a0 = m.getPosition(f.posIdx[i]).pos[A];
            b0 = m.getPosition(f.posIdx[i]).pos[B];
            a1 = m.getPosition(f.posIdx[(i+1)%f.count]).pos[A];
            b1 = m.getPosition(f.posIdx[(i+1)%f.count]).pos[B];

            da = a1 - a0;
            db = b1 - b0;
            a0_2 = a0 * a0; a0_3 = a0_2 * a0; a0_4 = a0_3 * a0;
            b0_2 = b0 * b0; b0_3 = b0_2 * b0; b0_4 = b0_3 * b0;
            a1_2 = a1 * a1; a1_3 = a1_2 * a1; 
            b1_2 = b1 * b1; b1_3 = b1_2 * b1;

            C1 = a1 + a0;
            Ca = a1*C1 + a0_2; Caa = a1*Ca + a0_3; Caaa = a1*Caa + a0_4;
            Cb = b1*(b1 + b0) + b0_2; Cbb = b1*Cb + b0_3; Cbbb = b1*Cbb + b0_4;
            Cab = 3*a1_2 + 2*a1*a0 + a0_2; Kab = a1_2 + 2*a1*a0 + 3*a0_2;
            Caab = a0*Cab + 4*a1_3; Kaab = a1*Kab + 4*a0_3;
            Cabb = 4*b1_3 + 3*b1_2*b0 + 2*b1*b0_2 + b0_3;
            Kabb = b1_3 + 2*b1_2*b0 + 3*b1*b0_2 + 4*b0_3;

            P1 += db*C1;
            Pa += db*Ca;
            Paa += db*Caa;
            Paaa += db*Caaa;
            Pb += da*Cb;
            Pbb += da*Cbb;
            Pbbb += da*Cbbb;
            Pab += db*(b1*Cab + b0*Kab);
            Paab += db*(b1*Caab + b0*Kaab);
            Pabb += da*(a1*Cabb + a0*Kabb);
        }

        P1 /= 2.0;
        Pa /= 6.0;
        Paa /= 12.0;
        Paaa /= 20.0;
        Pb /= -6.0;
        Pbb /= -12.0;
        Pbbb /= -20.0;
        Pab /= 24.0;
        Paab /= 60.0;
        Pabb /= -60.0;
    }

    void compFaceIntegrals(eEditMesh &m, eEmFace &f)
    {
        const eF32 *n;
        double w;
        double k1, k2, k3, k4;

        compProjectionIntegrals(m, f);

        //w = f->w;

        w = - f.normal[X] * m.getPosition(f.posIdx[0]).pos[X]
            - f.normal[Y] * m.getPosition(f.posIdx[0]).pos[Y]
            - f.normal[Z] * m.getPosition(f.posIdx[0]).pos[Z];

        n = f.normal;
        k1 = 1 / n[C]; k2 = k1 * k1; k3 = k2 * k1; k4 = k3 * k1;

        Fa = k1 * Pa;
        Fb = k1 * Pb;
        Fc = -k2 * (n[A]*Pa + n[B]*Pb + w*P1);

        Faa = k1 * Paa;
        Fbb = k1 * Pbb;
        Fcc = k3 * (SQR(n[A])*Paa + 2*n[A]*n[B]*Pab + SQR(n[B])*Pbb
            + w*(2*(n[A]*Pa + n[B]*Pb) + w*P1));

        Faaa = k1 * Paaa;
        Fbbb = k1 * Pbbb;
        Fccc = -k4 * (CUBE(n[A])*Paaa + 3*SQR(n[A])*n[B]*Paab 
            + 3*n[A]*SQR(n[B])*Pabb + CUBE(n[B])*Pbbb
            + 3*w*(SQR(n[A])*Paa + 2*n[A]*n[B]*Pab + SQR(n[B])*Pbb)
            + w*w*(3*(n[A]*Pa + n[B]*Pb) + w*P1));

        Faab = k1 * Paab;
        Fbbc = -k2 * (n[A]*Pabb + n[B]*Pbbb + w*Pbb);
        Fcca = k3 * (SQR(n[A])*Paaa + 2*n[A]*n[B]*Paab + SQR(n[B])*Pabb
            + w*(2*(n[A]*Paa + n[B]*Pab) + w*Pa));
    }

    void compVolumeIntegrals(eEditMesh &m)
    {
        double nx, ny, nz;
        T0 = T1[X] = T1[Y] = T1[Z] = T2[X] = T2[Y] = T2[Z] = TP[X] = TP[Y] = TP[Z] = 0;

        for (eU32 i = 0; i < m.getFaceCount(); i++) {

            eEmFace &f = m.getFace(i);

            nx = eAbs(f.normal.x);
            ny = eAbs(f.normal.y);
            nz = eAbs(f.normal.z);
            if (nx > ny && nx > nz) C = X;
            else C = (ny > nz) ? Y : Z;
            A = (C + 1) % 3;
            B = (A + 1) % 3;

            compFaceIntegrals(m, f);

            T0 += f.normal[X] * ((A == X) ? Fa : ((B == X) ? Fb : Fc));

            T1[A] += f.normal[A] * Faa;
            T1[B] += f.normal[B] * Fbb;
            T1[C] += f.normal[C] * Fcc;
            T2[A] += f.normal[A] * Faaa;
            T2[B] += f.normal[B] * Fbbb;
            T2[C] += f.normal[C] * Fccc;
            TP[A] += f.normal[A] * Faab;
            TP[B] += f.normal[B] * Fbbc;
            TP[C] += f.normal[C] * Fcca;
        }

        T1[X] /= 2; T1[Y] /= 2; T1[Z] /= 2;
        T2[X] /= 3; T2[Y] /= 3; T2[Z] /= 3;
        TP[X] /= 2; TP[Y] /= 2; TP[Z] /= 2;
    }

    eOP_VAR(eArray<RigidBody> m_rbs);
    eOP_VAR(eF32              m_lastTime);
eOP_END(eExplodeOp);
#endif