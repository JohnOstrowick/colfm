#include <QApplication>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QColumnView>
#include <QListView>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QTreeView>
#include <QColumnView>
#include <QSplitter>
#include <QDir>
#include <QIcon>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>          
#include <QHeaderView>
#include <QCursor>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QStyledItemDelegate>
#include <QProxyStyle>
#include <QStyle>
#include <QStatusBar>
#include <QLineEdit>
#include <QWidgetAction>
#include <QSizePolicy>
#include <QEvent>
#include <QMessageBox>
#include <QTextBrowser>
#include <QTreeWidget>          
#include <QTreeWidgetItem>      
#include <QToolButton>          
#include <QMenu>
#include <functional>
#include <QKeyEvent>
#include <QTimer>
#include <QShortcut>
#include <QSaveFile>
#include <QTextStream>
#include <QApplication>
#include <QItemSelectionModel>
#include <QMap>
#include <QPainter>
#include <QFileInfo> 
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDrag>
#include <QMimeData>
#include <QPixmap>


#include "helpers/info.h"
#include "helpers/breadcrumbs.h"
#include "helpers/labels.h"
#include "helpers/move.h"
#include "helpers/drag.h"

#define COLFM_MAIN 1
#include "helpers/contextmenu.h"
#include <unistd.h>   // at top of file for getuid()
#include "helpers/icons_qicon.h"

static QString __appCWD = QDir::currentPath();

QString getCWD() {
    return __appCWD;
}

void setCWD(const QString &p) {
    __appCWD = p;
}

/* -------- Settings -------- */
static const QSize kIconSize(32, 32);

enum class ViewMode { Tree, Column, Icon };

/* Force app-wide 32 px icon metrics */
class ForceIconStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    int pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *wid) const override {
        if (m == QStyle::PM_SmallIconSize ||
            m == QStyle::PM_ListViewIconSize ||
            m == QStyle::PM_IconViewIconSize ||
            m == QStyle::PM_ToolBarIconSize) return 32;
        return QProxyStyle::pixelMetric(m, opt, wid);
    }
};

/* Fixed decoration size */
class FixedIconDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        option->decorationSize = kIconSize;
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt(option);
        opt.decorationSize = kIconSize;
        return QStyledItemDelegate::sizeHint(opt, index);
    }
};

/* Custom icon provider (tints symlinks; ensures 32px base) */
static QPixmap boxIcon(const QPixmap &src, int box) {
    QPixmap out(box, box);
    out.fill(Qt::transparent);
    if (src.isNull()) return out;
    QPixmap scaled = src.scaled(box, box, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter p(&out); p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const int x = (box - scaled.width()) / 2;
    const int y = (box - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);
    return out;
}

class CustomIconProvider : public QFileIconProvider {
public:
    using QFileIconProvider::QFileIconProvider;
    QIcon icon(const QFileInfo &info) const override {
        QPixmap pix = QFileIconProvider::icon(info).pixmap(kIconSize);
        QImage  img = pix.toImage();

        if (info.isSymLink()) {
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 0) c.setRgb((c.red()+0)/2, (c.green()+180)/2, (c.blue()+180)/2, c.alpha());
                    img.setPixelColor(x, y, c);
                }
            return QIcon(boxIcon(QPixmap::fromImage(img), kIconSize.width()));                   // <— was QIcon(QPixmap::fromImage(img))
        }
        if (info.isExecutable() && !info.isDir()) {
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 0) c.setRgb((c.red()+128)/2, (c.green()+255)/2, (c.blue()+128)/2, c.alpha());
                    img.setPixelColor(x, y, c);
                }
            return QIcon(boxIcon(QPixmap::fromImage(img), kIconSize.width()));                  // <— was QIcon(QPixmap::fromImage(img))
        }

        // label colours (uses your existing helpers)
        LabelManager::readLabelFile(info.dir().absolutePath());
        const QMap<QString,QString> map = LabelManager::labelMap;
        const QString colourName = map.value(info.fileName());
        if (!colourName.isEmpty()) {
            const QColor accent = LabelManager::colourFromName(colourName);
            if (accent.isValid()) {
                QImage img2 = img;
                LabelManager::tintImage(img2, accent);
                return QIcon(boxIcon(QPixmap::fromImage(img2), kIconSize.width()));              // <— ensure exact size
            }
        }

        // default: still box to exact kIconSize
        return QIcon(boxIcon(pix, kIconSize.width()));              // <— was QFileIconProvider::icon(info)
    }
};

class FixedFSModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;
    void setIconSize(const QSize &s) { iconSz = s; }                 /* added */

    QVariant data(const QModelIndex &index, int role) const override {
        if (role == Qt::DecorationRole) {
            QVariant v = QFileSystemModel::data(index, role);
            QPixmap pm;
            if (v.canConvert<QIcon>()) {
                pm = qvariant_cast<QIcon>(v).pixmap(iconSz);         /* modified: dynamic size */
            } else if (v.canConvert<QPixmap>()) {
                pm = qvariant_cast<QPixmap>(v);
            }
            if (!pm.isNull() && pm.size() != iconSz) {
                pm = pm.scaled(iconSz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            if (!pm.isNull()) return pm;
            return v;
        }
        return QFileSystemModel::data(index, role);
    }
private:
    QSize iconSz = kIconSize; 
};

QString getCWD();

class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent=nullptr) : QMainWindow(parent) {
        model = new FixedFSModel(this);
        model->setIconProvider(new CustomIconProvider());
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        currentRoot = model->setRootPath(::getCWD());

        crumbs = new Breadcrumbs("Path", this);
	crumbs->setFocusPolicy(Qt::NoFocus);
        crumbs->setOnPathChosen([this](const QString &p){
            if (QDir(p).exists()) { pushHistory(); currentRoot = model->index(p); setViewMode(mode); }
            else statusBar()->showMessage("Path not found", 2000);
        });
        crumbs->setPath(model->filePath(currentRoot));

        tb = new QToolBar("Main Toolbar", this);
        tb->setMovable(false);
        addToolBar(Qt::TopToolBarArea, tb);

        drawButtons();
        addToolBar(Qt::TopToolBarArea, crumbs);

        setViewMode(ViewMode::Tree);
        setWindowTitle("ColFM — Multi-View File Manager");
	this->installEventFilter(this);
	QTimer::singleShot(0, this, [this]{
	    if (auto v = findChild<QAbstractItemView*>())
		v->setFocus(Qt::OtherFocusReason);
	});

	new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this, []{
	    if (auto w = QApplication::activeModalWidget())
		w->close();
	});
        resize(1400, 800);
        LabelManager::refreshHook = [this]{ onRefresh(); };
    }

public:
    QStringList selectItems();
    void drawButtons();
    void onAppIcon();
    void onFolderize();
    void onRefresh();

    void onMoveToTrash();
    void onEmptyTrash();
    void onOpenTrash();
    void onRestoreFromTrash();

    void onUp();
    void onOpen();
    void openWith(const QString &filePath);
    void onInfo();
    void onGetInfo();
    void onMove();
    void onMoveButton();

    void onDuplicate();
    void onGoHome();
    void onCreateSoftlink();
    void onZip();
    void onUnZip();

    void onToggleHidden();
    void onSettings(); 
    void writePrefs(int folderMB, int iconSize, int viewMode);
    void readPrefs(int &folderMB, int &iconSize, int &viewMode);
    void onProgress(QObject *target, QColor colour, QColor background, int width, int height, bool bevel);
    void onRename(); void onRenameSelected(const QString &path);


    void onViewTree();
    void onViewColumn();
    void onViewIcon();
    void doSearch();

    QModelIndex currentIndex() const;
    void previewFile(const QModelIndex &idx);
    void openFile(const QModelIndex &idx);
    void openApp(const QString &path);
    void openSelected();
    void openPath(const QString &absPath);
    void contextMenuEvent(QContextMenuEvent *event) override;
    void showContextMenu(QContextMenuEvent *event);

    void onNewFolder();
    void onNewWindow();
    void onNewTerminal();

    bool eventFilter(QObject *obj, QEvent *ev) override;

