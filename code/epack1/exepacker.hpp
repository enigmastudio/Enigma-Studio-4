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

#ifndef EXE_PACKER_HPP
#define EXE_PACKER_HPP

#include "../eshared/system/system.hpp"
#include "pestructs.hpp"

class eExePacker
{
public:
    eExePacker(const eByteArray &image);

    eBool               pack();
    const eByteArray &  getPackedImage() const;
    const eString &     getReport() const;

private:
    ePtr                _appendData(eConstPtr data, eU32 size);
    eByteArray          _getVirtualImage(eU32 &startVa) const;
    eBool               _readPe();
    void                _writePe();

private:
    struct PeSectionEx
    {
        ePeSection      header;
        eByteArray      data;
    };

    struct PeDosHeaderEx
    {
      ePeDosHeader      header;
      eU32              stubSize;
      eByteArray        stub;
    };

private:
    eByteArray          m_outImage;
    const eByteArray &  m_inImage;
    eString             m_report;

    PeDosHeaderEx       m_dosHeaderEx;
    ePeHeader           m_peHeader;
    eArray<PeSectionEx> m_peSecsEx;
};

#endif