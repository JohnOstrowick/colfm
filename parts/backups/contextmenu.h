#pragma once
#include <QMenu>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>

inline void ColFM::showContextMenu(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *actOpen = menu.addAction("Open");
    QAction *actRename = menu.addAction("Rename");
    QAction *actDelete = menu.addAction("Delete");

    // Open With submenu
    QMenu *openWithMenu = menu.addMenu("Open With");
    QFileInfo fi(currentFilePath());
    if (fi.exists()) {
        QString mimeType = QProcess::execute("xdg-mime", {"query", "filetype", fi.absoluteFilePath()}) == 0
            ? QString(QProcess::readAllStandardOutput()).trimmed()
            : "";

        // crude guessing based on extensions or assumed types
        QStringList apps;
        if (fi.suffix().contains("png", Qt::CaseInsensitive) ||
            fi.suffix().contains("jpg") || fi.suffix().contains("jpeg")) {
            apps << "gimp" << "krita" << "eog";
        } else if (fi.suffix().contains("pdf")) {
            apps << "evince" << "okular";
        } else if (fi.suffix().contains("mp4") || fi.suffix().contains("mkv")) {
            apps << "vlc" << "celluloid";
        } else if (fi.suffix().contains("txt") || fi.suffix().contains("md")) {
            apps << "gedit" << "mousepad" << "pluma";
        }

        for (const QString &app : apps) {
            QAction *appAction = openWithMenu->addAction(app);
            connect(appAction, &QAction::triggered, this, [this, app, fi]() {
                QProcess::startDetached(app, QStringList() << fi.absoluteFilePath());
            });
        }

        if (apps.isEmpty())
            openWithMenu->addAction("(no GUI apps found)")->setEnabled(false);
    }

    QAction *selectedAction = menu.exec(event->globalPos());

    if (selectedAction == actOpen) onOpen();
    else if (selectedAction == actRename) onRename();
    else if (selectedAction == actDelete) onMoveToTrash();
}