//private:
    QFileSystemModel *model{};
    ViewMode mode = ViewMode::Tree;
    QModelIndex currentRoot;
    QLabel *previewLabel{};
    bool showHidden = false;

    colfm::InfoWidget *infoPanel{};

    Breadcrumbs *crumbs{};
    QToolBar *tb{};
    QAction *actAppIcon{}, *actTrash{}, *actRefresh{}, *actOpenTrash{}, *actUp{};
    QAction *actRestoreFromTrash{};
    QAction *actOpen{}, *actClose{}, *actInfo{}, *actRename{}, *actMove{}, *actFolderize{}, *actDuplicate{}, *actLink{};
    QAction *actZip{},*actUnZip{};
    QAction *treeBtn{}, *columnBtn{}, *iconBtn{}, *toggleHiddenBtn{}, *settingsBtn{};
    QAction *actEmptyTrash{};
    QAction *actGoHome{};
    QAction *actNewFolder{}, *actNewWindow{};
    QAction *actNewTerminal{};
    QAction *actSearch{}, *actSettings{};
    // these are declared in their own header files
    //QAction *addIconSizePopup{};
   //QToolButton *labelBtn{};
    QAbstractItemView *currentView{};

    /* added: sidebar + simple back history */
    QTreeWidget *sidebar{};                     /* added */
    QStringList backStack;                      /* added */

    #include "helpers/views.h"

    QString getCWD();

    void pushHistory() {                         /* added */
        backStack.prepend(model->filePath(currentRoot));
        if (backStack.size() > 50) backStack.removeLast();
    }

    /* ---------------------- Sidebar Builder (single column) ---------------------- */
    #include "helpers/sidebar.h"
    
    void setViewMode(ViewMode m) {
        mode = m;
        infoPanel = nullptr;

        QWidget *old = centralWidget();
        if (old) old->deleteLater();

        QWidget *center = nullptr;
        QModelIndex root = currentRoot.isValid() ? currentRoot : model->index(QDir::homePath());
        switch (mode) {
            case ViewMode::Tree:   center = buildTreeWidget(root);   break;
            case ViewMode::Column: center = buildColumnWidget(root); break;
            case ViewMode::Icon:   center = buildIconWidget(root);   break;
        }

        QWidget *w = buildWithSidebar(center); /* modified: wrap with sidebar */
        setCentralWidget(w);

        // Replace right preview label with InfoWidget in Column mode (as before)
        if (mode == ViewMode::Column && previewLabel) {
            QWidget *pane = previewLabel->parentWidget();
            if (pane && pane->layout()) {
                pane->layout()->removeWidget(previewLabel);
                previewLabel->deleteLater();
                previewLabel = nullptr;

                infoPanel = new colfm::InfoWidget(pane);
                pane->layout()->addWidget(infoPanel);

                if (auto details = infoPanel->findChild<QTextBrowser*>()) {
                    details->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                    int h = qMax(120, details->sizeHint().height() - 100);
                    details->setMaximumHeight(h);
                }
            }
            const QModelIndex cur = currentIndex();
            if (cur.isValid()) previewFile(cur);
        }

        if (crumbs) crumbs->setPath(model->filePath(currentRoot));

        // Update Info panel on keyboard navigation + open on activation/double-click
        if (currentView && currentView->selectionModel()) {
            connect(currentView->selectionModel(), &QItemSelectionModel::currentChanged,
                    this, [this](const QModelIndex &cur, const QModelIndex&) { previewFile(cur); });
            connect(currentView, &QAbstractItemView::activated, this, [this](const QModelIndex &idx){
                if (!idx.isValid()) return;
                if (model->isDir(idx)) { pushHistory(); currentRoot = idx; setViewMode(mode); }
                else { openFile(idx); }
            });
            connect(currentView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &idx){
                if (!idx.isValid()) return;
                if (model->isDir(idx)) { pushHistory(); currentRoot = idx; setViewMode(mode); }
                else { openFile(idx); }
            });
        }

    }
};

