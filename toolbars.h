#pragma once
#include <QCursor>
#include <QMessageBox>
#include <QApplication>
#include <QShortcut>
#include <QKeyEvent>
#include <QDir>
#include <QIcon>
#include <QAction>
#include <QStatusBar>
#include <QKeySequence>
#include <QInputDialog>
#include <QProcess>
#include <QStringListModel>

// ----- ColFM methods (single definitions) -----

inline void ColFM::drawButtons() {
    tb->clear();

    // group 1
    actUp         = tb->addAction(QIcon("icons/up_level.png"),      "Go Up a Level");    actUp->setToolTip("Go to parent folder");
    actRefresh    = tb->addAction(QIcon("icons/refresh.png"),       "Refresh");          actRefresh->setToolTip("Reload current folder");
    tb->addSeparator();

    // group 2
    actOpenTrash  = tb->addAction(QIcon("icons/open_trash.png"),    "Open Trash");       actOpenTrash->setToolTip("Open the Trash folder");
    actTrash      = tb->addAction(QIcon("icons/move_to_trash.png"), "Move to Trash");    actTrash->setToolTip("Move selected items to Trash");
    // (Move out of Trash — to be added later)
    actEmptyTrash = tb->addAction(QIcon("icons/empty_trash.png"),   "Empty Trash");      actEmptyTrash->setToolTip("Empty the Trash");
    actRestoreFromTrash = tb->addAction(QIcon("icons/open.png"), "Restore From Trash");
    actRestoreFromTrash->setToolTip("Move selected items out of Trash");
    tb->addSeparator();

    // group 3
    actInfo       = tb->addAction(QIcon("icons/info.png"),          "Get Info");         actInfo->setToolTip("Show file information and preview");
    actRename     = tb->addAction(QIcon("icons/rename.png"),        "Rename");           actRename->setToolTip("Rename selected item");
    actMove       = tb->addAction(QIcon("icons/move.png"),          "Move");             actMove->setToolTip("Move selected item");
    actDuplicate  = tb->addAction(QIcon("icons/duplicate.png"),     "Duplicate");        actDuplicate->setToolTip("Copy / duplicate selected item");
    actLink       = tb->addAction(QIcon("icons/softlink.png"),      "Make Linkfile");    actLink->setToolTip("Create a symbolic link");
    tb->addSeparator();

    // group 4
    treeBtn         = tb->addAction(QIcon("icons/view_tree.png"),     "List View");        treeBtn->setToolTip("Switch to Tree/List view");
    columnBtn       = tb->addAction(QIcon("icons/view_columns.png"),  "Column View");      columnBtn->setToolTip("Switch to Column view");
    iconBtn         = tb->addAction(QIcon("icons/view_icons.png"),    "Icon View");        iconBtn->setToolTip("Switch to Icon view");
    toggleHiddenBtn = tb->addAction(QIcon("icons/eye-slash.png"),     "Show Hidden");      toggleHiddenBtn->setToolTip("Toggle hidden files");
    // (Icon size popup — to be added later)
    tb->addSeparator();

    // group 5 search
    actSearch = tb->addAction(QIcon("icons/search.png"), "Search");
    connect(actSearch, &QAction::triggered, this, &ColFM::onSearchPlocate);


    // Wire up
    connect(actTrash,         &QAction::triggered, this, &ColFM::onMoveToTrash);
    connect(actRefresh,       &QAction::triggered, this, &ColFM::onRefresh);
    connect(actOpenTrash,     &QAction::triggered, this, &ColFM::onOpenTrash);
    connect(actRestoreFromTrash, &QAction::triggered, this, &ColFM::onRestoreFromTrash);

    connect(actUp,            &QAction::triggered, this, &ColFM::onUp);
    connect(actInfo,          &QAction::triggered, this, &ColFM::onInfo);
    connect(actRename,        &QAction::triggered, this, &ColFM::onRename);
    connect(actMove,          &QAction::triggered, this, &ColFM::onMove);
    connect(actDuplicate,     &QAction::triggered, this, &ColFM::onDuplicate);
    connect(actLink,          &QAction::triggered, this, &ColFM::onCreateSoftlink);

    connect(toggleHiddenBtn,  &QAction::triggered, this, &ColFM::onToggleHidden);
    connect(treeBtn,          &QAction::triggered, this, &ColFM::onViewTree);
    connect(columnBtn,        &QAction::triggered, this, &ColFM::onViewColumn);
    connect(iconBtn,          &QAction::triggered, this, &ColFM::onViewIcon);

    // Global shortcuts for preview (Space, Ctrl+I)
    auto scSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    scSpace->setContext(Qt::ApplicationShortcut);
    connect(scSpace, &QShortcut::activated, this, [this]{
        auto i = currentIndex(); if (i.isValid()) previewFile(i);
    });

    auto scInfo  = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_I), this);
    scInfo->setContext(Qt::ApplicationShortcut);
    connect(scInfo,  &QShortcut::activated, this, [this]{
        auto i = currentIndex(); if (i.isValid()) previewFile(i);
    });
}

