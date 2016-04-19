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

#include <stdio.h>
#define NOMINMAX
#include <windows.h>
#include <dsound.h>

#include "../system/system.hpp"
#include "../math/math.hpp"
#include "tf4.hpp"
#include "tf4dx.hpp"

class eTfDxThread : public eThread
{
public:
    eTfDxThread(eTfDx &tfdx) : eThread(eTHP_HIGH),
        m_tfdx(tfdx)
    {
        eMemSet(m_silence, 0, sizeof(m_silence));
    }

    virtual eU32 operator () ()
    {
        ePROFILER_ADD_THIS_THREAD("Tunefish 4");
    
        while (!m_tfdx.joinRequest)
        {
            ePROFILER_BEGIN_THREAD_FRAME();
            _processSynth();
            ePROFILER_END_THREAD_FRAME();
        }

        return 0;
    }

    void _processSynth()
    {
        ePROFILER_FUNC();

        if (eTfDxNeedMore(m_tfdx))
        {
            const eU8 *data = nullptr;
            eTfPlayerProcess(*m_tfdx.player, &data);
            if (!data)
                data = m_silence;

            eTfDxFill(m_tfdx, data, sizeof(m_silence));
        }
        else
            yield();
    }

private:
    eTfDx & m_tfdx;
    eU8     m_silence[TF_FRAMESIZE*2*sizeof(eS16)];
};

eBool eTfDxInit(eTfDx &tfdx, eU32 sampleRate)
{
    eMemSet(&tfdx, 0, sizeof(eTfDx));

    if (FAILED(DirectSoundCreate8(NULL, &tfdx.dsound, NULL)))
        return eFALSE;
    if (FAILED(tfdx.dsound->SetCooperativeLevel(GetForegroundWindow(), DSSCL_NORMAL)))
        return eFALSE;
    
    tfdx.bufferSize = sampleRate*4; 
    tfdx.sampleRate = sampleRate;

    PCMWAVEFORMAT pcmwf;
    eMemSet(&pcmwf, 0, sizeof(pcmwf));
    pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.wf.nChannels = 2;
    pcmwf.wf.nSamplesPerSec = sampleRate;
    pcmwf.wf.nBlockAlign = 4;
    pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec*pcmwf.wf.nBlockAlign;
    pcmwf.wBitsPerSample = 16;

    DSBUFFERDESC dsbDesc;
    eMemSet(&dsbDesc, 0, sizeof(dsbDesc));
    dsbDesc.dwSize = sizeof(dsbDesc);
    dsbDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS;
    dsbDesc.dwBufferBytes = tfdx.bufferSize;
    dsbDesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;

    if (FAILED(tfdx.dsound->CreateSoundBuffer(&dsbDesc, (IDirectSoundBuffer **)&tfdx.dsoundBuffer, nullptr)))
        return eFALSE;

    return eTRUE;
}

void eTfDxShutdown(eTfDx &tfdx)
{
    eReleaseCom(tfdx.dsoundBuffer);
    eReleaseCom(tfdx.dsound);
}

void eTfDxStart(eTfDx &tfdx, eTfPlayer &player)
{
    if (!tfdx.dsoundBuffer)
        return;

    tfdx.player = &player;
    tfdx.joinRequest = eFALSE;
    tfdx.thread = new eTfDxThread(tfdx);
    tfdx.nextWriteOffset = tfdx.bufferSize / 2;

    HRESULT res = tfdx.dsoundBuffer->SetCurrentPosition(0);
    eASSERT(!FAILED(res));
    res = tfdx.dsoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
    eASSERT(!FAILED(res));
}

void eTfDxStop(eTfDx &tfdx)
{
    if (!tfdx.dsoundBuffer)
        return;

    const HRESULT res = tfdx.dsoundBuffer->Stop();
    eASSERT(!FAILED(res));

    tfdx.joinRequest = eTRUE;
    tfdx.thread->join();
    eDelete(tfdx.thread);
}

void eTfDxFill(eTfDx &tfdx, const eU8 *data, eU32 count)
{
    if (!tfdx.dsoundBuffer)
        return;

    ePtr write0, write1;
    DWORD length0, length1;
    HRESULT hr = tfdx.dsoundBuffer->Lock(tfdx.nextWriteOffset, count, &write0, &length0, &write1, &length1, 0);

    if (hr == DSERR_BUFFERLOST)
    {
        hr = tfdx.dsoundBuffer->Restore();
        eASSERT(!FAILED(hr));
        hr = tfdx.dsoundBuffer->Lock(0, count, &write0, &length0, &write1, &length1, 0);
    }

    eASSERT(!FAILED(hr));
    eMemCopy(write0, data, length0);

    if (write1)
        eMemCopy(write1, data+length0, length1);

    hr = tfdx.dsoundBuffer->Unlock(write0, length0, write1, length1);
    eASSERT(!FAILED(hr));
    tfdx.nextWriteOffset += count;
    tfdx.nextWriteOffset %= tfdx.bufferSize;
}

eBool eTfDxNeedMore(eTfDx &tfdx)
{
    if (!tfdx.dsoundBuffer)
        return eFALSE;

    DWORD playCursor, writeCursor;
    tfdx.dsoundBuffer->GetCurrentPosition(&playCursor, &writeCursor);

    eS32 writeOffset = tfdx.nextWriteOffset;
    const eS32 minDistance = tfdx.bufferSize/2;
    const eS32 maxDistance = minDistance + 4*TF_FRAMESIZE;
    eS32 distance = writeOffset - playCursor;

    if (distance < 0)
        distance += tfdx.bufferSize;

    const eBool needMore = distance < minDistance || distance > maxDistance;

    // START OF LOGGING CODE
    /*
    const eU32 DRAWLEN = 100;
    FILE *out = fopen("c:\\dev\\tfaudio.log", "ab");
    eU32 pc = playCursor * DRAWLEN / tfdx.bufferSize;
    eU32 wc = tfdx.nextWriteOffset * DRAWLEN / tfdx.bufferSize;
    for(eU32 i=0;i<DRAWLEN;i++)
    {
        if (wc > pc)
        {
            if (i > pc && i < wc)
                fprintf(out, "#");
            else
                fprintf(out, ".");
        }
        else
        {
            if (i < wc || i > pc)
                fprintf(out, "#");
            else
                fprintf(out, ".");
        }
    }
        
    if (!needMore)
        fprintf(out, " OK ");
    else
        fprintf(out, "    ");

    fprintf(out, " %i / %i - W:%i - R:%i\n", distance, minDistance, wc, pc);
    fclose(out);
    */
    // END OF LOGGING CODE 

    return needMore;
}