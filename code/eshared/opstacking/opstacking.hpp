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

#ifndef OP_STACKING_HPP
#define OP_STACKING_HPP

// defines that specify which operators
// have to be linked in and which not
#ifdef ePLAYER
    #include "../../eplayer4/production.hpp"
#endif

#include "script.hpp"
#include "parameter.hpp"
#include "ioperator.hpp"
#include "demoscript.hpp"
#include "opmacros.hpp"
#include "bitmapops.hpp"
#include "meshops.hpp"
#include "modelops.hpp"
#include "sequencerops.hpp"
#include "effectops.hpp"
#include "miscops.hpp"
#include "oppage.hpp"
#include "demodata.hpp"

class eOpStacking
{
public:
    static void shutdown()
    {
        eDemoData::free();
    }
};

#endif // OP_STACKING_HPP