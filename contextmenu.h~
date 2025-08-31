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
#include <functional>

#include "labels.h"   // use the central swatch row from LabelManager

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

// ------------------ Desktop free function (no ColFM) ------------------

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

    if (path == QStringLiteral("trash://") || kind == QStringLiteral("Trash")) {
        QAction *actOpenTrash  = menu.addAction("Open Trash");
        QAction *actEmptyTrash = menu.addAction("Empty Trash");
        QObject::connect(actOpenTrash,  &QAction::triggered, [](){ QProcess::startDetached("gio", { "open", "trash:///" }); });
        QObject::connect(actEmptyTrash, &QAction::triggered, [](){ QProcess::startDetached("gio", { "trash", "--empty" }); });

        // --- Swatches from LabelManager at the end ---
        QWidgetAction *wa = new QWidgetAction(&menu);
        wa->setDefaultWidget(LabelManager::buildSwatchRow(&menu, model, [dir]{ return dir; }));
        menu.addSeparator();
        menu.addAction(wa);

        menu.exec(globalPos);
        return;
    }

    QAction *actMoveToTrash = menu.addAction("Move to Trash");
    QAction *actRestore     = menu.addAction("Restore from Trash"); actRestore->setEnabled(false);
    menu.addSeparator();

    QAction *actInfo      = menu.addAction("Get Info");
    QAction *actRename    = menu.addAction("Rename");
    QAction *actMove      = menu.addAction("Move");
    QAction *actDuplicate = menu.addAction("Duplicate");
    QAction *actLink      = menu.addAction("Create Link");
    QAction *actTerminal  = menu.addAction("Open Terminal Here");
    QAction *actNewWin    = menu.addAction("Open in New Window");
    /* no unused variable warning:
       menu.addAction("Move all to new folder"); */

    QMenu *openWithMenu = menu.addMenu("Open With…");
    const QString mt = mimeForFilePath(path);
    const QStringList apps = getGuiAppsForMime(mt);
    if (apps.isEmpty()) {
        QAction *fallback = openWithMenu->addAction("System Default");
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

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    if (chosen == actMoveToTrash) {
        QProcess::startDetached("gio", { "trash", path });
    } else if (chosen == actInfo) {
        const QString info =
            QStringLiteral("Name: %1\nPath: %2\nType: %3\nSize: %4 bytes\nModified: %5")
                .arg(fi.fileName(),
                     fi.absoluteFilePath(),
                     fi.isDir() ? "Folder" : mimeForFilePath(path),
                     fi.isDir() ? "-" : QString::number(fi.size()),
                     fi.lastModified().toString(Qt::ISODate));
        QMessageBox::information(parent, "Info", info);
    } else if (chosen == actRename) {
        QProcess::startDetached("zenity",
                                { "--entry", "--title=Rename", "--text=New name:", "--entry-text=" + fi.fileName() });
    } else if (chosen == actMove) {
        // hook up your move dialog here if desired
    } else if (chosen == actDuplicate) {
        const QString dupPath = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() + " copy." + fi.suffix();
        QProcess::startDetached("cp", { path, dupPath });
    } else if (chosen == actLink) {
        const QString linkPath = fi.absolutePath() + QLatin1Char('/') + fi.fileName() + ".link";
        QProcess::startDetached("ln", { "-s", path, linkPath });
    } else if (chosen == actTerminal) {
        QProcess::startDetached("x-terminal-emulator", { "--working-directory", dir });
    } else if (chosen == actNewWin) {
        QProcess::startDetached("colfm", { dir });
    }
}

#endif // CONTEXTMENU_H
