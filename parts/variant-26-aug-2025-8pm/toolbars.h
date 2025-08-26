#pragma once
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QListView>
#include <QProcess>
#include <QRegularExpression>
#include <QShortcut>
#include <QStatusBar>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QWidget>

// ----- ColFM methods (single definitions) -----
#include "info.h"

inline void ColFM::drawButtons() {
    tb->clear();

    // group 1
    actUp         = tb->addAction(QIcon("icons/up_level.png"),      "Go Up a Level");    actUp->setToolTip("Go to parent folder");
    actRefresh    = tb->addAction(QIcon("icons/refresh.png"),       "Refresh");          actRefresh->setToolTip("Reload current folder");
    tb->addSeparator();
actGoHome = tb->addAction(QIcon("icons/home.png"), "Go to Home");
actGoHome->setToolTip("Go to your home directory");

    // group 2
    actOpenTrash  = tb->addAction(QIcon("icons/open_trash.png"),    "Open Trash");       actOpenTrash->setToolTip("Open the Trash folder");
    actTrash      = tb->addAction(QIcon("icons/move_to_trash.png"), "Move to Trash");    actTrash->setToolTip("Move selected items to Trash");
    // (Move out of Trash — to be added later)
    actEmptyTrash = tb->addAction(QIcon("icons/empty_trash.png"),   "Empty Trash");      actEmptyTrash->setToolTip("Empty the Trash");
    actRestoreFromTrash = tb->addAction(QIcon("icons/move_out_trash.png"), "Restore From Trash");
    actRestoreFromTrash->setToolTip("Move selected items out of Trash");
    tb->addSeparator();

    // group 3
    actOpen       = tb->addAction(QIcon("icons/open.png"),          "Open");         actOpen->setToolTip("Open item");
    actInfo       = tb->addAction(QIcon("icons/info.png"),          "Get Info");         actInfo->setToolTip("Show file information and preview");
    actRename     = tb->addAction(QIcon("icons/rename.png"),        "Rename");           actRename->setToolTip("Rename selected item");
    actMove       = tb->addAction(QIcon("icons/move.png"),          "Move");             actMove->setToolTip("Move selected item");
actFolderize = tb->addAction(QIcon("icons/folderize.png"), "Folderize");
actFolderize->setToolTip("Place selected items into a new folder");

    actDuplicate  = tb->addAction(QIcon("icons/duplicate.png"),     "Duplicate");        actDuplicate->setToolTip("Copy / duplicate selected item");
    actLink       = tb->addAction(QIcon("icons/softlink.png"),      "Make Linkfile");    actLink->setToolTip("Create a symbolic link");
    tb->addSeparator();

    // group 4
    treeBtn         = tb->addAction(QIcon("icons/view_tree.png"),     "List View");        treeBtn->setToolTip("Switch to Tree/List view");
    columnBtn       = tb->addAction(QIcon("icons/view_columns.png"),  "Column View");      columnBtn->setToolTip("Switch to Column view");
    iconBtn         = tb->addAction(QIcon("icons/view_icons.png"),    "Icon View");        iconBtn->setToolTip("Switch to Icon view");
    toggleHiddenBtn = tb->addAction(QIcon("icons/eye-slash.png"),     "Show Hidden");      toggleHiddenBtn->setToolTip("Toggle hidden files");
    settingsBtn     = tb->addAction(QIcon("icons/settings.png"),      "Settings");         settingsBtn->setToolTip("Open Settings dialog");
    // (Icon size popup — to be added later)
    tb->addSeparator();

actNewFolder = tb->addAction(QIcon("icons/newfolder.png"), "New Folder");
actNewFolder->setToolTip("Create a new folder");

actNewWindow = tb->addAction(QIcon("icons/newwindow.png"), "New Window");
actNewWindow->setToolTip("Open a new browser window");

actNewTerminal = tb->addAction(QIcon("icons/terminal.png"), "New Terminal");
actNewTerminal->setToolTip("Open Terminal in Current Folder");

    // group 5 search
    actSearch = tb->addAction(QIcon("icons/search.png"), "Search");

	connect(actSearch, &QAction::triggered, this, [this]() {
	    onSearchPlocate();
	});

    // Wire up
    connect(actTrash,         &QAction::triggered, this, &ColFM::onMoveToTrash);
    connect(actRefresh,       &QAction::triggered, this, &ColFM::onRefresh);
    connect(actOpenTrash,     &QAction::triggered, this, &ColFM::onOpenTrash);
    connect(actOpen,            &QAction::triggered, this, &ColFM::onOpen);

    connect(actRestoreFromTrash, &QAction::triggered, this, &ColFM::onRestoreFromTrash);
    connect(actEmptyTrash, &QAction::triggered, this, &ColFM::onEmptyTrash);

    connect(actUp,            &QAction::triggered, this, &ColFM::onUp);
connect(actGoHome, &QAction::triggered, this, &ColFM::onGoHome);
    connect(actInfo,          &QAction::triggered, this, &ColFM::onInfo);
    connect(actRename,        &QAction::triggered, this, &ColFM::onRename);
    connect(actMove,          &QAction::triggered, this, &ColFM::onMove);
connect(actFolderize, &QAction::triggered, this, &ColFM::onFolderize);
    connect(actDuplicate,     &QAction::triggered, this, &ColFM::onDuplicate);
    connect(actLink,          &QAction::triggered, this, &ColFM::onCreateSoftlink);

    connect(toggleHiddenBtn,  &QAction::triggered, this, &ColFM::onToggleHidden);
    connect(treeBtn,          &QAction::triggered, this, &ColFM::onViewTree);
    connect(columnBtn,        &QAction::triggered, this, &ColFM::onViewColumn);
    connect(iconBtn,          &QAction::triggered, this, &ColFM::onViewIcon);
    connect(settingsBtn,      &QAction::triggered, this, &ColFM::onSettings);

connect(actNewFolder, &QAction::triggered, this, &ColFM::onNewFolder);
connect(actNewWindow, &QAction::triggered, this, &ColFM::onNewWindow);
connect(actNewTerminal, &QAction::triggered, this, &ColFM::onNewTerminal);

}

