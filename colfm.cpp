#include <QApplication>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QColumnView>
#include <QListView>
#include <QTreeView>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QTreeView>
#include <QListView>
#include <QColumnView>
#include <QAbstractItemView>
#include <QSplitter>
#include <QDir>
#include <QIcon>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>          
#include <QHeaderView>
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
#include "info.h"
#include "breadcrumbs.h"

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
class CustomIconProvider : public QFileIconProvider {
public:
    using QFileIconProvider::QFileIconProvider;
    QIcon icon(const QFileInfo &info) const override {
        QPixmap pix = QFileIconProvider::icon(info).pixmap(kIconSize);
        QImage img = pix.toImage();

        if (info.isSymLink()) {
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 0) c.setRgb((c.red()+0)/2, (c.green()+180)/2, (c.blue()+180)/2, c.alpha());
                    img.setPixelColor(x, y, c);
                }
            return QIcon(QPixmap::fromImage(img));
        }
        if (info.isExecutable() && !info.isDir()) {
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 0) c.setRgb((c.red()+128)/2, (c.green()+255)/2, (c.blue()+128)/2, c.alpha());
                    img.setPixelColor(x, y, c);
                }
            return QIcon(QPixmap::fromImage(img));
        }
        return QFileIconProvider::icon(info);
    }
};


/* Column view: force 32px for spawned columns */
class ColumnView32 : public QColumnView {
public:
    using QColumnView::QColumnView;
protected:
    QAbstractItemView* createColumn(const QModelIndex &rootIndex) override {
        QAbstractItemView *v = QColumnView::createColumn(rootIndex);
        if (v) {
            v->setIconSize(kIconSize);
            v->setItemDelegate(new FixedIconDelegate(v));
        }
        return v;
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
    QSize iconSz = kIconSize;                                        /* added */
};

class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent=nullptr) : QMainWindow(parent) {
        model = new FixedFSModel(this);
        model->setIconProvider(new CustomIconProvider());
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        currentRoot = model->setRootPath(QDir::homePath());

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
	addIconSizePopup();
        addToolBar(Qt::TopToolBarArea, crumbs);

        setViewMode(ViewMode::Tree);
        setWindowTitle("ColFM — Multi-View File Manager");
	//qApp->installEventFilter(this);
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
    }

public:
    void drawButtons();
    void addIconSizePopup();
    void onRefresh();

    void onMoveToTrash();
    void onEmptyTrash();
    void onOpenTrash();
    void onRestoreFromTrash();

    void onUp();
    void onOpen();
    void onCloseAction();
    void onInfo();
    void onGetInfo();
    void onMove();
    void onDuplicate();
void onGoHome();
    void onCreateSoftlink();
    void onToggleHidden();
    void onSettings(); 
void writePrefs(int folderMB, int iconSize, int viewMode);
void readPrefs(int &folderMB, int &iconSize, int &viewMode);
void onProgress(QObject *target, QColor colour, QColor background, int width, int height, bool bevel);
    void onRename(); void onRenameSelected(const QString &path);
    void onViewTree();
    void onViewColumn();
    void onViewIcon();
    void onSearchPlocate();

    QModelIndex currentIndex() const;
    void previewFile(const QModelIndex &idx);
    void openFile(const QModelIndex &idx);
    void openApp(const QString &path);
    void openSelected();
    void openPath(const QString &absPath);

void onNewFolder();
void onNewWindow();
void onNewTerminal();

    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QFileSystemModel *model{};
    ViewMode mode = ViewMode::Tree;
    QModelIndex currentRoot;
    QLabel *previewLabel{};
    bool showHidden = false;

    colfm::InfoWidget *infoPanel{};

    Breadcrumbs *crumbs{};
    QToolBar *tb{};
    QAction *actTrash{}, *actRefresh{}, *actOpenTrash{}, *actUp{};
    QAction *actRestoreFromTrash{};
    QAction *actOpen{}, *actClose{}, *actInfo{}, *actRename{}, *actMove{}, *actDuplicate{}, *actLink{};
    QAction *treeBtn{}, *columnBtn{}, *iconBtn{}, *toggleHiddenBtn{}, *settingsBtn{};
    QAction *actEmptyTrash{};
    QAction *actGoHome{};
    QAction *actNewFolder{}, *actNewWindow{};
