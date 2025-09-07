#ifndef NEW_TERMINAL_H
#define NEW_TERMINAL_H

#include <QMessageBox>
#include <QProcess>

inline void ColFM::onNewTerminal() {
    QModelIndex idx = currentView ? currentView->rootIndex() : QModelIndex();
    QString path = model->filePath(idx);
    QString dir = QFileInfo(path).absoluteFilePath();

//    QMessageBox::information(this, "Launching Terminal", dir);

    if (!QProcess::startDetached("gnome-terminal", QStringList() << "--working-directory=" + dir)) {
        QMessageBox::critical(this, "Error", "Could not launch gnome-terminal.");
    }
}

#endif
