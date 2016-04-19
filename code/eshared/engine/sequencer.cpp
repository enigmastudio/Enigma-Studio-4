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
#include "engine.hpp"

eSequencer::eSequencer() :
    m_endTime(0.0f),
    m_aspectRatio(4.0f/3.0f),
    m_tempRt(nullptr),
    m_copyRt(nullptr),
    m_depthRt(nullptr),
    m_vsQuad(eGfx->loadVertexShader(eSHADER(vs_quad))),
    m_psMergeFx(eGfx->loadPixelShader(eSHADER(ps_fx_merge)))
{
    eASSERT(m_vsQuad);
    eASSERT(m_psMergeFx);
}

eSequencer::~eSequencer()
{
    _freeTargets();
}

eSeqResult eSequencer::run(eF32 time, eTexture2d *target, eTexture2d *depthTarget) const
{
    eASSERT(time >= 0.0f);
    eASSERT(target->target);

    _setupTargets(target->width, target->height);

    // clear output target
    eRenderState &rs = eGfx->freshRenderState();
    rs.depthTarget = m_depthRt;
    rs.targets[0] = target;
    eGfx->clear(eCM_ALL, eCOL_BLACK);

    // are there still entries to play?
    // used for indicating player to exit
    if (time > m_endTime)
        return eSEQ_FINISHED;

    // render all active entries
    eTexture2d *src = target;
    eTexture2d *dst = m_copyRt;

    for (eU32 i=0; i<MAX_TRACKS; i++)
    {
        const eSeqEntry *entry = _getEntryForTrack(time, i);
        if (entry)
        {
            // render entry to temporary target
			eGfx->pushRenderState();
            _renderEntry(*entry, time, m_tempRt, depthTarget);
			eGfx->popRenderState();
			
            // merge temporary and "screen" targets
            static eConstBuffer<ShaderConsts, eST_PS> cb;
            cb.data.viewport0.set(0.0f, 0.0f, 1.0f, 1.0f);
            cb.data.viewport1.set(0.0f, 0.0f, 1.0f, 1.0f);
            cb.data.clearCol.null();
            cb.data.blendMode = entry->blendMode;
            cb.data.blendRatios = entry->blendRatios;
			cb.data.depthTestOn = 0;
						
            rs.vs = m_vsQuad;
            rs.ps = m_psMergeFx;
            rs.textures[0] = src;
            rs.textures[1] = m_tempRt;
            rs.targets[0] = dst;
            rs.constBufs[eCBI_FX_PARAMS] = &cb;
            eRenderer->renderQuad(eRect(0, 0, target->width, target->height), eSize(target->width, target->height));

            eSwap(src, dst);
        }
    }

    // if last copy went to copy target,
    // copy back to input target
    if (dst == target)
    {
        rs.ps = nullptr;
        rs.vs = nullptr;
        rs.blending = eFALSE;
        rs.targets[0] = target;
        rs.texFlags[0] = eTMF_NEAREST;
        eRenderer->renderQuad(eRect(0, 0, target->width, target->height), eSize(target->width, target->height), src);
    }

    return eSEQ_PLAYING;
}

void eSequencer::addEntry(const eSeqEntry &entry, eU32 track)
{
    eASSERT(track < MAX_TRACKS);
    m_entries[track].append(entry);
    m_endTime = eMax(m_endTime, entry.startTime+entry.duration);
}

void eSequencer::merge(const eSequencer &seq)
{
    for (eU32 i=0; i<MAX_TRACKS; i++)
        for (eU32 j=0; j<seq.m_entries[i].size(); j++)
            addEntry(seq.m_entries[i][j], i);
}

void eSequencer::clear()
{
    for (eU32 i=0; i<MAX_TRACKS; i++)
        m_entries[i].clear();

    m_endTime = 0.0f;
}

void eSequencer::setAspectRatio(eF32 aspectRatio)
{
    eASSERT(aspectRatio > 0.0f);
    m_aspectRatio = aspectRatio;
}

const eArray<eSeqEntry> & eSequencer::getEntriesOfTrack(eU32 track) const
{
    eASSERT(track < MAX_TRACKS);
    return m_entries[track];
}

