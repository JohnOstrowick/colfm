#pragma once
#include <QDir>
#include <QIcon>
#include <QAction>
#include <QStatusBar>
#include <QKeySequence>
#include <QShortcut>
#include <QCursor>

// ----- out-of-class definitions for ColFM -----

inline void ColFM::drawButtons() {
    actTrash      = tb->addAction(QIcon("icons/move_to_trash.png"), "Move to Trash");      actTrash->setToolTip("Move selected items to Trash");
    actRefresh    = tb->addAction(QIcon("icons/refresh.png"),       "Refresh Folder");      actRefresh->setToolTip("Reload current folder");
    actOpenTrash  = tb->addAction(QIcon("icons/open_trash.png"),    "Open Trash");          actOpenTrash->setToolTip("Open the Trash folder");

    actUp         = tb->addAction(QIcon("icons/up_level.png"),      "Go Up a Level");       actUp->setToolTip("Go to parent folder");
    actOpen       = tb->addAction(QIcon("icons/open.png"),          "Open");                actOpen->setToolTip("Open selected item");
    actClose      = tb->addAction(QIcon("icons/close.png"),         "Close");               actClose->setToolTip("Close selection");
    actInfo       = tb->addAction(QIcon("icons/info.png"),          "File Info & Preview"); actInfo->setToolTip("Show file information and preview");
    actRename     = tb->addAction(QIcon("icons/rename.png"),        "Rename");              actRename->setToolTip("Rename selected item");
    actMove       = tb->addAction(QIcon("icons/move.png"),          "Move");                actMove->setToolTip("Move selected item");
    actDuplicate  = tb->addAction(QIcon("icons/duplicate.png"),     "Copy / Duplicate");    actDuplicate->setToolTip("Copy or duplicate selected item");
    actLink       = tb->addAction(QIcon("icons/softlink.png"),      "Create Softlink");     actLink->setToolTip("Create a symbolic link to selected item");

    tb->addSeparator();

    treeBtn       = tb->addAction(QIcon("icons/view_tree.png"),     "Tree/List View");      treeBtn->setToolTip("Switch to Tree/List view");
    columnBtn     = tb->addAction(QIcon("icons/view_columns.png"),  "Column View");         columnBtn->setToolTip("Switch to Column view");
    iconBtn       = tb->addAction(QIcon("icons/view_icons.png"),    "Icon View");           iconBtn->setToolTip("Switch to Icon view");

    toggleHiddenBtn = tb->addAction(QIcon("icons/eye-slash.png"),   "Show/Hide Invisibles");
    toggleHiddenBtn->setToolTip("Toggle hidden files");

    // Wire up toolbar actions
    connect(actTrash,       &QAction::triggered, this, &ColFM::onMoveToTrash);
    connect(actRefresh,     &QAction::triggered, this, &ColFM::onRefresh);
    connect(actOpenTrash,   &QAction::triggered, this, &ColFM::onOpenTrash);

    connect(actUp,          &QAction::triggered, this, &ColFM::onUp);
    connect(actOpen,        &QAction::triggered, this, &ColFM::onOpen);
    connect(actClose,       &QAction::triggered, this, &ColFM::onCloseAction);
    connect(actInfo,        &QAction::triggered, this, &ColFM::onInfo);
    connect(actRename,      &QAction::triggered, this, &ColFM::onRename);
    connect(actMove,        &QAction::triggered, this, &ColFM::onMove);
    connect(actDuplicate,   &QAction::triggered, this, &ColFM::onDuplicate);
    connect(actLink,        &QAction::triggered, this, &ColFM::onCreateSoftlink);

    connect(toggleHiddenBtn,&QAction::triggered, this, &ColFM::onToggleHidden);

    connect(treeBtn,        &QAction::triggered, this, &ColFM::onViewTree);
    connect(columnBtn,      &QAction::triggered, this, &ColFM::onViewColumn);
    connect(iconBtn,        &QAction::triggered, this, &ColFM::onViewIcon);

    // Global shortcuts (no event filter)
    actInfo->setShortcuts({ QKeySequence(Qt::CTRL | Qt::Key_I), QKeySequence(Qt::Key_Space) });
    actInfo->setShortcutContext(Qt::ApplicationShortcut);

    auto scInfo  = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_I), this);
    scInfo->setContext(Qt::ApplicationShortcut);
    connect(scInfo, &QShortcut::activated, this, &ColFM::onInfo);

    auto scSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    scSpace->setContext(Qt::ApplicationShortcut);
    connect(scSpace, &QShortcut::activated, this, &ColFM::onInfo);
}

inline void ColFM::onMoveToTrash()            { statusBar()->showMessage("TODO: Move to Trash", 2000); }
inline void ColFM::onRefresh() {
    const QString path = model->filePath(currentRoot);
    model->setRootPath(path);
    setViewMode(mode);
    statusBar()->showMessage("Folder refreshed", 1500);
}
inline void ColFM::onOpenTrash() {
    const QString trash = QDir::homePath() + "/.local/share/Trash/files";
    if (!QDir(trash).exists()) {
        statusBar()->showMessage("Trash folder not found", 2000);
        return;
    }
    currentRoot = model->setRootPath(trash);
    if (crumbs) crumbs->setPath(trash);
    setViewMode(mode);
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
inline void ColFM::onCloseAction()            { statusBar()->showMessage("TODO: Close", 2000); }

inline void ColFM::onInfo() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) idx = currentRoot;
    if (idx.isValid()) previewFile(idx);
}

inline void ColFM::onRename()                 { statusBar()->showMessage("TODO: Rename", 2000); }
inline void ColFM::onMove()                   { statusBar()->showMessage("TODO: Move", 2000); }
inline void ColFM::onDuplicate()              { statusBar()->showMessage("TODO: Duplicate", 2000); }
inline void ColFM::onCreateSoftlink()         { statusBar()->showMessage("TODO: Create Softlink", 2000); }

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
