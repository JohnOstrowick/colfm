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
#include <QActionGroup>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>
#include "iconsize.h"
#include "labels.h"
#include "new.h"
#include "icons_qicon.h"
// ----- ColFM methods (single definitions) -----
#include "info.h"
#include "link.h"
namespace Search { void doSearch(ColFM *); }
#include "views.h"
inline void ColFM::drawButtons() {

    tb->clear();
    // group 1
	// create a tool button with an icon and a popup menu
	QToolButton *btnAppIcon = new QToolButton(tb);
	btnAppIcon->setIcon(IconsData::getIcon("app_icon.png"));
	btnAppIcon->setToolTip("Application menu or actions");
	btnAppIcon->setPopupMode(QToolButton::InstantPopup);

	// build the application menu
	QMenu *appMenu = new QMenu(btnAppIcon);
	appMenu->addAction("Shutdown",         [this]{ shutdownNow(); });
	appMenu->addAction("Reboot",           [this]{ rebootNow(); });
	appMenu->addSeparator();
	appMenu->addAction("Force Quit",       [this]{ forceQuitApp(); });
	appMenu->addAction("Process Manager",  [this]{ openProcessManager(); });
	appMenu->addSeparator();
	appMenu->addAction("Lock Session", [this]{ lockSession(); });

	actNewTerminal = new QAction(IconsData::getIcon("terminal.png"), "New Terminal", this);
	actNewTerminal->setToolTip("Open Terminal in Current Folder");
	connect(actNewTerminal, &QAction::triggered, this, &ColFM::onNewTerminal);
	appMenu->addAction(actNewTerminal);

	btnAppIcon->setMenu(appMenu);
	tb->addWidget(btnAppIcon);


	// end app menu


// group 2 — File menu 
	QToolButton *btnFileMenu = new QToolButton(tb);
	btnFileMenu->setText("File");
	btnFileMenu->setToolTip("File operations");
	btnFileMenu->setPopupMode(QToolButton::InstantPopup);

	// build the File menu
	QMenu *fileMenu = new QMenu(btnFileMenu);

	// new folder
	actNewFolder = new QAction(IconsData::getIcon("newfolder.png"), "New Folder", this);
	actNewFolder->setToolTip("Create a new folder");
	connect(actNewFolder, &QAction::triggered, this, [this]{ onNewFolder(); });
	fileMenu->addAction(actNewFolder);

	// Rename
	actRename = new QAction(IconsData::getIcon("rename.png"), "Rename", this);
	actRename->setToolTip("Rename selected item");
	connect(actRename, &QAction::triggered, this, &ColFM::onRename);
	fileMenu->addAction(actRename);

	// open
	actOpen = new QAction(IconsData::getIcon("open.png"), "Open", this);
	actOpen->setToolTip("Open selected file or folder");
	connect(actOpen, &QAction::triggered, this, &ColFM::onOpen);
	fileMenu->addAction(actOpen);

	// duplicate
	actDuplicate = new QAction(IconsData::getIcon("duplicate.png"), "Duplicate", this);
	actDuplicate->setToolTip("Make a copy of the selected file or folder");
	connect(actDuplicate, &QAction::triggered, this, &ColFM::onDuplicate);
	fileMenu->addAction(actDuplicate);

	// move
	actMove = new QAction(IconsData::getIcon("move.png"), "Move", this);
	actMove->setToolTip("Move file or folder to another location");
	connect(actMove, &QAction::triggered, this, &ColFM::onMoveButton);
	fileMenu->addAction(actMove);

	// softlink
	actLink = new QAction(IconsData::getIcon("softlink.png"), "Make Linkfile", this);
	actLink->setToolTip("Create a soft link to the selected item");
	connect(actLink, &QAction::triggered, this, &ColFM::onCreateSoftlink);
	fileMenu->addAction(actLink);


//stuff
btnFileMenu->setMenu(fileMenu);
tb->addWidget(btnFileMenu);

// end file menu

// group 3 - view menu
// group: view
	QToolButton *btnViewMenu = new QToolButton(tb);
	btnViewMenu->setText("View");
	btnViewMenu->setToolTip("View and layout options");
	btnViewMenu->setPopupMode(QToolButton::InstantPopup);

	QMenu *viewMenu = new QMenu(btnViewMenu);

	// View mode actions
	viewMenu->addAction(IconsData::getIcon("view_columns.png"), "View as Columns", [this] {
	    setViewMode(ViewMode::Column);
	});
	viewMenu->addAction(IconsData::getIcon("view_icons.png"), "View as Icons", [this] {
	    setViewMode(ViewMode::Icon);
	});
	viewMenu->addAction(IconsData::getIcon("view_tree.png"), "View as List", [this] {
	    setViewMode(ViewMode::Tree);
	});

	viewMenu->addSeparator();

actToggleHidden = new QAction(IconsData::getIcon("eye-slash.png"), "Show Hidden Files", this);
actToggleHidden->setCheckable(true);
connect(actToggleHidden, &QAction::triggered, this, &ColFM::onToggleHidden);
    

	// Hidden files
	viewMenu->addAction(IconsData::getIcon("eye.png"), "Show Hidden Files", [this] {
	    onToggleHidden();
	});
	viewMenu->addAction(IconsData::getIcon("eye-slash.png"), "Hide Hidden Files", [this] {
	    onToggleHidden();
	});

	viewMenu->addSeparator();

	// Refresh
	viewMenu->addAction(IconsData::getIcon("refresh.png"), "Refresh", [this] {
	    onRefresh();
	});

	// Open New Window
	viewMenu->addAction(IconsData::getIcon("newwindow.png"), "Open New Window", [this] {
	    onNewWindow();
	});

// icon size
addIconSizePopup(viewMenu, [this]{ onRefresh(); });
//addIconSizePopup(viewMenu, iconView, [this]{ onRefresh(); });
//addIconSizePopup(viewMenu, currentView(), [this]{ onRefresh(); });
//addIconSizePopup(viewMenu, qobject_cast<QAbstractItemView*>(this->views->activeView()), [this]{ onRefresh(); });

// close off view menu
btnViewMenu->setMenu(viewMenu);
tb->addWidget(btnViewMenu);
// end view menu

// group 3 — Trash menu
	QToolButton *btnTrashMenu = new QToolButton(tb);
	btnTrashMenu->setText("Trash");
	btnTrashMenu->setToolTip("Trash operations");
	btnTrashMenu->setPopupMode(QToolButton::InstantPopup);

	// build the Trash menu
	QMenu *trashMenu = new QMenu(btnTrashMenu);

	// Open Trash
	actOpenTrash = new QAction(IconsData::getIcon("open_trash.png"), "Open Trash", this);
	actOpenTrash->setToolTip("Open the Trash folder");
	connect(actOpenTrash, &QAction::triggered, this, &ColFM::onOpenTrash);
	trashMenu->addAction(actOpenTrash);

	// Move to Trash
	actTrash = new QAction(IconsData::getIcon("move_to_trash.png"), "Move to Trash", this);
	actTrash->setToolTip("Move selected items to Trash");
	connect(actTrash, &QAction::triggered, this, &ColFM::onMoveToTrash);
	trashMenu->addAction(actTrash);

	// Restore From Trash
	actRestoreFromTrash = new QAction(IconsData::getIcon("move_out_trash.png"), "Restore From Trash", this);
	actRestoreFromTrash->setToolTip("Move selected items out of Trash");
	connect(actRestoreFromTrash, &QAction::triggered, this, &ColFM::onRestoreFromTrash);
	trashMenu->addAction(actRestoreFromTrash);

	// Empty Trash
	actEmptyTrash = new QAction(IconsData::getIcon("empty_trash.png"), "Empty Trash", this);
	actEmptyTrash->setToolTip("Empty the Trash");
	connect(actEmptyTrash, &QAction::triggered, this, &ColFM::onEmptyTrash);
	trashMenu->addAction(actEmptyTrash);

// attach the menu
btnTrashMenu->setMenu(trashMenu);
tb->addWidget(btnTrashMenu);
// end trash menu

// group 4 — Go menu
	QToolButton *btnGoMenu = new QToolButton(tb);
	btnGoMenu->setText("Go");
	btnGoMenu->setToolTip("Navigation");
	btnGoMenu->setPopupMode(QToolButton::InstantPopup);

	// build the Go menu
	QMenu *goMenu = new QMenu(btnGoMenu);

	// Go Up
	actUp = new QAction(IconsData::getIcon("up_level.png"), "Go Up", this);
	actUp->setToolTip("Go to parent folder");
	connect(actUp, &QAction::triggered, this, &ColFM::onUp);
	goMenu->addAction(actUp);

	// Back
	actBack = new QAction(IconsData::getIcon("undo.png"), "Back", this);
	actBack->setToolTip("Go back to previous folder");
	connect(actBack, &QAction::triggered, this, &ColFM::onBack);
	goMenu->addAction(actBack);

	// Home
	actGoHome = new QAction(IconsData::getIcon("home.png"), "Home", this);
	actGoHome->setToolTip("Go to your home directory");
	connect(actGoHome, &QAction::triggered, this, &ColFM::onGoHome);
	goMenu->addAction(actGoHome);

	// Desktop
	actGoDesktop = new QAction(IconsData::getIcon("desktop.png"), "Desktop", this);
	actGoDesktop->setToolTip("Go to Desktop");
	connect(actGoDesktop, &QAction::triggered, this, &ColFM::onGoDesktop);
	goMenu->addAction(actGoDesktop);

	// Documents
	actGoDocuments = new QAction(IconsData::getIcon("textcopy.png"), "Documents", this);
	actGoDocuments->setToolTip("Go to Documents");
	connect(actGoDocuments, &QAction::triggered, this, &ColFM::onGoDocuments);
	goMenu->addAction(actGoDocuments);

	// Downloads
	actGoDownloads = new QAction(IconsData::getIcon("download.png"), "Downloads", this);
	actGoDownloads->setToolTip("Go to Downloads");
	connect(actGoDownloads, &QAction::triggered, this, &ColFM::onGoDownloads);
	goMenu->addAction(actGoDownloads);

	// Pictures
	actGoPictures = new QAction(IconsData::getIcon("iconsize.png"), "Pictures", this);
	actGoPictures->setToolTip("Go to Pictures");
	connect(actGoPictures, &QAction::triggered, this, &ColFM::onGoPictures);
	goMenu->addAction(actGoPictures);

	// Media
	actGoMedia = new QAction(IconsData::getIcon("phone.png"), "Media", this);
	actGoMedia->setToolTip("Go to removable media");
	connect(actGoMedia, &QAction::triggered, this, &ColFM::onGoMedia);
	goMenu->addAction(actGoMedia);

	// Trash
	actOpenTrash = new QAction(IconsData::getIcon("open_trash.png"), "Trash", this);
	actOpenTrash->setToolTip("Open the Trash folder");
	connect(actOpenTrash, &QAction::triggered, this, &ColFM::onOpenTrash);
	goMenu->addAction(actOpenTrash);

// attach the menu
btnGoMenu->setMenu(goMenu);
tb->addWidget(btnGoMenu);
// end go menu

// group 5 — Properties menu
QToolButton *btnPropertiesMenu = new QToolButton(tb);
btnPropertiesMenu->setText("Properties");
btnPropertiesMenu->setToolTip("File properties and labels");
btnPropertiesMenu->setPopupMode(QToolButton::InstantPopup);

// build the Properties menu
QMenu *propertiesMenu = new QMenu(btnPropertiesMenu);

// Get Info
actInfo = new QAction(IconsData::getIcon("info.png"), "Get Info", this);
actInfo->setToolTip("Show file information and preview");
connect(actInfo, &QAction::triggered, this, &ColFM::onInfo);
propertiesMenu->addAction(actInfo);

// Colour Labels submenu
QMenu *labelMenu = new QMenu("Labels", propertiesMenu);
QWidgetAction *waLabel = new QWidgetAction(labelMenu);
waLabel->setDefaultWidget(LabelManager::buildSwatchRow(btnPropertiesMenu, model, [this](){ return getCWD(); }));
labelMenu->addAction(waLabel);
propertiesMenu->addMenu(labelMenu);

// attach the menu
btnPropertiesMenu->setMenu(propertiesMenu);
tb->addWidget(btnPropertiesMenu);

// end of properties menu

    //actUp         = tb->addAction(IconsData::getIcon("up_level.png"),      "Go Up a Level");    actUp->setToolTip("Go to parent folder");
//    actRefresh    = tb->addAction(IconsData::getIcon("refresh.png"),       "Refresh");          actRefresh->setToolTip("Reload current folder");
 //   tb->addSeparator();
    //actGoHome = tb->addAction(IconsData::getIcon("home.png"), "Go to Home");
    //actGoHome->setToolTip("Go to your home directory");
    // group 2
    //actOpenTrash  = tb->addAction(IconsData::getIcon("open_trash.png"),    "Open Trash");       actOpenTrash->setToolTip("Open the Trash folder");
    //actTrash      = tb->addAction(IconsData::getIcon("move_to_trash.png"), "Move to Trash");    actTrash->setToolTip("Move selected items to Trash");
    // (Move out of Trash — to be added later)
    //actEmptyTrash = tb->addAction(IconsData::getIcon("empty_trash.png"),   "Empty Trash");      actEmptyTrash->setToolTip("Empty the Trash");
    //actRestoreFromTrash = tb->addAction(IconsData::getIcon("move_out_trash.png"), "Restore From Trash");
    //actRestoreFromTrash->setToolTip("Move selected items out of Trash");
 //   tb->addSeparator();
    // group 3
    //actOpen       = tb->addAction(IconsData::getIcon("open.png"),          "Open");         actOpen->setToolTip("Open item");
    //actInfo       = tb->addAction(IconsData::getIcon("info.png"),          "Get Info");         actInfo->setToolTip("Show file information and preview");
//    actRename     = tb->addAction(IconsData::getIcon("rename.png"),        "Rename");           actRename->setToolTip("Rename selected item");
    //actMove       = tb->addAction(IconsData::getIcon("move.png"),          "Move");             actMove->setToolTip("Move selected item");
    actFolderize = tb->addAction(IconsData::getIcon("folderize.png"), "Folderize");
    actFolderize->setToolTip("Place selected items into a new folder");
    //actDuplicate  = tb->addAction(IconsData::getIcon("duplicate.png"),     "Duplicate");        actDuplicate->setToolTip("Copy / duplicate selected item");
   // actLink       = tb->addAction(IconsData::getIcon("softlink.png"),      "Make Linkfile");    actLink->setToolTip("Create a symbolic link");
  //  tb->addSeparator();
    actZip = tb->addAction(IconsData::getIcon("zip.png"), "Archive");
    actZip->setToolTip("Create archive from selected files");
    actUnZip = tb->addAction(IconsData::getIcon("unzip.png"), "Extract");
    actUnZip->setToolTip("Extract archive contents into folder");
    // group 4
   // treeBtn         = tb->addAction(IconsData::getIcon("view_tree.png"),     "List View");        treeBtn->setToolTip("Switch to Tree/List view");
    //columnBtn       = tb->addAction(IconsData::getIcon("view_columns.png"),  "Column View");      columnBtn->setToolTip("Switch to Column view");
    //iconBtn         = tb->addAction(IconsData::getIcon("view_icons.png"),    "Icon View");        iconBtn->setToolTip("Switch to Icon view");
    //toggleHiddenBtn = tb->addAction(IconsData::getIcon("eye-slash.png"),     "Show Hidden");      toggleHiddenBtn->setToolTip("Toggle hidden files");
    settingsBtn     = tb->addAction(IconsData::getIcon("settings.png"),      "Settings");         settingsBtn->setToolTip("Open Settings dialog");
    // (Icon size popup — to be added later)
    tb->addSeparator();
//    actNewFolder = tb->addAction(IconsData::getIcon("newfolder.png"), "New Folder");
  //  actNewFolder->setToolTip("Create a new folder");
    //actNewWindow = tb->addAction(IconsData::getIcon("newwindow.png"), "New Window");
    //actNewWindow->setToolTip("Open a new browser window");
//    actNewTerminal = tb->addAction(IconsData::getIcon("terminal.png"), "New Terminal");
 //   actNewTerminal->setToolTip("Open Terminal in Current Folder");
   // addIconSizePopup(this, tb, toggleHiddenBtn, model, [this]{ onRefresh(); });
    // --- Label (swatches) popup, sits next to Size ---
    //QToolButton *labelBtn = new QToolButton(tb);
    //labelBtn->setIcon(IconsData::getIcon("label.png"));   // or "icons/label.svg" if you have an SVG
    //labelBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    //labelBtn->setToolTip("Set label colour");
    //labelBtn->setPopupMode(QToolButton::InstantPopup);

    //QMenu *labelMenu = new QMenu(labelBtn);
    //QWidgetAction *waLabel = new QWidgetAction(labelMenu);
    //waLabel->setDefaultWidget(LabelManager::buildSwatchRow(labelBtn, model, [this](){ return getCWD(); }));
    //labelMenu->addAction(waLabel);
    //labelBtn->setMenu(labelMenu);
    // place it right after Size
    //tb->addWidget(labelBtn);

    // group 5 search
    actSearch = tb->addAction(IconsData::getIcon("search.png"), "Search");
    connect(actSearch, &QAction::triggered, this, [this]() {
        Search::doSearch(this);
    });
    // Wire up
    //connect(actAppIcon, &QAction::triggered, this, &ColFM::onAppIcon);
    //connect(actUp,            &QAction::triggered, this, &ColFM::onUp);
    //connect(actGoHome, &QAction::triggered, this, &ColFM::onGoHome);
    //connect(actRefresh,       &QAction::triggered, this, &ColFM::onRefresh);
//    connect(actOpen,            &QAction::triggered, this, &ColFM::onOpen);
    //connect(actTrash,         &QAction::triggered, this, &ColFM::onMoveToTrash);
    //connect(actOpenTrash,     &QAction::triggered, this, &ColFM::onOpenTrash);
    //connect(actRestoreFromTrash, &QAction::triggered, this, &ColFM::onRestoreFromTrash);
    //connect(actEmptyTrash, &QAction::triggered, this, &ColFM::onEmptyTrash);
    //connect(actInfo,          &QAction::triggered, this, &ColFM::onInfo);
//    connect(actRename,        &QAction::triggered, this, &ColFM::onRename);
//    connect(actMove,          &QAction::triggered, this, &ColFM::onMoveButton);
    connect(actFolderize, &QAction::triggered, this, &ColFM::onFolderize);
//    connect(actDuplicate,     &QAction::triggered, this, &ColFM::onDuplicate);
//    connect(actLink,          &QAction::triggered, this, &ColFM::onCreateSoftlink);
    connect(actZip, &QAction::triggered, this, &ColFM::onZip);
    connect(actUnZip, &QAction::triggered, this, &ColFM::onUnZip);
    //connect(toggleHiddenBtn,  &QAction::triggered, this, &ColFM::onToggleHidden);
    //connect(treeBtn,          &QAction::triggered, this, &ColFM::onViewTree);
    //connect(columnBtn,        &QAction::triggered, this, &ColFM::onViewColumn);
   // connect(iconBtn,          &QAction::triggered, this, &ColFM::onViewIcon);
    connect(settingsBtn,      &QAction::triggered, this, &ColFM::onSettings);
//    connect(actNewFolder, &QAction::triggered, this, [this]{ onNewFolder(); });
    //connect(actNewWindow, &QAction::triggered, this, &ColFM::onNewWindow);
}
// ---- Handlers (single definitions) ----

inline void ColFM::onBack()        { statusBar()->showMessage("TODO: Back", 2000); }
inline void ColFM::onGoDesktop()   { statusBar()->showMessage("TODO: Go Desktop", 2000); }
inline void ColFM::onGoDocuments() { statusBar()->showMessage("TODO: Go Documents", 2000); }
inline void ColFM::onGoDownloads() { statusBar()->showMessage("TODO: Go Downloads", 2000); }
inline void ColFM::onGoPictures()  { statusBar()->showMessage("TODO: Go Pictures", 2000); }
inline void ColFM::onGoMedia()     { statusBar()->showMessage("TODO: Go Media", 2000); }

// others are in their relevantly named files
inline void ColFM::onRefresh() {
    model->setIconProvider(new CustomIconProvider());  // rebuild icons (incl. folder label colours)
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
        toggleHiddenBtn->setIcon(IconsData::getIcon("eye.png"));
    } else {
        toggleHiddenBtn->setIcon(IconsData::getIcon("eye-slash.png"));
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
