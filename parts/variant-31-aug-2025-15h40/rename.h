#ifndef RENAME_H
#define RENAME_H

#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>

inline void ColFM::onRename() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;

    const QString path = model->filePath(idx);
    QFileInfo info(path);
    QString oldName = info.fileName();

    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "Rename",
        "Rename \"" + oldName + "\" to:",
        QLineEdit::Normal,
        oldName,
        &ok
    );

    if (ok && !newName.isEmpty() && newName != oldName) {
        //QMessageBox::information(this, "Rename", "You entered: " + newName);
	if (ok && !newName.isEmpty() && newName != oldName) {
	    QString newPath = info.absolutePath() + "/" + newName;

	    if (QFile::exists(newPath)) {
		QMessageBox::warning(this, "Rename Failed", "A file or folder with that name already exists.");
		return;
	    }

	    QDir dir;
	    if (!dir.rename(path, newPath)) {
		QMessageBox::critical(this, "Rename Failed", "Unable to rename item.");
	    } else {
		onRefresh();
	    }
	}

    }
}

void ColFM::onRenameSelected(const QString &path) {
    QFileInfo info(path);
    QString oldName = info.fileName();

    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "Rename",
        "Rename \"" + oldName + "\" to:",
        QLineEdit::Normal,
        oldName,
        &ok
    );

    if (ok && !newName.isEmpty() && newName != oldName) {
        QMessageBox::information(this, "Rename", "You entered: " + newName);
    }
}

#endif // RENAME_H
