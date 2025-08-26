#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QContextMenuEvent>
#include <QMenu>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>

inline QStringList getGuiAppsForMime(const QString &mimeType) {
    QStringList knownApps;

    // Mapping from known mime substrings to preferred GUI apps
    struct AppMap { QString mimeMatch; QStringList apps; };
    QList<AppMap> map = {
        { "image/",     {"gimp", "krita", "gwenview"} },
        { "audio/",     {"audacious", "vlc", "ocenaudio"} },
        { "video/",     {"vlc", "mpv", "kdenlive"} },
        { "text/",      {"kate", "gedit", "mousepad"} },
        { "pdf",        {"okular", "evince"} },
        { "officedocument", {"libreoffice", "onlyoffice-desktopeditors", "wps"} },
        { "presentation",   {"libreoffice", "onlyoffice"} },
        { "spreadsheet",    {"libreoffice", "onlyoffice"} },
        { "msword",     {"libreoffice", "onlyoffice"} },
        { "html",       {"firefox", "chromium", "brave"} },
        { "zip",        {"ark", "file-roller", "xarchiver"} }
    };

    for (const auto &entry : map) {
        if (mimeType.contains(entry.mimeMatch, Qt::CaseInsensitive)) {
            for (const QString &app : entry.apps) {
                if (!QStandardPaths::findExecutable(app).isEmpty()) {
                    knownApps << app;
                }
            }
        }
    }

    return knownApps;
}

inline void ColFM::openWith(const QString &filePath) {
    QFileInfo fi(filePath);
    if (!fi.exists()) return;

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(fi);

    QStringList apps = getGuiAppsForMime(type.name());

    if (apps.isEmpty()) {
        QMessageBox::information(this, "No Applications Found", "No known applications found for this file type.");
        return;
    }

    QMenu chooser;
    for (const QString &app : apps) {
        chooser.addAction(app, [=]() {
            QProcess::startDetached(app, { filePath });
        });
    }

    chooser.exec(QCursor::pos());
}

inline void ColFM::showContextMenu(QContextMenuEvent *event) {
    if (!currentView || !model) return;

    QPoint globalPos = event->globalPos();
	QPoint viewPos = currentView->viewport()->mapFromGlobal(globalPos);
	QModelIndex index = currentView->indexAt(viewPos);
	if (!index.isValid()) return;

    QString filePath = model->filePath(index);
    QFileInfo fi(filePath);

    QMenu menu(this);
    menu.addAction("Rename", this, &ColFM::onRename);
    menu.addAction("Move to Trash", this, &ColFM::onMoveToTrash);
    menu.addAction("Create Link", this, &ColFM::onCreateSoftlink);
    //menu.addAction("Open With...", [=]() { openWith(filePath); });
    if (!fi.isDir()) {
        menu.addAction("Open With...", [=]() { openWith(filePath); });
    }

    menu.exec(event->globalPos());
}

#endif // CONTEXTMENU_H
