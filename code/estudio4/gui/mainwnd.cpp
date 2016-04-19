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

#include <QtCore/QtAlgorithms>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDateTime>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QShortcut>

#include <QtXml/QDomDocument>
#include <QtGui/QCloseEvent>

#include "mainwnd.hpp"

#include "../../configinfo.hpp"

const QString eMainWnd::PROJECT_FILTER = "Enigma Studio 4 projects (*.e4prj)";
const QString eMainWnd::SCRIPT_FILTER = "Enigma Studio 4 script (*.e4scr)";
const QString eMainWnd::PROJECT_EXT = "e4prj";
const QString eMainWnd::SCRIPT_EXT = "e4scr";
const QString eMainWnd::EDITOR_CAPTION = QString("Enigma Studio ")+eENIGMA4_VERSION;
const QString eMainWnd::BACKUP_FILENAME = "backup.e4prj";

eMainWnd::eMainWnd(const QString &projectFile)
{
    setupUi(this);
    eSetLogHandler(_logHandler, this);
    m_pathView->setEditWidgetsParent(m_pathEditWidgetsCont->widget()); // set view line edits for editing path are created on

    _readSettings();
    _setupViewMenu();
    _setupStatusBar();
    _makeConnections();
    _createRecentFileActs();
    _setCurrentFile("");
    _loadHelp();

	//setGeometry(10, 30, 2540, 1200);   // fix for my window hanging issue (chris)

    // shortcut for play/pause button
    QAction *act = new QAction(this);
    act->setShortcut(QKeySequence("Space"));
    act->setShortcutContext(Qt::WindowShortcut);
    connect(act, SIGNAL(triggered()), m_btnDemoPlayPause, SLOT(click()));
    addAction(act);

    // shortcut for reloading Qt style sheet
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    shortcut->setContext(Qt::ApplicationShortcut);
    connect(shortcut, SIGNAL(activated()), this, SLOT(_onReloadStylesheet()));

    // group actions from settings menu
    QActionGroup *ac = new QActionGroup(this);
    ac->addAction(m_actShadowQualiLow);
    ac->addAction(m_actShadowQualiMedium);
    ac->addAction(m_actShadowQualiHigh);

    // initialize profiler tree-view (make first
    // column stretching)
    m_profilerTree->header()->setStretchLastSection(false);
    m_profilerTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    for (eInt i=0; i<m_profilerTree->columnCount(); i++)
        m_profilerTree->setItemDelegateForColumn(i, &m_profItemDel);
    for (eInt i=1; i<m_profilerTree->columnCount(); i++)
        m_profilerTree->header()->setSectionResizeMode(i, QHeaderView::Fixed);
    for (eInt i=0; i<m_profilerTree->columnCount(); i++)
        m_profilerTree->resizeColumnToContents(i);

    // set section resizing modes of other tree-views
    m_storedOpsTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_storedOpsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pageTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pageTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    // launch timers for updating statusbar and backuping
    m_profGraphTimerId = startTimer(200);
    m_statusBarTimerId = startTimer(200);
    m_backupTimerId = startTimer(AUTO_BACKUP_TIME_MS);

    if (projectFile.length() > 0)
        _loadFromXml(projectFile);
    else
        _loadRecentProject();
}

eMainWnd::~eMainWnd()
{
    killTimer(m_backupTimerId);
    killTimer(m_statusBarTimerId);
    killTimer(m_profGraphTimerId);

    _clearProject();
    _writeSettings();
}

void eMainWnd::_clearProject(eBool deleteBackup)
{
    // unset data from views
    eGuiOperator::setViewingOp(eNOID);
    eGuiOperator::setEditingOp(eNOID);
    m_paramView->setOperator(nullptr);
    m_renderView->setViewOp(nullptr);
    m_pathView->setViewOp(nullptr);
    m_pathView->setPathOp(nullptr);
    m_renderView->setEditOp(nullptr);
    m_demoSeqView->clear();
    m_pageTree->clear();

    // free data
    Q_FOREACH(eGuiOpPage *guiOpPage, m_guiOpPages)
        eDelete(guiOpPage);

    m_guiOpPages.clear();

    if (deleteBackup)
        _deleteBackup();
}

// add actions to show/hide dockable widgets
void eMainWnd::_setupViewMenu()
{
    m_viewMenu->addAction(m_renderDock->toggleViewAction());
    m_viewMenu->addAction(m_paramDock->toggleViewAction());
    m_viewMenu->addAction(m_profilerDock->toggleViewAction());
    m_viewMenu->addAction(m_logDock->toggleViewAction());

    m_renderDock->toggleViewAction()->setShortcut(QKeySequence("Alt+F1"));
    m_paramDock->toggleViewAction()->setShortcut(QKeySequence("Alt+F2"));
    m_profilerDock->toggleViewAction()->setShortcut(QKeySequence("Alt+F3"));
    m_logDock->toggleViewAction()->setShortcut(QKeySequence("Alt+F4"));
}

void eMainWnd::_loadHelp()
{
    QFile helpFile(":/html/help.html");
    if (helpFile.open(QFile::ReadOnly))
        m_txtHelp->setHtml(QString(helpFile.readAll()));
}

void eMainWnd::_setupStatusBar()
{
    static const eInt stretchs[SBP_COUNT] = {4, 8, 5, 3};

    for (eU32 i=0; i<SBP_COUNT; i++)
        statusBar()->addPermanentWidget(&m_sbLabels[i], stretchs[i]);
}

