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

#include "../../../eshared.hpp"
#include "tinylsys3.hpp"
#include "rewritesystem.hpp"
#include "turtleinterpreter.hpp"
#include "attractor.hpp"

eDEF_OPERATOR_SOURCECODE(LSYS);

#if defined(HAVE_OP_MISC_L3ATTRACTOR) || defined(eEDITOR)
eOP_DEF(eLSys3AttractorOp, eIGenericOp, "L3Attractor", "Misc", eColor(170, 170, 170), ' ', eOC_MISC, 1, 1, eOP_INPUTS(eOC_MESH))
    eOP_INIT()
    {
        this->m_genData = &m_attractor;
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eLSys3AttractorOp, amount, "Amount", -eF32_MAX, eF32_MAX, 1,
	    eOP_PAR_ENUM(eLSys3AttractorOp, axis, "Axis", "X|Y|Z", 0,
	    eOP_PAR_ENUM(eLSys3AttractorOp, normalize, "Normalize", "Off|On", 0,
	    eOP_PAR_ENUM(eLSys3AttractorOp, massDep, "MassDependent", "Off|On", 0,
		eOP_PAR_END)))))
    {
		m_attractor.set(((eIMeshOp*)this->getAboveOp(0))->getResult().mesh, axis, amount, massDep, normalize);
    }
	eOP_EXEC2_END

	eLsys3Attractor m_attractor;
eOP_END(eLSys3AttractorOp);
#endif


#if defined(HAVE_OP_MODEL_LSYS3) || defined(eEDITOR)
#define L3AI eOC_MAT | eOC_MISC
eOP_DEF_MODEL(eLSys3Op, "LSys3", ' ', 0, 10, eOP_INPUTS(L3AI, L3AI, L3AI, L3AI, L3AI, L3AI, L3AI, L3AI, L3AI, L3AI))
    eOP_INIT()
    {
        m_mi = nullptr;
    }

