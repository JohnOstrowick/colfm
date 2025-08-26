#ifndef RENAME_H
#define RENAME_H

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>

// Core implementation
inline void ColFM::onRename() {
    // Find active file view
    QAbstractItemView *view = findChild<QAbstractItemView*>();
    if (!view) return;

    // Use current index, or first row if nothing selected
    QModelIndex idx = view->currentIndex();
    if (!idx.isValid()) {
        QAbstractItemModel *m = view->model();
        if (!m) return;
        const QModelIndex root = view->rootIndex();
        const int rows = m->rowCount(root);
        if (rows <= 0) return;
        idx = m->index(0, 0, root);
        if (auto *sel = view->selectionModel())
            sel->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    // Resolve path and current name
    auto *fsm = qobject_cast<QFileSystemModel*>(view->model());
    if (!fsm) return;
    const QString path = fsm->filePath(idx);
    if (path.isEmpty()) return;

    const QFileInfo fi(path);
    const QString currentName = fi.fileName();
    const QString dirPath     = fi.absolutePath();

    // Prompt for new name
    bool ok = false;
    const QString newName = QInputDialog::getText(
        view, "Rename", "New name:", QLineEdit::Normal, currentName, &ok
    ).trimmed();

    if (!ok) return;                             // cancelled
    if (newName.isEmpty() || newName == currentName) return;

    if (newName.contains('/')) {                 // basic validation
        QMessageBox::warning(view, "Rename", "Name may not contain '/'.");
        return;
    }

    const QString newPath = QDir(dirPath).filePath(newName);

    // Do the rename
    if (!QFile::rename(path, newPath)) {
        QMessageBox::warning(view, "Rename", "Could not rename item.");
        return;
    }

    // Re-select renamed item
    QModelIndex newIdx = fsm->index(newPath);
    if (newIdx.isValid() && view->selectionModel()) {
        view->selectionModel()->setCurrentIndex(newIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        view->scrollTo(newIdx, QAbstractItemView::PositionAtCenter);
    }
}

// Wrappers in case the toolbar is wired to different slot names.
// (Clicking the Rename button will call one of these in various builds.)
inline void ColFM::onRenameAction()   { onRename(); }
inline void ColFM::onRenameSelected() { onRename(); }

#endif // RENAME_H
