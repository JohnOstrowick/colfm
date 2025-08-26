#ifndef NEW_TERMINAL_H
#define NEW_TERMINAL_H

#include <QMessageBox>
#include <QProcess>

inline void ColFM::onNewTerminal() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);

    int confirm = QMessageBox::question(this, "Open Terminal",
                    QString("Open terminal in:\n%1 ?").arg(path),
                    QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) return;

    // Attempt to open a new terminal window in the selected path
    QString terminal = "gnome-terminal";
    QStringList args = { "--working-directory", path };

    bool ok = QProcess::startDetached(terminal, args);
    if (!ok) {
        QMessageBox::critical(this, "Error", "Failed to launch gnome-terminal.");
    }
}

#endif