eOP_DEINIT()
    {
        eDelete(m_mi);
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_TEXT(eLSys3Op, rules, "Rules", "W;F;\nF;FF;",
        eOP_PAR_FXYZ(eLSys3Op, rotUnits, "RotationUnits", -eF32_MAX, eF32_MAX, 1.0f, 1.0f, 1.0f,
        eOP_PAR_ENUM(eLSys3Op, texMode, "Tex Mode", "Default|Bidirectional|Fixed", 0,
        eOP_PAR_LINK_PATH(eLSys3Op, shapePathOp, "Shape", eOC_PATH, eFALSE,
        eOP_PAR_BOOL(eLSys3Op, showProduction, "Show Production", eFALSE,
        eOP_PAR_BOOL(eLSys3Op, twoSided, "Two Sides", eFALSE,
        eOP_PAR_FXYZW(eLSys3Op, scaleDecayUV, "Scale,Decay,U,V", -eF32_MAX, eF32_MAX, 1.0f, 1.0f, 1.0f, 1.0f,
		eOP_PAR_IXYXY(eLSys3Op, iterSeedDetails, "Iter,Seed,Details", 1, 10000, 1, 1, 1, 1,
        eOP_PAR_FXYZW(eLSys3Op, axiomParams, "AxiomParams", -eF32_MAX, eF32_MAX, 1.0f, 1.0f, 1.0f, 1.0f,
		eOP_PAR_END))))))))))
    {
		ePROFILER_FUNC();

		const eU32& iterations = iterSeedDetails.x0;
		const eU32& ranSeed = iterSeedDetails.y0;
		const eU32& branchSegments = iterSeedDetails.x1;
		const eU32& discSegments = iterSeedDetails.y1;

		m_attractors.clear();
		m_materialClusters.clear();
		m_finalMesh.clear();
		
		// get materials and attractors
		m_finalMesh.m_clusters.reserve(this->getAboveOpCount());
		for(eU32 i = 0; i < this->getAboveOpCount(); i++) {
			eIOperator* inp = this->getAboveOp(i);
			if(inp->getResultClass() == eOC_MAT) {
				// create material cluster (if necc)
				m_materialClusters.append(&m_finalMesh._findCluster(&((eIMaterialOp*)inp)->getResult().mat, eMesh::ClusterType::CT_TRIANGLE));
			}
			if(inp->getResultClass() == eOC_MISC) 
				m_attractors.append((eLsys3Attractor*)((eIGenericOp *)inp)->getResult().genericDataPtr);
		}

		if(m_materialClusters.size() == 0) {
			// add default material cluster
			m_materialClusters.append(&m_finalMesh._findCluster(eMaterial::getDefault(), eMesh::ClusterType::CT_TRIANGLE));
		}

		// trim string
		eString rstr(rules);
		eU32 pos = 0;
		while(pos < rstr.length()) {
			rstr[pos] = rstr[pos] & 0xff;
			if((rstr[pos] == ' ') ||
			   (rstr[pos] == '\n'))
			   rstr.removeAt(pos);
			else
				pos++;
		}

		m_rewriteSystem.readRules(rstr);

		m_production.clear();
		m_rewriteSystem.apply(eRewriteSystem::NewSymInstance('W', &axiomParams.x, 4), m_production, iterations, ranSeed);

#ifdef eEDITOR
		if(showProduction)
			eWriteToLog(eString("Result: ") + eIntToStr(m_production.size()) + " symbols : " + eRewriteSystem::toString(m_production, eFALSE));
#endif
		m_results.clear();
		m_interpreter.interpret(m_results, m_production, rotUnits * ePI, scaleDecayUV.y);
		m_interpreter.postProcess(m_results, m_attractors);

		// prepare local mesh
		eU32 neededVertices = m_results.size() * (discSegments + 1) * (branchSegments + 1);
		eU32 neededFaces = m_results.size() * discSegments * branchSegments * 2 * (twoSided ? 2 : 1);
		m_finalMesh.reserve(neededFaces, neededVertices);
		// prepare material clusters
		for(eU32 i = 0; i < m_materialClusters.size(); i++)
			m_materialClusters[i]->prims.reserve(neededFaces);

		m_shape.clear();
		if(shapePathOp == nullptr) {
			for(eU32 ds = 0; ds <= discSegments; ds++) {
				eF32 t = (eF32)ds / (eF32)discSegments;
				m_shape.append(eVector3(eSin(2.0f * ePI * t), eCos(2.0f * ePI * t),0)); 
			}
		} else {
			const ePath4& shapePath = shapePathOp->getResult().path;
			for(eU32 ds = 0; ds <= discSegments; ds++) {
				eF32 t = (eF32)ds / (eF32)discSegments;
				m_shape.append(shapePath.evaluateUnitTime(t).toVec3()); 
			}
		}

		{
			ePROFILER_ZONE("LSys3 Draw Mesh");

			eArray<eFullVtx>& verts = m_finalMesh.getVertexArray();
			eVector3 vmin(eF32_MAX);
			eVector3 vmax(-eF32_MAX);

			eU32 numFacesPerSymbol = branchSegments * discSegments * 2 * (twoSided ? 2 : 1);
			eU32 numVerticesPerSymbol = (branchSegments + 1) * (discSegments + 1);

			if(m_results.size() > 1)
				m_results[0].state.m_width = m_results[1].state.m_width;
			// draw results
			// every symbol has a parent (except for symbol 0)
			for(eU32 ridx = 1; ridx < m_results.size(); ridx++) {
				eLSys3InterpreterNode& n = m_results[ridx];
				const TurtleState& t0 = m_results[n.parent].state;
				const TurtleState& t1 = n.state;
				const eLsys3PostProcRec& r0 = m_results[n.parent].postProc;
				const eLsys3PostProcRec& r1 = m_results[ridx].postProc;


				eVector3 control0 = r0.globalPos * scaleDecayUV.x;
				eVector3 control3 = r1.globalPos * scaleDecayUV.x;
				// bezier spline
				eF32 distLen = (control3 - control0).length();
				eVector3 control1 = control0   + r0.globalRot.getVector(2) * 0.333333f * distLen;
				eVector3 control2 = control3   - r1.globalRot.getVector(2) * 0.333333f * distLen;

				// NOTE: we update only the bbox by the center positions to save calculation time
				//       this might lead to errors, which can be prevented by slightly increasing the bbox afterwards
				vmin.minComponents(control0);
				vmax.maxComponents(control0);
				vmin.minComponents(control3);
				vmax.maxComponents(control3);
				{
					ePROFILER_ZONE("LSys3 Gen Vertices");
					eF32 vstep = 1.0f / (eF32)(branchSegments);
					eF32 t = 0.0f;
					const eF32 tex_u_add = (texMode == 0 ? 1.0f : (texMode == 1 ? 2.0f : 0.0f)) / (eF32)discSegments;
					eF32 width = t0.m_width * scaleDecayUV.x;
					eF32 width_add = ((t1.m_width - t0.m_width) * scaleDecayUV.x) / (eF32)(branchSegments);
					eVector2 uv_base( (texMode == 0 ? 0.0f  : (texMode == 1 ? -1.0f : t0.m_parameters[1] * scaleDecayUV.z)),
						              t0.m_lenSum * scaleDecayUV.w * 0.01f);
					eVector2 uv_add( (texMode != 2 ? 0.0f : ((t1.m_parameters[1] - t0.m_parameters[1]) * scaleDecayUV.z) / (eF32)(branchSegments)),
						             ((t1.m_lenSum - t0.m_lenSum) * scaleDecayUV.w * 0.01f) / (eF32)(branchSegments));
					eU32 oldVertsSize = verts.size();
					verts.resize(oldVertsSize + numVerticesPerSymbol);
					eFullVtx* vptr = &verts[oldVertsSize];
					__declspec(align(16)) eVector3 cpos;
					__declspec(align(16)) eQuat crot;
					__declspec(align(16)) eVector3 distNorm;
					const __m128 mcp0 = _mm_loadu_ps(&control0.x);
					const __m128 mcp1 = _mm_loadu_ps(&control1.x);
					const __m128 mcp2 = _mm_loadu_ps(&control2.x);
					const __m128 mcp3 = _mm_loadu_ps(&control3.x);
					{
						ePROFILER_ZONE("LSys3 Gen Vertices Inner");
						for(eU32 step = 0; step <= branchSegments; step++) {
							// calculate current disc
							crot = r0.globalRot.slerp(t, r1.globalRot);

							// calc bezier
							__m128 m3 = _mm_set1_ps(3);
							__m128 mt = _mm_set1_ps(t);
							__m128 mt3 = _mm_mul_ps(mt, m3);
							__m128 mtt = _mm_mul_ps(mt, mt);
							__m128 mtt3 = _mm_mul_ps(mtt, m3);
							__m128 mtinv = _mm_set1_ps(1.0f - t);
							__m128 mtinv3 = _mm_mul_ps(mtinv, m3);
							__m128 mttinv = _mm_mul_ps(mtinv, mtinv);
							__m128 mttinv3 = _mm_mul_ps(mttinv, m3);
    
							__m128 respos0 = _mm_add_ps(_mm_mul_ps(mcp0, _mm_mul_ps(mttinv, mtinv)), 
														_mm_mul_ps(mcp1, _mm_mul_ps(mt3, mttinv)));
							__m128 mt_tinv_6 = _mm_mul_ps(mt3, mtinv3);
							__m128 restan0 = _mm_sub_ps(_mm_mul_ps(mcp1, _mm_sub_ps(mttinv3, mt_tinv_6)), 
														_mm_mul_ps(mcp0, mttinv3));

							__m128 respos1 = _mm_add_ps(_mm_mul_ps(mcp2, _mm_mul_ps(mtinv3, mtt)), 
														_mm_mul_ps(mcp3, _mm_mul_ps(mtt, mt)));
							__m128 restan1 = _mm_add_ps(_mm_mul_ps(mcp2, _mm_sub_ps(mt_tinv_6, mtt3)), 
														_mm_mul_ps(mcp3, mtt3));
							_mm_store_ps(&cpos.x, _mm_add_ps(respos0, respos1));
							_mm_store_ps(&distNorm.x, _mm_add_ps(restan0, restan1));
/*
							// correct rotation
							eVector3 look = crot.getVector(2);
							eVector3 side = (look^distNorm);
							eF32 sideLenSqr = side.sqrLength();
							if(sideLenSqr > eALMOST_ZERO) {
								side /= eSqrt(sideLenSqr);
								eF32 dot = eClamp(-1.0f, look * distNorm, 1.0f);
								eF32 alpha = eACos(dot) * (1.0f / (2.0f * ePI));
								eQuat rotation(side, alpha);
								crot = rotation * crot;
							}
*/
							// calc tangents
							crot.conjugate();
							eVector2 uv = uv_base;
							for(eU32 ds = 0; ds <= discSegments; ds++) {
								const eVector3 normal = (m_shape[ds] * width) * crot.normalized();
								(vptr++)->set(cpos + normal, normal, uv, eCOL_WHITE);
								uv.x += tex_u_add;
							}
							t += vstep;
							width += width_add;
							uv_base += uv_add;
						}
					}
				}
				{
					ePROFILER_ZONE("LSys3 Connect Faces");
					eMesh::Cluster& cluster = *m_materialClusters[t1.m_material % m_materialClusters.size()];
					// append primitive indices to cluster
					eU32 cPrimBase = cluster.prims.size();
					eU32 primBase = m_finalMesh.m_prims.size();
					cluster.prims.resize(cPrimBase + numFacesPerSymbol);
					for(eU32 i = 0; i < numFacesPerSymbol; i++)
						cluster.prims[cPrimBase + i] = primBase + i;
					// resize primitives
					m_finalMesh.m_prims.resize(primBase + numFacesPerSymbol);
					eMeshPrimitive* primPtr = &m_finalMesh.m_prims[primBase];
					// connect vertices
					eU32 prevVerts = (ridx - 1) * numVerticesPerSymbol;
					eU32 curVerts = prevVerts + (discSegments + 1);
					for(eU32 step = 1; step <= branchSegments; step++) {
						for(eU32 i = 0; i < discSegments; i++) {
							eMeshPrimitive &prim = *(primPtr++);
							prim.mat = cluster.mat;
							prim.count = 3;
							prim.indices[0] = curVerts;
							prim.indices[1] = curVerts + 1;
							prim.indices[2] = prevVerts;

							eMeshPrimitive &prim2 = *(primPtr++);
							prim2.mat = cluster.mat;
							prim2.count = 3;
							prim2.indices[0] = curVerts + 1;
							prim2.indices[1] = prevVerts + 1;
							prim2.indices[2] = prevVerts;

							if(twoSided) {
								eMeshPrimitive &prim = *(primPtr++);
								prim.mat = cluster.mat;
								prim.count = 3;
								prim.indices[0] = prevVerts;
								prim.indices[1] = curVerts + 1;
								prim.indices[2] = curVerts;

								eMeshPrimitive &prim2 = *(primPtr++);
								prim2.mat = cluster.mat;
								prim2.count = 3;
								prim2.indices[0] = prevVerts;
								prim2.indices[1] = prevVerts + 1;
								prim2.indices[2] = curVerts + 1;
							}

							prevVerts++;
							curVerts++;
						}
						prevVerts++;
						curVerts++;
					}
				}
			}
			m_finalMesh.getBoundingBox().updateExtent(vmin);
			m_finalMesh.getBoundingBox().updateExtent(vmax);
		}
		eASSERT(m_finalMesh.getVertexCount() <= neededVertices);
		eASSERT(m_finalMesh.getPrimitiveCount() <= neededFaces);
		m_finalMesh.finishLoading(eMT_DYNAMIC);
	    eDelete(m_mi);
		eBool castsShadows = eFALSE;
		m_mi = new eMeshInst(m_finalMesh, castsShadows);
		m_sceneData.addRenderable(m_mi);
    }
	eOP_EXEC2_END

	eMesh									m_finalMesh;
	eMeshInst								*m_mi;
	eLsys3TurtleInterpreter					m_interpreter;
	eArray<eRewriteSystem::SymInstance>		m_production;
	eRewriteSystem							m_rewriteSystem;
	eArray<eLsys3Attractor*>				m_attractors;
	eArray<eMesh::Cluster*>					m_materialClusters;
	eArray<eVector3>						m_shape;
	eArray<eLSys3InterpreterNode>			m_results;
