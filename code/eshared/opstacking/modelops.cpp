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

#include "../eshared.hpp"

eDEF_OPERATOR_SOURCECODE(MODEL);

// Model (model) operator
// ----------------------
// Converts the input mesh operator into
// a model operator.

#if defined(HAVE_OP_MODEL_MODEL) || defined(eEDITOR)
eOP_DEF_MODEL(eModelModelOp, "Model", 'o', 1, 1, eOP_INPUTS(eOC_MESH))
    eOP_INIT()
    {
        m_mi = nullptr;
    }

    eOP_DEINIT()
    {
        eDelete(m_mi);
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_BOOL(eModelModelOp, castsShadows, "Throws shadow", eTRUE,
        eOP_PAR_LABEL(eModelModelOp, label0, "Level of detail", "Level of detail",
        eOP_PAR_FLOAT(eModelModelOp, dist1, "Distance 1", 0.0f, eF32_MAX, 20.0f,
        eOP_PAR_LINK_MESH(eModelModelOp, lodMesh1, "Mesh 1", eOC_MESH, eFALSE,
        eOP_PAR_FLOAT(eModelModelOp, dist2, "Distance 2", 0.0f, eF32_MAX, 40.0f,
        eOP_PAR_LINK_MESH(eModelModelOp, lodMesh2, "Mesh 2", eOC_MESH, eFALSE,
        eOP_PAR_FLOAT(eModelModelOp, dist3, "Distance 3", 0.0f, eF32_MAX, 60.0f,
        eOP_PAR_LINK_MESH(eModelModelOp, lodMesh3, "Mesh 3", eOC_MESH, eFALSE,
		eOP_PAR_END)))))))))
    {
        const eIMeshOp *meshOp = (eIMeshOp *)getAboveOp(0);
        eU32 lodLevels = 0;

        _createMesh(meshOp, 0.0f, lodLevels, castsShadows);
        _createMesh(lodMesh1, dist1, lodLevels, castsShadows);
        _createMesh(lodMesh2, dist2, lodLevels, castsShadows);
        _createMesh(lodMesh3, dist3, lodLevels, castsShadows);

        eDelete(m_mi);
        m_mi = new eMeshInst(m_lodMeshes, lodLevels, castsShadows);
        m_sceneData.addRenderable(m_mi);
    }
	eOP_EXEC2_END

    void _createMesh(const eIMeshOp *op, eF32 minDist, eU32 &lodLevel, eBool castsShadows)
    {
        if (op)
        {
            m_meshes[lodLevel].fromEditMesh(op->getResult().mesh, (op->isAnimated() ? eMT_DYNAMIC : eMT_STATIC));
            m_lodMeshes[lodLevel].mesh = &m_meshes[lodLevel];
            m_lodMeshes[lodLevel].minDist = minDist;
            lodLevel++;
        }
    }

    eOP_VAR(eLodMesh    m_lodMeshes[4]);
    eOP_VAR(eMesh       m_meshes[4]);
    eOP_VAR(eMeshInst * m_mi);
eOP_END(eModelModelOp);
#endif

// Transform (model) operator
// --------------------------
// Transforms a model-operator.

#if defined(HAVE_OP_MODEL_TRANSFORM) || defined(eEDITOR)
eOP_DEF_MODEL(eModelTransformOp, "Transform", 't', 1, 1, eOP_INPUTS(eOC_MODEL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZ(eModelTransformOp, trans, "Translate", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eModelTransformOp, rot, "Rotate", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eModelTransformOp, scale, "Scale", eF32_MIN, eF32_MAX, 1, 1, 1,
		eOP_PAR_END))))
    {
        eSceneData &sd = ((eIModelOp *)getAboveOp(0))->getResult().sceneData;
        m_sceneData.merge(sd, eTransform(eQuat(rot*eTWOPI), trans, scale));
    }
	eOP_EXEC2_END

    eOP_INTERACT(eSceneData &sd, eOpInteractionInfos &oii)
    {
        eVector3 &transl = (eVector3 &)getParameter(0).getAnimValue().fxyz;
        eVector3 &rot = (eVector3 &)getParameter(1).getAnimValue().fxyz;
        eVector3 &scale = (eVector3 &)getParameter(2).getAnimValue().fxyz;

        if (oii.srtCtrl.interact(oii.input, oii.cam, sd, scale, rot, transl) == eTWR_NOCHANGES)
        {
            oii.srtCtrl.setPosition(m_sceneData.getBoundingBox().getCenter());
            return eFALSE;
        }
        else
        {
            setChanged();
            return eTRUE;
        }
    }
