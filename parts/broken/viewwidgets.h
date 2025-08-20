#pragma once
#include <QTreeView>
#include <QListView>
#include <QColumnView>
#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPalette>

// Out-of-class definitions for ColFM view builders
class FixedIconDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index)
 const override {
        QStyledItemDelegate::initStyleOption(option, index);
        option->decorationSize = kIconSize;
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index)
 const override {
        QStyleOptionViewItem opt(option);
        opt.decorationSize = kIconSize;
        return QStyledItemDelegate::sizeHint(opt, index);
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

inline QWidget* ColFM::buildTreeWidget(const QModelIndex &root) {
    auto *view = new QTreeView();
    view->setModel(model);
    view->setRootIndex(root);
    view->setHeaderHidden(false);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setIconSize(kIconSize);
    view->setItemDelegate(new FixedIconDelegate(view));

    view->header()->setSectionResizeMode(QHeaderView::Interactive);
    view->header()->setStretchLastSection(false);
    view->setColumnWidth(0, 600);

	QObject::connect(view, &QTreeView::doubleClicked, this, [this, view](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            view->setRootIndex(idx);
            currentRoot = idx;
            if (crumbs) crumbs->setPath(model->filePath(idx));
        } else {
            openFile(idx);
        }
    });

    return view;
}

inline QWidget* ColFM::buildColumnWidget(const QModelIndex &root) {
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    auto *cv = new ColumnView32(splitter);
    cv->setModel(model);
    cv->setRootIndex(root);
    cv->setIconSize(kIconSize);
    cv->setResizeGripsVisible(true);
    cv->setSelectionBehavior(QAbstractItemView::SelectRows);
    cv->setColumnWidths({400,400,400});
    cv->setItemDelegate(new FixedIconDelegate(cv));

    QObject::connect(cv, &QColumnView::clicked, this, [this](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            if (crumbs) crumbs->setPath(model->filePath(idx)); // append/update path
        } else {
            previewFile(idx);
        }
    });
    QObject::connect(cv, &QColumnView::doubleClicked, this, [this, cv](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            cv->setRootIndex(idx);
            currentRoot = idx;
            if (crumbs) crumbs->setPath(model->filePath(idx));
        } else {
            openFile(idx);
        }
    });

    QWidget *previewPane = new QWidget();
    QPalette pal = previewPane->palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 30));
    previewPane->setAutoFillBackground(true);
    previewPane->setPalette(pal);
    previewLabel = new QLabel("Preview");
    previewLabel->setStyleSheet("QLabel { color: white; padding: 8px; }");
    auto *previewLayout = new QVBoxLayout(previewPane);
    previewLayout->setContentsMargins(0,0,0,0);
    previewLayout->addWidget(previewLabel);

    splitter->addWidget(cv);
    splitter->addWidget(previewPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    return splitter;
}

inline QWidget* ColFM::buildIconWidget(const QModelIndex &root) {
    auto *view = new QListView();
    view->setModel(model);
    view->setRootIndex(root);
    view->setViewMode(QListView::IconMode);
    view->setIconSize(kIconSize);
    view->setItemDelegate(new FixedIconDelegate(view));
    view->setGridSize(QSize(64,64));
    view->setSpacing(8);
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setUniformItemSizes(true);

    QObject::connect(view, &QListView::doubleClicked, this, [this, view](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            view->setRootIndex(idx);
            currentRoot = idx;
            if (crumbs) crumbs->setPath(model->filePath(idx));
        } else {
            openFile(idx);
        }
    });

    return view;
}
