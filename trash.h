#ifndef TRASH_H
#define TRASH_H

#include <QMessageBox>

inline void ColFM::onMoveToTrash() {
    statusBar()->showMessage("TODO: Move selected item to Trash", 2000);
}

inline void ColFM::onRestoreFromTrash() {
    statusBar()->showMessage("TODO: Restore item from Trash", 2000);
}

inline void ColFM::onEmptyTrash() {
    const QString trashPath = QDir::homePath() + "/.local/share/Trash/files";

    if (!QDir(trashPath).exists()) {
        statusBar()->showMessage("Trash folder not found", 2000);
        return;
    }

    int confirm = QMessageBox::question(
        this,
        "Empty Trash",
        "Are you sure you want to permanently delete all files and folders in the Trash?",
        QMessageBox::Yes | QMessageBox::Cancel
    );

    if (confirm != QMessageBox::Yes)
        return;

    QDir trashDir(trashPath);
    for (const QFileInfo &item : trashDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        if (item.isDir()) {
            QDir(item.absoluteFilePath()).removeRecursively();
        } else {
            QFile::remove(item.absoluteFilePath());
        }
    }

    statusBar()->showMessage("Trash emptied", 2000);
    onRefresh();
}

inline void ColFM::onOpenTrash() {
    const QString trash = QDir::homePath() + "/.local/share/Trash/files";
    if (!QDir(trash).exists()) {
        statusBar()->showMessage("Trash folder not found", 2000);
        return;
    }
    currentRoot = model->setRootPath(trash);        // used currently
    // currentRoot = model->index(trash);           // alternative kept (not deleted)
    if (crumbs) crumbs->setPath(trash);
    setViewMode(mode);
}

#endif // TRASH_H
