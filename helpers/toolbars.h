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
#include "link.h"
namespace Search { void doSearch(ColFM *); }
#include "views.h"
#include "editmenu.h"

#include "nav.h"
#include "info.h"
inline void ColFM::drawButtons() {

    tb->clear();

// group 1 - appmenu
	// create a tool button with an icon and a popup menu
	QToolButton *btnAppIcon = new QToolButton(tb);
	btnAppIcon->setIcon(IconsData::getIcon("app_icon.png"));
	btnAppIcon->setToolTip("Application menu or actions");
	btnAppIcon->setPopupMode(QToolButton::InstantPopup);

	// build the application menu
	QMenu *appMenu = new QMenu(btnAppIcon);
	appMenu->addAction(IconsData::getIcon("shutdown.png"), "Shutdown", [this]{ shutdownNow(); });
	appMenu->addAction(IconsData::getIcon("reboot.png"), "Reboot", [this]{ rebootNow(); });
	appMenu->addSeparator();
	appMenu->addAction(IconsData::getIcon("force_quit.png"), "Force Quit", [this]{ forceQuitApp(); });
	appMenu->addAction(IconsData::getIcon("process_manager.png"), "Process Manager", [this]{ openProcessManager(); });
	appMenu->addSeparator();
	appMenu->addAction(IconsData::getIcon("lock_session.png"), "Lock Session", [this]{ lockSession(); });

	// new terminal
	actNewTerminal = new QAction(IconsData::getIcon("terminal.png"), "New Terminal", this);
	actNewTerminal->setToolTip("Open Terminal in Current Folder");
	connect(actNewTerminal, &QAction::triggered, this, &ColFM::onNewTerminal);
	appMenu->addAction(actNewTerminal);

	// add settings

	actSettings = new QAction(IconsData::getIcon("settings.png"), "Settings", this);
	actSettings->setToolTip("Open Settings dialog");
	connect(actSettings, &QAction::triggered, this, &ColFM::onSettings);
	appMenu->addAction(actSettings);

	// wrap it up
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

	//wrap it up
	btnFileMenu->setMenu(fileMenu);
	tb->addWidget(btnFileMenu);

// end file menu

// group 3 — Edit menu
	QToolButton *btnEditMenu = new QToolButton(tb);
	btnEditMenu->setText("Edit");
	btnEditMenu->setToolTip("Edit operations");
	btnEditMenu->setPopupMode(QToolButton::InstantPopup);

	// build the Edit menu
	QMenu *editMenu = new QMenu(btnEditMenu);

	// Cut
	actCut = new QAction(IconsData::getIcon("cut.png"), "Cut", this);
	actCut->setToolTip("Cut selected items");
	//connect(actCut, &QAction::triggered, this, &ColFM::onCut);
	editMenu->addAction(actCut);

	// Copy
	actCopy = new QAction(IconsData::getIcon("textcopy.png"), "Copy", this);
	actCopy->setToolTip("Copy selected items");
	//connect(actCopy, &QAction::triggered, this, &ColFM::onCopy);
	editMenu->addAction(actCopy);

	// Paste
	actPaste = new QAction(IconsData::getIcon("clipboard.png"), "Paste", this);
	actPaste->setToolTip("Paste items");
	//connect(actPaste, &QAction::triggered, this, &ColFM::onPaste);
	editMenu->addAction(actPaste);

	// Undo
	actUndo = new QAction(IconsData::getIcon("undo.png"), "Undo", this);
	actUndo->setToolTip("Undo last action");
	//connect(actUndo, &QAction::triggered, this, &ColFM::onUndo);
	editMenu->addAction(actUndo);

		// ---- Handlers (single definitions) ----
	connect(actCut, &QAction::triggered, this, [this] {
		QModelIndex idx = currentIndex();
		if (idx.isValid())
			onCut(model->filePath(idx));
	});

	connect(actCopy, &QAction::triggered, this, [this] {
		QModelIndex idx = currentIndex();
		if (idx.isValid())
			onCopy(model->filePath(idx));
	});

	connect(actPaste, &QAction::triggered, this, [this] {
		onPaste(model->filePath(currentRoot));
	});

	connect(actUndo, &QAction::triggered, this, [this] {
		onUndo();
	});

	// attach the menu
	btnEditMenu->setMenu(editMenu);
	tb->addWidget(btnEditMenu);
// group 3 end - edit menu

// group 4 — Properties menu
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
	labelMenu->setIcon(IconsData::getIcon("label.png"));
	QWidgetAction *waLabel = new QWidgetAction(labelMenu);
	waLabel->setDefaultWidget(LabelManager::buildSwatchRow(btnPropertiesMenu, model, [this](){ return getCWD(); }));
	labelMenu->addAction(waLabel);
	propertiesMenu->addMenu(labelMenu);

	// attach the menu
	btnPropertiesMenu->setMenu(propertiesMenu);
	tb->addWidget(btnPropertiesMenu);

// end of properties menu

// group 5 — Archiving menu
	QToolButton *btnArchiveMenu = new QToolButton(tb);
	btnArchiveMenu->setText("Archive");
	btnArchiveMenu->setToolTip("Archiving operations");
	btnArchiveMenu->setPopupMode(QToolButton::InstantPopup);

	// build the Archiving menu
	QMenu *archiveMenu = new QMenu(btnArchiveMenu);

	// Folderize
	actFolderize = new QAction(IconsData::getIcon("folderize.png"), "Folderize", this);
	actFolderize->setToolTip("Place selected items into a new folder");
	connect(actFolderize, &QAction::triggered, this, &ColFM::onFolderize);
	archiveMenu->addAction(actFolderize);

	// Zip (Archive)
	actZip = new QAction(IconsData::getIcon("zip.png"), "Archive", this);
	actZip->setToolTip("Create archive from selected files");
	connect(actZip, &QAction::triggered, this, &ColFM::onZip);
	archiveMenu->addAction(actZip);

	// UnZip (Extract)
	actUnZip = new QAction(IconsData::getIcon("unzip.png"), "Extract", this);
	actUnZip->setToolTip("Extract archive contents into folder");
	connect(actUnZip, &QAction::triggered, this, &ColFM::onUnZip);
	archiveMenu->addAction(actUnZip);

	// attach the menu
	btnArchiveMenu->setMenu(archiveMenu);
	tb->addWidget(btnArchiveMenu);
// end of archive menu

// group 6 - view menu
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
	
	// close off view menu
	btnViewMenu->setMenu(viewMenu);
	tb->addWidget(btnViewMenu);
// end view menu

// group 7 — Trash menu
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

// group 8 — Go menu
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
	actGoDownloads = new QAction(IconsData::getIcon("downloads.png"), "Downloads", this);
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

// group 9 search
    actSearch = tb->addAction(IconsData::getIcon("search.png"), "Search");
    connect(actSearch, &QAction::triggered, this, [this]() {
        Search::doSearch(this);
    });
// end search function
	

};

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
