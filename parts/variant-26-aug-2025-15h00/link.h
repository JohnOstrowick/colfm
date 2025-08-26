#ifndef LINK_H
#define LINK_H

#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

inline void ColFM::onCreateSoftlink() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;

    const QString path = model->filePath(idx);
    QFileInfo info(path);
    QString linkName = info.fileName();

    bool ok;
    QString targetDir = QInputDialog::getText(
        this,
        "Create Link",
        "Where to place the link (target directory)?",
        QLineEdit::Normal,
        ".",  // default: current directory
        &ok
    );

    if (!ok || targetDir.isEmpty()) return;

    QString linkPath = QDir(targetDir).absoluteFilePath(linkName);

    QString cmd = QString("ln -s \"%1\" \"%2\"").arg(path, linkPath);
    int result = system(cmd.toUtf8().constData());

    if (result != 0) {
        QMessageBox::critical(this, "Link Failed", "Could not create symbolic link.");
    } else {
        statusBar()->showMessage("Symbolic link created", 2000);
        onRefresh();
    }
}

#endif // LINK_H
