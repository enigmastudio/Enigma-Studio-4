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

#include <QtWidgets/QApplication>
#include <QtCore/QProcess>
#include <QtCore/QtPlugin>
#include <QtCore/QFile>
#include <QtCore/QDir>

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

#include "gui/mainwnd.hpp"
#include "../configinfo.hpp"
#include "../eshared/eshared.hpp"

static void initApplication(eBool disableStyles)
{
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setApplicationName("Enigma Studio");
    QApplication::setApplicationVersion(eENIGMA4_VERSION);
    QApplication::setOrganizationName("Brain Control");
    QApplication::setOrganizationDomain("http://www.braincontrol.org");

    if (disableStyles)
        return;

#ifdef eRELEASE
    QFile cssFile(":/style/estudio4.css");
#else
    QFile cssFile(":/style/estudio4_debug.css");
#endif

    if (cssFile.open(QFile::ReadOnly))
        qApp->setStyleSheet(QString(cssFile.readAll()));
}

eInt main(eInt argc, eChar **argv)
{
    eSimdSetArithmeticFlags(eSAF_RTN|eSAF_FTZ);
    eLeakDetectorStart();
    ePROFILER_ADD_THIS_THREAD("Main thread");
	qputenv("QT_HASH_SEED","303"); // avoid salting and with that XML randomization

    // sets the current directory to Enigma Studio's
    // executable file path as we look for the shaders
    // relatively to that directory, not the project's
    // directory as set by the IDE
    QApplication app(argc, argv);
    QDir::setCurrent(app.applicationDirPath());
    eBool disableStyles = eFALSE;
    QString projectFile = "";
    const QStringList &args = app.arguments();

    for (eInt i=1; i<args.size(); i++)
    {
        if (args[i] == "--nostyle")
            disableStyles = eTRUE;
        else if (args[i] == "--novsync")
            eEngine::forceNoVSync(eTRUE);
        else
            projectFile == args[i];
    }

    initApplication(disableStyles);
    eMainWnd mainWnd(projectFile);
    app.setActiveWindow(&mainWnd);
    mainWnd.show();
    return app.exec();
}