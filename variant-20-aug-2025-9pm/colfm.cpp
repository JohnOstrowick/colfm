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

// Force app-wide 32 px icon metrics
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

// Fixed-size icon delegate for views
class FixedIconDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
        QSize s = QStyledItemDelegate::sizeHint(opt, idx);
        if (s.width() < 32) s.setWidth(32);
        if (s.height() < 32) s.setHeight(32);
        return s;
    }
};

// Custom icon provider (placeholder)
class CustomIconProvider : public QFileIconProvider {
public:
    using QFileIconProvider::QFileIconProvider;
    QIcon icon(IconType type) const override {
        return QFileIconProvider::icon(type);
    }
    QIcon icon(const QFileInfo &info) const override {
        return QFileIconProvider::icon(info);
    }
};

// FS model with fixed roles
class FixedFSModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;
    QVariant data(const QModelIndex &idx, int role) const override {
        return QFileSystemModel::data(idx, role);
    }
};

// ColumnView subclass that forces 32x32 icons and delegate for every spawned column
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

#include "breadcrumbs.h"

class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent=nullptr) : QMainWindow(parent) {
        model = new FixedFSModel(this);
        model->setIconProvider(new CustomIconProvider());
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot); // dotfiles hidden by default
        currentRoot = model->setRootPath(QDir::homePath());

        // Breadcrumbs toolbar (above main toolbar)
        crumbs = new Breadcrumbs("Path", this);
        //addToolBar(Qt::TopToolBarArea, crumbs);
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

    // helpers declared here; bodies provided elsewhere
    QModelIndex currentIndex() const;
    void previewFile(const QModelIndex &idx);
    void openFile(const QModelIndex &idx);
    void openApp(const QString &path);
    void openSelected();
    void openPath(const QString &absPath);

    // event filter implemented inline in toolbars.h
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    FixedFSModel *model{};
    ViewMode mode = ViewMode::Tree;
    QModelIndex currentRoot;
    QLabel *previewLabel{};   // temporary (column preview text)
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

#include "viewwidgets.h"

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

// minimal preview hook; column view uses right-hand label for now
// this stops LD from crashing
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


// ---- helper method DEFINITIONS (moved outside the class) ----
/*QModelIndex ColFM::currentIndex() const {
    return currentView ? currentView->currentIndex() : QModelIndex();
}
void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    if (!previewLabel) return;
    previewLabel->setText(model->filePath(idx));
}
void ColFM::openFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    qDebug() << "Open file:" << model->filePath(idx);
}
*/

// include inline toolbar definitions AFTER the class (and after helper defs)
#include "toolbars.h"
#include "handleopen.h"
//#include "info.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(new ForceIconStyle(app.style()));
    app.setWindowIcon(QIcon("icons/app_icon.png")); // TODO: if not shown, keep as bug
    ColFM w; w.show();
    return app.exec();
}
