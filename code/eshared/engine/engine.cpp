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

eGraphics *         eGfx;
eDeferredRenderer * eRenderer;

eBool eEngine::m_forceNoVSync = eFALSE;

#ifdef eEDITOR
eArray<eShaderDefinition>	eEngine::_usedShaders;
#endif

eEngine::eEngine()
{
    eGfx = new eGraphics;
    eGfx->initialize();
}

eEngine::eEngine(eInt windowFlags, const eSize &wndSize, ePtr hwnd)
{
    eGfx = new eGraphics;
    eGfx->initialize();
    openWindow(windowFlags, wndSize, hwnd);
}

eEngine::~eEngine()
{
#ifndef eCFG_NO_CLEANUP
    eDelete(eRenderer);
    eIEffect::shutdown();
    eMaterial::shutdown();
    eDelete(eGfx);
#endif
}

void eEngine::openWindow(eInt windowFlags, const eSize &wndSize, ePtr hwnd)
{
    windowFlags = (m_forceNoVSync ? windowFlags&(~eWF_VSYNC) : windowFlags);
    eGfx->openWindow(wndSize.width, wndSize.height, windowFlags, hwnd);
    eMaterial::initialize();
    eRenderer = new eDeferredRenderer;
}

void eEngine::forceNoVSync(eBool forceNoVSync)
{
    m_forceNoVSync = forceNoVSync;
}

#ifdef eEDITOR
void eEngine::_addUsedShader(const eString& opClass, const eString& str, const eString& shaderContent) {
	// avoid double insertion
	for(eU32 i = 0; i < _usedShaders.size(); i++)
		if((_usedShaders[i].opClass == opClass) &&
			(_usedShaders[i].shaderName == str))
			return;

	eShaderDefinition& def = _usedShaders.append();
	def.opClass = opClass;
	def.shaderName = str;
	def.shaderContent = shaderContent;
}

void eEngine::_addEffectVar(const eString& opClass, eIEffect& var) {
	for(eU32 i = 0; i < var.m_usedShaders.size(); i++)
		_addUsedShader(opClass, var.m_usedShaders[i].name, var.m_usedShaders[i].shaderContent);
}
#endif