// ColFM — Qt6 lightweight file manager
// Build: g++ -std=c++17 -fPIC colfm.cpp -o colfm `pkg-config --cflags --libs Qt6Widgets`

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QFileSystemModel>
#include <QTreeView>
#include <QListView>
#include <QColumnView>
#include <QStatusBar>
#include <QHeaderView>
#include <QDir>
#include <QIcon>
#include <QAction>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileInfo>
#include <QModelIndex>
#include <QKeySequence>
#include <QShortcut>
#include <QAbstractItemView>
#include <QWidget>
#include <QString>
#include <QSize>
#include <QLabel>
#include "toolbars.h"


class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent = nullptr);

    // ===== Types / state used by inline headers =====
    enum class ViewMode { Tree = 0, Column = 1, Icon = 2 };

    QFileSystemModel   *model{};
    QAbstractItemView  *currentView{};   // whichever view is active
    QToolBar           *tb{};
    Crumbs             *crumbs{};        // may be null

    // Root + mode expected by viewwidgets.h / handleopen.h
    QModelIndex currentRoot;
    ViewMode    mode{ViewMode::Tree};
    bool        showHidden{false};

    // Preview bits used by handleopen.h / viewwidgets.h
    QWidget    *previewPane{};           // container in column mode
    QLabel     *previewLabel{};          // image/text preview label

    // Common icon size used by views/delegates
    QSize       kIconSize{48, 48};

    // Actions (non-trash)
    QAction *actRefresh{}, *actUp{}, *actOpen{}, *actInfo{},
            *actRename{}, *actMove{}, *actDuplicate{}, *actLink{},
            *treeBtn{}, *columnBtn{}, *iconBtn{}, *toggleHiddenBtn{};

    // Trash actions
    QAction *actTrash{};             // Move to Trash
    QAction *actOpenTrash{};         // Show Trash (open Trash folder)
    QAction *actMoveOutOfTrash{};    // Restore / Move out of Trash
    QAction *actEmptyTrash{};        // Empty Trash

    // ===== Declarations for functions defined inline in headers =====
    // actions.h
    void drawButtons();
    void onRefresh();
    void onOpenTrash();
    void onMoveToTrash();
    void onMoveOutOfTrash();
    void onEmptyTrash();

    // handleopen.h
    QModelIndex currentIndex() const;
    bool isImageFile(const QString &path) const;
    void setPreviewHtml(const QString &html, const QString &path);
    void previewFile(const QModelIndex &idx);
    void openApp(const QString &path);
    void openFile(const QModelIndex &idx);
    void onOpen();
    void onInfo();

    // viewwidgets.h
    QWidget* buildTreeWidget(const QModelIndex &root);
    QWidget* buildColumnWidget(const QModelIndex &root);
    QWidget* buildIconWidget(const QModelIndex &root);
    void onViewTree();
    void onViewColumn();
    void onViewIcon();
    void setViewMode(ViewMode newMode);

    // other actions possibly defined inline elsewhere
    void onUp();
    void onRename();
    void onMove();
    void onDuplicate();
    void onCreateSoftlink();
    void onToggleHidden();
};

// ===== Constructor =====
ColFM::ColFM(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("ColFM");
    resize(1100, 720);

    // Model
    model = new QFileSystemModel(this);
    model->setResolveSymlinks(true);
    model->setReadOnly(false);

    const QString home = QDir::homePath();
    currentRoot = model->setRootPath(home); // keep both index and path coherent

    // Toolbar
    tb = addToolBar("Main");
    tb->setMovable(true);
    tb->setFloatable(false);
    tb->setIconSize(QSize(24, 24));

    statusBar()->showMessage("Ready", 1000);

    // Buttons & shortcuts (implemented inline in actions.h)
    // NOTE: setViewMode() will create and assign currentView/central widget.
    // We build the UI order as: Toolbars, then initial view.
    // drawButtons() calls setViewMode(mode) for refreshes, so call it after we build the first view.
    // Build the initial view first:
    // (viewwidgets.h will allocate the proper widget based on mode)
    // We’ll set the initial view AFTER includes.
}

// ===== Include order matters: Crumbs (toolbars.h) must come BEFORE viewwidgets.h =====
#include "viewwidgets.h"  // uses Crumbs, kIconSize, previewLabel, mode/currentRoot
#include "handleopen.h"   // open/info/preview helpers that use previewLabel/mode
#include "actions.h"      // drawButtons and trash handlers (call setViewMode(mode))

// Now that inline code is available, finish constructing the window by creating the initial view
// We do this in a tiny static-init style block by calling setViewMode from the constructor context.
static inline void __colfm_finish_init(ColFM* self) {
    // Create the initial view and hook it up
    self->setViewMode(self->mode);
    if (!self->centralWidget()) {
        // viewwidgets.h should have set the central widget; safeguard in case it didn't
        if (self->currentView) self->setCentralWidget(self->currentView);
    }
    // Finally lay out the toolbar buttons now that view is present
    self->drawButtons();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    ColFM w;
    __colfm_finish_init(&w);
    w.show();
    return app.exec();
}