eOP_END(eModelTransformOp);
#endif

// Merge (model) operator
// ----------------------
// Merges multiple model operators.

#if defined(HAVE_OP_MODEL_MERGE) || defined(eEDITOR)
eOP_DEF_MODEL(eModelMergeOp, "Merge", 'm', 1, 32, eOP_INPUTS(eOP_32INPUTS(eOC_MODEL)))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        for (eU32 i=0; i<getAboveOpCount(); i++)
        {
            eSceneData &sd = ((eIModelOp *)getAboveOp(i))->getResult().sceneData;
            m_sceneData.merge(sd);
        }
    }
	eOP_EXEC2_END
eOP_END(eModelMergeOp);
#endif

// Multiply (model) operator
// -------------------------
// Duplicates and randomizes scene-graph instances.

#if defined(HAVE_OP_MODEL_MULTIPLY) || defined(eEDITOR)
eOP_DEF_MODEL(eModelMultiplyOp, "Multiply", 'u', 1, 1, eOP_INPUTS(eOC_MODEL))
    static void addElement(ePtr parent, eConstPtr element, const eTransform &transf)
    {
        ((eSceneData *)parent)->merge(*(const eSceneData *)element, transf);
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eModelMultiplyOp, mode, "Mode", "Linear|Circular|Along path", 0,
        eOP_PAR_INT(eModelMultiplyOp, count, "Count", 1, 65536, 1,
        eOP_PAR_LABEL(eModelMultiplyOp, label0, "Linear", "Linear",
        eOP_PAR_FXYZ(eModelMultiplyOp, trans, "Translate", eF32_MIN, eF32_MAX, 1, 0, 0,
        eOP_PAR_FXYZ(eModelMultiplyOp, rot, "Rotate", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eModelMultiplyOp, scale, "Lin.Scale", eF32_MIN, eF32_MAX, 1, 1, 1,
        eOP_PAR_LABEL(eModelMultiplyOp, label1, "Circular", "Circular",
        eOP_PAR_FLOAT(eModelMultiplyOp, radius, "Radius", eF32_MIN, eF32_MAX, 5.0f,
        eOP_PAR_FXYZ(eModelMultiplyOp, circularRot, "Rotation", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_LABEL(eModelMultiplyOp, label2, "Along path", "Along path",
        eOP_PAR_FLOAT(eModelMultiplyOp, timeStart, "Time start", eF32_MIN, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eModelMultiplyOp, timeEnd, "Time end", eF32_MIN, eF32_MAX, 1.0f,
        eOP_PAR_FXYZ(eModelMultiplyOp, pathScale, "Scale", eF32_MIN, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_FXYZ(eModelMultiplyOp, upVector, "Up vector", eF32_MIN, eF32_MAX, 0.0f, 1.0f, 0.0f,
        eOP_PAR_LINK_PATH(eModelMultiplyOp, pathOp, "Path", eOC_PATH, eFALSE,
        eOP_PAR_LABEL(eModelMultiplyOp, label3, "Randomize", "Randomize",
        eOP_PAR_FXYZ(eModelMultiplyOp, randTrans, "Rand. trans.", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eModelMultiplyOp, randRot, "Rand. rotation", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_FXYZ(eModelMultiplyOp, randScale, "Rand. scale", eF32_MIN, eF32_MAX, 0, 0, 0,
        eOP_PAR_INT(eModelMultiplyOp, seed, "Seed", 0, 65535, 0,
		eOP_PAR_END)))))))))))))))))))))
    {
        const eSceneData &sd = ((const eIModelOp *)getAboveOp(0))->getResult().sceneData;
        eIMeshOp::processMultiply(mode, count, trans, rot, scale, radius, circularRot, timeStart, timeEnd, pathScale,
                                  upVector, pathOp, randTrans, randRot, randScale, seed, &m_sceneData, &sd, addElement);
    }
	eOP_EXEC2_END