void eMainWnd::_makeConnections()
{
    connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(_onAbout()));
    connect(m_fileNewAct, SIGNAL(triggered()), this, SLOT(_onFileNew()));
    connect(m_fileOpenAct, SIGNAL(triggered()), this, SLOT(_onFileOpen()));
    connect(m_fileSaveAct, SIGNAL(triggered()), this, SLOT(_onFileSave()));
    connect(m_fileSaveAsAct, SIGNAL(triggered()), this, SLOT(_onFileSaveAs()));
    connect(m_fileExitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(m_showEngineStatsAct, SIGNAL(triggered()), this, SLOT(_onShowEngineStats()));
    connect(m_showOpStatsAct, SIGNAL(triggered()), this, SLOT(_onShowOpStats()));
    connect(m_fullScreenAct, SIGNAL(triggered()), this, SLOT(_onToggleAppFullscreen()));
    connect(m_fileExportAct, SIGNAL(triggered()), this, SLOT(_onFileExport()));

    connect(m_actShadowQualiLow, SIGNAL(triggered()), this, SLOT(_onSettingsShadowQualityLow()));
    connect(m_actShadowQualiMedium, SIGNAL(triggered()), this, SLOT(_onSettingsShadowQualityMedium()));
    connect(m_actShadowQualiHigh, SIGNAL(triggered()), this, SLOT(_onSettingsShadowQualityHigh()));

    connect(m_pageTree, SIGNAL(onPageAdd(eOperatorPage *&)), this, SLOT(_onPageAdd(eOperatorPage *&)));
    connect(m_pageTree, SIGNAL(onPageRemove(eOperatorPage *)), this, SLOT(_onPageRemove(eOperatorPage *)));
    connect(m_pageTree, SIGNAL(onPageSwitch(eOperatorPage *)), this, SLOT(_onPageSwitch(eOperatorPage *)));
    connect(m_pageTree, SIGNAL(onPageRenamed(eOperatorPage *)), this, SLOT(_onPageRenamed(eOperatorPage *)));

    connect(m_pageView, SIGNAL(onOperatorShow(eIOperator *)), this, SLOT(_onOperatorShow(eIOperator *)));
    connect(m_pageView, SIGNAL(onOperatorAdded(eIOperator *)), this, SLOT(_onOperatorAdded(eIOperator *)));
    connect(m_pageView, SIGNAL(onOperatorRemoved(eIOperator *)), this, SLOT(_onOperatorRemoved(eIOperator *)));
    connect(m_pageView, SIGNAL(onOperatorSelected(eIOperator *)), this, SLOT(_onOperatorSelected(eIOperator *)));
    connect(m_pageView, SIGNAL(onPathOperatorEdit(eIOperator *)), this, SLOT(_onPathOperatorEdit(eIOperator *)));
    connect(m_pageView, SIGNAL(onDemoOperatorEdit(eIOperator *)), this, SLOT(_onDemoOperatorEdit(eIOperator *)));
    connect(m_pageView, SIGNAL(onGotoOperator(eID)), this, SLOT(_onOperatorGoto(eID)));
    connect(m_pathView, SIGNAL(onFinishedEditing()), this, SLOT(_onSwitchToPageView()));
    connect(m_demoSeqView, SIGNAL(onFinishedEditing()), this, SLOT(_onSwitchToPageView()));
    connect(m_demoSeqView, SIGNAL(onOperatorSelected(eIOperator *)), this, SLOT(_onOperatorSelected(eIOperator *)));
    connect(m_paramView, SIGNAL(onOperatorChanged(eIOperator *, const eParameter *)), this, SLOT(_onOperatorChanged(eIOperator *, const eParameter *)));
    connect(m_paramView, SIGNAL(onOperatorChanged(eIOperator *, const eParameter *)), m_demoSeqView, SLOT(onOperatorChanged()));
    connect(m_paramView, SIGNAL(onOperatorGoto(eID)), this, SLOT(_onOperatorGoto(eID)));
    connect(m_renderView, SIGNAL(onToggleFullscreen()), this, SLOT(_onToggleViewFullscreen()));

    connect(m_timelineView, SIGNAL(onTimeChanged(eF32)), this, SLOT(_onTimelineTimeChanged(eF32)));
    connect(m_timeEdit, SIGNAL(textEdited(const QString &)), this, SLOT(_onTimeEditEdited(const QString &)));
    connect(m_pathView, SIGNAL(onTimeChanged(eF32)), this, SLOT(_onPathViewTimeChanged(eF32)));
    connect(m_demoSeqView, SIGNAL(onTimeChanged(eF32)), this, SLOT(_onDemoSeqTimeChanged(eF32)));

    connect(m_storedOpsTree, SIGNAL(itemSelectionChanged()), this, SLOT(_onStoredOpTreeSelectionChanged()));
    connect(m_btnDemoPlayPause, SIGNAL(clicked()), m_timelineView, SLOT(togglePlayPause()));
    connect(m_btnDemoSkipForward, SIGNAL(clicked()), m_timelineView, SLOT(skipForward()));
    connect(m_btnDemoSkipBackward, SIGNAL(clicked()), m_timelineView, SLOT(skipBackward()));
    connect(m_cbDemoLoop, SIGNAL(toggled(bool)), m_timelineView, SLOT(toggleLooping()));
}

void eMainWnd::_addDefaultPage()
{
    eGuiOpPage *guiOpPage = new eGuiOpPage("Start page");
    eOperatorPage *opPage = guiOpPage->getPage();
    m_guiOpPages.insert(opPage, guiOpPage);
    m_pageTree->addPage(opPage);
    m_pageTree->selectPage(opPage);
}

void eMainWnd::_createRecentFileActs()
{
    QMenu *fileMenu = menuBar()->actions().at(0)->menu();

    m_recSepAct = fileMenu->insertSeparator(fileMenu->actions().at(fileMenu->actions().size()-2));
    m_recSepAct->setVisible(false);

    for (eU32 i=0; i<MAX_RECENT_FILES; i++)
    {
        m_recentActs[i] = new QAction(this);
        m_recentActs[i]->setVisible(false);
        connect(m_recentActs[i], SIGNAL(triggered()), this, SLOT(_onFileOpenRecent()));
        fileMenu->insertAction(fileMenu->actions().at(fileMenu->actions().size()-2), m_recentActs[i]);
    }

    _updateRecentFileActs();
}

