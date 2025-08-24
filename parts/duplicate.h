#ifndef DUPLICATE_H
#define DUPLICATE_H

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMessageBox>

inline void ColFM::onDuplicate() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;

    const QString path = model->filePath(idx);
    QFileInfo info(path);
    QString baseName = info.completeBaseName();  // filename without extension
    QString extension = info.completeSuffix();   // e.g. "txt"
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

    if (info.isDir()) {
        if (!QDir().mkpath(newPath)) {
            QMessageBox::critical(this, "Duplicate Failed", "Could not create duplicate folder.");
            return;
        }

        QDir sourceDir(path);
        QDir targetDir(newPath);
        for (const QString &entry : sourceDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
            QFile::copy(path + "/" + entry, newPath + "/" + entry);
        }
    } else {
        if (!QFile::copy(path, newPath)) {
            QMessageBox::critical(this, "Duplicate Failed", "Could not copy file.");
            return;
        }
    }

    onRefresh();
}

#endif // DUPLICATE_H