eOP_END(eLSys3Op);
#endif

// Swarm (model) operator
// -----------------------------------
// Simulates a swarm of entities
#if defined(HAVE_OP_MODEL_SWARM) || defined(eEDITOR)
eOP_DEF_MODEL(eModelSwarmOp, "Swarm", 'p', 1, 1, eOP_INPUTS(eOC_MESH))
	eOP_INIT() 
	{
		m_lastCount = 0;
		m_lastSeed = 0;
		m_lastTime = 0;
		m_lastPopDistance = 0;
    }

public:
	struct tEntry {
		eU8			mdlIdx;
        eVector3    position;
        eF32		velocity;
        eQuat       rotation;
        eF32        size;
    };

//#define eSWARM_CALCULATION_FPS 60
//#define eSWARM_TIMESTEP_S (1.0f / (eF32)eSWARM_CALCULATION_FPS) 

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eModelSwarmOp, time, "Time", -eF32_MAX, eF32_MAX, 0.0f,
//        eOP_PAR_FLOAT(eModelSwarmOp, calcTimeStep, "Timestep", 0.0f, 1.0f, 0.0f,
        eOP_PAR_INT(eModelSwarmOp, seed, "Seed", 1, 10000, 1,
        eOP_PAR_LABEL(eModelSwarmOp, label0, "Surface Population", "Surface Population",
        eOP_PAR_INT(eModelSwarmOp, countMax, "Count", 2, 100000, 5,
        eOP_PAR_FLOAT(eModelSwarmOp, popDistance, "Populate Distance", 0, eF32_MAX, 0.0f,
        eOP_PAR_BOOL(eModelSwarmOp, keepOrient, "Keep Orientation", eFALSE,
        eOP_PAR_FXYZ(eModelSwarmOp, initRotation, "Rotation", 0, eF32_MAX, 0.0f, 0.0f, 1.0f,
        eOP_PAR_LINK_MODEL(eModelSwarmOp, modelOp, "Model", eOC_MODEL, eTRUE,
        eOP_PAR_LINK_MODEL(eModelSwarmOp, modelOp2, "Model2", eOC_MODEL, eFALSE,
        eOP_PAR_LINK_MODEL(eModelSwarmOp, modelOp3, "Model3", eOC_MODEL, eFALSE,
        eOP_PAR_FLOAT(eModelSwarmOp, size, "Size", 0, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eModelSwarmOp, sizeVariation, "Size Variation", 0, 1.0f, 1.0f,
        eOP_PAR_FLOAT(eModelSwarmOp, minDist, "Min Distance", 0, eF32_MAX, 1.0f,
        eOP_PAR_FXYZW(eModelSwarmOp, behaviour, "Acc.,Attr,HLevel,DUMMY", 0, eF32_MAX, 0.0f, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZW(eModelSwarmOp, swarmBehaviour, "Coll., Cohe., Align., DUMMY", 0, eF32_MAX, 0.0f, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FLOAT(eModelSwarmOp, desiredAttrDist, "Desired Attr Dist", 0, eF32_MAX, 0.0f,
        eOP_PAR_LINK_MESH(eModelSwarmOp, attractorMeshOp, "AttractorMesh", eOC_MESH, eFALSE,
        eOP_PAR_INT(eModelSwarmOp, attractorSamples, "AttractorSamples", 0, 100000, 10,
        eOP_PAR_INT(eModelSwarmOp, flockSize, "Flock Size", 1, 32, 4,
		eOP_PAR_END))))))))))))))))))))
	{
		ePROFILER_ZONE("Swarm Op");
		m_sceneData.clear();

		const eF32 acceleration = behaviour.x;
		const eF32 attraction = behaviour.y;
		const eF32 hleveling = behaviour.z;
		const eF32 collisionAvoidance = swarmBehaviour.x;
		const eF32 cohesion = swarmBehaviour.y;
		const eF32 alignment = swarmBehaviour.z;

		eU32 numAttractorSamples = (attractorSamples == 0) ? countMax : attractorSamples;
		eF32 minDistSqr = minDist * minDist;
		eF32 popDistSqr = popDistance * popDistance;
		eU32 ranSeed = seed;
		m_models.clear();
		m_models.append(&modelOp->getResult().sceneData);
		if(modelOp2)
			m_models.append(&modelOp2->getResult().sceneData);
		if(modelOp3)
			m_models.append(&modelOp3->getResult().sceneData);

		eF32 sightAngleCos = 1.0f - 2.0f * 0.8f;
		// sample surface mesh
		if(getAboveOp(0)->getChanged() || 
#ifdef eEDITOR
			(time < m_lastTime) || 
			(countMax != m_lastCount) || 
			(seed != m_lastSeed) ||
#endif
			(m_lastPopDistance != popDistance)) {
			m_accumulatedTime = 0.0f;
			m_lastTime = time;
			const eEditMesh& inp = ((eIMeshOp *)getAboveOp(0))->getResult().mesh;
			inp.calculatePartialSurfaceSums(m_faceSums);
			// reset entries
			m_entries.clear();
			m_entries.reserve(countMax);
			for(eU32 i = 0; i < countMax; i++) {
				tEntry& e = m_entries.append();
				eVector3 normal;
				inp.getSurfacePoint(m_faceSums, eRandomF(ranSeed), eRandomF(ranSeed), eRandomF(ranSeed), e.position, normal);
				e.mdlIdx = eRandom(0, m_models.size(), ranSeed);
				e.velocity = 0.0f;
				// rotate 
				e.rotation = eQuat(eVector3(0,0,1), initRotation.z * (-1.0f + 2.0f * eRandomF(ranSeed)) * ePI * 2.0f);
    			e.rotation *= eQuat(eVector3(1,0,0), initRotation.x * (-1.0f + 2.0f * eRandomF(ranSeed)) * ePI * 2.0f);
				e.rotation *= eQuat(eVector3(0,1,0), initRotation.y * (-1.0f + 2.0f * eRandomF(ranSeed)) * ePI * 2.0f);
				if(!keepOrient) {
					// rotate to normal
					eVector3 look = e.rotation.lerpAlongShortestArc(eVector3(0,0,1),normal,1.0f);
				}
				e.size = 1.0f - sizeVariation * eRandomF(ranSeed);

				// test population distance
				bool isnear = false;
				for(eU32 k = 0; k < (m_entries.size() - 1); k++) 
					isnear |= (e.position - m_entries[k].position).sqrLength() < popDistSqr;
				if(isnear)
					m_entries.resize(m_entries.size() - 1);
			}
		}

		m_accumulatedTime += time - m_lastTime;

		const eF32 timeStep = m_accumulatedTime; 

		eU32 loops = 10;
		do {

			// sample attractor mesh
			if(attractorMeshOp) {
				if(attractorMeshOp->getChanged()) {
					const eEditMesh& att = (attractorMeshOp)->getResult().mesh;
					m_attractorPositions.resize(numAttractorSamples);
					att.calculatePartialSurfaceSums(m_faceSums);
					eVector3 dummyNormal;
					for(eU32 i = 0; i < numAttractorSamples; i++) 
						att.getSurfacePoint(m_faceSums, eRandomF(ranSeed), eRandomF(ranSeed), eRandomF(ranSeed), m_attractorPositions[i], dummyNormal);

				}
				if((attractorSamples == 0) || (attractorMeshOp->getChanged())) {
					// build attractor tree
					m_attractorKdTree.clear();
					for(eU32 i = 0; i < numAttractorSamples; i++) 
						m_attractorKdTree.add(m_attractorPositions[i], &m_attractorPositions[i]);
					m_attractorKdTree.finishLoading();
				}
			}


			if(collisionAvoidance != 0.0f) {
				// build entity kd-tree
				m_kdTree.clear();
				for(eU32 k = 0; k < m_entries.size(); k++) {
					tEntry& e = m_entries[k];
					m_kdTree.add(e.position, &m_entries[k]);
				}
				m_kdTree.finishLoading();
			}

			m_accumulatedTime -= timeStep;
//			eF32 dt = eClamp(0.0f, time - m_lastTime, 0.1f);
			eF32 dt = eClamp(0.0f, timeStep, 1.0f);
//			const eF32 dt = eSWARM_TIMESTEP_S;
			for(eU32 k = 0; k < m_entries.size(); k++) {
				tEntry& entry = m_entries[k];

				eF32 requestedAccel = 0.0f;
				eVector3 direction = entry.rotation.getVector(2);
				direction.normalize();

				if(attractorMeshOp) {
					// steer towards best attractor
					m_attractorKdTree.getKNearest(m_kdLookupResults,1, entry.position);

					eEmVtxPos& vtx = *(eEmVtxPos*)m_kdLookupResults[0].t;
					eVector3 bestDirection = (vtx.pos - entry.position);
					eF32 bestDirLen = bestDirection.length();
					if(bestDirLen > eALMOST_ZERO) {
						bestDirection /= bestDirLen;
						direction = entry.rotation.lerpAlongShortestArc(direction, bestDirection, attraction * dt);

						if(bestDirLen <= desiredAttrDist + eALMOST_ZERO)
							requestedAccel -= attraction * eSqr((desiredAttrDist - bestDirLen) / desiredAttrDist);
						else
							requestedAccel += attraction * eSqr(1.0f - 1.0f / (1.0f + (bestDirLen - desiredAttrDist)));
					}

					// remove best attractor
					if(attractorSamples == 0)
						m_attractorKdTree.remove(m_kdLookupResults[0].t, *m_kdLookupResults[0].node);
				}

				if((collisionAvoidance != 0.0f) && (flockSize > 1)) {
					// lookup flockmates
					m_kdTree.getKNearest(m_kdLookupResults,flockSize, entry.position);
					// calculate center
					eVector3 avgDirection = direction;
					eVector3 center = entry.position;
					eF32 avgVelocity = entry.velocity;
					eU32 nbrs = 1;
					for(eU32 f = 1; f < m_kdLookupResults.size(); f++) {
						tEntry& other = *((tEntry*)m_kdLookupResults[f].t);
						eVector3 deltaOtherNorm = (entry.position - other.position).normalized();
						if(eIsFloatZero(deltaOtherNorm.sqrLength())) 
							deltaOtherNorm = eVector3(0,1,0);
					
						// test sight angle
						eF32 dotProduct = direction * deltaOtherNorm;
						if(dotProduct > sightAngleCos) 
						{
							nbrs++;
							center += other.position;
							// avoid collision
							if((dotProduct != 0.0f) && (m_kdLookupResults[f].distanceSqr < minDistSqr)) {
								eF32 amount = (1.0f - m_kdLookupResults[f].distanceSqr / minDistSqr);
								direction = entry.rotation.lerpAlongShortestArc(direction, deltaOtherNorm, collisionAvoidance * amount * dt);
							}
							// match velocity
							avgVelocity += other.velocity;
							avgDirection += other.rotation.getVector(2);
						}
					}
					if(nbrs > 1) {
						avgVelocity /= (eF32)nbrs;
						center /= (eF32)nbrs;
						// steer towards center of other entities
						eVector3 centerDir = (center - entry.position);
						eF32 centerDirLen = centerDir.length();
						if(!eIsFloatZero(centerDirLen)) {
							centerDir /= centerDirLen; // normalize
							direction = entry.rotation.lerpAlongShortestArc(direction, centerDir, cohesion * dt);
							requestedAccel += alignment * (avgVelocity - entry.velocity);
							entry.rotation.normalize();
						}
						eF32 avgDirectionLen = avgDirection.length();
						if(!eIsFloatZero(avgDirectionLen)) {
							avgDirection /= avgDirectionLen;
							direction = entry.rotation.lerpAlongShortestArc(direction, avgDirection, alignment * dt);
							entry.rotation.normalize();
						}
					}
				}

				if(hleveling != 0.0f) {
					// try to become horizontally
					eVector3 side = entry.rotation.getVector(0);
					eVector3 bestSide = direction^eVector3(0,-1,0);
					eF32 bestSideLen = bestSide.length();
					if(!eIsFloatZero(bestSideLen)) {
						bestSide /= bestSideLen;
						eF32 cosine = side * bestSide;
	//					if(cosine > -0.9f) 
	//					if(!eAreFloatsEqual(cosine, 1.0f)) 
						if((!eAreFloatsEqual(cosine, 1.0f)) && (!eAreFloatsEqual(cosine, -1.0f))) 
						{
							eF32 angle = eACos(cosine);
	//						eVector3 rotAxis = (bestSide^side);
							eVector3 rotAxis = (side^bestSide);
							eF32 rotAxisLength = rotAxis.length();
							if(rotAxisLength > eALMOST_ZERO) {
								rotAxis /= rotAxisLength;
	//							eF32 amount = angle / ePI;
								eF32 amount = 1.0f;
								entry.rotation = (eQuat(rotAxis, angle * eClamp(0.0f, amount * hleveling * dt, 1.0f)) * entry.rotation).normalized();
		//						entry.rotation = (eQuat(rotAxis, angle * eClamp(0.0f, angle * hleveling * dt, 1.0f)) * entry.rotation).normalized();
		//						entry.rotation = (eQuat(rotAxis, angle * hleveling * dt) * entry.rotation).normalized();
								direction = entry.rotation.getVector(2);
							}
						}
					}
				}

	/**/
				requestedAccel = acceleration * eClamp(-1.0f, requestedAccel, 1.0f);
				requestedAccel -= 0.2f * entry.velocity;
				entry.position += direction * (entry.velocity * dt + 0.5f * requestedAccel * dt * dt);
				entry.velocity += requestedAccel * dt;
			}
		} while((m_accumulatedTime > 0.0f) && (--loops != 0));

		// draw
		for(eU32 k = 0; k < m_entries.size(); k++) {
			tEntry& entry = m_entries[k];

			eTransform tr;
			tr.scale(eVector3(entry.size * size));
			tr.rotate(entry.rotation.conjugated().normalized());
			tr.translate(entry.position);
			m_sceneData.merge(*m_models[entry.mdlIdx], tr);
		}
		m_lastCount = countMax;
		m_lastSeed = seed;
		m_lastTime = time;
		m_lastPopDistance = popDistance;
    }
	eOP_EXEC2_END

	eArray<eVector3>	m_attractorPositions;
	eArray<eF32>	m_faceSums;

	eU32			m_lastCount;
	eU32			m_lastSeed;
	eF32			m_lastTime;
	eF32			m_lastPopDistance;
	eF32			m_accumulatedTime;
    eArray<tEntry>  m_entries;
	eGenericKDTree	m_kdTree;
	eGenericKDTree	m_attractorKdTree;
	eArray<eGenericKDTree::Result>	m_kdLookupResults;
	eArray<eSceneData*> m_models;
