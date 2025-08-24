#ifndef NEW_H
#define NEW_H

#pragma once
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDir>

inline void ColFM::onNewWindow() {
    QProcess::startDetached("/usr/local/bin/colfm");
}

inline void ColFM::onNewFolder() {
    bool ok;
    QString folderName = QInputDialog::getText(this, "Create Folder", "New folder name:", QLineEdit::Normal, "new_folder", &ok);
    if (!ok || folderName.isEmpty()) return;

    const QString parentDir = model->filePath(currentRoot);  // <- get current view folder
    QDir dir(parentDir);
    if (dir.mkdir(folderName)) {
        statusBar()->showMessage("Folder created: " + folderName, 2000);
        onRefresh();
    } else {
        QMessageBox::warning(this, "Error", "Could not create folder.");
    }
}

#endif // NEW_H
