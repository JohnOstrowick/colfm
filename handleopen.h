// handleopen.h
#pragma once

#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QItemSelectionModel>
#include <QFileInfo>
#include <QDir>

// ---- ColFM open helpers only (no Get Info, no preview logic) ----

inline QModelIndex ColFM::currentIndex() const {
    if (!currentView) return QModelIndex();
    auto *sel = currentView->selectionModel();
    if (!sel) return QModelIndex();

    QModelIndex idx = sel->currentIndex();
    if (!idx.isValid()) {
        const auto rows = sel->selectedRows();
        if (!rows.isEmpty()) return rows.first();
    }
    return idx;
}

inline void ColFM::openApp(const QString &path) {
    // Launch an app/executable directly.
    QProcess::startDetached(path);
}

inline void ColFM::openFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;

    const QString path = model->filePath(idx);
    QFileInfo fi(path);

    if (!fi.exists()) {
        statusBar()->showMessage("File not found", 2000);
        return;
    }

    // Navigate into directories
    if (fi.isDir()) {
        currentRoot = idx;                  // remember where we are
        if (crumbs) crumbs->setPath(path);  // update breadcrumbs
        setViewMode(mode);                  // rebuild view at new root
        return;
    }

    // Launch executables directly
    if (fi.isExecutable() && fi.isFile()) {
        openApp(path);
        return;
    }

    // Open with system default application
    const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    if (!ok) statusBar()->showMessage("Could not open", 2000);
}

// Convenience: open current selection
inline void ColFM::openSelected() {
    openFile(currentIndex());
}

// Convenience: open by absolute path (resolves to model index)
inline void ColFM::openPath(const QString &absPath) {
    if (absPath.isEmpty()) return;
    const QModelIndex idx = model->index(absPath);
    openFile(idx);
}
