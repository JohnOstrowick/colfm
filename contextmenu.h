#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

// Centralised context-menu helpers and implementations for both ColFM (main)
// and the desktop companion. Guard ColFM-specific code with COLFM_MAIN so the
// desktop build can include this header without pulling in ColFM symbols.

#include <QContextMenuEvent>
#include <QMenu>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QPoint>
#include <QWidget>
#include <QStringList>
#include <QFileSystemModel>

// ---------- Generic helpers (safe for both apps) ----------

inline QStringList getGuiAppsForMime(const QString &mimeType) {
    // Light-weight heuristics. Desktop reads per-file associations elsewhere;
    // this provides reasonable defaults. Callers may still fall back to xdg-open.
    QStringList apps;

    const QString mt = mimeType.toLower();

    if (mt.startsWith(QStringLiteral("image/"))) {
        apps << QStringLiteral("gpicview") << QStringLiteral("eog") << QStringLiteral("xdg-open");
    } else if (mt.startsWith(QStringLiteral("video/"))) {
        apps << QStringLiteral("mpv") << QStringLiteral("vlc") << QStringLiteral("xdg-open");
    } else if (mt.startsWith(QStringLiteral("audio/"))) {
        apps << QStringLiteral("vlc") << QStringLiteral("mpv") << QStringLiteral("xdg-open");
    } else if (mt == QStringLiteral("application/pdf")) {
        apps << QStringLiteral("evince") << QStringLiteral("okular") << QStringLiteral("xdg-open");
    } else if (mt.startsWith(QStringLiteral("text/")) || mt.contains(QStringLiteral("xml"))) {
        apps << QStringLiteral("gedit") << QStringLiteral("kate") << QStringLiteral("xdg-open");
    } else if (mt == QStringLiteral("inode/directory")) {
        apps << QStringLiteral("xdg-open");
    } else {
        // Unknown: let the system decide.
        apps << QStringLiteral("xdg-open");
    }

    // De-duplicate while preserving order.
    QStringList deduped;
    for (const QString &a : apps) {
        if (!deduped.contains(a)) deduped << a;
    }
    return deduped;
}

inline void launchAppOnPath(const QString &app, const QString &path) {
    if (app.isEmpty()) {
        QProcess::startDetached(QStringLiteral("xdg-open"), { path });
    } else {
        QProcess::startDetached(app, { path });
    }
}

inline QString mimeForFilePath(const QString &path) {
    QMimeDatabase mdb;
    return mdb.mimeTypeForFile(path, QMimeDatabase::MatchContent).name();
}

inline QString pathFromModelIndex(QAbstractItemModel *model, const QModelIndex &idx) {
    // Try common roles first (DesktopModel custom roles), then QFileSystemModel.
    QVariant v = model->data(idx, Qt::UserRole + 1); // PathRole (typical)
    if (!v.isValid() || v.toString().isEmpty()) {
        v = model->data(idx, QFileSystemModel::FilePathRole);
    }
    if (!v.isValid() || v.toString().isEmpty()) {
        v = model->data(idx, Qt::DisplayRole);
    }
    return v.toString();
}

inline QString kindFromModelIndex(QAbstractItemModel *model, const QModelIndex &idx) {
    QVariant v = model->data(idx, Qt::UserRole + 2); // KindRole (typical)
    return v.isValid() ? v.toString() : QString();
}

