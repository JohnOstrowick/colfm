#ifndef ARCHIVEZIP_H
#define ARCHIVEZIP_H

#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include "folderize.h"

inline bool isToolAvailable(const QString &tool) {
    QProcess proc;
    proc.start("sh", QStringList() << "-c" << "command -v " + tool);
    proc.waitForFinished();
    return proc.exitCode() == 0;
}

inline void ColFM::onZip() {
    auto items = currentView->selectionModel()->selectedRows();
    if (items.isEmpty()) {
        QMessageBox::warning(this, "Archive", "No files selected.");
        return;
    }

    QStringList formats;
    if (isToolAvailable("zip")) formats << "zip";
    if (isToolAvailable("gzip")) formats << "tar.gz";
    if (isToolAvailable("bzip2")) formats << "tar.bz2";

    if (formats.isEmpty()) {
        QMessageBox::critical(this, "Error", "No supported compression tools found.");
        return;
    }

    QString format = QInputDialog::getItem(this, "Choose Format", "Archive format:", formats, 0, false);
    if (format.isEmpty()) return;

    QString cwd = ColFM::getCWD();
    auto *model = static_cast<QFileSystemModel *>(currentView->model());

    QString archiveBaseName;

    if (items.size() > 1) {
        onFolderize();  // creates "Folderized/"
        archiveBaseName = "Folderized";
    } else {
        QString src = model->filePath(items.first());
        QFileInfo info(src);
        archiveBaseName = info.completeBaseName();
    }

    QString outFile;

    if (format == "zip") {
        outFile = cwd + "/" + archiveBaseName + ".zip";
        if (items.size() > 1)
            QProcess::execute("zip -r \"" + outFile + "\" \"" + cwd + "/Folderized\"");
        else {
            QString src = model->filePath(items.first());
            QProcess::execute("zip \"" + outFile + "\" \"" + src + "\"");
        }
    } else if (format == "tar.gz") {
        outFile = cwd + "/" + archiveBaseName + ".tar.gz";
        if (items.size() > 1)
            QProcess::execute("tar -czf \"" + outFile + "\" -C \"" + cwd + "\" Folderized");
        else {
            QString src = model->filePath(items.first());
            QFileInfo info(src);
            QProcess::execute("tar -czf \"" + outFile + "\" -C \"" + info.path() + "\" \"" + info.fileName() + "\"");
        }
    } else if (format == "tar.bz2") {
        outFile = cwd + "/" + archiveBaseName + ".tar.bz2";
        if (items.size() > 1)
            QProcess::execute("tar -cjf \"" + outFile + "\" -C \"" + cwd + "\" Folderized");
        else {
            QString src = model->filePath(items.first());
            QFileInfo info(src);
            QProcess::execute("tar -cjf \"" + outFile + "\" -C \"" + info.path() + "\" \"" + info.fileName() + "\"");
        }
    }

    statusBar()->showMessage("Archive created: " + outFile, 3000);
}

inline void ColFM::onUnZip() {
    auto items = currentView->selectionModel()->selectedRows();
    if (items.isEmpty()) {
        QMessageBox::warning(this, "Extract", "No archive selected.");
        return;
    }

    QString cwd = ColFM::getCWD();
    auto *model = static_cast<QFileSystemModel *>(currentView->model());

    for (const QModelIndex &index : items) {
        QString src = model->filePath(index);
        QFileInfo info(src);
        QString base = info.completeBaseName();
        QString dest = cwd + "/" + base;
        QDir().mkpath(dest);

        QString cmd;
        if (src.endsWith(".zip", Qt::CaseInsensitive)) {
            cmd = "unzip -o \"" + src + "\" -d \"" + dest + "\"";
        } else if (src.endsWith(".tar.gz", Qt::CaseInsensitive) || src.endsWith(".tgz", Qt::CaseInsensitive)) {
            cmd = "tar -xzf \"" + src + "\" -C \"" + dest + "\"";
        } else if (src.endsWith(".tar.bz2", Qt::CaseInsensitive) || src.endsWith(".tbz", Qt::CaseInsensitive)) {
            cmd = "tar -xjf \"" + src + "\" -C \"" + dest + "\"";
        } else if (src.endsWith(".tar", Qt::CaseInsensitive)) {
            cmd = "tar -xf \"" + src + "\" -C \"" + dest + "\"";
        } else {
            QMessageBox::warning(this, "Unknown Format", "Cannot extract: " + src);
            continue;
        }

        QProcess::execute(cmd);
        statusBar()->showMessage("Extracted to " + dest, 2000);
    }
}

#endif // ARCHIVEZIP_H