// ---- Handlers (single definitions) ----

inline void ColFM::onMoveToTrash()            { statusBar()->showMessage("TODO: Move to Trash", 2000); }

inline void ColFM::onRefresh() {
    const QString path = model->filePath(currentRoot);
    model->setRootPath(path);
    setViewMode(mode);
    if (crumbs) crumbs->setPath(path);
    statusBar()->showMessage("Folder refreshed", 1500);
}

inline void ColFM::onRestoreFromTrash() {
    statusBar()->showMessage("TODO: Restore from Trash", 2000);
}

inline void ColFM::onOpenTrash() {
    const QString trash = QDir::homePath() + "/.local/share/Trash/files";
    if (!QDir(trash).exists()) {
        statusBar()->showMessage("Trash folder not found", 2000);
        return;
    }
    currentRoot = model->setRootPath(trash);        // used currently
    // currentRoot = model->index(trash);           // alternative kept (not deleted)
    if (crumbs) crumbs->setPath(trash);
    setViewMode(mode);
}

inline void ColFM::onSearchPlocate() {
    bool ok = false;
    const QString query = QInputDialog::getText(
        this, "Search with plocate", "Enter search term:",
        QLineEdit::Normal, QString(), &ok
    );
    if (!ok || query.trimmed().isEmpty()) return;

    // Run plocate: case-insensitive, match on basename to cut noise
    QProcess proc;
    proc.start("plocate", QStringList() << "-i" << "--basename" << query.trimmed());
    proc.waitForFinished();

    const QString out = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList results = out.split('\n', Qt::SkipEmptyParts);

    if (results.isEmpty()) {
        statusBar()->showMessage("No results found", 2000);
        return;
    }

    // Show results in a temporary list view (acts like a normal view)
    auto *lv = new QListView();
    auto *modelList = new QStringListModel(results, lv);
    lv->setModel(modelList);
    lv->setUniformItemSizes(true);
    lv->setSelectionMode(QAbstractItemView::SingleSelection);
    currentView = lv;                    // so toolbar helpers still work
    setCentralWidget(lv);                // replace main area with results

    statusBar()->showMessage(QString("Found %1 result(s)").arg(results.size()), 2000);

    // Open on double-click: directories open; files preview/open
    connect(lv, &QListView::doubleClicked, this, [this, modelList](const QModelIndex &i){
        if (!i.isValid()) return;
        const QString path = modelList->data(i, Qt::DisplayRole).toString();
        QFileInfo fi(path);

        if (fi.isDir()) {
            currentRoot = model->index(fi.absoluteFilePath());
            setViewMode(mode); // rebuilds main view in current mode
            if (crumbs) crumbs->setPath(fi.absoluteFilePath());
        } else {
            // Select parent dir and preview/open the file
            const QString parent = fi.absolutePath();
            currentRoot = model->index(parent);
            setViewMode(mode);
            if (crumbs) crumbs->setPath(parent);

            const QModelIndex fileIdx = model->index(path);
            if (fileIdx.isValid()) {
                // Your existing preview/open helpers
                previewFile(fileIdx);    // or openFile(fileIdx) if you prefer
            }
        }
    });
}

inline bool ColFM::eventFilter(QObject *obj, QEvent *ev) {
    // Avoid re-entrancy (e.g., Space inside a modal QMessageBox)
    if (QApplication::activeModalWidget()) return QObject::eventFilter(obj, ev);

    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        const bool isSpace = (ke->key() == Qt::Key_Space) && (ke->modifiers() == Qt::NoModifier);
        const bool isCtrlI = (ke->key() == Qt::Key_I) && (ke->modifiers() & Qt::ControlModifier);
        if (isSpace || isCtrlI) {
            QModelIndex idx = currentIndex();
            if (idx.isValid()) previewFile(idx);
            return true; // consume
        }
    }
    return QObject::eventFilter(obj, ev);
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
    QMessageBox::information(this, "Debug", "Get Info triggered");
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


