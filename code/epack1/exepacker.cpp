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

#include "exepacker.hpp"
#include "stub.hpp"

#include "../eshared/packing/packing.hpp"

eExePacker::eExePacker(const eByteArray &image) :
    m_inImage(image)
{
}

eBool eExePacker::pack()
{
    m_report = "";
    m_outImage.clear();

    if (!_readPe())
      return eFALSE;

    eMemCopy(&m_dosHeaderEx.header, "MZbraincntrl", 12);

    eU32 stubSize = (eU32)&eStubEnd-(eU32)&eStubBegin;
    eU32 alignedStubSize = eAlign(stubSize, m_peHeader.optHeader.fileAlignment);
    eByteArray stub(alignedStubSize);
    eMemSet(&stub[0], 0, alignedStubSize);
    eMemCopy(&stub[0], eStubBegin, stubSize);

    //stub.append(m_peSecsEx[0].data);
    eU32 startVa; // new
    eByteArray virtImg = _getVirtualImage(startVa);// new
    stub.append(virtImg);// new

    //eU32 virtImgSize = m_peSecsEx[0].data.size();
    eU32 virtImgSize = virtImg.size(); // new
    m_peSecsEx.clear(); // new

    PeSectionEx newSec;
    eMemSet(&newSec.header, 0, sizeof(newSec.header));
    eStrCopy(newSec.header.name, ".stub");
    newSec.header.dataSize = eAlign(stub.size(), m_peHeader.optHeader.fileAlignment);
    newSec.header.flags = ePESEC_INITED_DATA|ePESEC_MEM_EXECUTE|ePESEC_MEM_READ|ePESEC_MEM_WRITE;
    newSec.header.virtSize = stubSize;
    newSec.header.dataPtr = eAlign(m_peSecsEx.last().header.dataPtr+m_peSecsEx.last().header.dataSize, m_peHeader.optHeader.fileAlignment);
    newSec.header.virtAddr = eAlign(m_peSecsEx.last().header.virtAddr+m_peSecsEx.last().header.virtSize, m_peHeader.optHeader.sectionAlignment);

    m_peSecsEx[0].header.dataPtr = eAlign(sizeof(m_dosHeaderEx.header)+m_dosHeaderEx.stub.size()+sizeof(m_peHeader)+(m_peSecsEx.size()+1)*sizeof(ePeSection), 512);
    m_peSecsEx[0].header.flags = ePESEC_INITED_DATA|ePESEC_MEM_EXECUTE|ePESEC_MEM_READ|ePESEC_MEM_WRITE;
    m_peSecsEx[0].data.clear();
    m_peSecsEx[0].header.dataSize = 0;
    m_peSecsEx[0].header.dataPtr = 0;

    eStubData sd;
    sd.imageBase = m_peHeader.optHeader.imageBase;
    sd.orgEntryPoint = m_peHeader.optHeader.entryPoint;
    sd.orgImpTabAddr = m_peHeader.optHeader.dataDirs[ePEDIR_IMPORT].virtAddr;
    sd.virtImgBegin = newSec.header.virtAddr+newSec.header.dataSize;
    sd.virtImgSize = virtImgSize;
    sd.virtImgSecRva = m_peSecsEx[0].header.virtAddr;

    newSec.header.dataSize += virtImgSize;

    for (eU32 i=0; i<stub.size(); i++)
    {
      eU32 dw = *(eU32 *)&stub[i];
      if (dw == eSTUB_DATA_BEGIN_MAGIC)
      {
        eMemCopy((eStubData *)(&stub[i+4]), &sd, sizeof(sd));
        break;
      }
    }

    newSec.data = stub;
    m_peSecsEx.append(newSec);

    m_peHeader.optHeader.dataDirs[ePEDIR_IAT].virtAddr = m_peSecsEx[1].header.virtAddr+(eU32)iatBegin-(eU32)eStubBegin;
    m_peHeader.optHeader.dataDirs[ePEDIR_IAT].size = (eU32)iatEnd-(eU32)iatBegin;
    m_peHeader.optHeader.dataDirs[ePEDIR_IMPORT].virtAddr = m_peSecsEx[1].header.virtAddr+(eU32)impTabBegin-(eU32)eStubBegin;
    m_peHeader.optHeader.dataDirs[ePEDIR_IMPORT].size = (eU32)impTabEnd-(eU32)impTabBegin;

    m_peHeader.fileHeader.numSecs++;
    m_peHeader.optHeader.imageSize = newSec.header.virtAddr+newSec.header.virtSize;
    m_peHeader.optHeader.entryPoint = newSec.header.virtAddr;

    _writePe();
    return eTRUE;
}

const eByteArray & eExePacker::getPackedImage() const
{
    return m_outImage;
}

const eString & eExePacker::getReport() const
{
    return m_report;
}

