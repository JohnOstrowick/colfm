#pragma once
#include <QDir>
#include <QString>
#include <QModelIndex>

// Navigation + refresh related handlers for ColFM
inline void ColFM::onRefresh() {
    model->setIconProvider(new CustomIconProvider());  // rebuild icons (incl. folder label colours)
    const QString path = model->filePath(currentRoot);
    model->setRootPath(path);
    setViewMode(mode);
    if (crumbs) crumbs->setPath(path);
    statusBar()->showMessage("Folder refreshed", 1500);
}

inline void ColFM::onUp() {
    QString path = crumbs ? crumbs->editField()->text() : model->filePath(currentRoot);
    QDir d(path);
    if (!d.cdUp()) return;
    const QString up = d.absolutePath();
    currentRoot = model->index(up);
    if (crumbs) crumbs->setPath(up);
    setViewMode(mode);
}

inline void ColFM::onChDir(const QString &path) {
    if (!QDir(path).exists()) return;
    currentRoot = model->index(path);
    if (crumbs) crumbs->setPath(path);
    setViewMode(mode);
}

inline void ColFM::onGoDesktop()   { onChDir(QDir::homePath() + "/Desktop"); }
inline void ColFM::onGoDocuments() { onChDir(QDir::homePath() + "/Documents"); }
inline void ColFM::onGoDownloads() { onChDir(QDir::homePath() + "/Downloads"); }
inline void ColFM::onGoPictures()  { onChDir(QDir::homePath() + "/Pictures"); }
inline void ColFM::onGoMedia()     { onChDir("/media"); }

inline void ColFM::onOpen() {
    const QModelIndex idx = currentIndex();
    if (idx.isValid()) openFile(idx);
}
