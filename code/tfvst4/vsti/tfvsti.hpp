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

#ifndef TFVSTI_HPP
#define TFVSTI_HPP

#define TF_VSTI_NUM_PROGRAMS  128
#define TF_VSTI_NUM_OUTPUTS   2

#ifdef TF_LOGGING
#define LOG(x) FILE *out = fopen("logfile.txt", "ab"); \
	fprintf(out, "%s", x); \
	fclose(out);
#else
#define LOG(x)
#endif

#include <QtCore/QFile>
#include <QtCore/QString>

#define NOMINMAX
#include <windows.h>

#include "../qt_winmigrate/qmfcapp.h"
#include "../qt_winmigrate/qwinwidget.h"

#include "../vstsdk/audioeffectx.h"

#include "../../eshared/system/system.hpp"
#include "../../eshared/math/math.hpp"
#include "../../eshared/synth/tf4.hpp"

#include "tfdial.hpp"
#include "tfwindow.hpp"
#include "tfeditor.hpp"
#include "tfrecorder.hpp"
#include "tfsynthprogram.hpp"
#include "tfvstsynth.hpp"
#include "tffreqview.hpp"
#include "tfmanage.hpp"
#include "tfhelpers.hpp"

#endif