#ifndef FOLDERIZE_H
#define FOLDERIZE_H

#include <QInputDialog>
#include <QFile>
#include <QDir>
#include <QMessageBox>

inline void ColFM::onFolderize() {
    QModelIndexList selection = currentView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::information(this, "No selection", "Please select one or more items to folderize.");
        return;
    }

    // Prompt for folder name
    QString folderName = QInputDialog::getText(this, "Folderize",
                                               "Enter name for the new folder:",
                                               QLineEdit::Normal,
                                               "Folderized");
    if (folderName.trimmed().isEmpty())
        return;

    // Get current path
    QModelIndex index = currentView->rootIndex();
    QString currentPath = model->filePath(index);
    QDir dir(currentPath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "Error", "Current directory is not accessible.");
        return;
    }

    // Make sure folder doesn't already exist
    QString newFolderPath = dir.absoluteFilePath(folderName);
    if (!dir.mkdir(folderName)) {
        QMessageBox::warning(this, "Error", "Could not create folder: " + newFolderPath);
        return;
    }

    // Move selected items
    for (const QModelIndex &idx : selection) {
        QString srcPath = model->filePath(idx);
        QFileInfo fi(srcPath);
        QString dstPath = newFolderPath + "/" + fi.fileName();
        QDir().rename(srcPath, dstPath);  // works for both files and folders
    }

    statusBar()->showMessage(QString("Moved %1 item(s) to %2").arg(selection.size()).arg(folderName), 3000);
    onRefresh();
}

#endif // FOLDERIZE_H