eF32 eSequencer::getAspectRatio() const
{
    return m_aspectRatio;
}

eF32 eSequencer::getEndTime() const
{
    return m_endTime;
}

const eSeqEntry * eSequencer::_getEntryForTrack(eF32 time, eU32 track) const
{
    eASSERT(track < MAX_TRACKS);
    eASSERT(time >= 0.0f);

    for (eU32 i=0; i<m_entries[track].size(); i++)
    {
        const eSeqEntry *entry = &m_entries[track][i];
        if (entry->startTime <= time && entry->startTime+entry->duration > time)
            return entry;
    }

    return nullptr;
}

void eSequencer::_setupTargets(eU32 width, eU32 height) const
{
    eASSERT(width > 0);
    eASSERT(height > 0);

    if (!m_tempRt || width != m_tempRt->width || height != m_tempRt->height)
    {
        _freeTargets();
        m_tempRt = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB8);
        m_copyRt = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB8);
        m_depthRt = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_DEPTH32F);
    }
}

void eSequencer::_freeTargets() const
{
    eGfx->removeTexture2d(m_copyRt);
    eGfx->removeTexture2d(m_tempRt);
    eGfx->removeTexture2d(m_depthRt);
}

void eSequencer::_renderEntry(const eSeqEntry &entry, eF32 time, eTexture2d *target, eTexture2d *depthTarget) const
{
    if (entry.type == eSET_OVERLAY)
    {
        const eSeqOverlay &ol = entry.overlay;
        const eVector2 ul(ol.rect.x, 1.0f-ol.rect.y);
        const eVector2 br(ol.rect.z, 1.0f-ol.rect.w);

        const eRect r(eFtoL(ul.x*(eF32)target->width),
                      eFtoL(br.y*(eF32)target->height),
                      eFtoL(br.x*(eF32)target->width),
                      eFtoL(ul.y*(eF32)target->height));

        eRenderState &rs = eGfx->freshRenderState();
        rs.depthTarget = m_depthRt;
        rs.targets[0] = target;
        rs.texFlags[0] = (entry.overlay.filtered ? eTMF_TRILINEAR : eTMF_NEAREST)|entry.overlay.texAddr;
        rs.viewport.right = target->width;
        rs.viewport.bottom = target->height;

        eGfx->clear(eCM_COLOR, eCOL_BLACK);
        eRenderer->renderQuad(r, eSize(target->width, target->height), ol.texture, ol.tileUv, ol.scrollUv);
    }
    else if (entry.type == eSET_TEXT)
    {
        eRenderState &rs = eGfx->freshRenderState();
        rs.depthTarget = m_depthRt;
        rs.targets[0] = target;
        rs.texFlags[0] = (entry.text.filtered ? eTMF_TRILINEAR : eTMF_NEAREST)|eTMF_CLAMP;
        rs.viewport.right = target->width;
        rs.viewport.bottom = target->height;
        eGfx->clear(eCM_COLOR, eCOL_BLACK);

        const eSeqText &txt = entry.text;
        eTexture2d *tex = txt.font->getTextureAtlas();
        eF32 y = txt.pos.y;
        eF32 x = txt.pos.x;

        for (eU32 i=0; i<eStrLength(entry.text.text); i++)
        {
            const eFontLetter &l = entry.text.font->getLetter(txt.text[i]);
            const eF32 w = txt.size.x*(eF32)l.width*0.01f;
            const eRect r(eFtoL(x*target->width), eFtoL((1.0f-y-txt.size.y)*target->height), eFtoL((x+w)*target->width), eFtoL((1.0f-y)*target->height));
            const eVector2 tileUv((eF32)l.width/(eF32)tex->width, (eF32)txt.font->getHeight()/(eF32)tex->height);

            eRenderer->renderQuad(r, eSize(target->width, target->height), tex, tileUv, l.uv);
            x += w;
        }
    }
    else if (entry.type == eSET_SCENE)
        entry.scene.effect->run((time-entry.startTime)*entry.scene.timeScale+entry.scene.timeOffset, target, depthTarget);
}