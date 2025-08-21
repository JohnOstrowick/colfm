/* colfm.cpp — sidebar cleaned up (single column), bigger row spacing, icons, padded eject button.
 * Minimal edits only; anything replaced is commented with  /* replaced *\/ .
 */

#include <QApplication>
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
#include <QHBoxLayout>          /* added */
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
#include <QTreeWidget>          /* added */
#include <QTreeWidgetItem>      /* added */
#include <QToolButton>          /* added */
#include <functional>

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

/* Clamp DecorationRole to 32×32 */
class FixedFSModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;
    QVariant data(const QModelIndex &index, int role) const override {
        if (role == Qt::DecorationRole) {
            QVariant v = QFileSystemModel::data(index, role);
            QPixmap pm;
            if (v.canConvert<QIcon>())      pm = qvariant_cast<QIcon>(v).pixmap(kIconSize);
            else if (v.canConvert<QPixmap>()) pm = qvariant_cast<QPixmap>(v);
            if (!pm.isNull() && pm.size() != kIconSize)
                pm = pm.scaled(kIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            if (!pm.isNull()) return pm;
            return v;
        }
        return QFileSystemModel::data(index, role);
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

class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent=nullptr) : QMainWindow(parent) {
        model = new FixedFSModel(this);
        model->setIconProvider(new CustomIconProvider());
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        currentRoot = model->setRootPath(QDir::homePath());

        crumbs = new Breadcrumbs("Path", this);
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
        resize(1400, 800);
    }

public:
    void drawButtons();
    void onMoveToTrash();
    void onRefresh();
    void onOpenTrash();
    void onRestoreFromTrash();
    void onUp();
    void onOpen();
    void onCloseAction();
    void onInfo();
    void onRename();
    void onMove();
    void onDuplicate();
    void onCreateSoftlink();
    void onToggleHidden();
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
    QAction *treeBtn{}, *columnBtn{}, *iconBtn{}, *toggleHiddenBtn{};
    QAction *actEmptyTrash{};
    QAction *actSearch{};
    QAbstractItemView *currentView{};

    /* added: sidebar + simple back history */
    QTreeWidget *sidebar{};                     /* added */
    QStringList backStack;                      /* added */

    #include "viewwidgets.h"

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
        addItem("Desktop",  home + "/Desktop", false, false);
        addItem("Downloads",home + "/Downloads", false, false);
        addItem("Trash",    home + "/.local/share/Trash/files", false, false);

        // Visible (non-dot) folders in ~
        QDir hd(home);
        for (const QFileInfo &fi : hd.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            if (fi.fileName().startsWith('.')) continue;
            if (fi.fileName() == "Desktop" || fi.fileName() == "Downloads") continue;
            addItem(fi.fileName(), fi.absoluteFilePath(), false, false);
        }

        // Divider (simple spacer row)
        auto *div = new QTreeWidgetItem(sidebar);
        div->setFlags(Qt::NoItemFlags);
        div->setFirstColumnSpanned(true);
        sidebar->setItemWidget(div, 0, new QLabel("────────", sidebar));

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

void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid() || !model) return;
    const QString path = model->filePath(idx);

    if (mode == ViewMode::Column) {
        if (infoPanel) { infoPanel->setFile(path); return; }
        if (previewLabel) { previewLabel->setText(path); previewLabel->setToolTip(path); return; }
    }
    if (statusBar()) statusBar()->showMessage(path, 1500);
}

#include "toolbars.h"
#include "handleopen.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(new ForceIconStyle(app.style()));
    app.setWindowIcon(QIcon("icons/app_icon.png"));
    ColFM w; w.show();
    return app.exec();
}
