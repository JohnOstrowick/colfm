#ifndef DUPLICATE_H
#define DUPLICATE_H

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>

// Recursive copy
inline bool copyDirRecursively(const QString &srcPath, const QString &dstPath) {
    QDir srcDir(srcPath);
    if (!srcDir.exists()) return false;

    QDir dstDir(dstPath);
    dstDir.mkpath(".");

    for (const QFileInfo &item : srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        QString src = item.absoluteFilePath();
        QString dst = dstPath + "/" + item.fileName();

        if (item.isDir()) {
            if (!copyDirRecursively(src, dst)) return false;
        } else {
            if (!QFile::copy(src, dst)) return false;
        }
    }
    return true;
}

// Shallow folder size checker
inline qint64 folderSize(const QString &folderPath) {
    qint64 total = 0;
    QDir dir(folderPath);
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);

    for (const QFileInfo &fi : entries) {
        if (fi.isDir()) {
            total += folderSize(fi.absoluteFilePath());
        } else {
            total += fi.size();
        }
    }

    return total;
}

inline void ColFM::onDuplicate() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;

    const QString path = model->filePath(idx);
    QFileInfo info(path);
    QString baseName = info.completeBaseName();
    QString extension = info.completeSuffix();
    QString dirPath = info.absolutePath();
    QString copyName;

    QString stem = baseName + "_copy";
    int n = 1;
    do {
        if (n == 1)
            copyName = stem;
        else
            copyName = stem + "_" + QString::number(n);

        if (!extension.isEmpty())
            copyName += "." + extension;

        ++n;
    } while (QFile::exists(dirPath + "/" + copyName));

    QString newPath = dirPath + "/" + copyName;

    int thresholdMB, dummy1, dummy2;
    readPrefs(thresholdMB, dummy1, dummy2);
    qint64 thresholdBytes = thresholdMB * 1000000LL;

    bool showProgress = false;
    /*if (info.isDir()) {
        if (folderSize(path) >= thresholdBytes)
            showProgress = true;
    } else {
        if (info.size() >= thresholdBytes)
            showProgress = true;
    }
    */
    if (info.isDir()) {
    qint64 sz = folderSize(path);
    QMessageBox::information(this, "Debug", "Folder size: " + QString::number(sz));
    if (sz >= thresholdBytes)
        showProgress = true;
    }


    QProgressDialog *progress = nullptr;
    /* temp comment */
    if (showProgress) {
        progress = new QProgressDialog("Copying...", nullptr, 0, 0, this);
        progress->setWindowModality(Qt::ApplicationModal);
        progress->setAutoClose(true);
        progress->setCancelButton(nullptr);
        progress->setMinimumWidth(400);
        progress->setStyleSheet(
            "QProgressBar::chunk { background-color: #46B5CE; } "
            "QProgressBar { background-color: #333333; }"
        );
        progress->show();
        qApp->processEvents();
    }
    
    if (showProgress) {
    int response = QMessageBox::question(
        this,
        "Large Copy",
        "This item is large. Proceed with making a copy?",
        QMessageBox::Ok | QMessageBox::Cancel
    );
    if (response != QMessageBox::Ok)
        return;
}

    bool ok = false;
    if (info.isDir()) {
        if (!QDir().mkpath(newPath)) {
            QMessageBox::critical(this, "Duplicate Failed", "Could not create destination folder.");
            return;
        }
        ok = copyDirRecursively(path, newPath);
    } else {
        ok = QFile::copy(path, newPath);
    }

    if (progress) progress->close();

    if (!ok) {
        QMessageBox::critical(this, "Duplicate Failed", "Could not complete copy.");
    } else {
        onRefresh();
    }
}

#endif // DUPLICATE_H