// ---------- Desktop context menu (free function) ----------
// Used by colfm_desktop; shares helpers above; no dependency on ColFM class.
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

    // Trash-specific actions (pseudo-item provided by the model)
    if (kind == QLatin1String("trash")) {
        menu.addAction(QStringLiteral("Open Trash"), [](){
            QProcess::startDetached(QStringLiteral("xdg-open"), { QStringLiteral("trash:///") });
        });
        menu.addAction(QStringLiteral("Empty Trash"), [](){
            QProcess::startDetached(QStringLiteral("gio"), { QStringLiteral("trash"), QStringLiteral("--empty") });
        });
        menu.exec(globalPos);
        return;
    }

    // --- Order and labels matched to the ColFM menu you like ---

    // Move to Trash
    QAction *actMoveToTrash = menu.addAction(QStringLiteral("Move to Trash"));

    // Restore from Trash (not applicable here; desktop isn’t listing trash contents)
    QAction *actRestore = menu.addAction(QStringLiteral("Restore from Trash"));
    actRestore->setEnabled(false);

    // Open Trash / Empty Trash
    QAction *actOpenTrash  = menu.addAction(QStringLiteral("Open Trash"));
    QAction *actEmptyTrash = menu.addAction(QStringLiteral("Empty Trash"));

    menu.addSeparator();

    // Get Info
    QAction *actInfo = menu.addAction(QStringLiteral("Get Info"));

    // Rename
    QAction *actRename = menu.addAction(QStringLiteral("Rename"));

    // Move
    QAction *actMove = menu.addAction(QStringLiteral("Move"));

    // Duplicate
    QAction *actDuplicate = menu.addAction(QStringLiteral("Duplicate"));

    // Create Link (soft link)
    QAction *actCreateLink = menu.addAction(QStringLiteral("Create Link"));

    // Open Terminal Here
    QAction *actTerm = menu.addAction(QStringLiteral("Open Terminal Here"));

    // Open in New Window (ColFM)
    QAction *actNewWin = menu.addAction(QStringLiteral("Open in New Window"));

    // Move all to new folder (single-item scope on desktop)
    QAction *actFolderize = menu.addAction(QStringLiteral("Move all to new folder"));

    // Open With…
    QMenu *openWithMenu = menu.addMenu(QStringLiteral("Open With…"));
    {
        const QString mt = mimeForFilePath(path);
        const QStringList apps = getGuiAppsForMime(mt);
        if (apps.isEmpty()) {
            QAction *fallback = openWithMenu->addAction(QStringLiteral("System Default"));
            QObject::connect(fallback, &QAction::triggered, [path](){
                QProcess::startDetached(QStringLiteral("xdg-open"), { path });
            });
        } else {
            for (const QString &app : apps) {
                QAction *a = openWithMenu->addAction(app);
                QObject::connect(a, &QAction::triggered, [app, path](){
                    launchAppOnPath(app, path);
                });
            }
        }
    }

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    // --- Handlers ---

    if (chosen == actMoveToTrash) {
        QProcess::startDetached(QStringLiteral("gio"), { QStringLiteral("trash"), path });
        return;
    }

    if (chosen == actOpenTrash) {
        QProcess::startDetached(QStringLiteral("xdg-open"), { QStringLiteral("trash:///") });
        return;
    }
    if (chosen == actEmptyTrash) {
        QProcess::startDetached(QStringLiteral("gio"), { QStringLiteral("trash"), QStringLiteral("--empty") });
        return;
    }

    if (chosen == actInfo) {
        // Minimal info dialog to match "Get Info"
        const QString info =
            QStringLiteral("Name: %1\nPath: %2\nType: %3\nSize: %4 bytes\nModified: %5")
                .arg(fi.fileName(),
                     fi.absoluteFilePath(),
                     fi.isDir() ? QStringLiteral("Folder") : mimeForFilePath(path),
                     fi.isDir() ? QStringLiteral("-") : QString::number(fi.size()),
                     fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm")));
        QMessageBox::information(parent, QStringLiteral("Get Info"), info);
        return;
    }

    if (chosen == actRename) {
        bool ok = false;
        const QString newName = QInputDialog::getText(parent,
                                QStringLiteral("Rename"),
                                QStringLiteral("New name:"),
                                QLineEdit::Normal,
                                fi.fileName(), &ok);
        if (ok && !newName.isEmpty() && newName != fi.fileName()) {
            const QString target = QDir(dir).filePath(newName);
            QFile::rename(path, target);
        }
        return;
    }

    if (chosen == actMove) {
        bool ok = false;
        const QString targetDir = QInputDialog::getText(parent,
                                   QStringLiteral("Move to…"),
                                   QStringLiteral("Target folder:"),
                                   QLineEdit::Normal,
                                   dir, &ok);
        if (ok && !targetDir.isEmpty() && targetDir != dir) {
            const QString target = QDir(targetDir).filePath(fi.fileName());
            QFile::rename(path, target);
        }
        return;
    }

    if (chosen == actDuplicate) {
        QString base = fi.completeBaseName();
        QString ext  = fi.completeSuffix();
        if (!ext.isEmpty()) ext = QStringLiteral(".") + ext;
        QString candidate = QDir(dir).filePath(base + QStringLiteral(" copy") + ext);
        int n = 2;
        while (QFileInfo::exists(candidate)) {
            candidate = QDir(dir).filePath(base + QStringLiteral(" copy ") + QString::number(n) + ext);
            ++n;
        }
        QFile::copy(path, candidate);
        return;
    }

    if (chosen == actCreateLink) {
        // Create a symlink alongside the original: <name>.link -> original
        QString linkName = fi.fileName() + QStringLiteral(".link");
        QString linkPath = QDir(dir).filePath(linkName);
        int n = 2;
        while (QFileInfo::exists(linkPath)) {
            linkName = fi.completeBaseName() + QStringLiteral(".link.") + QString::number(n);
            linkPath = QDir(dir).filePath(linkName);
            ++n;
        }
#ifdef Q_OS_UNIX
        // Use ln -s to create a symlink
        QProcess::startDetached(QStringLiteral("ln"), { QStringLiteral("-s"), fi.fileName(), linkPath });
#else
        QFile::link(path, linkPath); // hardlink fallback on non-UNIX
#endif
        return;
    }

    if (chosen == actTerm) {
        QProcess::startDetached(QStringLiteral("x-terminal-emulator"),
                                { QStringLiteral("--working-directory"), dir });
        return;
    }

    if (chosen == actNewWin) {
        const QString exe = QCoreApplication::applicationDirPath() + QStringLiteral("/colfm");
        QProcess::startDetached(exe, { path });
        return;
    }

    if (chosen == actFolderize) {
        // Create new folder and move current item into it
        QString newFolder = QDir(dir).filePath(QStringLiteral("New Folder"));
        int n = 2;
        while (QFileInfo::exists(newFolder)) {
            newFolder = QDir(dir).filePath(QStringLiteral("New Folder ") + QString::number(n));
            ++n;
        }
        QDir().mkpath(newFolder);
        QFile::rename(path, QDir(newFolder).filePath(fi.fileName()));
        return;
    }

    // Open With… handled via per-action lambdas above
}
// ---------- ColFM (main app) context menu (member functions) ----------
#ifdef COLFM_MAIN