void eMainWnd::_updateRecentFileActs()
{
    QSettings settings;

    settings.beginGroup("Main window");
    const QStringList files = settings.value("Recent files").toStringList();
    const eInt fileCount = eMin(files.size(), (eInt)MAX_RECENT_FILES);

    for (eInt i=0; i<fileCount; i++)
    {
        const QString fileName = QFileInfo(files[i]).fileName();
        const QString text = tr("&%1 %2").arg(i+1).arg(fileName);

        m_recentActs[i]->setText(text);
        m_recentActs[i]->setData(files[i]);
        m_recentActs[i]->setVisible(true);
    }

    for (eInt j=fileCount; j<MAX_RECENT_FILES; j++)
    {
        m_recentActs[j]->setVisible(false);
    }

    m_recSepAct->setVisible(fileCount > 0);
    settings.endGroup();
}

void eMainWnd::_updateStatusBar()
{
    // update project information
    static QString buf; // reduce memory allocations

    const eGuiOpPage *guiOpPage = (eGuiOpPage *)m_pageView->scene();
    const ePoint &insertAt = (guiOpPage ? guiOpPage->getInsertAt() : ePoint(0, 0));
    buf = "Position: ";
    buf += eIntToStr(insertAt.x);
    buf += "/";
    buf += eIntToStr(insertAt.y);
    buf += ", Pages: ";
    buf += eIntToStr(eDemoData::getPageCount());
    buf += ", Operators: ";
    buf += eIntToStr(eDemoData::getTotalOpCount());
    buf += ", Stack update: ";
    buf += eIntToStr(m_renderView->getLastCalcMs());
    buf += " ms";
    _setStatusText(SBP_STATS_OPSTACKING, buf);

    // update memory statistics
    buf = "RAM: ";
    buf += _byteSizeToStr(eGetAllocatedMemory());
    buf += " (";
    buf += _byteSizeToStr(eGetTotalVirtualMemory());
    buf += "), VRAM: ";
    buf += _byteSizeToStr(eGfx->getEngineStats().usedGpuMem);
    buf += " (";
    buf += _byteSizeToStr(eGfx->getEngineStats().availGpuMem);
    buf += " )";
    _setStatusText(SBP_STATS_MEMORY, buf);

    // update showed operator infos
    const eIOperator *op = m_renderView->getViewOp();
    const eOpClass opc = (op ? op->getResultClass() : eOC_BMP);

    buf = "-";

    if (op)
    {
        if (opc == eOC_BMP)
        {
            const eIBitmapOp::Result &res = ((eIBitmapOp *)op)->getResult();
            buf  = QString("Size: ")+eIntToStr(res.width)+"x"+eIntToStr(res.height)+", ";
            buf += "Zoom: ";
            buf += QString::number(m_renderView->getZoomFactor())+"x";
        }
        else if (opc == eOC_R2T)
        {
            const eIRenderToTextureOp::Result &res = ((eIRenderToTextureOp *)op)->getResult();
            buf  = QString("Size: ")+eIntToStr(res.renderTarget->width)+"x"+eIntToStr(res.renderTarget->height)+", ";
            buf += "Zoom: ";
            buf += QString::number(m_renderView->getZoomFactor())+"x";
        }
        else if (opc == eOC_MESH)
        {
            const eIMeshOp::Result &res = ((eIMeshOp *)op)->getResult();
            const eVector3 &camPos = m_renderView->getCameraPos();
            buf.sprintf("Camera: %.2f/%.2f/%.2f", camPos.x, camPos.y, camPos.z);
            buf += ", Positions: ";
            buf += eIntToStr(res.mesh.getPositionCount());
            //buf += ", Normals: "; // only use for debugging
            //buf += eIntToStr(res.mesh.getNormalCount());
            //buf += ", Properties: ";
            //buf += eIntToStr(res.mesh.getPropertyCount());
            //buf += ", Wedges: ";
            //buf += eIntToStr(res.mesh.getWedgeCount());
            buf += ", Faces: ";
            buf += eIntToStr(res.mesh.getFaceCount());
        }
        else if (opc == eOC_MODEL)
        {
            const eVector3 &camPos = m_renderView->getCameraPos();
            buf.sprintf("Camera: %.2f/%.2f/%.2f", camPos.x, camPos.y, camPos.z);
            buf += ", Objects: ";
            buf += eIntToStr(((eIModelOp *)op)->getResult().sceneData.getRenderableTotal());
        }
        else if (opc == eOC_FX)
        {
            const QString camModes[] = {"fixed", "free", "link"};
            buf = "Camera mode: "+camModes[m_renderView->getFxOpCameraMode()];
        }
    }

    _setStatusText(SBP_STATS_CUROP, buf);

    // set render statistics
    const eRenderStats &stats = eGfx->getRenderStats();
    buf  = "FPS: ";
    buf += QString::number(1000.0f/m_renderView->getAvgFrameMs(), 'f', 2);
    buf += " (";
    buf += QString::number(m_renderView->getAvgFrameMs(), 'f', 2);
    buf += " ms), Batches: ";
    buf += eIntToStr(stats.batches);
    buf += ", Instances: ";
    buf += eIntToStr(stats.instances);
    buf += ", Triangles: ";
    buf += eIntToStr(stats.triangles);
    buf += ", Lines: ";
    buf += eIntToStr(stats.lines);
    buf += ", Vertices: ";
    buf += eIntToStr(stats.vertices);

    _setStatusText(SBP_STATS_3DENGINE, buf);
}