QStringList ColFM::selectItems() {
    QStringList paths;
    if (!currentView || !model) return paths;

    // Prefer explicit multi-selection; first column only to avoid duplicate rows
    if (currentView->selectionModel()) {
        const QModelIndexList rows = currentView->selectionModel()->selectedRows(0);
        for (const QModelIndex &idx : rows) {
            if (idx.isValid()) paths << model->filePath(idx);
        }
    }

    // Fallback: current index, else item under mouse
    if (paths.isEmpty()) {
        QModelIndex idx = currentView->currentIndex();
        if (!idx.isValid()) {
            const QPoint p = currentView->mapFromGlobal(QCursor::pos());
            idx = currentView->indexAt(p);
        }
        if (idx.isValid()) paths << model->filePath(idx);
    }

    return paths;
}



// function to return our CWD anywhere
QString ColFM::getCWD() {
    QModelIndex idx = currentIndex();
    if (!idx.isValid() && currentView) {
        QPoint vp = currentView->viewport()->mapFromGlobal(QCursor::pos());
        idx = currentView->indexAt(vp);
    }
    if (!idx.isValid()) return QDir::homePath();
    return model->filePath(idx);
}

/* ============================================================================================ */

void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid() || !model) return;
    const QString path = model->filePath(idx);

    if (mode == ViewMode::Column) {
        if (infoPanel) { infoPanel->setFile(path); return; }
        if (previewLabel) { previewLabel->setText(path); previewLabel->setToolTip(path); return; }
    }
    if (statusBar()) statusBar()->showMessage(path, 1500);
}

void ColFM::onGetInfo(){ QModelIndex idx=currentIndex(); if(idx.isValid()) previewFile(idx); }

#include "helpers/rename.h"
#include "helpers/duplicate.h"
#include "helpers/keys.h"
#include "helpers/toolbars.h"
#include "helpers/handleopen.h"
#include "helpers/settings.h"
#include "helpers/progress.h"
#include "helpers/trash.h"
#include "helpers/link.h"
#include "helpers/new.h"
#include "helpers/home.h"
#include "helpers/new_terminal.h"
#include "helpers/folderize.h"
#include "helpers/archivezip.h"
#include "helpers/search.h"
#include "helpers/appicon.h"

void ColFM::contextMenuEvent(QContextMenuEvent *event) {
    if (!currentView || !model) return;
    const QPoint viewPos = currentView->viewport()->mapFromGlobal(event->globalPos());
    const QModelIndex idx = currentView->indexAt(viewPos);
    if (!idx.isValid()) return;
    ::showContextMenu(currentView, idx, model, event->globalPos());
}

bool ColFM::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        if (handleKeyEvent(this, static_cast<QKeyEvent*>(event)))
            return true;
    }
    return QObject::eventFilter(obj, event);
}

inline QString resolveStartPath(const QStringList &args) {
    // Default: current working directory
    QString path = QDir::homePath();

    if (args.size() > 1) {
        QFileInfo fi(args.at(1));
        if (fi.exists()) {
            if (fi.isDir()) {
                path = fi.absoluteFilePath();
            } else {
                path = fi.absolutePath();  // if it's a file, use parent dir
            }
        }
    }
    return path;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(new ForceIconStyle(app.style()));
    //app.setWindowIcon(QIcon("icons/app_icon.png"));
    app.setWindowIcon(IconsData::getIcon("app_icon.png"));

    const QString startPath = resolveStartPath(app.arguments());
    setCWD(startPath);                      // <- tells UI which folder to show

    ColFM w;
    w.show();   // disabled for smoke test

    const QString desktopExe = QCoreApplication::applicationDirPath() + "/colfm_desktop";
    int running = QProcess::execute("pgrep", {"-u", QString::number(getuid()), "colfm_desktop"});
    if (running != 0) QProcess::startDetached(desktopExe, {});
    return app.exec();
}

// Step 2 — free helper to invoke the Move… dialog without adding class members
void ColFM::onMoveButton() {
    Move::promptAndMove(selectItems(), this);
}
