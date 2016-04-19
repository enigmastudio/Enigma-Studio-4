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

eDEF_OPERATOR_SOURCECODE(MISC);

// Demo (misc) operator
// --------------------
// Operator which holds all data relevant for
// a complete demo (sequencer + sound).

eOP_DEF(eDemoOp, eIDemoOp, "Demo", "Misc", eColor(170, 170, 170), 'd', eOC_DEMO, 1, 32, eOP_INPUTS(eOP_32INPUTS(eOC_SEQ)))
	eOP_INIT() 
	{
		m_songDelay = 0.0f;
	}

	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eDemoOp, aspectRatioSel, "Letter box", "4:3|16:9|16:10|2.35:1", 0,
        eOP_PAR_STRING(eDemoOp, prodName, "Production name", "Unnamed",
        eOP_PAR_LINK_DEMO(eDemoOp, loadingScreenOp, "Loading screen", eOC_DEMO, eFALSE,
        eOP_PAR_FILE(eDemoOp, songPath, "Song", "",
		eOP_PAR_FLOAT(eDemoOp, songDelay, "Song delay", 0.0f, eF32_MAX, 0.0f,
		eOP_PAR_END))))))
    {
        const eF32 aspectRatios[] = {4.0f/3.0f, 16.0f/9.0f, 16.0f/10.0f, 2.35f};
        m_seq.setAspectRatio(aspectRatios[aspectRatioSel]);
        m_seq.clear();

        for (eU32 i=0; i<getAboveOpCount(); i++)
        {
            const eSequencer &curSeq = ((eISequencerOp *)getAboveOp(i))->getResult().seq;
            m_seq.merge(curSeq);
        }

        m_demo.setSequencer(&m_seq);

        // load song into TF4
        if (m_songPath != songPath || songDelay != m_songDelay)
        {
#ifdef ePLAYER
            const eByteArray &song = getParameter(3).getAnimValue().fileData;
#else
            const eByteArray &song = eFile::readAll(songPath);
#endif
            eTfPlayerLoadSong(*eSfx, (song.isEmpty() ? nullptr : &song[0]), song.size(), songDelay);
            m_songPath = songPath;
			m_songDelay = songDelay;
        }
    }
	eOP_EXEC2_END

    eOpProcessResult process(eF32 time, eOpCallback callback, ePtr param)
    {
        // process the whole stack
        if (m_processAll)
            return eIOperator::process(time, callback, param);

        // only process active sequencer operators
        eIOpPtrArray oldInOps = m_inputOps;
        m_inputOps.clear();

        for (eU32 i=0; i<oldInOps.size(); i++)
        {
            eISequencerOp *seqOp = (eISequencerOp *)oldInOps[i];
            const eF32 startTime = seqOp->getParameter(0).getAnimValue().flt;
            const eF32 duration = seqOp->getParameter(1).getAnimValue().flt;
            const eU32 track = seqOp->getParameter(2).getAnimValue().integer;

            if (time >= startTime && time <= startTime+duration)
            {
                seqOp->setSubTime(startTime);
                m_inputOps.append(seqOp);
            }
        }

        const eOpProcessResult res = eIOperator::process(time, callback, param);
        m_inputOps = oldInOps;

        for (eU32 i=0; i<m_inputOps.size(); i++)
            m_inputOps[i]->setSubTime(0.0f);

        return res;
    }

    eOP_VAR(eSequencer     m_seq);
    eOP_VAR(static eString m_songPath);
	eOP_VAR(eF32		   m_songDelay);
eOP_END(eDemoOp);

eString eDemoOp::m_songPath;

// Render-to-texture (misc) operator
// ---------------------------------
// Represents a render target which can be rendered to.

#if defined(HAVE_OP_R2T_R2T) || defined(eEDITOR)
eOP_DEF(eRenderToTextureOp, eIRenderToTextureOp, "R2T", "Misc", eColor(170, 170, 170), ' ', eOC_R2T, 0, 0, eOP_INPUTS())
    eOP_DEINIT()
    {
        eGfx->removeTexture2d(m_renderTarget);
        eGfx->removeTexture2d(m_depthTarget);
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eRenderToTextureOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048", 9,
        eOP_PAR_ENUM(eRenderToTextureOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048", 9,
		eOP_PAR_END)))
    {
        const eU32 sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
        const eU32 width = sizes[widthSel];
        const eU32 height = sizes[heightSel];

        if (!m_renderTarget || m_renderTarget->width != width || m_renderTarget->height != height)
        {
            eGfx->removeTexture2d(m_renderTarget);
            m_renderTarget = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB8);
            eGfx->removeTexture2d(m_depthTarget);
            m_depthTarget = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_DEPTH32F);
        }
    }
	eOP_EXEC2_END
