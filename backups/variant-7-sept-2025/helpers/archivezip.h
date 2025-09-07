#ifndef ARCHIVEZIP_H
#define ARCHIVEZIP_H

#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QFileInfoList>
#include "folderize.h"
#include "progress.h"

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

    QString archiveTarget;
    QString archiveCommand;

    if (items.size() > 1) {
        QString target = cwd + "/Folderized";
        QDir(target).removeRecursively();
        QDir().mkdir(target);

        for (const QModelIndex &index : items) {
            QString src = model->filePath(index);
            QFileInfo fi(src);
            QString baseName = fi.fileName();
            QString dest = target + "/" + baseName;
            QProcess::execute("cp -dpRuf \"" + src + "\" \"" + dest + "\"");
        }

        qint64 sizeBytes = folderSize(target);
        if (sizeBytes > 30 * 1024 * 1024)
            onProgress(this, QColor("#46B5CE"), QColor("#EEEEEE"), 240, 18, true);

        if (format == "zip") {
            archiveTarget = target + ".zip";
            archiveCommand = "cd \"" + cwd + "\" && zip -r \"" + archiveTarget + "\" Folderized";
        } else if (format == "tar.gz") {
            archiveTarget = target + ".tar.gz";
            archiveCommand = "tar -czf \"" + archiveTarget + "\" -C \"" + cwd + "\" Folderized";
        } else if (format == "tar.bz2") {
            archiveTarget = target + ".tar.bz2";
            archiveCommand = "tar -cjf \"" + archiveTarget + "\" -C \"" + cwd + "\" Folderized";
        }

        int result = QProcess::execute("sh", QStringList() << "-c" << archiveCommand);
        QMessageBox::information(this, "Archive Result", "Command: " + archiveCommand + "\nExit Code: " + QString::number(result));

    } else {
        QString src = model->filePath(items.first());
        QFileInfo info(src);
        QString name = info.completeBaseName();
        QString path = info.absolutePath();

        if (format == "zip") {
            archiveTarget = cwd + "/" + name + ".zip";
            archiveCommand = "cd \"" + path + "\" && zip -r \"" + archiveTarget + "\" \"" + info.fileName() + "\"";
        } else if (format == "tar.gz") {
            archiveTarget = cwd + "/" + name + ".tar.gz";
            archiveCommand = "tar -czf \"" + archiveTarget + "\" -C \"" + path + "\" \"" + info.fileName() + "\"";
        } else if (format == "tar.bz2") {
            archiveTarget = cwd + "/" + name + ".tar.bz2";
            archiveCommand = "tar -cjf \"" + archiveTarget + "\" -C \"" + path + "\" \"" + info.fileName() + "\"";
        }

        int result = QProcess::execute("sh", QStringList() << "-c" << archiveCommand);
        QMessageBox::information(this, "Archive Result", "Command: " + archiveCommand + "\nExit Code: " + QString::number(result));
    }

    onProgress(this, QColor("#46B5CE"), QColor("#EEEEEE"), 0, 0, false);
    onRefresh();
    statusBar()->showMessage("Archive created: " + archiveTarget, 3000);
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

        QProcess::execute("sh", QStringList() << "-c" << cmd);
        statusBar()->showMessage("Extracted to " + dest, 2000);
    }

    onRefresh();
}

#endif // ARCHIVEZIP_H