// ---- Handlers (single definitions) ----
// others are in their relevantly named files

inline void ColFM::onRefresh() {
    const QString path = model->filePath(currentRoot);
    model->setRootPath(path);
    setViewMode(mode);
    if (crumbs) crumbs->setPath(path);
    statusBar()->showMessage("Folder refreshed", 1500);
}

inline void ColFM::onUp() {
    QString path = crumbs ? crumbs->editField()->text() : model->filePath(currentRoot);
    QDir d(path);
    if (!d.cdUp()) return;
    const QString up = d.absolutePath();
    currentRoot = model->index(up);
    if (crumbs) crumbs->setPath(up);
    setViewMode(mode);
}

inline void ColFM::onOpen()                   { const QModelIndex idx = currentIndex(); if (idx.isValid()) openFile(idx); }

inline void ColFM::onInfo() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);
    colfm::showInfoDialog(this, path);
}

inline void ColFM::onMove()                   { statusBar()->showMessage("TODO: Move", 2000); }

inline void ColFM::onToggleHidden() {
    showHidden = !showHidden;
    QDir::Filters f = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (showHidden) {
        f |= QDir::Hidden;
        toggleHiddenBtn->setIcon(QIcon("icons/eye.png"));
    } else {
        toggleHiddenBtn->setIcon(QIcon("icons/eye-slash.png"));
    }
    model->setFilter(f);
    setViewMode(mode);
}

inline void ColFM::onViewTree()   { setViewMode(ViewMode::Tree); }
inline void ColFM::onViewColumn() { setViewMode(ViewMode::Column); }
inline void ColFM::onViewIcon()   { setViewMode(ViewMode::Icon); }

// Helpers for per-volume plocate DBs (~/.cache/plocate/vol_<sanitized>.db)
inline QString _volDbPath(const QString &volPath) {
    QString safe = volPath; safe.replace('/', '_');
    QDir cache(QDir::homePath() + "/.cache/plocate");
    cache.mkpath(".");
    return cache.filePath("vol" + safe + ".db");
}

inline bool _ensureVolDb(const QString &volPath, int maxAgeSecs = 3600) { // 1h freshness
    const QString db = _volDbPath(volPath);
    QFileInfo fi(db);
    const bool stale = !fi.exists() || fi.lastModified().secsTo(QDateTime::currentDateTime()) > maxAgeSecs;
    if (!stale) return true;
    QProcess p;
    p.start("updatedb", QStringList() << "-o" << db << "-U" << volPath);
    p.waitForFinished(-1);
    return (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0 && QFileInfo::exists(db));
}