eOP_END(eModelSwarmOp);
#endif



// Swarm POV (misc) operator
// ----------------------
// Holds a camera and a viewport.
/*
#if defined(HAVE_OP_MISC_SWARMPOV) || defined(eEDITOR)
eOP_DEF(eSwarmPovOp, eIPovOp, "SwarmPOV", "Misc", eColor(170, 170, 170), 's', eOC_POV, 1, 1, eOP_INPUTS(eOC_MODEL))

	eOP_INIT() 
	{
		m_lastTime = eF32_MAX;
	}

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eSwarmPovOp, time, "Time", -eF32_MAX, eF32_MAX, 0.0f,
        eOP_PAR_INT(eSwarmPovOp, entityNr, "Swarm Member No", 0, 65536, 0,
        eOP_PAR_FXYZ(eSwarmPovOp, offsetPos, "Offset Pos", -eF32_MAX, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FXYZ(eSwarmPovOp, offsetRot, "Offset Rot", -eF32_MAX, eF32_MAX, 0.0f, 0.0f, 0.0f,
        eOP_PAR_FLOAT(eSwarmPovOp, smoothCam, "Smooth Cam Movement", eALMOST_ZERO, eF32_MAX, 1.0f,
        eOP_PAR_FXYZW(eSwarmPovOp, viewport, "Viewport", 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        eOP_PAR_FLOAT(eSwarmPovOp, fovY, "Field of view (Y)", eALMOST_ZERO, 180.0f, 45.0f,
        eOP_PAR_ENUM(eSwarmPovOp, aspectSel, "Aspect ratio", "4:3|16:9|16:10|2.35:1", 0,
        eOP_PAR_FLOAT(eSwarmPovOp, zNear, "Near plane", eF32_MIN, eF32_MAX, 0.01f,
        eOP_PAR_FLOAT(eSwarmPovOp, zFar, "Far plane", eF32_MIN, eF32_MAX, 1000.0f,
		eOP_PAR_END)))))))))))
    {
		// exponentially decreasing smoothing
		if(m_lastTime >= time) {
			m_lastTime = time;
			m_smoothsum = 0.0f;
			m_camPos = eVector3();
			m_rotation = eQuat();
		}
		eF32 deltaTime = time - m_lastTime;
		m_lastTime = time;
		eF32 decay = ePow(0.5f, deltaTime * smoothCam);
//		decay = 0.0f;

		const eModelSwarmOp& swarm = *(const eModelSwarmOp*)this->getAboveOp(0)->getResultOp();
		const eModelSwarmOp::tEntry& e = swarm.m_entries[entityNr >= swarm.m_entries.size() ? swarm.m_entries.size() - 1 : entityNr];
		const eVector3 objCamPos    = e.position + swarm.m_originPos + e.rotation.getVector(0) * offsetPos.x + e.rotation.getVector(1) * offsetPos.y + e.rotation.getVector(2) * offsetPos.z; 

		const eQuat orot(offsetRot);
		m_rotation = e.rotation * orot + m_rotation * decay;
		m_camPos = objCamPos + m_camPos * decay;
		m_smoothsum = 1.0f + m_smoothsum * decay;

		eVector3 camPos = (m_camPos / m_smoothsum);
//		eVector3 camPos = objCamPos;
		eQuat rotation = (m_rotation / m_smoothsum).normalized();
        // setup view matrix
        eMatrix4x4 mtxView;
        mtxView.lookAt(camPos, 
					   camPos + rotation.getVector(2),
					   rotation.getVector(1));
        // setup camera
        const eF32 aspectRatios[] = {4.0f/3.0f, 16.0f/9.0f, 16.0f/10.0f, 2.35f};
        m_cam = eCamera(fovY, aspectRatios[aspectSel], zNear, zFar);
        m_cam.setViewMatrix(mtxView);
        m_viewport = viewport;
    }
	eOP_EXEC2_END

	eQuat		m_rotation;
	eVector3	m_camPos;
	eF32		m_smoothsum;
	eF32		m_lastTime;
eOP_END(eSwarmPovOp);
#endif
*/
