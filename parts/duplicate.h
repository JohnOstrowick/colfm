#ifndef DUPLICATE_H
#define DUPLICATE_H

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

// recursive folder copy
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

// quick folder size (non-recursive)
inline qint64 folderSize(const QString &folderPath) {
    qint64 total = 0;
    QDir dir(folderPath);
    for (const QFileInfo &fi : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files))
        total += fi.size();
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

    // Read settings
    int thresholdMB, dummy1, dummy2;
    readPrefs(thresholdMB, dummy1, dummy2);
    qint64 thresholdBytes = thresholdMB * 1000000LL;

    // Prepare async copy logic
    auto doCopy = [=]() {
        bool ok = false;

        if (info.isDir()) {
            if (!QDir().mkpath(newPath)) return false;
            ok = copyDirRecursively(path, newPath);
        } else {
            ok = QFile::copy(path, newPath);
        }

        QMetaObject::invokeMethod(qApp, [this, ok]() {
            if (!ok) {
                QMessageBox::critical(this, "Duplicate Failed", "Could not complete copy.");
            } else {
                onRefresh();
            }
        }, Qt::QueuedConnection);

        return;
    };

    // Decide if progress bar is needed
    bool showProgress = false;
    if (info.isDir()) {
        if (folderSize(path) >= thresholdBytes)
            showProgress = true;
    } else {
        if (info.size() >= thresholdBytes)
            showProgress = true;
    }

    // Run async and block with modal progress
    QFuture<void> future = QtConcurrent::run(doCopy);

    if (showProgress) {
        QProgressDialog progress("Copying...", "", 0, 0, this);
        progress.setWindowModality(Qt::ApplicationModal);
        progress.setAutoClose(true);
        progress.setCancelButton(nullptr);
        progress.setMinimumWidth(400);
        progress.setStyleSheet(
            "QProgressBar::chunk { background-color: #46B5CE; } "
            "QProgressBar { background-color: #333333; }"
        );
        progress.show();
        while (!future.isFinished()) {
            qApp->processEvents();
        }
    }
}

#endif // DUPLICATE_H
