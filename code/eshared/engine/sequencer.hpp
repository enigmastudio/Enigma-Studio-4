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

#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

enum eSeqEntryType
{
    eSET_SCENE,
    eSET_OVERLAY,
    eSET_TEXT,
};

enum eSeqBlendMode
{
    eSBM_ADD,
    eSBM_SUB,
    eSBM_MUL,
    eSBM_BRIGHTER,
    eSBM_DARKER,
    eSBM_NONE,
};

enum eSeqResult
{
    eSEQ_PLAYING,
    eSEQ_FINISHED
};

struct eSeqScene
{
    eIEffect *                  effect;
    eF32                        timeOffset;
    eF32                        timeScale;
};

struct eSeqOverlay
{
    eTexture2d *                texture;
    eBool                       filtered;
    eFXYZW                      rect;
    eFXY                        scrollUv;
    eFXY                        tileUv;
    eInt                        texAddr;
};

struct eSeqText
{
    const eFont *               font;
    const eChar *               text;
    eFXY                        pos;
    eFXY                        size;
	eBool						filtered;
};

struct eSeqEntry
{   
    union
    {
        eSeqScene               scene;
        eSeqOverlay             overlay;
        eSeqText                text;
    };

    eSeqEntryType               type;
    eF32                        startTime;
    eF32                        duration;
    eSeqBlendMode               blendMode;
    eVector2                    blendRatios;
};

class eSequencer
{
public:
    eSequencer();
    ~eSequencer();
    
    eSeqResult                  run(eF32 time, eTexture2d *target, eTexture2d *depthTarget) const;

    void                        addEntry(const eSeqEntry &entry, eU32 track);
    void                        merge(const eSequencer &seq);
    void                        clear();

    void                        setAspectRatio(eF32 aspectRatio);

    const eArray<eSeqEntry> &   getEntriesOfTrack(eU32 track) const;
    eF32                        getAspectRatio() const;
    eF32                        getEndTime() const;
    
private:
    const eSeqEntry *           _getEntryForTrack(eF32 time, eU32 track) const;
    void                        _setupTargets(eU32 width, eU32 height) const;
    void                        _freeTargets() const;
    void                        _renderEntry(const eSeqEntry &entry, eF32 time, eTexture2d *target, eTexture2d *depthTarget) const;
    void                        _mergeTargets(eTexture2d *target) const;

public:
    static const eInt           MAX_TRACKS = 16;

private:
    struct ShaderConsts
    {
        eVector4                viewport0;
        eVector4                viewport1;
        eVector4                clearCol;
        eSeqBlendMode           blendMode;
        eVector2                blendRatios;
		eInt					depthTestOn;
    };

private:
    eF32                        m_endTime;
    eF32                        m_aspectRatio;
    mutable eTexture2d *        m_tempRt;
    mutable eTexture2d *        m_copyRt;
    mutable eTexture2d *        m_depthRt;
    eArray<eSeqEntry>           m_entries[MAX_TRACKS];
    eVertexShader *             m_vsQuad;
    ePixelShader *              m_psMergeFx;
};

#endif // SEQUENCER_HPP