#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QContextMenuEvent>
#include <QMenu>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QMimeDatabase>
#include <QMimeType>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QPoint>
#include <QWidget>
#include <QStringList>
#include <QFileSystemModel>
#include <QWidgetAction>
#include <QApplication>
#include <QAbstractItemView>
#include <QIODevice>
#include <QLocale>
#include <QVariant>
#include <QAction>

#include <functional>

#include "labels.h"   // central swatch row

// ------------------------ small helpers ------------------------

inline QStringList getGuiAppsForMime(const QString &mimeType) {
    QStringList apps; const QString mt = mimeType.toLower();
    if (mt.startsWith(QStringLiteral("image/")))       apps << "gpicview" << "eog" << "xdg-open";
    else if (mt.startsWith(QStringLiteral("video/")))  apps << "mpv" << "vlc" << "xdg-open";
    else if (mt.startsWith(QStringLiteral("audio/")))  apps << "vlc" << "mpv" << "xdg-open";
    else if (mt == QStringLiteral("application/pdf"))  apps << "evince" << "okular" << "xdg-open";
    else if (mt.startsWith(QStringLiteral("text/")) || mt.contains(QStringLiteral("xml")))
                                                     apps << "gedit" << "kate" << "xdg-open";
    else if (mt == QStringLiteral("inode/directory"))  apps << "xdg-open";
    else                                               apps << "xdg-open";
    return apps;
}

inline QString mimeForFilePath(const QString &path) {
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    if (!mt.isValid() || mt.name() == QStringLiteral("application/octet-stream"))
        mt = db.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
    return mt.isValid() ? mt.name() : QStringLiteral("application/octet-stream");
}

inline QString pathFromModelIndex(QAbstractItemModel *model, const QModelIndex &idx) {
    if (!model || !idx.isValid()) return {};
    if (auto *fsm = qobject_cast<QFileSystemModel*>(model)) return fsm->filePath(idx);
    const QVariant v = model->data(idx, Qt::UserRole + 1);
    if (v.isValid()) return v.toString();
    return model->data(idx, Qt::DisplayRole).toString();
}

inline QString kindFromModelIndex(QAbstractItemModel *model, const QModelIndex &idx) {
    if (!model || !idx.isValid()) return {};
    if (auto *fsm = qobject_cast<QFileSystemModel*>(model)) return fsm->isDir(idx) ? "Folder" : "File";
    const QFileInfo fi(pathFromModelIndex(model, idx));
    if (!fi.exists()) return {};
    return fi.isDir() ? "Folder" : "File";
}

inline void launchAppOnPath(const QString &app, const QString &path) {
    if (app.isEmpty() || path.isEmpty()) return;
    if (app == QStringLiteral("xdg-open")) QProcess::startDetached("xdg-open", { path });
    else                                   QProcess::startDetached(app, { path });
}

// Trigger a toolbar action on the parent window by its displayed text.
inline void triggerWindowActionByText(QWidget *parent, const QString &text) {
    if (!parent) return;
    if (QWidget *w = parent->window()) {
        const auto acts = w->findChildren<QAction*>();
        for (QAction *a : acts) {
            if (!a) continue;
            if (a->text() == text) { a->trigger(); return; }
        }
    }
}

// ------------------ Desktop + App free function ------------------