eOP_END(eModelMultiplyOp);
#endif

// Center (model) operator
// -----------------------
// Centers all renderables of scene graph around
// the origin of the coordinate system (0, 0, 0).

#if defined(HAVE_OP_MODEL_CENTER) || defined(eEDITOR)
eOP_DEF_MODEL(eModelCenterOp, "Center", 'e', 1, 1, eOP_INPUTS(eOC_MODEL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        eSceneData &sd = ((eIModelOp *)getAboveOp(0))->getResult().sceneData;
        eTransform transf;
        transf.translate(-sd.getBoundingBox().getCenter());
        m_sceneData.merge(sd, transf);
    }
	eOP_EXEC2_END
eOP_END(eModelCenterOp);
#endif

// Light (model) operator
// ----------------------
// Adds a light to the scene graph.

#if defined(HAVE_OP_MODEL_LIGHT) || defined(eEDITOR)
eOP_DEF_MODEL(eLightOp, "Light", 'l', 0, 1, eOP_INPUTS(eOC_MODEL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZ(eLightOp, pos, "Position", eF32_MIN, eF32_MAX, 10.0f, 10.0f, 10.0f,
        eOP_PAR_FLOAT(eLightOp, range, "Range", eALMOST_ZERO, eF32_MAX, 50.0f,
        eOP_PAR_RGB(eLightOp, diffuseCol, "Diffuse", 255, 255, 255,
        eOP_PAR_RGB(eLightOp, ambientCol, "Ambient", 0, 0, 0,
        eOP_PAR_RGB(eLightOp, specCol, "Specular", 255, 255, 255,
        eOP_PAR_LABEL(eLightOp, label0, "Shadows", "Shadows",
        eOP_PAR_BOOL(eLightOp, castsShadows, "Casts shadows", eFALSE,
        eOP_PAR_FLAGS(eLightOp, shadowedFaces, "Shadowed faces", "X+|X-|Y+|Y-|Z+|Z-", 0x3f, // all on = first 6 bits set
        eOP_PAR_FLOAT(eLightOp, penumbraSize, "Penumbra size", 0.0f, eF32_MAX, 2.0f,
        eOP_PAR_FLOAT(eLightOp, shadowBias, "Shadow bias", 0.0f, 1.0f, 0.01f,
		eOP_PAR_END)))))))))))
    {
        if (getAboveOpCount() > 0)
        {
            eSceneData &sd = ((const eIModelOp *)getAboveOp(0))->getResult().sceneData;
            m_sceneData.merge(sd);
        }

        m_light.setDiffuse(diffuseCol);
        m_light.setAmbient(ambientCol);
        m_light.setSpecular(specCol);
        m_light.setPosition(pos);
        m_light.setRange(range);
        m_light.setPenumbraSize(penumbraSize);
        m_light.setShadowBias(shadowBias*0.1f); // *0.1, because available precision in editor is too small.

        for (eInt i=0; i<eCMF_COUNT; i++)
        {
            const eBool enable = (eGetBit(shadowedFaces, i) && castsShadows);
            m_light.setCastsShadows((eCubeMapFace)i, enable);
        }

        m_sceneData.addLight(&m_light);
    }
	eOP_EXEC2_END

	eOP_INTERACT(eSceneData &sd, eOpInteractionInfos &oii)
    {
        // light can only be moved by translate widget
        eVector3 &pos = (eVector3 &)getParameter(0).getAnimValue().fxyz;
        eBool res = eFALSE;

        oii.srtCtrl.setActiveWidget(eTWT_TRANS);

        if (oii.srtCtrl.interact(oii.input, oii.cam, sd, eVector3(1.0f), eVector3(0.0f), pos) != eTWR_NOCHANGES)
        {
            setChanged();
            res = eTRUE;
        }
        else
            oii.srtCtrl.setPosition(pos);

        // add light indicator to scene
        _addCircleToMesh(oii.infoMesh, 50, eVector3(   0.0f,    0.0f, eHALFPI));
        _addCircleToMesh(oii.infoMesh, 50, eVector3(   0.0f, eHALFPI,    0.0f));
        _addCircleToMesh(oii.infoMesh, 50, eVector3(eHALFPI,    0.0f,    0.0f));
        
        oii.infoMesh.finishLoading(eMT_DYNAMIC);
        oii.infoTransf.scale(eVector3(getParameter(1).getAnimValue().flt));
        oii.infoTransf.translate(pos);
        return res;
    }
    
    void _addCircleToMesh(eMesh &mesh, eU32 segCount, const eVector3 &rot)
    {
        eASSERT(segCount >= 3);

        const eU32 oldVtxCount = mesh.getVertexCount();
        const eF32 step = eTWOPI/(eF32)segCount;

        for (eU32 i=0; i<segCount; i++)
        {
            eVector3 pos;
            eSinCos(step*(eF32)i, pos.x, pos.y);
            pos.rotate(rot);
            mesh.addVertex(pos, eCOL_ORANGE);
        }

        for (eU32 i=0; i<segCount; i++)
            mesh.addLine(oldVtxCount+i, oldVtxCount+(i+1)%segCount, eMaterial::getWireframe());

        mesh.addLine(oldVtxCount, oldVtxCount+segCount/2, eMaterial::getWireframe());
    }

    eOP_VAR(eLight           m_light);
eOP_END(eLightOp);
#endif

// Particle system (model) operator
// --------------------------------
// Adds a particle system to model's scene graph.

#if defined(HAVE_OP_MODEL_PARTICLES) || defined(eEDITOR)
eREGISTER_ENGINE_SHADER(ps_particles);
eREGISTER_ENGINE_SHADER(vs_particles);
eOP_DEF_MODEL(eParticleSystemOp, "Particles", 'a', 0, 0, eOP_INPUTS())
    eOP_INIT()
    {		
        m_psysInst = new eParticleSysInst(m_psys);
    }

    eOP_DEINIT()
    {
        eDelete(m_psysInst);
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eParticleSystemOp, time, "Time", -eF32_MAX, eF32_MAX, 0.0f,
        eOP_PAR_LABEL(eParticleSystemOp, label0, "Emitter", "Emitter",
        eOP_PAR_ENUM(eParticleSystemOp, emitterMode, "Emitter Mode", "Faces|Edges|Vertices", 0,
        eOP_PAR_FLOAT(eParticleSystemOp, emissionFreq, "Emission frequency [1/s]", 0.01f, 100000.0f, 100.0f,
        eOP_PAR_LINK_MESH(eParticleSystemOp, emitterOp, "Emitter mesh", eOC_MESH, eTRUE,
        eOP_PAR_LABEL(eParticleSystemOp, label1, "Life of particle", "Life of particle",
        eOP_PAR_FLOAT(eParticleSystemOp, randomization, "Randomization", 0.0f, 1.0f, 1.0f,
        eOP_PAR_FLOAT(eParticleSystemOp, lifeTime, "Life time", 0.01f, 100.0f, 10.0f,
        eOP_PAR_FLOAT(eParticleSystemOp, emissionVel, "Emission velocity", -eF32_MAX, eF32_MAX, 1.0f,
        eOP_PAR_LINK_PATH(eParticleSystemOp, sizeOp, "Size", eOC_PATH, eFALSE,
        eOP_PAR_LINK_PATH(eParticleSystemOp, colorOp, "Color", eOC_PATH, eFALSE,
        eOP_PAR_LINK_PATH(eParticleSystemOp, rotOp, "Rotation", eOC_PATH, eFALSE,
        eOP_PAR_FLOAT(eParticleSystemOp, stretch, "Particle stretch", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eParticleSystemOp, gravity, "Gravity", -eF32_MAX, eF32_MAX, 0.0f,
        eOP_PAR_LABEL(eParticleSystemOp, label2, "Blending", "Blending",
        eOP_PAR_LINK_BMP(eParticleSystemOp, texOp, "Texture", eOC_BMP, eFALSE,
        eOP_PAR_ENUM(eParticleSystemOp, blendSrc, "Blending source", "Zero|One|Source color|1 - Source color|Source alpha|1 - Source alpha|Dest. alpha|1 - Dest. alpha|Dest. color|1 - Dest. color", 4,
        eOP_PAR_ENUM(eParticleSystemOp, blendDst, "Blending dest.", "Zero|One|Source color|1 - Source color|Source alpha|1 - Source alpha|Dest. alpha|1 - Dest. alpha|Dest. color|1 - Dest. color", 5,
        eOP_PAR_ENUM(eParticleSystemOp, blendOp, "Blending op.", "Add|Subtract|Reverse subtract|Minimum|Maximum", 0,
		eOP_PAR_END))))))))))))))))))))
    {
        if (colorOp && colorOp->getChanged())
            m_colorPath.sample(colorOp->getResult().path);
        if (sizeOp && sizeOp->getChanged())
            m_sizePath.sample(sizeOp->getResult().path);
        if (rotOp && rotOp->getChanged())
            m_rotPath.sample(rotOp->getResult().path);
      
        m_psys.setEmitter(emitterOp ? &emitterOp->getResult().mesh : nullptr, (ePsEmitterMode&)emitterMode);
        m_psys.setStretch(stretch);
        m_psys.setRandomization(randomization);
        m_psys.setEmission(emissionFreq, emissionVel);
        m_psys.setLifeTime(lifeTime);
        m_psys.setMaterial((eBlendMode&)blendSrc, (eBlendMode&)blendDst, (eBlendOp&)blendOp, texOp->getResult().uav->tex);
        m_psys.setGravity(gravity, eVector3(0.0f, -gravity, 0.0f));
        m_psys.setPaths((colorOp ? &m_colorPath : nullptr), (sizeOp ? &m_sizePath : nullptr), (rotOp ? &m_rotPath : nullptr));
        m_psys.update(time);

        m_sceneData.addRenderable(m_psysInst);
    }
	eOP_EXEC2_END

    eOP_VAR(ePath4Sampler       m_colorPath);
    eOP_VAR(ePath4Sampler       m_sizePath);
    eOP_VAR(ePath4Sampler       m_rotPath);
    eOP_VAR(eParticleSystem     m_psys);
    eOP_VAR(eParticleSysInst *  m_psysInst);