void eMainWnd::_updateProfilerTree()
{
    QList<QTreeWidgetItem *> items;
    QStringList strs;
    const eChar *threadName = nullptr;
    eF32 totalFrameMs;
    eU32 numZones;

    for (eU32 i=0; i<eProfiler::getThreadCount(); i++)
    {
        const eProfilerZone *zones = eProfiler::getThreadZones(i, numZones, totalFrameMs, threadName);
        items.append(new QTreeWidgetItem(QStringList() << threadName));

        for (eU32 j=0; j<numZones; j++)
        {
            const eProfilerZone &pz = zones[j];
            if (!pz.getCallCount())
                continue;
          
            strs.clear();
            strs << (const eChar *)pz.getName();
            strs << QString::number(pz.getSelfTimeMs(), 'f', 1);
            strs << QString::number(pz.getHierTimeMs(), 'f', 1);
            strs << eIntToStr(pz.getCallCount());
            strs << QString::number(pz.getStatistics().stdDev, 'f', 1);
            strs << QString::number(pz.getStatistics().mean, 'f', 1);
            strs << QString::number(pz.getStatistics().min, 'f', 1);
            strs << QString::number(pz.getStatistics().max, 'f', 1);

            QTreeWidgetItem *item = new QTreeWidgetItem(items.last(), strs);
            item->setData(0, Qt::UserRole, pz.getSelfTimeMs()/totalFrameMs);
            item->setData(0, Qt::UserRole+1, QColor(pz.getColor().toArgb()));
            item->setData(0, Qt::UserRole+2, m_profilerTree->viewport()->width());
        }
    }

    m_profilerTree->clear();
    m_profilerTree->addTopLevelItems(items);
    m_profilerTree->expandAll();
}

void eMainWnd::_setStatusText(StatusBarPane pane, const QString &text)
{
    eASSERT(pane >= 0 && pane < SBP_COUNT);
    m_sbLabels[pane].setText(text);
}

void eMainWnd::_deleteBackup()
{
    const QFileInfo fileInfo(m_prjFilePath);
    const QString backupFile = fileInfo.path()+"/"+fileInfo.baseName()+".backup.e4prj";
 
    if (QFile::exists(backupFile))
        QFile::remove(backupFile);
}

void eMainWnd::_saveBackup()
{
#ifndef eDEBUG
    QString fileName = BACKUP_FILENAME;

    // if there is a given project file name,
    // insert '.backup' before file extension
    if (m_prjFilePath != "")
    {
        const eInt extSepIndex = m_prjFilePath.lastIndexOf(".");
        eASSERT(extSepIndex != -1);
        fileName = m_prjFilePath;
        fileName.insert(extSepIndex, ".backup");
    }

    _saveToXml(fileName, eTRUE);
#endif
}

eBool eMainWnd::_askForSaving()
{
    if (isWindowModified())
    {
        switch (QMessageBox::warning(this, "Warning",
                                     "The document has been modified.\n"
                                     "Do you want to save your changes?",
                                     QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,
                                     QMessageBox::Discard))
        {
        case QMessageBox::Save:
            return _onFileSave();
        case QMessageBox::Cancel:
                return eFALSE;
        case QMessageBox::Discard:
                return eTRUE;
        }
    }

    return eTRUE;
}

void eMainWnd::_setCurrentFile(const QString &filePath)
{
    QString filePathFinal = filePath;

    if (filePathFinal != "")
    {
        QFile file(filePathFinal+".orig");
        if (file.open(QIODevice::ReadOnly))
        {
            filePathFinal = QString(file.readLine());
            file.close();
        }
    }
    
    m_prjFilePath = filePathFinal;
    setWindowModified(false);

    // update recent files list in settings
    if (filePathFinal != "")
    {
        QSettings settings;

        settings.beginGroup("Main window");
        QStringList files = settings.value("Recent files").toStringList();
        files.removeAll(filePathFinal);
        files.prepend(filePathFinal);

        while (files.size() > MAX_RECENT_FILES)
            files.removeLast();

        settings.setValue("Recent files", files);
        settings.endGroup();

        setWindowTitle(EDITOR_CAPTION+" - ["+filePathFinal+"[*]]");
        _updateRecentFileActs();
        _writeSettings(); // store last project file name that it's save in case of a crash
    }
    else
        setWindowTitle(EDITOR_CAPTION+" - [untitled.e4prj[*]]");
}