inline void showContextMenu(QWidget *parent,
                            const QModelIndex &idx,
                            QAbstractItemModel *model,
                            const QPoint &globalPos) {
    if (!model || !idx.isValid()) return;

    const QString path = pathFromModelIndex(model, idx);
    if (path.isEmpty()) return;

    const QString kind = kindFromModelIndex(model, idx);
    const QFileInfo fi(path);
    const QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();

    QMenu menu(parent);

    // ===== Toolbar parity (except icon size & view) =====
    QAction *actRefresh   = menu.addAction(QObject::tr("Refresh"));
    QObject::connect(actRefresh, &QAction::triggered, [=]{ triggerWindowActionByText(parent, QObject::tr("Refresh")); });

    QAction *actUp        = menu.addAction(QObject::tr("Up"));
    QObject::connect(actUp, &QAction::triggered,      [=]{ triggerWindowActionByText(parent, QObject::tr("Up")); });

    QAction *actGoHome    = menu.addAction(QObject::tr("Home"));
    QObject::connect(actGoHome, &QAction::triggered,  [=]{ triggerWindowActionByText(parent, QObject::tr("Home")); });

    QAction *actNewFolder = menu.addAction(QObject::tr("New Folder"));
    QObject::connect(actNewFolder, &QAction::triggered,[=]{ triggerWindowActionByText(parent, QObject::tr("New Folder")); });

    QAction *actNewWin    = menu.addAction(QObject::tr("New Window"));
    QObject::connect(actNewWin, &QAction::triggered,  [=]{ triggerWindowActionByText(parent, QObject::tr("New Window")); });

    QAction *actNewTerm   = menu.addAction(QObject::tr("New Terminal"));
    QObject::connect(actNewTerm, &QAction::triggered, [=]{ triggerWindowActionByText(parent, QObject::tr("New Terminal")); });

    QAction *actSearch    = menu.addAction(QObject::tr("Search"));
    QObject::connect(actSearch, &QAction::triggered,  [=]{ triggerWindowActionByText(parent, QObject::tr("Search")); });

    QAction *actSettings  = menu.addAction(QObject::tr("Settings"));
    QObject::connect(actSettings, &QAction::triggered,[=]{ triggerWindowActionByText(parent, QObject::tr("Settings")); });

    menu.addSeparator();

    if (path == QStringLiteral("trash://") || kind == QStringLiteral("Trash")) {
        QAction *actOpenTrash  = menu.addAction(QObject::tr("Open Trash"));
        QObject::connect(actOpenTrash,  &QAction::triggered, [=]{ triggerWindowActionByText(parent, QObject::tr("Open Trash")); });

        QAction *actEmptyTrash = menu.addAction(QObject::tr("Empty Trash"));
        QObject::connect(actEmptyTrash, &QAction::triggered, [=]{ triggerWindowActionByText(parent, QObject::tr("Empty Trash")); });

        // --- Swatches from LabelManager at the end ---
        QWidgetAction *wa = new QWidgetAction(&menu);
        wa->setDefaultWidget(LabelManager::buildSwatchRow(&menu, model, [dir]{ return dir; }));
        menu.addSeparator();
        menu.addAction(wa);

        menu.exec(globalPos);
        return;
    }

    // File/Folder specific actions (match toolbar set)
    QAction *actMoveToTrash = menu.addAction(QObject::tr("Move to Trash"));
    QObject::connect(actMoveToTrash, &QAction::triggered, [=]{ triggerWindowActionByText(parent, QObject::tr("Move to Trash")); });

    QAction *actRestore     = menu.addAction(QObject::tr("Restore from Trash"));
    QObject::connect(actRestore, &QAction::triggered,     [=]{ triggerWindowActionByText(parent, QObject::tr("Restore from Trash")); });

    menu.addSeparator();

    QAction *actInfo      = menu.addAction(QObject::tr("Get Info"));
    QObject::connect(actInfo, &QAction::triggered,        [=]{ triggerWindowActionByText(parent, QObject::tr("Get Info")); });

    QAction *actRename    = menu.addAction(QObject::tr("Rename"));
    QObject::connect(actRename, &QAction::triggered,      [=]{ triggerWindowActionByText(parent, QObject::tr("Rename")); });

    QAction *actMove      = menu.addAction(QObject::tr("Move"));
    QObject::connect(actMove, &QAction::triggered,        [=]{ triggerWindowActionByText(parent, QObject::tr("Move")); });

    QAction *actFolderize = menu.addAction(QObject::tr("Folderize"));
    QObject::connect(actFolderize, &QAction::triggered,   [=]{ triggerWindowActionByText(parent, QObject::tr("Folderize")); });

    QAction *actDuplicate = menu.addAction(QObject::tr("Duplicate"));
    QObject::connect(actDuplicate, &QAction::triggered,   [=]{ triggerWindowActionByText(parent, QObject::tr("Duplicate")); });

    QAction *actLink      = menu.addAction(QObject::tr("Create Link"));
    QObject::connect(actLink, &QAction::triggered,        [=]{ triggerWindowActionByText(parent, QObject::tr("Create Link")); });

    QAction *actZip       = menu.addAction(QObject::tr("Archive"));
    QObject::connect(actZip, &QAction::triggered,         [=]{ triggerWindowActionByText(parent, QObject::tr("Archive")); });

    QAction *actUnZip     = menu.addAction(QObject::tr("Extract"));
    QObject::connect(actUnZip, &QAction::triggered,       [=]{ triggerWindowActionByText(parent, QObject::tr("Extract")); });

    // Open With…
    QMenu *openWithMenu = menu.addMenu(QObject::tr("Open With…"));
    const QString mt = mimeForFilePath(path);
    const QStringList apps = getGuiAppsForMime(mt);
    if (apps.isEmpty()) {
        QAction *fallback = openWithMenu->addAction(QObject::tr("System Default"));
        QObject::connect(fallback, &QAction::triggered, [path](){ launchAppOnPath("xdg-open", path); });
    } else {
        for (const QString &app : apps) {
            QAction *a = openWithMenu->addAction(app);
            QObject::connect(a, &QAction::triggered, [app, path](){ launchAppOnPath(app, path); });
        }
    }

    // --- Swatches from LabelManager at the end ---
    {
        QWidgetAction *wa = new QWidgetAction(&menu);
        wa->setDefaultWidget(LabelManager::buildSwatchRow(&menu, model, [dir]{ return dir; }));
        menu.addSeparator();
        menu.addAction(wa);
    }

    menu.exec(globalPos);
}

#endif // CONTEXTMENU_H