eOP_END(eParticleSystemOp);
#endif

// Heightfield (model) operator
// -------------------------
// creates a heightfield model from a base model + heightmap bitmap

#if defined(HAVE_OP_MODEL_HEIGHTFIELD) || defined(eEDITOR)
eOP_DEF_MODEL(eModelHeightfieldOp, "Heightfield", ' ', 1, 1, eOP_INPUTS(eOC_MODEL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eModelHeightfieldOp, gridsize, "Gridsize", eF32_MIN, eF32_MAX, 1, 1,
        eOP_PAR_FLOAT(eModelHeightfieldOp, height, "Height", eF32_MIN, eF32_MAX, 1,
        eOP_PAR_LINK_BMP(eModelHeightfieldOp, bmpOp, "Texture", eOC_BMP, eTRUE,
		eOP_PAR_END))))
    {
        const eIBitmapOp::Result &res = bmpOp->getResult();

        if (bmpOp->getChanged())
            eGfx->readTexture2d(res.uav->tex, m_bmpData);

        eSceneData &sd = ((eIModelOp *)getAboveOp(0))->getResult().sceneData;
        eColor *col = &m_bmpData[0];
        eF32 halfWidth = (eF32)res.width/2.0f;
        eF32 halfHeight = (eF32)res.height/2.0f;

        for (eU32 y=0; y<res.height; y++)
        {
            const eF32 ypos = (y-halfHeight)*gridsize.y;

            for (eU32 x=0; x<res.width; x++)
            {
                const eF32 xpos = (x-halfWidth)*(-gridsize.x);
                m_sceneData.merge(sd, eTransform(eQuat(), eVector3(xpos, col->grayScale()*height, ypos), eVector3(1.0f, 1.0f, 1.0f), eTO_RTS));
                col++;
            }
        }
    }
	eOP_EXEC2_END

    eOP_VAR(eArray<eColor>      m_bmpData);
