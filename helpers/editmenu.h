#pragma once
#include <QClipboard>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>

// Cut: copy path to clipboard and mark for move
inline void ColFM::onCut(const QString &path) {
    QApplication::clipboard()->setText(path);
    statusBar()->showMessage("Cut: " + path, 2000);
    // TODO: mark internally as "cut" for move
}

// Copy: copy path to clipboard
inline void ColFM::onCopy(const QString &path) {
    QApplication::clipboard()->setText(path);
    statusBar()->showMessage("Copied: " + path, 2000);
}

// Paste: paste item from clipboard into current directory
inline void ColFM::onPaste(const QString &targetDir) {
    QString src = QApplication::clipboard()->text();
    if (src.isEmpty()) {
        statusBar()->showMessage("Clipboard empty", 2000);
        return;
    }

    QString dest = targetDir + "/" + QFileInfo(src).fileName();
    if (!QFile::copy(src, dest)) {
        statusBar()->showMessage("Paste failed: " + src, 2000);
    } else {
        statusBar()->showMessage("Pasted into: " + targetDir, 2000);
    }

    onRefresh();
}

// Undo: placeholder
inline void ColFM::onUndo() {
    statusBar()->showMessage("Undo not yet implemented", 2000);
}
