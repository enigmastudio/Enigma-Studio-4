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

#ifndef FONT_HPP
#define FONT_HPP

struct eFontLetter
{
    eChar               chr;
    eRect               rect;
    eU32                width;
    eVector2            uv;
    eBool               used;
};

class eFont
{
public:
    eFont();
    ~eFont();

    void                create(const eChar *fontFace, const eChar *letters, eU32 height, const eColor color = eCOL_WHITE);

    eTexture2d *        getTextureAtlas() const;
    eU32                getHeight() const;
    eU32                getLetterCount() const;
    const eFontLetter & getLetter(eChar chr) const;

private:
    eU32                m_numLetters;
    eFontLetter         m_letters[256];
    eTexture2d *        m_tex;
    eArray<eColor>      m_bmp;
    eU32                m_height;
};

#endif // FONT_HPP