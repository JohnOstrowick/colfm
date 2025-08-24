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
        "Are you sure you want to permanently delete all items in the Trash?",
        QMessageBox::Yes | QMessageBox::Cancel
    );

    if (confirm == QMessageBox::Yes) {
        QDir trashDir(trashPath);
        for (const QString &entry : trashDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
            trashDir.remove(entry);  // note: won't delete subdirs yet
        }
        statusBar()->showMessage("Trash emptied", 2000);
        onRefresh();  // optional
    }
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