QAction *actNewTerminal{};
    QAction *actSearch{};
    QAbstractItemView *currentView{};

    /* added: sidebar + simple back history */
    QTreeWidget *sidebar{};                     /* added */
    QStringList backStack;                      /* added */

    #include "viewwidgets.h"

    QString getCWD();

    void pushHistory() {                         /* added */
        backStack.prepend(model->filePath(currentRoot));
        if (backStack.size() > 50) backStack.removeLast();
    }

    /* ---------------------- Sidebar Builder (single column) ---------------------- */
    void populateSidebar() {                     /* added */
        if (!sidebar) return;

        sidebar->clear();
        sidebar->setColumnCount(1);                          /* replaced: was 2 */
        sidebar->setHeaderHidden(true);
        sidebar->setRootIsDecorated(false);
        sidebar->setSelectionMode(QAbstractItemView::SingleSelection);
        sidebar->setFocusPolicy(Qt::NoFocus);
        sidebar->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        sidebar->setFixedWidth(300);
        sidebar->setAlternatingRowColors(false);
        // optional dark bg:
        // sidebar->setStyleSheet("QTreeWidget{background:#1e1e1e;}");

        /* helper: one row widget per item with icon + text + (optional) eject button */
        auto addItem = [&](const QString &label, const QString &path, bool ejectable, bool isDrive){
            auto *it = new QTreeWidgetItem(sidebar);
            it->setData(0, Qt::UserRole, path);
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

            QWidget *row = new QWidget(sidebar);
            auto *h = new QHBoxLayout(row);
            h->setContentsMargins(8, 6, 8, 6);          // extra leading (top/bottom) and side padding
            h->setSpacing(6);                            // a bit more kerning

            // icon (16x16)
            QLabel *ic = new QLabel(row);
            QPixmap pm;
            if (isDrive) pm = QPixmap("icons/disk.png");      // user-supplied
	    else if (path.endsWith("/.local/share/Trash/files")) pm = QPixmap("icons/open_trash.png");
            else         pm = style()->standardIcon(QStyle::SP_DirIcon).pixmap(16,16); // folder
            ic->setPixmap(pm.scaled(16,16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            h->addWidget(ic, 0, Qt::AlignVCenter);

            // label
            QLabel *lab = new QLabel(label, row);
            lab->setStyleSheet("color:#ddd; font-size:14px; letter-spacing:0.4px;");
            h->addWidget(lab, 1, Qt::AlignVCenter);

            if (ejectable) {
                QToolButton *ej = new QToolButton(row);
                ej->setIcon(QIcon("icons/eject.png"));       // white on black PNG (user provides)
                ej->setIconSize(QSize(18,18));
                ej->setAutoRaise(true);
                ej->setStyleSheet("QToolButton { background:#000; border:none; padding:6px; }"); // padding => visible
                ej->setToolTip("Eject");
                ej->setProperty("mount", path);
                h->addWidget(ej, 0, Qt::AlignRight | Qt::AlignVCenter);

                QObject::connect(ej, &QToolButton::clicked, this, [this, ej]{
                    const QString mp = ej->property("mount").toString();
                    statusBar()->showMessage(QString("Eject %1 (not implemented)").arg(mp), 1500);
                });
            }

            sidebar->setItemWidget(it, 0, row);
            it->setSizeHint(0, QSize(100, row->sizeHint().height() + 4)); // a little extra line height
            return it;
        };

        const QString home = QDir::homePath();
        // Always show Desktop, Downloads, Trash first
        addItem("Home",  home + "./", false, false);
        addItem("Desktop",  home + "/Desktop", false, false);
        addItem("Downloads",home + "/Downloads", false, false);

	// trash icon
        //addItem("Trash",    home + "/.local/share/Trash/files", false, false);
        QTreeWidgetItem *trashItem = addItem("Trash", home + "/.local/share/Trash/files", false, false);
	QObject::connect(sidebar, &QTreeWidget::itemClicked, this, [this, trashItem](QTreeWidgetItem *clicked, int){
	    if (clicked == trashItem) {
		onOpenTrash();
	    }
	});

	// Divider (simple spacer row)
        auto *div = new QTreeWidgetItem(sidebar);
        div->setFlags(Qt::NoItemFlags);
        div->setFirstColumnSpanned(true);
        sidebar->setItemWidget(div, 0, new QLabel("────────", sidebar));

        // Visible (non-dot) folders in ~
        QDir hd(home);
        for (const QFileInfo &fi : hd.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            if (fi.fileName().startsWith('.')) continue;
            if (fi.fileName() == "Desktop" || fi.fileName() == "Downloads") continue;
            addItem(fi.fileName(), fi.absoluteFilePath(), false, false);
        }

	// Divider (simple spacer row)
	auto *div2 = new QTreeWidgetItem(sidebar);
	div2->setFlags(Qt::NoItemFlags);
	div2->setFirstColumnSpanned(true);
	sidebar->setItemWidget(div2, 0, new QLabel("────────", sidebar));

        // Drives under /media/$USER
        QString user = QFileInfo(home).fileName();
        QDir md(QString("/media/%1").arg(user));
        QStringList drives;
        if (md.exists())
            for (const QFileInfo &fi : md.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
                drives << fi.absoluteFilePath();

        if (drives.isEmpty()) {
            addItem("/ root", "/", false, true);
        } else {
            for (const QString &mp : drives) {
                addItem(QFileInfo(mp).fileName(), mp, true, true); // ejectable drive
            }
        }

        // (no setColumnWidth calls — single column now)
    }

    /* builds outer layout with left sidebar + current view widget */
    QWidget* buildWithSidebar(QWidget *center) { /* added */
        auto *outer = new QSplitter(Qt::Horizontal);
        sidebar = new QTreeWidget(outer);
        populateSidebar();
        outer->addWidget(sidebar);
        outer->addWidget(center);
        outer->setStretchFactor(0, 0);
        outer->setStretchFactor(1, 1);
        outer->setCollapsible(0, false);
        outer->setCollapsible(1, false);

        // Navigation from sidebar (single column)
        connect(sidebar, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *it, int){
            if (!it) return;
            const QString path = it->data(0, Qt::UserRole).toString();
            if (!path.isEmpty()) { pushHistory(); currentRoot = model->index(path); setViewMode(mode); }
        });
        connect(sidebar, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *it, int){
            if (!it) return;
            const QString path = it->data(0, Qt::UserRole).toString();
            if (!path.isEmpty()) { pushHistory(); currentRoot = model->index(path); setViewMode(mode); }
        });
        return outer;
    }

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

/* Requires: #include <QMenu> once at the top of colfm.cpp */
void ColFM::addIconSizePopup() {
    QAction *insertBefore = nullptr;
    const auto acts = tb->actions();
    int pos = acts.indexOf(toggleHiddenBtn);
    if (pos >= 0 && pos + 1 < acts.size()) insertBefore = acts.at(pos + 1);

    QToolButton *btn = new QToolButton(tb);
    btn->setToolTip("Icon size");
    btn->setText("Size");
    btn->setPopupMode(QToolButton::InstantPopup);

    QMenu *m = new QMenu(btn);
    btn->setMenu(m);

    auto addSizeAction = [&](int s){
        QAction *a = m->addAction(QString::number(s));
        a->setCheckable(true);
        if (s == 32) a->setChecked(true);
        QObject::connect(a, &QAction::triggered, this, [this, m, s]{
            // uncheck/check
            for (QAction *x : m->actions()) x->setChecked(false);
            for (QAction *x : m->actions()) if (x->text().toInt() == s) { x->setChecked(true); break; }

            const QSize newSz(s, s);

            // 1) tell the model to use the new decoration size
            if (auto f = dynamic_cast<FixedFSModel*>(model)) {       /* added */
                f->setIconSize(newSz);
            }

            // 2) rebuild the center so column subviews pick up fresh settings
            onRefresh();                                             /* added */

            // 3) apply runtime delegate + icon size to all current views
            class LocalAdjDelegate : public QStyledItemDelegate {
            public:
                explicit LocalAdjDelegate(const QSize &sz, QObject *parent=nullptr)
                    : QStyledItemDelegate(parent), dec(sz) {}
                void initStyleOption(QStyleOptionViewItem *opt, const QModelIndex &idx) const override {
                    QStyledItemDelegate::initStyleOption(opt, idx);
                    opt->decorationSize = dec;
                }
            private:
                QSize dec;
            };

            if (auto root = this->centralWidget()) {
                const auto views = root->findChildren<QAbstractItemView*>();
                for (QAbstractItemView *v : views) {
                    v->setIconSize(newSz);
                    v->setItemDelegate(new LocalAdjDelegate(newSz, v));
                    v->viewport()->update();
                }
            }

            if (statusBar()) statusBar()->showMessage(QString("Icon size: %1").arg(s), 1200);
        });
    };

    addSizeAction(16);
    addSizeAction(24);
    addSizeAction(32);
    addSizeAction(48);
    addSizeAction(64);
    addSizeAction(128);

    if (insertBefore) tb->insertWidget(insertBefore, btn);
    else              tb->addWidget(btn);
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

#include "rename.h"
#include "duplicate.h"
#include "keys.h"
#include "toolbars.h"
#include "handleopen.h"
#include "settings.h"
#include "progress.h"
#include "trash.h"
#include "link.h"
#include "new.h"
#include "home.h"
#include "new_terminal.h"

bool ColFM::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        if (handleKeyEvent(this, static_cast<QKeyEvent*>(event)))
            return true;
    }
    return QObject::eventFilter(obj, event);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(new ForceIconStyle(app.style()));
    app.setWindowIcon(QIcon("icons/app_icon.png"));
    ColFM w; w.show();
    return app.exec();
}