eBool eMainWnd::_loadFromXml(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString backupFile = fileInfo.path() + "/" + fileInfo.baseName() + ".backup.e4prj";
    QFileInfo backupFileInfo(backupFile);
    QDateTime backupModified = backupFileInfo.lastModified();
    QDateTime fileModified = fileInfo.lastModified();
    eBool backupIsNewer = fileModified < backupModified;

    if (QFile::exists(backupFile) && backupIsNewer)
    {
        if (QMessageBox::question(this, "Backup found", "There is a backup for this project. Would you like to load that?") == QMessageBox::Yes)
        {
            if (_loadFromXml(backupFile))
            {
                _saveToXml(filePath);
                return eTRUE;
            }

            return eFALSE;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Couldn't open project file!");
        return eFALSE;
    }

    // load XML data and check for correct version
    QDomDocument xml;
    xml.setContent(&file);
    file.close();

    if (xml.firstChildElement("enigma").attribute("version") != eENIGMA4_VERSION)
    {
        if (QMessageBox::warning(this,
                                 "Warning",
                                 "This project was created using another version of Enigma Studio.\n"\
                                 "Do you want to try opening this project anyway (could crash)?",
                                 QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
        {
            return eFALSE;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    _clearProject(eFALSE);

    // load pages and operators
    const QDomElement rootEl = xml.documentElement();
    QDomElement pagesEl = rootEl.firstChildElement("pages").firstChildElement("page");

    while (!pagesEl.isNull())
    {
        const eID pageId = pagesEl.attribute("id").toInt();
        eOperatorPage *opPage = eDemoData::addPage(pageId);
        eGuiOpPage *guiOpPage = new eGuiOpPage(opPage);

        m_guiOpPages.insert(opPage, guiOpPage);
        guiOpPage->loadFromXml(pagesEl);

        pagesEl = pagesEl.nextSiblingElement("page");
    }

    // load tree-view items and select first page
    m_pageTree->loadFromXml(rootEl);
    
    if (m_pageTree->topLevelItemCount() > 0)
        m_pageTree->topLevelItem(0)->setSelected(true);

    // finalize operator data
    eDemoData::compileScripts();
    eDemoData::connectPages();

    _setCurrentFile(filePath);
    QApplication::restoreOverrideCursor();
    setWindowModified(false);
    return eTRUE;
}

eBool eMainWnd::_saveToXml(const QString &filePath, eBool backup)
{
    // try to open file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return eFALSE;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // initialize XML stream
    QDomDocument xml;
    xml.appendChild(xml.createProcessingInstruction("xml", "version=\"1.0\""));
    QDomElement rootEl = xml.createElement("enigma");
    rootEl.setAttribute("version", eENIGMA4_VERSION);
    xml.appendChild(rootEl);

    // save pages with operators
    QDomElement pagesEl = xml.createElement("pages");
    rootEl.appendChild(pagesEl);

    QMap<QString, eGuiOpPage*> saveMap;
    Q_FOREACH(eGuiOpPage *guiOpPage, m_guiOpPages)
        saveMap[QString::number(guiOpPage->getPage()->getId())] = guiOpPage;

    Q_FOREACH(eGuiOpPage *guiOpPage, saveMap)
        guiOpPage->saveToXml(pagesEl);

    // write page tree-view data
    m_pageTree->saveToXml(rootEl);

    // write XML stream to text file
    QTextStream ts(&file);
    xml.save(ts, 2);
    file.close();

    if (!backup)
        _setCurrentFile(filePath);

    QApplication::restoreOverrideCursor();
    return eTRUE;
}

void eMainWnd::_loadRecentProject()
{
    if (QFile::exists(m_lastPrjPath) && _loadFromXml(m_lastPrjPath))
        return;
    else
    {
        const QString defaultFilePath = QApplication::applicationDirPath()+"/default.e4prj";

        if (QFile::exists(defaultFilePath))
        {
            _loadFromXml(defaultFilePath);
            return;
        }
    }

    _addDefaultPage();
}

void eMainWnd::_newProject(eBool loadDefaultPrj)
{
    _clearProject();

    if (loadDefaultPrj)
        _loadRecentProject();
    else
        _addDefaultPage();

    setWindowModified(false);
}

void eMainWnd::_writeSettings() const
{
    QSettings settings;
    settings.beginGroup("Main window");
    settings.setValue("Geometry", saveGeometry());
    settings.setValue("State", saveState());
    settings.setValue("Last project", m_prjFilePath);
    settings.endGroup();
}

void eMainWnd::_readSettings()
{
    QSettings settings;
    settings.beginGroup("Main window");
    restoreGeometry(settings.value("Geometry").toByteArray());
    restoreState(settings.value("State").toByteArray()); 
    m_lastPrjPath = settings.value("Last project", "").toString();
    settings.endGroup();
}

QString eMainWnd::_byteSizeToStr(eU32 numBytes) const
{
    if (numBytes > 1<<30)
        return QString::number((eF32)numBytes/(eF32)(1<<30), 'f', 2)+" GB";
    else if (numBytes > 1<<20)
        return QString::number((eF32)numBytes/(eF32)(1<<20), 'f', 2)+" MB";
    else if (numBytes > 1<<10)
        return QString::number((eF32)numBytes/(eF32)(1<<10), 'f', 2)+" KB";
    else
        return QString::number((eF32)numBytes, 'f', 2)+" Bytes";
}

void eMainWnd::_logHandler(const eChar *msg, ePtr param)
{
    eMainWnd *mainWnd = (eMainWnd *)param;
    mainWnd->m_logEdit->appendPlainText(msg);
    mainWnd->m_logDock->raise();

    // that's a very bad idea! don't process any
    // Qt events here. otherwise, timer events,
    // e.g. the one triggering the viewport repaint
    // are processed too. as the log handler might be
    // called from code regions that are as well called
    // from the timer handler very nasty bugs can arise.
    //
    // qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    qApp->flush();
}

void eMainWnd::_onReloadStylesheet()
{
#ifdef eDEBUG
    const QString cssFileName = "../code/estudio4/resources/estudio4.css";
#else
    const QString cssFileName = "../code/estudio4/resources/estudio4.css";
#endif

    QFile cssFile(QApplication::applicationDirPath()+"/"+cssFileName);
    if (cssFile.open(QFile::ReadOnly))
        qApp->setStyleSheet(QString(cssFile.readAll()));
}

void eMainWnd::_onAbout()
{
    static QString text = QString(
        "<html>"
          "<body>"
            "<b>Enigma Studio %1 (%2)</b><br>"
            "<br>"
            "Copyright &copy; 2003-2012 by Brain Control,<br>"
            "all rights reserved."
            "<br>"
            "<table width=""240"" height=""100%"">"
              "<tr>"
                "<td><i>Programming:</i></td>"
                "<td>&bull; David 'hunta' Geier</td>"
              "</tr>"
              "<tr>"
                "<td></td>"
                "<td>&bull; Chris 'payne' Loos</td>"
              "</tr>"
              "<tr>"
                "<td></td>"
                "<td>&bull; Martin 'pap' Raack</td>"
              "</tr>"
            "</table>"
            "<br>"
            "<br>"
            "Visit us under <a href=""http://www.braincontrol.org"">www.braincontrol.org</a>."
          "</body>"
        "</html>").arg(eENIGMA4_VERSION, QString(eENIGMA4_CONFIG));

    QMessageBox::about(this, "About", text);
}

void eMainWnd::_onFileNew()
{
    if (_askForSaving())
    {
        _newProject(eFALSE);
        _setCurrentFile("");
    }
}

void eMainWnd::_onFileOpen()
{
    if (_askForSaving())
    {
        const QString filePath = QFileDialog::getOpenFileName(this, "", "", PROJECT_FILTER);
        if (filePath != "")
            _loadFromXml(filePath);
    }
}

eBool eMainWnd::_onFileSave()
{
    if (m_prjFilePath == "")
        return _onFileSaveAs();

    return _saveToXml(m_prjFilePath);
}

eBool eMainWnd::_onFileSaveAs()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "", m_prjFilePath, PROJECT_FILTER);
    if (filePath == "")
        return eFALSE;
    
    return _saveToXml(filePath);
}

void eMainWnd::_onFileExport()
{
    // check if a demo operator is selected
    if (!m_pageView->scene() ||
        m_pageView->scene()->selectedItems().size() != 1 ||
        ((eGuiOperator *)m_pageView->scene()->selectedItems().at(0))->getOperator()->getMetaInfos().name != "Demo")
    {
        QMessageBox::information(this, "Information", "You have to select a demo operator first!");
        return;
    }

    // retrieve selected demo operator
    eIDemoOp *demoOp = (eIDemoOp *)((eGuiOperator *)m_pageView->scene()->selectedItems().at(0))->getOperator();
    if (demoOp->getError() != eOE_OK)
    {
        QMessageBox::information(this, "Information", "The stack of the selected demo operator is erroneous!");
        return;
    }

    // create script
    const QFileInfo fi(m_prjFilePath);
    const QString initDir = fi.path()+"/"+fi.baseName()+"."+SCRIPT_EXT;
    const QString filePath = QFileDialog::getSaveFileName(this, "", initDir, SCRIPT_FILTER);
    if (filePath == "")
        return;

    eDemoScript script;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    eDemoData::exportScript(script, demoOp);
    QApplication::restoreOverrideCursor();

    // store binary file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Couldn't create script file!");
        return;
    }

    const eByteArray &data = script.getFinalScript();
    file.write((const eChar *)&data[0], data.size());
    file.close();

    // store header file of used operators
    QFile opsFile(filePath+".ops.h");
    if (!opsFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Couldn't create header file of used operators!");
        return;
    }

    opsFile.write(script.getUsedOpNames());
	opsFile.write(script.getOpParamIfDefs());
    opsFile.close();

	// store sourcecodefile
    QFile opcodeFile(filePath+".opcode.cpp");
    if (!opcodeFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Couldn't create sourcecode file of used operators!");
        return;
    }

	eChar buffer[10000];
	for(eU32 i = 0; i < eDemoData::m_operatorSourceFiles.size(); i++) {
//		eWriteToLog(eString("Adding Source File: ") + eDemoData::m_operatorSourceFiles[i]);
		QString sourcePath(eDemoData::m_operatorSourceFiles[i]);
		QString destPath = filePath;
		sourcePath = sourcePath.replace("\\", "/").toLower();
		destPath = destPath.replace("\\", "/").toLower();

		eS32 shared = 0;
		while((shared < destPath.length()) && (shared < sourcePath.length()) && (destPath[shared] == sourcePath[shared])) {
			shared++;
		}
		eS32 endShared = sourcePath.length();
		while(endShared > 0) {
			endShared--;
			if(sourcePath[endShared] == '/')
				break;
		}

		QString prefix = "";
		eS32 testIdx = shared + 1;
		while(testIdx < destPath.length()) 
			if(destPath[testIdx++] == '/')
				prefix += "../";

		for(eS32 k = shared; k <= endShared; k++)
			prefix += QString(sourcePath[k]);
/*
		eWriteToLog(eString("SOURCE: ") + eString(sourcePath.toLocal8Bit().data()));
		eWriteToLog(eString("  DEST: ") + eString(destPath.toLocal8Bit().data()));
		eWriteToLog(eString("-> shared: ") + eString(eIntToStr(shared)) + " Prefix: " + eString(prefix.toLocal8Bit().data()));
*/
		QFile sourceFile(sourcePath);
		if (!sourceFile.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(this, "Error", QString() + "Couldn't read operator sourcecode file " + eDemoData::m_operatorSourceFiles[i]);
			return;
		}
		for(;;) {
			qint64 result = sourceFile.readLine(buffer, sizeof(buffer));
			if(result == -1)
				break;
			QString line(buffer);
			if(line.startsWith("#include \"")) {
				QString includePath = line.split('\"')[1];
				opcodeFile.write((QString("#include \"") + prefix + includePath + QString("\"\r\n")).toLocal8Bit().data());
			} else
				opcodeFile.write(buffer);
		}	
	}
	opcodeFile.write("\r\neCREATEOP;");
	opcodeFile.write("\r\neEXECOP;");
    opcodeFile.close();



    // store script as header file with hex array
	QString hex = "#define eDEMODATA_LENGTH " + QString::number(data.size(), 10) + "\r\n";
	hex += "const eU8 demoData[] = {\r\n    ";

    for (eU32 i=0; i<data.size(); i++)
    {
        hex += "0x";
        hex += QString::number(data[i], 16).rightJustified(2, '0');
        hex += ",";

        if ((i+1)%16 == 0)
            hex += "\r\n    ";
    }

    hex += "\r\n};";



    QFile hexFile(filePath + ".bin.h");
    if (!hexFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Couldn't create hex header file!");
        return;
    }

    hexFile.write(hex.toLocal8Bit());
    hexFile.close();
}

void eMainWnd::_onFileOpenRecent()
{
    const QAction *action = qobject_cast<QAction *>(sender());

    if (_askForSaving())
        _loadFromXml(action->data().toString());
}

void eMainWnd::_onPageAdd(eOperatorPage *&opPage)
{
    eGuiOpPage *guiOpPage = new eGuiOpPage("New page");
    opPage = guiOpPage->getPage();
    m_guiOpPages.insert(opPage, guiOpPage);
    m_pageTree->selectPage(opPage);
    setWindowModified(true);
}

void eMainWnd::_onPageRemove(eOperatorPage *opPage)
{
    if (m_guiOpPages.contains(opPage))
    {
        eGuiOpPage *guiOpPage = m_guiOpPages.value(opPage);
        eDelete(guiOpPage);
        m_guiOpPages.remove(opPage);
        setWindowModified(true);
    }
}

void eMainWnd::_onPageSwitch(eOperatorPage *opPage)
{
    if (!opPage)
    {
        m_storedOpsTree->clear();
        m_pageView->setScene(nullptr);
    }
    else
    {
        // has page really changed, or is it just a redundant call?
        eASSERT(m_guiOpPages.contains(opPage));
        eGuiOpPage *guiOpPage = m_guiOpPages.value(opPage);

        if (guiOpPage != (eGuiOpPage *)m_pageView->scene())
        {
            m_pageView->setScene(guiOpPage);
            m_pageView->scrollTo(guiOpPage->getViewPosition());
            m_pageView->scene()->invalidate();

            // fill list with stored operators of given page
            m_storedOpsTree->clear();

            for (eU32 i=0; i<opPage->getOperatorCount(); i++)
            {
                const eIOperator *op = opPage->getOperatorByIndex(i);
                const QString userName = op->getUserName();
                const QString strPos = QString("%1/%2").arg(op->getPosition().x).arg(op->getPosition().y);

                if (userName != "")
                {
                    QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << userName << strPos);
                    item->setData(0, Qt::UserRole, qVariantFromValue<eID>(op->getId()));
                    m_storedOpsTree->addTopLevelItem(item);
                }
            }
        }
    }
}

void eMainWnd::_onPageRenamed(eOperatorPage *opPage)
{
    setWindowModified(true);
}

void eMainWnd::_onOperatorShow(eIOperator *op)
{
    m_renderView->setViewOp(op);
    m_pathView->setViewOp(op);
}

void eMainWnd::_onOperatorAdded(eIOperator *op)
{
    setWindowModified(true);
    m_pageTree->updateOpCounts();
}

void eMainWnd::_onOperatorRemoved(eIOperator *op)
{
    // remove operator from views if set
    if (m_paramView->getOperator() == op)
        m_paramView->setOperator(nullptr);
    if (m_renderView->getViewOp() == op)
        m_renderView->setViewOp(nullptr);
    if (m_renderView->getEditOp() == op)
        m_renderView->setEditOp(nullptr);
    if (m_pathView->getPathOp() == op)
        m_pathView->setPathOp(nullptr);
    if (m_pathView->getViewOp() == op)
        m_pathView->setViewOp(nullptr);
    if (m_demoSeqView->getDemoOp() == op)
        m_demoSeqView->setDemoOp(nullptr);

    // remove operator from 'stored operator' list
    for (eInt i=0; i<m_storedOpsTree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = m_storedOpsTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toInt() == op->getId())
        {
            eDelete(item);
            break;
        }
    }

    setWindowModified(true);
    m_pageTree->updateOpCounts();
}