eOP_END(eRenderToTextureOp);
#endif

// Material (misc) operator
// ------------------------
// Creates a material which can be mapped onto a mesh.

eOP_DEF(eMaterialOp, eIMaterialOp, "Material", "Misc", eColor(170, 170, 170), 'm', eOC_MAT, 0, 0, eOP_INPUTS())
    eOP_INIT()
    {
        m_texDiffuse    = nullptr;
        m_texNormal     = nullptr;
        m_texSpecular   = nullptr;
        m_texEnv        = nullptr;
        m_ownsDiffuse   = eFALSE;
        m_ownsNormal    = eFALSE;
        m_ownsSpecular  = eFALSE;
        m_ownsEnv       = eFALSE;
	    eREGISTER_OP_SHADER(eMaterialOp, ps_nolight);
	    eREGISTER_OP_SHADER(eMaterialOp, ps_forward_light);
	    eREGISTER_OP_SHADER(eMaterialOp, ps_deferred_geo);
	    eREGISTER_OP_SHADER(eMaterialOp, vs_instanced_geo);
    }

    eOP_DEINIT()
    {
        _freeTextures();
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eMaterialOp, renderPass, "Render pass", 0, 255, 0,
        eOP_PAR_BOOL(eMaterialOp, lighted, "Lighted", eTRUE,
        eOP_PAR_BOOL(eMaterialOp, flatShaded, "Flat", eFALSE,
        eOP_PAR_BOOL(eMaterialOp, twoSided, "Two sided", eFALSE,
        eOP_PAR_ENUM(eMaterialOp, zBuffer, "Z-Buffer", "On|Readonly (for blending)|Off", 0,
        eOP_PAR_LABEL(eMaterialOp, label0, "Blending", "Blending",
        eOP_PAR_BOOL(eMaterialOp, blending, "Blending on", eFALSE,
        eOP_PAR_ENUM(eMaterialOp, blendSrc, "Blending source", "Zero|One|Source color|1 - Source color|Source alpha|1 - Source alpha|Dest. alpha|1 - Dest. alpha|Dest. color|1 - Dest. color", 1,
        eOP_PAR_ENUM(eMaterialOp, blendDst, "Blending dest.", "Zero|One|Source color|1 - Source color|Source alpha|1 - Source alpha|Dest. alpha|1 - Dest. alpha|Dest. color|1 - Dest. color", 1,
        eOP_PAR_ENUM(eMaterialOp, blendOp, "Blending op.", "Add|Subtract|Reverse subtract|Minimum|Maximum", 0,
//        eOP_PAR_LABEL(eMaterialOp, label1, "Lighting", "Lighting",
        eOP_PAR_RGB(eMaterialOp, diffuseCol, "Diffuse", 255, 255, 255,
        eOP_PAR_RGB(eMaterialOp, specCol, "Specular", 255, 255, 255,
        eOP_PAR_FLOAT(eMaterialOp, shininess, "Shininess", eALMOST_ZERO, 1.0f, 0.25f,
        eOP_PAR_ENUM(eMaterialOp, gFilter, "Filtering", "Nearest|Bilinear|Trilinear", 2,
        eOP_PAR_ENUM(eMaterialOp, gAddr, "Addressing", "Wrap|Clamp|Mirror", 0,
        eOP_PAR_BOOL(eMaterialOp, gMip, "Mipmapping", eTRUE,
        eOP_PAR_LINK_BMP(eMaterialOp, diffOp, "Diff. texture", eOC_BMP|eOC_R2T, eFALSE,
        eOP_PAR_LINK_BMP(eMaterialOp, bumpOp, "Bump texture", eOC_BMP|eOC_R2T, eFALSE,
        eOP_PAR_LINK_BMP(eMaterialOp, specOp, "Spec. texture", eOC_BMP|eOC_R2T, eFALSE,
        eOP_PAR_LINK_BMP(eMaterialOp, envOp, "Env. texture", eOC_BMP|eOC_R2T, eFALSE,
		eOP_PAR_END)))))))))))))))))))))
    {
        _loadTexture(m_ownsDiffuse, gMip, diffOp, m_texDiffuse);
        _loadTexture(m_ownsNormal, gMip, bumpOp, m_texNormal);
        _loadTexture(m_ownsSpecular, gMip, specOp, m_texSpecular);
        _loadTexture(m_ownsEnv, gMip, envOp, m_texEnv);

        const eInt tff[] = {eTMF_NEAREST, eTMF_BILINEAR, eTMF_TRILINEAR};
        const eInt taf[] = {eTMF_WRAP, eTMF_CLAMP, eTMF_MIRROR};

        ePixelShader *psNoLight = eGfx->loadPixelShader(eSHADER(ps_nolight));
        ePixelShader *psForwLight = eGfx->loadPixelShader(eSHADER(ps_forward_light));
        ePixelShader *psDefGeo = eGfx->loadPixelShader(eSHADER(ps_deferred_geo));
        eVertexShader *vsInstGeo = eGfx->loadVertexShader(eSHADER(vs_instanced_geo));

        m_mat.textures[eMTU_DIFFUSE] = m_texDiffuse;
        m_mat.texFlags[eMTU_DIFFUSE] = tff[gFilter]|taf[gAddr];
        m_mat.textures[eMTU_NORMAL] = m_texNormal;
        m_mat.texFlags[eMTU_NORMAL] = tff[gFilter]|taf[gAddr];
        m_mat.textures[eMTU_SPECULAR] = m_texSpecular;
        m_mat.texFlags[eMTU_SPECULAR] = tff[gFilter]|taf[gAddr];
        m_mat.textures[eMTU_ENV] = m_texEnv;
        m_mat.texFlags[eMTU_ENV] = tff[gFilter]|taf[gAddr];
        m_mat.lighted = lighted;
        m_mat.blending = blending;
        m_mat.flatShaded = flatShaded;
        m_mat.blendSrc = (eBlendMode&)blendSrc;
        m_mat.blendDst = (eBlendMode&)blendDst;
        m_mat.blendOp = (eBlendOp&)blendOp;
        m_mat.diffuseCol = diffuseCol;
        m_mat.specCol = specCol;
        m_mat.shininess = shininess;
        m_mat.cullMode = (twoSided ? eCULL_NONE : eCULL_BACK);
        m_mat.depthTest = (zBuffer <= 1);
        m_mat.depthWrite = (zBuffer != 1);
        m_mat.vs = vsInstGeo;
        m_mat.ps = (!lighted ? psNoLight : (blending ? psForwLight : psDefGeo));
    }
	eOP_EXEC2_END

    void _loadTexture(eBool &owns, eBool mipMapped, const eIOperator *op, eTexture2d *&outTex)
    {
        //eTexture2d *tex = nullptr;

        if (op && op->getResultClass() == eOC_BMP)
        {
            const eIBitmapOp::Result &res = ((eIBitmapOp *)op)->getResult();

            // textures coming from GPU bitmap generator
            // don't have mipmaps => create mipmapped version
			/*
			if (mipMapped)
            {
                tex = eGfx->addTexture2d(res.width, res.height, 0, eTFO_ARGB8);
                eGfx->updateTexture2d(tex, res.uav->tex, ePoint(0, 0), eRect(0, 0, res.width, res.height));
                owns = eTRUE;
            }
            else
            {
                tex = res.uav->tex;
                owns = eFALSE;
            }
			*/
			// TODO: EVIL HACK TO MAKE BROKEN MIPMAPPING IN PLAYER WORK! FIX PLEASE!


            if (!outTex || outTex->width != res.width || outTex->height != res.height)
            {
                eGfx->removeTexture2d(outTex);
			    outTex = eGfx->addTexture2d(res.width, res.height, mipMapped ? 0 : eTEX_NOMIPMAPS, eTFO_ARGB8);
            }

            eGfx->updateTexture2d(outTex, res.uav->tex, ePoint(0, 0), eRect(0, 0, res.width, res.height));
            owns = eTRUE;
        }
#if defined(HAVE_OP_R2T_R2T) || defined(eEDITOR)
        else if (op && op->getResultClass() == eOC_R2T)
        {
            owns = eFALSE;
            outTex = ((eIRenderToTextureOp *)op)->getResult().renderTarget;
        }
#endif
    }

    void _freeTextures()
    {
        if (m_ownsDiffuse)
            eGfx->removeTexture2d(m_texDiffuse);
        if (m_ownsNormal)
            eGfx->removeTexture2d(m_texNormal);
        if (m_ownsSpecular)
            eGfx->removeTexture2d(m_texSpecular);
        if (m_ownsEnv)
            eGfx->removeTexture2d(m_texEnv);
    }

    eOP_VAR(eTexture2d * m_texDiffuse);
    eOP_VAR(eTexture2d * m_texNormal);
    eOP_VAR(eTexture2d * m_texSpecular);
    eOP_VAR(eTexture2d * m_texEnv);

    eOP_VAR(eBool m_ownsDiffuse);
    eOP_VAR(eBool m_ownsNormal);
    eOP_VAR(eBool m_ownsSpecular);
    eOP_VAR(eBool m_ownsEnv);
