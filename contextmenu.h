#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QContextMenuEvent>
#include <QMenu>

inline void ColFM::showContextMenu(QContextMenuEvent *event) {
    QMenu *menu = new QMenu(this);

// Determine file path of selected item
    QModelIndex idx = currentIndex();
    if (!idx.isValid()) return;

    QString filePath = model->filePath(idx);
    QFileInfo fi(filePath);
    if (!fi.exists()) return;

    // Use 'file --mime-type' to get file type
    QProcess proc;
    proc.start("file", QStringList() << "--mime-type" << "-b" << filePath);
    proc.waitForFinished();
    QString mime = QString(proc.readAllStandardOutput()).trimmed();

    // Determine applications via 'xdg-mime'
    QProcess proc2;
    proc2.start("xdg-mime", QStringList() << "query" << "default" << mime);
    proc2.waitForFinished();
    QString defaultApp = QString(proc2.readAllStandardOutput()).trimmed();

    // Add "Open With" submenu
    QMenu *openWithMenu = menu->addMenu("Open With");

    if (!defaultApp.isEmpty()) {
        QAction *openDefault = new QAction(QString("Default (%1)").arg(defaultApp), this);
        connect(openDefault, &QAction::triggered, this, [filePath, defaultApp]{
            QProcess::startDetached("xdg-open", QStringList() << filePath);
        });
        openWithMenu->addAction(openDefault);
    }

    // Fallback apps
    QStringList apps = { "xdg-open", "vlc", "gedit", "code", "eog", "nano", "less" };
    for (const QString &app : apps) {
        QAction *a = new QAction(app, this);
        connect(a, &QAction::triggered, this, [filePath, app]{
            QProcess::startDetached(app, QStringList() << filePath);
        });
        openWithMenu->addAction(a);
    }



    if (actOpen)                menu->addAction(actOpen->toolTip(),        actOpen,        &QAction::trigger);
    if (actNewWindow)           menu->addAction(actNewWindow->toolTip(),   actNewWindow,   &QAction::trigger);
    if (actNewFolder)           menu->addAction(actNewFolder->toolTip(),   actNewFolder,   &QAction::trigger);
    if (actInfo)                menu->addAction(actInfo->toolTip(),        actInfo,        &QAction::trigger);
    if (actRename)              menu->addAction(actRename->toolTip(),      actRename,      &QAction::trigger);
    if (actDuplicate)           menu->addAction(actDuplicate->toolTip(),   actDuplicate,   &QAction::trigger);
    if (actTrash)               menu->addAction(actTrash->toolTip(),       actTrash,       &QAction::trigger);
    if (actRestoreFromTrash)    menu->addAction(actRestoreFromTrash->toolTip(), actRestoreFromTrash, &QAction::trigger);
    if (actOpenTrash)           menu->addAction(actOpenTrash->toolTip(),   actOpenTrash,   &QAction::trigger);
    if (actEmptyTrash)          menu->addAction(actEmptyTrash->toolTip(),  actEmptyTrash,  &QAction::trigger);
    if (actMove)                menu->addAction(actMove->toolTip(),        actMove,        &QAction::trigger);
    if (actLink)                menu->addAction(actLink->toolTip(),        actLink,        &QAction::trigger);
    if (actSettings)            menu->addAction(actSettings->toolTip(),    actSettings,    &QAction::trigger);

    menu->exec(event->globalPos());
    delete menu;
}

#endif
