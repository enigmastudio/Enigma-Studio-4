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

const eF32 eDemo::MAX_RUNNING_TIME_MINS = 5.0f;

eDemo::eDemo() :
    m_seq(nullptr),
    m_target(nullptr)
{
}

eDemo::~eDemo()
{
    eGfx->removeTexture2d(m_target);
}

eSeqResult eDemo::render(eF32 time) const
{
    eASSERT(time >= 0.0f);

    if (!m_seq)
        return eSEQ_FINISHED;

    _setupTarget(m_seq->getAspectRatio());

    // clear background
    eRenderState &rs = eGfx->freshRenderState();
    rs.targets[0] = eGraphics::TARGET_SCREEN;
    eGfx->clear(eCM_ALL, eCOL_BLACK);

    // render frame on target
    eGfx->pushRenderState();
    const eSeqResult res = m_seq->run(time, m_target, m_depthTarget);
    eGfx->popRenderState();

    // copy to screen
    rs.targets[0] = eGraphics::TARGET_SCREEN;
	rs.depthTarget = nullptr;
    eRenderer->renderQuad(m_renderRect, eGfx->getWndSize(), m_target);
    return res;
}

void eDemo::setSequencer(const eSequencer *seq)
{
    m_seq = seq;
}

const eSequencer * eDemo::getSequencer() const
{
    return m_seq;
}

void eDemo::_setupTarget(eF32 aspectRatio) const
{
    eASSERT(aspectRatio > 0.0f);

    // calculate target size based on aspect ratio
    const eU32 winWidth = eGfx->getWndWidth();
    const eU32 winHeight = eGfx->getWndHeight();

    eU32 targetWidth = winWidth;
    eU32 targetHeight = eFtoL(winWidth/aspectRatio);

    if (targetHeight > winHeight)
    {
        targetHeight = winHeight;
        targetWidth = eFtoL(winHeight*aspectRatio);
    }

    // update render rectangle
    m_renderRect.left = (winWidth-targetWidth)/2;
    m_renderRect.top = (winHeight-targetHeight)/2;
    m_renderRect.right = winWidth-m_renderRect.left;
    m_renderRect.bottom = winHeight-m_renderRect.top;

    // recreate target if needed
    if (!m_target || m_target->width != targetWidth || m_target->height != targetHeight)
    {
        eGfx->removeTexture2d(m_target);
        m_target = eGfx->addTexture2d(targetWidth, targetHeight, eTEX_TARGET, eTFO_ARGB8);
		m_depthTarget = eGfx->addTexture2d(targetWidth, targetHeight, eTEX_NOMIPMAPS, eTFO_DEPTH32F);
    }
}