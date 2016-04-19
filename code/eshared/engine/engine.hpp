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

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "graphics.hpp"
#include "graphicsdx11.hpp"

extern eGraphics * eGfx;

#include "material.hpp"
#include "editmesh.hpp"
#include "path.hpp"
#include "camera.hpp"
#include "renderjob.hpp"
#include "light.hpp"
#include "irenderable.hpp"
#include "scenedata.hpp"
#include "culler.hpp"
#include "scene.hpp"
#include "particlesys.hpp"
#include "mesh.hpp"
#include "deferredrenderer.hpp"
#include "transfwidgets.hpp"

extern eDeferredRenderer * eRenderer;

#include "font.hpp"
#include "effect.hpp"
#include "sequencer.hpp"
#include "demo.hpp"
#include "shaders/shaders.hpp"

#ifdef ePLAYER
#include "../../eplayer4/production.hpp"
#endif


#ifdef eEDITOR
#define eREGISTER_OP_SHADER(OPCLASS, DEF)	eEngine::_addUsedShader(#OPCLASS, #DEF, eSHADERCONTENT(DEF,))
#define eREG_GLUE_FUNC2(x,y) x##y
#define eREG_GLUE_FUNC1(x,y) eREG_GLUE_FUNC2(x,y)
#define eREGISTER_ENGINE_SHADER_INTERNAL(SUFFIX, OPNAME, DEF, COUNTER)						\
	eU32 eREG_GLUE_FUNC1(regFnc##DEF,COUNTER) () {                                  \
		eEngine::_addUsedShader(OPNAME, #DEF, eSHADERCONTENT(DEF, SUFFIX));					\
		return 0;																	\
	}																				\
	eU32 eREG_GLUE_FUNC1(regVal##DEF,COUNTER) = eREG_GLUE_FUNC1(regFnc##DEF, COUNTER)();
#define eREGISTER_ENGINE_SHADER_INTERNAL1(SUFFIX, OPNAME,DEF, COUNTER) eREGISTER_ENGINE_SHADER_INTERNAL(SUFFIX, OPNAME, DEF, COUNTER)
#define eREGISTER_ENGINE_SHADER(DEF) eREGISTER_ENGINE_SHADER_INTERNAL1( , "", DEF, eREG_GLUE_FUNC1(__COUNTER__, __LINE__))
#define eREGISTER_ENGINE_SHADER_WITHSUFFIX(DEF, SUFFIX) eREGISTER_ENGINE_SHADER_INTERNAL1(SUFFIX, "", DEF, eREG_GLUE_FUNC1(__COUNTER__, __LINE__))
#define eREGISTER_EFFECT_SHADER(OPNAME, DEF) eREGISTER_ENGINE_SHADER_INTERNAL1( , OPNAME, DEF, eREG_GLUE_FUNC1(__COUNTER__, __LINE__))
#else
#define eREGISTER_OP_SHADER(OPCLASS, DEF)
#define eREGISTER_ENGINE_SHADER(DEF)
#define eREGISTER_ENGINE_SHADER_WITHSUFFIX(DEF, SUFFIX)
#endif

struct eShaderDefinition {
	eString		opClass;
	eString		shaderName;
	eString		shaderContent;
};


class eEngine
{
public:
    eEngine();
    eEngine(eInt windowFlags, const eSize &wndSize, ePtr hwnd);
    ~eEngine();

    void            openWindow(eInt windowFlags, const eSize &wndSize, ePtr hwnd);
    static void     forceNoVSync(eBool vsync);

#ifdef eEDITOR
	static eArray<eShaderDefinition>	_usedShaders;
	static void							_addUsedShader(const eString& opClass, const eString& str, const eString& shaderContent);
	static void							_addEffectVar(const eString& opClass, eIEffect& var);
#else
	static const char *			demodata_shader_indices[];
	static eByteArray			demodata_shaders;
#endif

private:
    static eBool    m_forceNoVSync;
};

#endif // ENGINE_HPP