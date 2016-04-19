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

#ifndef MAIN_WND_HPP
#define MAIN_WND_HPP

#include <QtCore/QQueue>
#include <QtWidgets/QItemDelegate>
#include <QtWidgets/QLabel>

#include "ui_mainwnd.hpp"

// Enigma Studio's application main window
class eMainWnd : public QMainWindow, public Ui::MainWnd
{
    Q_OBJECT

private:
    enum StatusBarPane
    {
        SBP_STATS_CUROP,
        SBP_STATS_3DENGINE,
        SBP_STATS_OPSTACKING,
        SBP_STATS_MEMORY,
        SBP_COUNT
    };

public:
    eMainWnd(const QString &projectFile);
    virtual ~eMainWnd();

private:
    void                        _loadHelp();
    void                        _clearProject(eBool deleteBackup = eTRUE);
    void                        _setupViewMenu();
    void                        _setupStatusBar();
    void                        _makeConnections();
    void                        _addDefaultPage();
    void                        _createRecentFileActs();
    void                        _updateRecentFileActs();
    void                        _updateStatusBar();
    void                        _updateProfilerTree();
    void                        _setStatusText(StatusBarPane pane, const QString &text);
    void                        _saveBackup();
    void                        _deleteBackup();
    eBool                       _askForSaving();
    void                        _setCurrentFile(const QString &filePath);
    eBool                       _loadFromXml(const QString &filePath);
    eBool                       _saveToXml(const QString &filePath, eBool backup=eFALSE);
    void                        _newProject(eBool loadDefaultPrj);
    void                        _loadRecentProject();
    void                        _writeSettings() const;
    void                        _readSettings();
    QString                     _byteSizeToStr(eU32 numBytes) const;
    static void                 _logHandler(const eChar *msg, ePtr param);

private Q_SLOTS:
    void                        _onReloadStylesheet();

    void                        _onAbout();
    void                        _onFileNew();
    void                        _onFileOpen();
    eBool                       _onFileSave();
    eBool                       _onFileSaveAs();
    void                        _onFileExport();
    void                        _onFileOpenRecent();

    void                        _onPageAdd(eOperatorPage *&opPage);
    void                        _onPageRemove(eOperatorPage *opPage);
    void                        _onPageSwitch(eOperatorPage *opPage);
    void                        _onPageRenamed(eOperatorPage *opPage);

    void                        _onOperatorShow(eIOperator *op);
    void                        _onOperatorAdded(eIOperator *op);
    void                        _onOperatorRemoved(eIOperator *op);
    void                        _onOperatorSelected(eIOperator *op);
    void                        _onOperatorChanged(eIOperator *op, const eParameter *param);
    void                        _onOperatorGoto(eID opId);
    void                        _onPathOperatorEdit(eIOperator *op);
    void                        _onDemoOperatorEdit(eIOperator *op);
    void                        _onSwitchToPageView();

    void                        _onSettingsShadowQualityLow();
    void                        _onSettingsShadowQualityMedium();
    void                        _onSettingsShadowQualityHigh();
    void                        _onShowEngineStats();
    void                        _onShowOpStats();
    void                        _onToggleAppFullscreen();
    void                        _onToggleViewFullscreen();
    void                        _onStoredOpTreeSelectionChanged();

    void                        _onDemoSeqTimeChanged(eF32 time);
    void                        _onPathViewTimeChanged(eF32 time);
    void                        _onTimelineTimeChanged(eF32 time);
    void                        _onTimeEditEdited(const QString &text);

private:
    virtual void                closeEvent(QCloseEvent *ce);
    virtual void                timerEvent(QTimerEvent *te);

private:
    class ProfilerGraphItemDelegate : public QItemDelegate
    {
    public:
        virtual void            paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    };

private:
    static const QString        PROJECT_FILTER;
    static const QString        SCRIPT_FILTER;
    static const QString        PROJECT_EXT;
    static const QString        SCRIPT_EXT;
    static const QString        EDITOR_CAPTION;
    static const QString        BACKUP_FILENAME;
    static const eU32           MAX_RECENT_FILES = 5;
    static const eU32           AUTO_BACKUP_TIME_MS = 60*1000; // 1 minute

private:
    QAction *                   m_recSepAct;
    QAction *                   m_recentActs[MAX_RECENT_FILES];
    QString                     m_lastPrjPath;
    QLabel                      m_sbLabels[SBP_COUNT];
    eGuiOpPagePtrMap            m_guiOpPages;
    QString                     m_prjFilePath;
    eInt                        m_profGraphTimerId;
    eInt                        m_statusBarTimerId;
    eInt                        m_backupTimerId;
    ProfilerGraphItemDelegate   m_profItemDel;
};

#endif // MAIN_WND_HPP