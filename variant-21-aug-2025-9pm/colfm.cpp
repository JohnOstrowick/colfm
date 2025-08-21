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
#include <functional>

// -------- Settings --------
static const QSize kIconSize(32, 32);

enum class ViewMode { Tree, Column, Icon };

// Force app-wide 32 px icon metrics (kept from earlier revs)
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

// Force the decoration (painted icon) to a fixed size — needed for consistent 32px
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

// Custom icon provider: tint symlinks teal, executables light green, and start from a 32px pixmap
class CustomIconProvider : public QFileIconProvider {
public:
    using QFileIconProvider::QFileIconProvider;
    QIcon icon(const QFileInfo &info) const override {
        // Start from the platform icon but force a 32px pixmap so overlays don’t end up 16px
        QPixmap pix = QFileIconProvider::icon(info).pixmap(kIconSize);
        QImage img = pix.toImage();

        if (info.isSymLink()) {
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 0) {
                        c.setRgb((c.red()+0)/2, (c.green()+180)/2, (c.blue()+180)/2, c.alpha());
                        img.setPixelColor(x, y, c);
                    }
                }
            return QIcon(QPixmap::fromImage(img));
        }

        if (info.isExecutable() && !info.isDir()) {
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 0) {
                        c.setRgb((c.red()+128)/2, (c.green()+255)/2, (c.blue()+128)/2, c.alpha());
                        img.setPixelColor(x, y, c);
                    }
                }
            return QIcon(QPixmap::fromImage(img));
        }

        return QFileIconProvider::icon(info);
    }
};

// QFileSystemModel that guarantees 32x32 decoration pixmaps (fixes symlink size regressions)
class FixedFSModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;
    QVariant data(const QModelIndex &index, int role) const override {
        if (role == Qt::DecorationRole) {
            QVariant v = QFileSystemModel::data(index, role);
            QPixmap pm;
            if (v.canConvert<QIcon>()) {
                pm = qvariant_cast<QIcon>(v).pixmap(kIconSize);
            } else if (v.canConvert<QPixmap>()) {
                pm = qvariant_cast<QPixmap>(v);
            }
            if (!pm.isNull() && pm.size() != kIconSize) {
                pm = pm.scaled(kIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            if (!pm.isNull()) return pm;
            return v;
        }
        return QFileSystemModel::data(index, role);
    }
};

// ---- New: Breadcrumbs toolbar (editable)
#include "breadcrumbs.h"

class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent=nullptr) : QMainWindow(parent) {
        // OLD:
        // model = new QFileSystemModel(this);
        // NEW: use FixedFSModel to clamp DecorationRole to 32px
        model = new FixedFSModel(this);
        model->setIconProvider(new CustomIconProvider());  // ensure symlink/exec tinting + 32px base
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot); // dotfiles hidden by default
        currentRoot = model->setRootPath(QDir::homePath());

        // Breadcrumbs toolbar (above main toolbar)
        crumbs = new Breadcrumbs("Path", this);
        crumbs->setOnPathChosen([this](const QString &p){
            if (QDir(p).exists()) {
                currentRoot = model->index(p);
                setViewMode(mode);
            } else {
                statusBar()->showMessage("Path not found", 2000);
            }
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

    // ---- toolbar slots (declared; bodies in toolbars.h) ----
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

    // helpers declared here; bodies provided elsewhere (handleopen.h for open*)
    QModelIndex currentIndex() const;
    void previewFile(const QModelIndex &idx);
    void openFile(const QModelIndex &idx);
    void openApp(const QString &path);
    void openSelected();
    void openPath(const QString &absPath);

    // event filter implemented inline in toolbars.h
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    // NOTE: keep as QFileSystemModel* so we don’t need FixedFSModel forward-declare here
    QFileSystemModel *model{};
    ViewMode mode = ViewMode::Tree;
    QModelIndex currentRoot;
    QLabel *previewLabel{};   // right pane text label (column view); keep
    bool showHidden = false;

    Breadcrumbs *crumbs{};
    QToolBar *tb{};
    QAction *actTrash{}, *actRefresh{}, *actOpenTrash{}, *actUp{};
    QAction *actRestoreFromTrash{};
    QAction *actOpen{}, *actClose{}, *actInfo{}, *actRename{}, *actMove{}, *actDuplicate{}, *actLink{};
    QAction *treeBtn{}, *columnBtn{}, *iconBtn{}, *toggleHiddenBtn{};
    QAction *actEmptyTrash{};
    QAction *actSearch{};
    QAbstractItemView *currentView{};

#include "viewwidgets.h"  // builds Tree/Icon/Column widgets; uses kIconSize via delegate

    void setViewMode(ViewMode m) {
        mode = m;
        QWidget *old = centralWidget();
        if (old) old->deleteLater();

        QWidget *w = nullptr;
        QModelIndex root = currentRoot.isValid() ? currentRoot : model->index(QDir::homePath());

        switch (mode) {
            case ViewMode::Tree:   w = buildTreeWidget(root);   break;
            case ViewMode::Column: w = buildColumnWidget(root); break;
            case ViewMode::Icon:   w = buildIconWidget(root);   break;
        }
        setCentralWidget(w);

        if (crumbs) crumbs->setPath(model->filePath(currentRoot));
    }
};

// ---- helper method DEFINITIONS kept minimal (others live in headers) ----
void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid() || !model) return;
    const QString path = model->filePath(idx);
    if (mode == ViewMode::Column) {
        if (previewLabel) {
            previewLabel->setText(path);
            previewLabel->setToolTip(path);
        }
    } else {
        if (statusBar()) statusBar()->showMessage(path, 1500);
    }
}

// include inline toolbar definitions AFTER the class (and after helper defs)
#include "toolbars.h"
#include "handleopen.h"
//#include "info.h" // included from toolbars.h when needed

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(new ForceIconStyle(app.style()));
    app.setWindowIcon(QIcon("icons/app_icon.png")); // TODO: if not shown, keep as bug
    ColFM w; w.show();
    return app.exec();
}