void eMainWnd::_onOperatorSelected(eIOperator *op)
{
    m_paramView->setOperator(op);
    m_renderView->setEditOp(op);
}

void eMainWnd::_onOperatorChanged(eIOperator *op, const eParameter *param)
{
    // parameter is null => name changed or hide
    // or bypass buttons clicked (repaint needed).
    // otherwise, if changed parameter is a link
    // repaint, too.
    if (!param || param->getType() == ePT_LINK)
        m_pageView->scene()->invalidate();

    // add operator to 'stored operator' list if
    // a user name was specified (parameter is null)
    if (!param)
    {
        eBool addOp = eTRUE;

        for (eInt i=0; i<m_storedOpsTree->topLevelItemCount(); i++)
        {
            QTreeWidgetItem *item = m_storedOpsTree->topLevelItem(i);
            if (item->data(0, Qt::UserRole).toInt() == op->getId())
            {
                if (op->getUserName() == "") // remove
                    eDelete(item);
                else // rename
                {
                    item->setText(0, QString(op->getUserName()));
                    item->setText(1, QString("%1/%2").arg(op->getPosition().x).arg(op->getPosition().y));
                }

                addOp = eFALSE;
                break;
            }
        }

        if (addOp)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << QString(op->getUserName()));
            item->setData(0, Qt::UserRole, qVariantFromValue(op->getId()));
            m_storedOpsTree->addTopLevelItem(item);
        }
    }

    // flag document as modified
    setWindowModified(true);
}

