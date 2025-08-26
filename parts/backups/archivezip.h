#ifndef ARCHIVEZIP_H
#define ARCHIVEZIP_H

#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include "folderize.h"

inline bool isToolAvailable(const QString &tool) {
    return !QProcess::execute("which " + tool) != -1;
}

inline void ColFM::onZip() {
    if (selectedFiles.isEmpty()) {
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

    QString choice = QInputDialog::getItem(this, "Choose Format", "Archive format:", formats, 0, false);
    if (choice.isEmpty()) return;

    QString archiveName = QInputDialog::getText(this, "Archive Name", "Enter archive name (without extension):");
    if (archiveName.isEmpty()) return;

    QString outPath = currentDir + "/" + archiveName;

    if (selectedFiles.size() > 1) {
        folderize();  // Assumes this packs all selectedFiles into currentDir + "/Folderized/"
        outPath += (choice.startsWith("tar") ? ".tar" : "." + choice);
        QString folder = currentDir + "/Folderized";

        if (choice == "zip") {
            QProcess::execute("zip -r \"" + outPath + "\" \"" + folder + "\"");
        } else if (choice == "tar.gz") {
            QProcess::execute("tar -czf \"" + outPath + ".gz\" -C \"" + currentDir + "\" Folderized");
        } else if (choice == "tar.bz2") {
            QProcess::execute("tar -cjf \"" + outPath + ".bz2\" -C \"" + currentDir + "\" Folderized");
        }

    } else {
        QString file = selectedFiles.first();
        QFileInfo info(file);
        outPath += (choice.startsWith("tar") ? ".tar" : "." + choice);

        if (choice == "zip") {
            QProcess::execute("zip \"" + outPath + "\" \"" + file + "\"");
        } else if (choice == "tar.gz") {
            QProcess::execute("tar -czf \"" + outPath + ".gz\" -C \"" + info.path() + "\" \"" + info.fileName() + "\"");
        } else if (choice == "tar.bz2") {
            QProcess::execute("tar -cjf \"" + outPath + ".bz2\" -C \"" + info.path() + "\" \"" + info.fileName() + "\"");
        }
    }

    statusBar()->showMessage("Archive created: " + outPath, 3000);
}

inline void ColFM::onUnZip() {
    if (selectedFiles.isEmpty()) {
        QMessageBox::warning(this, "Extract", "No archive selected.");
        return;
    }

    for (const QString &archive : selectedFiles) {
        QFileInfo info(archive);
        QString baseName = info.completeBaseName();
        QString destFolder = currentDir + "/" + baseName;

        QDir().mkpath(destFolder); // create target folder

        QString cmd;
        if (archive.endsWith(".zip", Qt::CaseInsensitive)) {
            cmd = "unzip -o \"" + archive + "\" -d \"" + destFolder + "\"";
        } else if (archive.endsWith(".tar.gz", Qt::CaseInsensitive) || archive.endsWith(".tgz", Qt::CaseInsensitive)) {
            cmd = "tar -xzf \"" + archive + "\" -C \"" + destFolder + "\"";
        } else if (archive.endsWith(".tar.bz2", Qt::CaseInsensitive) || archive.endsWith(".tbz", Qt::CaseInsensitive)) {
            cmd = "tar -xjf \"" + archive + "\" -C \"" + destFolder + "\"";
        } else if (archive.endsWith(".tar", Qt::CaseInsensitive)) {
            cmd = "tar -xf \"" + archive + "\" -C \"" + destFolder + "\"";
        } else {
            QMessageBox::warning(this, "Unknown Format", "Cannot extract: " + archive);
            continue;
        }

        QProcess::execute(cmd);
        statusBar()->showMessage("Extracted to " + destFolder, 2000);
    }
}

#endif // ARCHIVEZIP_H
