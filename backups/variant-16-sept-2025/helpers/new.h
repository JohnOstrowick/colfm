#ifndef NEW_H
#define NEW_H

#pragma once
#include <QCoreApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <QApplication>
#include <QAbstractItemView>
#include <QModelIndex>

// Assumes you already have: QFileSystemModel *model; QModelIndex currentRoot;

inline void ColFM::onNewWindow() {
    const QString exe = QCoreApplication::applicationFilePath();

    QString dir;

    // Prefer the currently focused item-view's selection
    if (QWidget *fw = QApplication::focusWidget()) {
        if (auto *av = qobject_cast<QAbstractItemView*>(fw)) {
            const QModelIndex idx = av->currentIndex();
            if (idx.isValid()) {
                const QFileInfo sfi(model->filePath(idx));
                dir = sfi.isDir() ? sfi.absoluteFilePath() : sfi.absolutePath();
            }
        }
    }

    // Fallback to the window's current root
    if (dir.isEmpty()) {
        dir = model->filePath(currentRoot);
    }

    // Final fallback to home
    if (dir.isEmpty()) {
        dir = QDir::homePath();
    }

    QProcess::startDetached(exe, { dir });
}

inline void ColFM::onNewFolder() {
    bool ok;
    QString folderName = QInputDialog::getText(this, "Create Folder", "New folder name:", QLineEdit::Normal, "new_folder", &ok);
    if (!ok || folderName.isEmpty()) return;
	const QString cwd = ColFM::getCWD();
	const QString parentDir = QFileInfo(cwd).isDir() ? QFileInfo(cwd).absoluteFilePath()
							 : QFileInfo(cwd).absolutePath();
    QDir dir(parentDir);
    if (dir.mkdir(folderName)) {
        statusBar()->showMessage("Folder created: " + folderName, 2000);
    } else {
        QMessageBox::warning(this, "Error", "Could not create folder.");
    }
}

#endif // NEW_H