eOP_END(eMaterialOp);

// Scene (misc) operator
// ---------------------
// Holds all scene data organized in a spatial
// subdivision data structure.

#if defined(HAVE_OP_MISC_SCENE) || defined(eEDITOR)
eOP_DEF(eSceneOp, eISceneOp, "Scene", "Misc", eColor(170, 170, 170), 's', eOC_SCENE, 1, 1, eOP_INPUTS(eOC_MODEL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        const eSceneData &sd = ((eIModelOp *)getAboveOp(0))->getResult().sceneData;
        m_scene.setSceneData(sd);
        
    }
	eOP_EXEC2_END
eOP_END(eSceneOp);
#endif

// Camera (misc) operator
// ----------------------
// Holds a camera and a viewport.

#if defined(HAVE_OP_MISC_POV) || defined(eEDITOR)

void lookupSceneDataEntry(const eSceneData &sd, const eTransform &transf, eU32& cntLeft, eTransform& result) {
    for (eU32 i=0; i<sd.getEntryCount(); i++) {
        const eSceneData::Entry &entry = sd.getEntry(i);
		eTransform tnew = entry.transf*transf;
        if (entry.renderable) {
			if(cntLeft == 0) {
				result = tnew;
				return;
			}
			cntLeft--;
        } else {
			if(entry.sceneData->getRenderableTotal() <= cntLeft)
				cntLeft -= entry.sceneData->getRenderableTotal();
			else {
				// go deeper
				lookupSceneDataEntry(*entry.sceneData, tnew, cntLeft, result);
				return;
			}
		}
    }

}

eOP_DEF(ePovOp, eIPovOp, "POV", "Misc", eColor(170, 170, 170), 'c', eOC_POV, 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXYZW(ePovOp, viewport, "Viewport", 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        eOP_PAR_FLOAT(ePovOp, fovY, "Field of view (Y)", eALMOST_ZERO, 180.0f, 45.0f,
        eOP_PAR_ENUM(ePovOp, aspectSel, "Aspect ratio", "4:3|16:9|16:10|2.35:1", 0,
        eOP_PAR_FLOAT(ePovOp, zNear, "Near plane", eF32_MIN, eF32_MAX, 0.01f,
        eOP_PAR_FLOAT(ePovOp, zFar, "Far plane", eF32_MIN, eF32_MAX, 1000.0f,
        eOP_PAR_ENUM(ePovOp, camType, "Camera type", "Target camera|Direction camera|Path camera", 1,
        eOP_PAR_FLOAT(ePovOp, tilt, "Tilt", eF32_MIN, eF32_MAX, 0.0f,
        eOP_PAR_LABEL(ePovOp, label1, "Direction camera", "Direction camera",
        eOP_PAR_FXYZ(ePovOp, pos1, "Position 2", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 10.0f,
        eOP_PAR_FXYZ(ePovOp, dir, "Direction", eF32_MIN, eF32_MAX, 0.0f, 0.0f, -1.0f,
        eOP_PAR_LABEL(ePovOp, label2, "Path camera", "Path camera",
        eOP_PAR_FLOAT(ePovOp, pathTime, "Path time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_LINK_PATH(ePovOp, pathOp, "Path", eOC_PATH, eFALSE,
		eOP_PAR_END))))))))))))))
    {
        // setup view matrix
        eMatrix4x4 mtxView;

		if (camType == 0) // target camera
        {
			// dir is target
			if (dir != pos1) // rotation axis valid?
            {
                const eVector3 rotAxis = (dir-pos1).normalized();
                const eVector3 upAxis = (rotAxis.isLinearDependent(eVEC3_YAXIS) ? eVEC3_XAXIS : eVEC3_YAXIS);
                mtxView.lookAt(pos1, dir, upAxis*eQuat(rotAxis, tilt));
            }        
		}
        else if (camType == 1) // direction camera
        {
            if (!eIsFloatZero(dir.sqrLength())) // rotation axis valid?
            {
                const eVector3 rotAxis = dir.normalized();
                const eVector3 upAxis = (rotAxis.isLinearDependent(eVEC3_YAXIS) ? eVEC3_XAXIS : eVEC3_YAXIS);
                mtxView.lookAt(pos1, pos1+dir, upAxis*eQuat(rotAxis, tilt));
            }
        }
        else // path camera
        {
            const ePath4 &path = pathOp->getResult().path;
            const eVector3 &v0 = path.evaluate(pathTime).toVec3();
            const eVector3 &v1 = path.evaluate(pathTime+eALMOST_ZERO).toVec3();
            const eVector3 camDir = v1-v0;

            if (!eIsFloatZero(camDir.sqrLength())) // rotation axis valid?
            {
                const eVector3 rotAxis = camDir.normalized();
                const eVector3 upAxis = (rotAxis.isLinearDependent(eVEC3_YAXIS) ? eVEC3_XAXIS : eVEC3_YAXIS);
                mtxView.lookAt(v0, v1, upAxis*eQuat(rotAxis, tilt));
            }
        }

        // setup camera
        const eF32 aspectRatios[] = {4.0f/3.0f, 16.0f/9.0f, 16.0f/10.0f, 2.35f};
        m_cam = eCamera(fovY, aspectRatios[aspectSel], zNear, zFar);
        m_cam.setViewMatrix(mtxView);
        m_viewport = viewport;
    }
	eOP_EXEC2_END