void eMainWnd::_onOperatorGoto(eID opId)
{
    eIOperator *op = eDemoData::findOperator(opId);
    if (!op)
        return;

    eOperatorPage *opPage = op->getOwnerPage();
    eASSERT(m_guiOpPages.contains(opPage));
    eGuiOpPage *guiOpPage = m_guiOpPages.value(opPage);
    eGuiOperator *guiOp = guiOpPage->getGuiOperator(opId);

    m_pageTree->selectPage(opPage); // single selection automatically removes old selection
    m_pageView->setScene(guiOpPage);
    m_pageView->scene()->invalidate();
    m_pageView->gotoOperator(guiOp);
}

void eMainWnd::_onPathOperatorEdit(eIOperator *op)
{
    m_pathView->setPathOp((eIPathOp *)op);
    m_tabWidget->setCurrentWidget(m_tabPathView);
}

void eMainWnd::_onDemoOperatorEdit(eIOperator *op)
{
    m_demoSeqView->setDemoOp((eIDemoOp *)op);
    m_tabWidget->setCurrentWidget(m_tabDemoSeqView);
}

void eMainWnd::_onSwitchToPageView()
{
    m_tabWidget->setCurrentWidget(m_tabPageView);
}

void eMainWnd::_onSettingsShadowQualityLow()
{
    eRenderer->setShadowQuality(eSQ_LOW);
}

