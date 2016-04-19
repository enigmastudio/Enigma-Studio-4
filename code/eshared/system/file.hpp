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

#ifndef FILE_HPP
#define FILE_HPP

#ifndef ePLAYER

enum eFileSeekMode
{
    eSEEK_BEGIN,
    eSEEK_CURRENT,
    eSEEK_END
};

enum eFileOpenMode
{
    eFOM_READ      = 0x01,
    eFOM_WRITE     = 0x02,
    eFOM_READWRITE = eFOM_READ|eFOM_WRITE
};

class eFile
{
public:
    eFile(const eChar *fileName);
    ~eFile();

    eBool               open(eFileOpenMode openMode);
    eBool               close();
    void                clear();
    eU32                read(ePtr buffer, eU32 byteCount);
    eU32                readAll(eByteArray &buffer);
    void                write(eConstPtr buffer, eU32 byteCount);
    void                write(const eByteArray &buffer);
    eU32                seek(eFileSeekMode seekMode, eU32 offset);
    eU32                tell() const;
    eU32                getSize() const;
    static eByteArray   readAll(const eChar *fileName, eBool *success=nullptr);

private:
    const eChar *       m_fileName;
    eFileOpenMode       m_openMode;
    eBool               m_changed;
    eBool               m_opened;
    eByteArray          m_data;
    eU32                m_pos;
};

#endif

#endif // FILE_HPP