ePtr eExePacker::_appendData(eConstPtr data, eU32 size)
{
    eASSERT(size > 0);

    const eU32 oldSize = m_outImage.size();
    const eU8 *bytes = (eU8 *)data;

    for (eU32 i=0; i<size; i++)
        m_outImage.append(bytes[i]);

    return &m_outImage[oldSize];
}

eByteArray eExePacker::_getVirtualImage(eU32 &startVa) const
{
    // get start and end VAs to calculate image size
    startVa = m_peSecsEx[0].header.virtAddr;
    eU32 endVa = startVa+m_peSecsEx[0].header.virtSize;

    for (eU32 i=1; i<m_peSecsEx.size(); i++)
    {
        const ePeSection &peh = m_peSecsEx[i].header;
        startVa = eMin(startVa, peh.virtAddr);
        endVa = eMax(endVa, peh.virtAddr+peh.virtSize);
    }

    const eU32 imageSize = endVa-startVa+1;

    // create virtual image
    eByteArray image(imageSize);
    eMemSet(&image[0], 0, imageSize);

    for (eU32 i=0; i<m_peSecsEx.size(); i++)
    {
        const ePeSection &sec = m_peSecsEx[i].header;
        if (sec.dataPtr && sec.dataSize)
        {
            eASSERT(sec.virtAddr-startVa+sec.virtSize <= image.size());
            eMemCopy(&image[sec.virtAddr-startVa], &m_inImage[sec.dataPtr], sec.virtSize);
        }
    }

    return image;
}

eBool eExePacker::_readPe()
{
    const ePeDosHeader *dh = (ePeDosHeader *)&m_inImage[0];
    const ePeHeader *ph = (ePeHeader *)&m_inImage[dh->peHeaderAddr];

    if (!eMemEqual(dh->magic, "MZ", 2))
    {
        m_report = "File is not a valid executable!";
        return eFALSE;
    }

    if (!eMemEqual(ph->magic, "PE\x00\x00", 4))
    {
        m_report = "File is not a valid portable executable!";
        return eFALSE;
    }

    if (ph->optHeader.dataDirs[ePEDIR_EXPORTS].size || ph->optHeader.dataDirs[ePEDIR_TLS].size)
    {
        m_report = "File has exports or uses unsupported features like TLS!";
        return eFALSE;
    }

    eMemCopy(&m_dosHeaderEx.header, dh, sizeof(m_dosHeaderEx.header));
    eMemCopy(&m_peHeader, ph, sizeof(m_peHeader));

    m_dosHeaderEx.stubSize = m_dosHeaderEx.header.peHeaderAddr-sizeof(ePeDosHeader);
    m_dosHeaderEx.stub.resize(m_dosHeaderEx.stubSize);
    eMemCopy(&m_dosHeaderEx.stub[0], &m_inImage[sizeof(m_dosHeaderEx.header)], m_dosHeaderEx.stubSize);

    for (eU32 i=0; i<m_peHeader.fileHeader.numSecs; i++)
    {
        PeSectionEx &sec = m_peSecsEx.append();
        sec.header = *(ePeSection *)(((eU8 *)&ph->optHeader)+ph->fileHeader.optHeaderSize+i*sizeof(ePeSection));
        sec.data.resize(sec.header.dataSize);
        eMemCopy(&sec.data[0], &m_inImage[sec.header.dataPtr], sec.header.dataSize);
    }

    eU32 ii = ph->optHeader.dataDirs[ePEDIR_IMPORT].virtAddr-m_peSecsEx[0].header.virtAddr+m_peSecsEx[0].header.dataPtr;
    ePeImportDesc *importDesc = (ePeImportDesc *)&m_inImage[ii];

    return eTRUE;
}

void eExePacker::_writePe()
{
  _appendData(&m_dosHeaderEx.header, sizeof(m_dosHeaderEx.header));
  _appendData(&m_dosHeaderEx.stub[0], m_dosHeaderEx.stub.size());
  _appendData(&m_peHeader, sizeof(m_peHeader));

  // writing section headers
  for (eU32 i=0; i<m_peSecsEx.size(); i++)
    _appendData(&m_peSecsEx[i].header, sizeof(m_peSecsEx[i].header));

  // write sections' data
  for (eU32 i=0; i<m_peSecsEx.size(); i++)
  {
    eU32 cnt = eAbs((eInt)m_outImage.size()-(eInt)eAlign(m_outImage.size(), m_peHeader.optHeader.fileAlignment));//m_peSecsEx[i].data.size()
    eU8 nullVal = 0x00; // do padding
    for (eU32 j=0; j<cnt; j++)
      _appendData(&nullVal, 1);

    if (!m_peSecsEx[i].data.isEmpty())
        _appendData(&m_peSecsEx[i].data[0], m_peSecsEx[i].data.size());
  }
}