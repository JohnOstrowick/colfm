#ifndef TRASH_H
#define TRASH_H

#include <QMessageBox>

inline void ColFM::onMoveToTrash() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;

    QString path = model->filePath(idx);
    QFileInfo info(path);
    QString target = QDir::homePath() + "/.local/share/Trash/files/" + info.fileName();

    if (QFile::exists(target)) {
        QMessageBox::warning(this, "Move to Trash", "Item already exists in Trash.");
        return;
    }

    if (!QFile::rename(path, target)) {
        QMessageBox::critical(this, "Move to Trash", "Failed to move item to Trash.");
        return;
    }

    statusBar()->showMessage("Item moved to Trash", 2000);
    onRefresh();
}

inline void ColFM::onRestoreFromTrash() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;

    QString path = model->filePath(idx);
    QFileInfo info(path);
    QString target = QDir::homePath() + "/Desktop/" + info.fileName();

    if (QFile::exists(target)) {
        QMessageBox::warning(this, "Restore from Trash", "Item already exists on Desktop.");
        return;
    }

    if (!QFile::rename(path, target)) {
        QMessageBox::critical(this, "Restore from Trash", "Failed to restore item.");
        return;
    }

    QMessageBox::information(this, "Restore", "Item restored to Desktop.");
    onRefresh();
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
