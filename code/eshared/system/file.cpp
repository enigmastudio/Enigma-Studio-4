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

#ifndef ePLAYER

#include <fstream>
#include "system.hpp"

using namespace std;

eFile::eFile(const eChar *fileName) :
    m_fileName(fileName),
    m_openMode(eFOM_READ),
    m_changed(eFALSE),
    m_opened(eFALSE),
    m_pos(0)
{
}

eFile::~eFile()
{
    close();
}

eBool eFile::open(eFileOpenMode openMode)
{
    m_pos = 0;
    m_openMode = openMode;

    eASSERT(openMode&eFOM_READ || openMode&eFOM_WRITE);   
    fstream::openmode fsom = fstream::binary;

    if (openMode&eFOM_READ)
        fsom |= fstream::in;
    if (openMode&eFOM_WRITE)
        fsom |= fstream::out;

    fstream f(m_fileName, fsom);
    if (!f.is_open())
        return eFALSE;

    // retrieve file size
    f.seekg(0, ifstream::end);
    const eU32 size = (eU32)f.tellg();
    f.seekg(0, ifstream::beg);

    // buffer file
    if (size > 0)
    {
        m_data.resize(size);
        f.read((eChar *)&m_data[0], m_data.size());
    }

    m_opened = eTRUE;
    return eTRUE;
}

eBool eFile::close()
{
    if (m_opened && m_openMode&eFOM_WRITE && m_changed)
    {
        fstream f(m_fileName, fstream::binary|fstream::out|fstream::trunc);

        if (f.is_open())
            f.write((eChar *)&m_data[0], m_data.size());
        else
            return eFALSE;
    }

    m_opened = eFALSE;
    return eTRUE;
}

void eFile::clear()
{
    eASSERT(m_opened);
    eASSERT(m_openMode&eFOM_WRITE);

    m_data.clear();
    m_pos = 0;
    m_changed = eTRUE;
}

// returns number of read bytes
eU32 eFile::read(ePtr buffer, eU32 byteCount)
{
    eASSERT(m_opened);
    eASSERT(m_openMode&eFOM_READ);

    const eU32 readCount = (m_pos+byteCount <= m_data.size() ? byteCount : m_data.size()-m_pos);
    eMemCopy(buffer, &m_data[0]+m_pos, readCount);
    m_pos += readCount;
    return readCount;
}

eU32 eFile::readAll(eByteArray &buffer)
{
    eASSERT(m_openMode&eFOM_READ);
    buffer = m_data;
    return buffer.size();
}

void eFile::write(eConstPtr buffer, eU32 byteCount)
{
    eASSERT(m_opened);
    eASSERT(m_openMode&eFOM_WRITE);

    if (m_pos+byteCount > m_data.size())
        m_data.resize(m_pos+byteCount);

    eMemCopy(&m_data[m_pos], buffer, byteCount);
    m_pos += byteCount;
    m_changed = eTRUE;
}

void eFile::write(const eByteArray &buffer)
{
    write(&buffer[0], buffer.size());
}

// returns new absolute file position
eU32 eFile::seek(eFileSeekMode seekMode, eU32 offset)
{
    eASSERT(m_opened);

    switch (seekMode)
    {
    case eSEEK_BEGIN:
        m_pos = eMin(offset, m_data.size());
        break;

    case eSEEK_CURRENT:
        m_pos = eMin(m_pos+offset, m_data.size());
        break;

    case eSEEK_END:
        m_pos = eMax(0, (eInt)m_data.size()-(eInt)offset);
        break;
    }

    return m_pos;
}

eU32 eFile::tell() const
{
    eASSERT(m_opened);
    return m_pos;
}

eU32 eFile::getSize() const
{
    eASSERT(m_opened);
    return m_data.size();
}

eByteArray eFile::readAll(const eChar *fileName, eBool *success)
{
    eByteArray buf;
    eFile f(fileName);
    const eBool res = f.open(eFOM_READ);
    f.readAll(buf);
    if (success)
        *success = res;
    return buf;
}

#endif