// Expectation: these are compiled inside colfm.cpp TU where class ColFM,
// members like currentView/model, and slots onMoveToTrash/onInfo/etc. exist.

inline void ColFM::openWith(const QString &filePath) {
    const QString mt = mimeForFilePath(filePath);
    const QStringList apps = getGuiAppsForMime(mt);

    if (apps.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("No Applications Found"),
                                 QStringLiteral("No known applications found for this file type."));
        return;
    }
    // Show a small chooser
    QMenu m(this);
    for (const QString &a : apps) {
        QAction *act = m.addAction(a);
        connect(act, &QAction::triggered, [a, filePath]() {
            launchAppOnPath(a, filePath);
        });
    }
    m.exec(QCursor::pos());
}

inline void ColFM::showContextMenu(QContextMenuEvent *event) {
    if (!currentView || !model || !event) return;

    const QPoint globalPos = event->globalPos();
    const QPoint viewPos   = currentView->viewport()->mapFromGlobal(globalPos);
    const QModelIndex index = currentView->indexAt(viewPos);
    if (!index.isValid()) return;

    const QString filePath = model->filePath(index);
    QMenu menu(this);

    // Trash-specific actions could be gated here if your model exposes it.
    menu.addAction(QStringLiteral("Move to Trash"),          this, &ColFM::onMoveToTrash);
    menu.addAction(QStringLiteral("Restore from Trash"),     this, &ColFM::onRestoreFromTrash);
    menu.addAction(QStringLiteral("Open Trash"),             this, &ColFM::onOpenTrash);
    menu.addAction(QStringLiteral("Empty Trash"),            this, &ColFM::onEmptyTrash);
    menu.addSeparator();

    menu.addAction(QStringLiteral("Get Info"),               this, &ColFM::onInfo);
    menu.addAction(QStringLiteral("Rename"),                 this, &ColFM::onRename);
    menu.addAction(QStringLiteral("Move"),                   this, &ColFM::onMove);
    menu.addAction(QStringLiteral("Duplicate"),              this, &ColFM::onDuplicate);
    menu.addAction(QStringLiteral("Create Link"),            this, &ColFM::onCreateSoftlink);
    menu.addAction(QStringLiteral("Open Terminal Here"),     this, &ColFM::onNewTerminal);
    menu.addAction(QStringLiteral("Open in New Window"),     this, &ColFM::onNewWindow);
    menu.addAction(QStringLiteral("Move all to new folder"), this, &ColFM::onFolderize);

    // Open With…
    QMenu *openWithMenu = menu.addMenu(QStringLiteral("Open With…"));
    const QString mt = mimeForFilePath(filePath);
    const QStringList apps = getGuiAppsForMime(mt);

    if (apps.isEmpty()) {
        QAction *fallback = openWithMenu->addAction(QStringLiteral("System Default"));
        connect(fallback, &QAction::triggered, [filePath](){
            QProcess::startDetached(QStringLiteral("xdg-open"), { filePath });
        });
    } else {
        for (const QString &app : apps) {
            QAction *a = openWithMenu->addAction(app);
            connect(a, &QAction::triggered, [app, filePath](){
                launchAppOnPath(app, filePath);
            });
        }
    }

    menu.exec(globalPos);
}

#endif // COLFM_MAIN

#endif // CONTEXTMENU_H