eOP_END(ePovOp);
#endif

// Path (misc) operator
// --------------------
// Holds a path.

#if defined(HAVE_OP_MISC_PATH) || defined(eEDITOR)
eOP_DEF(ePathOp, eIPathOp, "Path", "Misc", eColor(170, 170, 170), 'p', eOC_PATH, 0, 0, eOP_INPUTS())
#ifdef eEDITOR
    eOP_INIT()
    {
        ePath4 &path = getParameter(0).getBaseValue().path;
        for (eInt i=0; i<4; i++)
        {
            path.getSubPath(i).addKey(0.0f, (eF32)i, ePI_CUBIC);
            path.getSubPath(i).addKey(1.0f, (eF32)i, ePI_CUBIC);
        }
    }
#endif

	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_PATH(ePathOp, path, "Path");
		eOP_PAR_END)
    {
        m_path = path;
    }
	eOP_EXEC2_END
eOP_END(ePathOp);
#endif

// Load (misc) operator
// --------------------
// Loads a previously saved operator and
// passes the output to its output operators.

#if defined(HAVE_OP_MISC_LOAD) || defined(eEDITOR)
eOP_DEF(eLoadOp, eIStructureOp, "Load", "Misc", eColor(170, 170, 170), 'l', eOC_ALL, 0, 0, eOP_INPUTS())
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_LINK(eLoadOp, loadedOp, "Load", eOC_ALL, eTRUE,
		eOP_PAR_END))
    {
    }
	eOP_EXEC2_END

    virtual eIOperator * _getRealOp() const
    {
        return eDemoData::findOperator(m_params[0]->getAnimValue().linkedOpId);
    }
eOP_END(eLoadOp);
#endif

// Store (misc) operator
// ---------------------
// Stores an operator so it can be loaded
// somewhere else using the load operator.

#if defined(HAVE_OP_MISC_STORE) || defined(eEDITOR)
eOP_DEF(eStoreOp, eIStructureOp, "Store", "Misc", eColor(170, 170, 170), 's', eOC_ALL, 1, 1, eOP_INPUTS(eOC_ALL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
    }
	eOP_EXEC2_END
eOP_END(eStoreOp);
#endif

// Nop (misc) operator
// -------------------
// Does nothing more than forwarding the
// input operator.

#if defined(HAVE_OP_MISC_NOP) || defined(eEDITOR)
eOP_DEF(eNopOp, eIStructureOp, "Nop", "Misc", eColor(170, 170, 170), 'n', eOC_ALL, 1, 1, eOP_INPUTS(eOC_ALL))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
    }
	eOP_EXEC2_END
eOP_END(eNopOp);
#endif