void eMainWnd::_onSettingsShadowQualityMedium()
{
    eRenderer->setShadowQuality(eSQ_MEDIUM);
}

void eMainWnd::_onSettingsShadowQualityHigh()
{
    eRenderer->setShadowQuality(eSQ_HIGH);
}

void eMainWnd::_onShowEngineStats()
{
    const eEngineStats &stats = eGfx->getEngineStats();
    eWriteToLog("----------------------------------------------");
    eWriteToLog("Engine statistics");
    eWriteToLog("----------------------------------------------");
    eWriteToLog(eString("     2D textures:\t")+eIntToStr(stats.numTex2d));
    eWriteToLog(eString("     3D textures:\t")+eIntToStr(stats.numTex3d));
    eWriteToLog(eString("   Cube textures:\t")+eIntToStr(stats.numTexCube));
    eWriteToLog(eString("      Geometries:\t")+eIntToStr(stats.numGeos));
    eWriteToLog(eString("Geometry buffers:\t")+eIntToStr(stats.numGeoBuffs));
    eWriteToLog(eString("         Shaders:\t")+eIntToStr(stats.numShaders));
    eWriteToLog(eString("   State objects:\t")+eIntToStr(stats.numStates));
}

void eMainWnd::_onShowOpStats()
{
    const eOpMemoryMgr &opMemMgr = eIOperator::getMemoryMgr();
    const eOpClassBudget *bm = opMemMgr.getBudget(eOC_MESH);
    const eOpClassBudget *bb = opMemMgr.getBudget(eOC_BMP);

    eWriteToLog("----------------------------------------------");
    eWriteToLog("Operator budgets");
    eWriteToLog("----------------------------------------------");
    eWriteToLog(eString("  Mesh:\t")+eIntToStr(bm->usedResSize)+" / "+eIntToStr(bm->maxResSize)+
                " Bytes ("+eIntToStr((eInt)((eF32)bm->usedResSize/(eF32)bm->maxResSize*100.0f))+" %)");
    eWriteToLog(eString("Bitmap:\t")+eIntToStr(bb->usedResSize)+" / "+eIntToStr(bb->maxResSize)+
                " Bytes ("+eIntToStr((eInt)((eF32)bb->usedResSize/(eF32)bb->maxResSize*100.0f))+" %)");
}

void eMainWnd::_onToggleAppFullscreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

void eMainWnd::_onToggleViewFullscreen()
{
    // depending on visibility of central
    // widget toggle fullscreen mode
    const eBool fullScreen = centralWidget()->isVisible();

    // store main window's dock widget states
    // before hiding them
    static QByteArray state;

    if (fullScreen)
    {
        state = saveState();
        centralWidget()->setVisible(false);
        m_logDock->close();
        m_paramDock->close();
        m_profilerDock->close();
    }
    else
    {
        centralWidget()->setVisible(true);
        restoreState(state);
    }
}

void eMainWnd::_onStoredOpTreeSelectionChanged()
{
    if (m_storedOpsTree->selectedItems().empty())
        return;

    QTreeWidgetItem *item = m_storedOpsTree->selectedItems().at(0);
    _onOperatorGoto(item->data(0, Qt::UserRole).toInt());
}

void eMainWnd::_onDemoSeqTimeChanged(eF32 time)
{
    _onTimelineTimeChanged(time);
}

void eMainWnd::_onPathViewTimeChanged(eF32 time)
{
    _onTimelineTimeChanged(time);
}

void eMainWnd::_onTimelineTimeChanged(eF32 time)
{
    m_demoSeqView->setTime(time);
    m_renderView->setTime(time);
    m_pathView->setTime(time);
    m_timelineView->setTime(time);

    // update pane with current time
    const eU32 mins = (eU32)(time/60.0f);
    const eU32 secs = (eU32)time%60;
    const eU32 hund = (eU32)((time-(eU32)time)*100.0f);

    static QString buffer;
    buffer.sprintf("%.2i:%.2i:%.2i", mins, secs, hund);
    m_timeEdit->setText(buffer);
}

void eMainWnd::_onTimeEditEdited(const QString &text)
{
    const eU32 mins = text.mid(0, 2).toInt();
    const eU32 secs = text.mid(3, 2).toInt();
    const eU32 hund = text.mid(6, 2).toInt();
    const eF32 time = (eF32)mins*60.0f+(eF32)secs+(eF32)hund/100.0f;
    
    _onTimelineTimeChanged(time);
}

void eMainWnd::closeEvent(QCloseEvent *ce)
{
    QMainWindow::closeEvent(ce);

    // only ask for saving in release mode. when
    // testing in debug mode it's just annoying.
#ifdef eRELEASE
    if (!_askForSaving())
    {
        ce->ignore();
        return;
    }
#endif

    ce->accept();
}

void eMainWnd::timerEvent(QTimerEvent *te)
{
    QMainWindow::timerEvent(te);

    if (te->timerId() == m_profGraphTimerId)
        _updateProfilerTree();
    else if (te->timerId() == m_backupTimerId)
        _saveBackup();
    else if (te->timerId() == m_statusBarTimerId)
    {
        _updateStatusBar();

#ifdef eDEBUG
        eGfx->reloadEditedShaders();
#endif
    }
}

void eMainWnd::ProfilerGraphItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // is first column and thread item (no parent)?
    if (index.parent().isValid() && index.column() == 0)
    {
        const eF32 p = index.data(Qt::UserRole).toFloat();
        const QColor c = index.data(Qt::UserRole+1).value<QColor>();
        const eInt w = index.data(Qt::UserRole+2).toInt();
        
        QRectF r = option.rect;
        r.setWidth((w-r.left())*p);

        painter->fillRect(r, c);
    }

    QItemDelegate::paint(painter, option, index);
}