eOP_END(eModelHeightfieldOp);
#endif

// Tunnel (model) operator
// -------------------------
// Creates a tunnel from base object and heighfield bitmap

#if defined(HAVE_OP_MODEL_TUNNEL) || defined(eEDITOR)
eOP_DEF_MODEL(eModelTunnelOp, "Tunnel", ' ', 1, 1, eOP_INPUTS(eOC_MODEL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZ(eModelTunnelOp, mapToScale, "Map to scale", eF32_MIN, eF32_MAX, 1, 1, 1,
        eOP_PAR_FLOAT(eModelTunnelOp, height, "Height", eF32_MIN, eF32_MAX, 1,
        eOP_PAR_FLOAT(eModelTunnelOp, radius, "Radius", eF32_MIN, eF32_MAX, 1,
        eOP_PAR_FLOAT(eModelTunnelOp, rotAmpli, "Rotate amplitude", eF32_MIN, eF32_MAX, 1,
        eOP_PAR_FLOAT(eModelTunnelOp, rotFreq, "Rotate frequency", eF32_MIN, eF32_MAX, 1,
        eOP_PAR_FLOAT(eModelTunnelOp, rotTime, "Rotate time", eF32_MIN, eF32_MAX, 1,
        eOP_PAR_LINK_BMP(eModelTunnelOp, bmpOp, "Texture", eOC_BMP, eTRUE,
		eOP_PAR_END))))))))
    {
        const eIBitmapOp::Result &res = bmpOp->getResult();

        if (bmpOp->getChanged())
            eGfx->readTexture2d(res.uav->tex, m_bmpData);

        eSceneData &sd = ((const eIModelOp *)getAboveOp(0))->getResult().sceneData;
        eColor *col = &m_bmpData[0];
        eF32 halfHeight = (eF32)res.height/2.0f;
        eF32 radStep = eTWOPI/(eF32)res.width;

        for (eU32 y=0; y<res.height; y++)
        {
            const eF32 ypos = (y-halfHeight)*height;
            const eF32 rot = eSin(((eF32)y/(eF32)res.height)*rotFreq+(eF32)rotTime)*rotAmpli;

            for (eU32 x=0; x<res.width; x++)
            {
                eVector3 scale = eVector3(1.0f, 1.0f, 1.0f)*((eF32)col->grayScale()/256.0f);
                m_sceneData.merge(sd, eTransform(eQuat(eVector3(0.0f, radStep*x+rot, 0.0f)), eVector3(radius, ypos, 0.0f), scale, eTO_STR));
                col++;
            }
        }
    }
	eOP_EXEC2_END

    eOP_VAR(eArray<eColor> m_bmpData);
eOP_END(eModelTunnelOp);
#endif