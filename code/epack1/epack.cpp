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

#include <iostream>

#include "exepacker.hpp"
#include "../eshared/packing/packing.hpp"

using namespace std;

const eChar VERSION[] = "1.0a";

static void testAlgos()
{
    eByteArray src, dataBwt, dataMtf, dataRle;
    eBurrowsWheeler bwt;
    eMoveToFront mtf;
    eTextRle rle;

    src = eFile::readAll("shaders.txt");

    bwt.pack(src, dataRle);
    //mtf.pack(dataBwt, dataMtf);
    //rle.pack(dataBwt, dataRle);

    eFile f("shaders_packed");
    f.open(eFOM_WRITE);
    f.write(dataRle);
    f.close();

	eString hex = "unsigned char data[] = {\r\n    ";

    for (eU32 i=0; i<dataRle.size(); i++)
    {
        hex += eIntToStr(dataRle[i]);
        hex += ",";

        if ((i+1)%16 == 0)
            hex += "\r\n    ";
    }

    hex += "\r\n};";

    eFile ff("shaders_packed.hpp");
    ff.open(eFOM_WRITE);
    ff.write(&hex[0], hex.length());
    ff.close();
}

eInt main(eInt argc, eChar *argv[])
{
    eLeakDetectorStart();
    testAlgos();

    cout << "-----------------------------------------------------------" << endl;
    cout << "Enigma Pack " << VERSION << " - Brain Control Executable Compressor" << endl;
    cout << "Copyright (c) 2012 by Brain Control, all rights reserved." << endl;
    cout << "developed by David 'hunta' Geier" << endl;
    cout << "-----------------------------------------------------------" << endl;

    if (argc < 3)
    {
        cout << "Usage: epack1.exe <input file> <output file>" << endl;
        return 0;
    }

    eFile fin(argv[1]);
    eFile fout(argv[2]);

    if (!fin.open(eFOM_READ))
    {
        cout << "Could not open input file '" << argv[1] << "'!" << endl;
        return -1;
    }

    if (!fout.open(eFOM_WRITE))
    {
        cout << "Could not open output file '" << argv[2] << "'!" << endl;
        return -1;
    }

    eByteArray image;
    fin.readAll(image);

    eExePacker ep(image);
    eInt res = -1;

    if (ep.pack())
    {       
        fout.clear();
        fout.write(ep.getPackedImage());
        res = 0;
    }

    cout << ep.getReport() << endl;
    return res;
}