// Runs dialog + plocate; returns fully-filtered results.
// Optionally returns chosen scope root and hidden toggle.
inline QStringList runPlocateDialogAndSearch(QWidget *parent,
                                             QString *outScopeRoot = nullptr,
                                             bool *outShowHidden = nullptr) {
    // --- dialog ---
    QDialog dlg(parent);
    dlg.setWindowTitle("Search with plocate");

    auto *edit  = new QLineEdit(&dlg); edit->setPlaceholderText("Search term…");
    auto *scope = new QComboBox(&dlg);
    auto *showHidden = new QCheckBox("Show hidden files", &dlg); showHidden->setChecked(false);

    const QString home = QDir::homePath();
    scope->addItem("My Home", home);       // default
    scope->addItem("Entire System", "/");
    scope->addItem("/var", "/var");

    // Placeholder + per-volume media entries
    const QString user = qEnvironmentVariable("USER");
    const QString mediaRoot = QString("/media/%1").arg(user);
    scope->addItem("USB Drives", mediaRoot);

    QDir mediaDir(mediaRoot);
    if (mediaDir.exists()) {
        for (const QString &vol : mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString volPath = mediaDir.absoluteFilePath(vol);
            scope->addItem(QString("Media: %1").arg(vol), volPath);
        }
    }

    auto *form = new QFormLayout();
    form->addRow("Search term:", edit);
    form->addRow("Scope:", scope);
    form->addRow("", showHidden);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addWidget(btns);

    edit->setFocus();
    if (dlg.exec() != QDialog::Accepted) return {};

    const QString query = edit->text().trimmed();
    if (query.isEmpty()) return {};

    const QString selRoot = scope->currentData().toString();
    if (outScopeRoot) *outScopeRoot = selRoot;
    if (outShowHidden) *outShowHidden = showHidden->isChecked();

    // --- build per-volume DB(s) if scope is under /media/$USER ---
    QStringList dbArgs;
    if (selRoot.startsWith(mediaRoot)) {
        QStringList volumes;
        if (selRoot == mediaRoot) {
            for (const QString &vol : mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                volumes << mediaDir.absoluteFilePath(vol);
            }
        } else {
            volumes << selRoot; // specific volume
        }
        for (const QString &volPath : volumes) {
            if (_ensureVolDb(volPath)) {
                dbArgs << "-d" << _volDbPath(volPath);
            }
        }
    }

    // --- run plocate ---
    QStringList args;
    args << "-i" << "--basename" << query;
    if (!dbArgs.isEmpty()) args << dbArgs;  // use per-volume DBs when available

    QProcess proc;
    proc.start("plocate", args);
    proc.waitForFinished(-1);

    QStringList all = QString::fromUtf8(proc.readAllStandardOutput())
                          .split('\n', Qt::SkipEmptyParts);

    // --- scope filter (if not using custom dbs) ---
    QStringList scoped;
    if (!dbArgs.isEmpty()) {
        scoped = all; // already scoped by -d
    } else if (selRoot == "/") {
        scoped = all;
    } else {
        auto inScope = [&](const QString &p){
            return p == selRoot || p.startsWith(selRoot + "/");
        };
        for (const auto &p : all) if (inScope(p)) scoped << p;
    }

    // --- hidden filter ---
    if (!showHidden->isChecked()) {
        QRegularExpression hiddenRe("(^|/)\\.[^/]+"); // any dot-starting path component
        QStringList filtered;
        for (const QString &p : scoped) {
            if (hiddenRe.match(p).hasMatch()) continue;
            filtered << p;
        }
        scoped = filtered;
    }

    return scoped;
}

// --- Wire ColFM to the helper ---
inline void ColFM::onSearchPlocate() {
    QString scopeRoot; bool showHidden = false;
    const QStringList results = runPlocateDialogAndSearch(this, &scopeRoot, &showHidden);
    if (results.isEmpty()) {
        statusBar()->showMessage("No results found", 2000);
        return;
    }

    auto *lv = new QListView();
    auto *listModel = new QStringListModel(results, lv);
    lv->setModel(listModel);
    lv->setUniformItemSizes(true);
    lv->setSelectionMode(QAbstractItemView::SingleSelection);
    currentView = lv;
    setCentralWidget(lv);

    connect(lv, &QListView::doubleClicked, this, [this, listModel](const QModelIndex &i){
        if (!i.isValid()) return;
        const QString path = listModel->data(i, Qt::DisplayRole).toString();
        QFileInfo fi(path);
        if (fi.isDir()) {
            currentRoot = model->index(fi.absoluteFilePath());
            setViewMode(mode);
            if (crumbs) crumbs->setPath(fi.absoluteFilePath());
        } else {
            const QString parent = fi.absolutePath();
            currentRoot = model->index(parent);
            setViewMode(mode);
            if (crumbs) crumbs->setPath(parent);
            const QModelIndex fileIdx = model->index(path);
            if (fileIdx.isValid()) previewFile(fileIdx);
        }
    });
}
