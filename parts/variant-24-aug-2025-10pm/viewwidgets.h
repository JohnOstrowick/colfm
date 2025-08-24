#pragma once
#include <QTreeView>
#include <QListView>
#include <QColumnView>
#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPalette>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QAbstractItemView>
#include <QProxyStyle>
#include <QColor>
//#include "info.h" /* commented out: avoid including inside class to prevent Qt types being seen as ColFM::X (kak.txt) */

/* NOTE:
 * This header is included INSIDE class ColFM in colfm.cpp.
 * The following inline functions are therefore ColFM members and may access:
 *   model, crumbs, currentView, previewLabel, currentRoot, mode, kIconSize
 *
 * FixedIconDelegate and ColumnView32 are defined in colfm.cpp (global scope).
 * To avoid duplicate definitions, we DO NOT define them here.
 */

/* commented out duplicate definitions — now provided in colfm.cpp
// ---- Delegate that enforces 32×32 decoration size --------------------------
class FixedIconDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        option->decorationSize = kIconSize;
    }
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
        QSize s = QStyledItemDelegate::sizeHint(opt, idx);
        if (s.width()  < kIconSize.width())  s.setWidth(kIconSize.width());
        if (s.height() < kIconSize.height()) s.setHeight(kIconSize.height());
        return s;
    }
};

// ---- Column view subclass: apply 32×32 to every spawned column -------------
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
*/

/* added: helper to ensure we always have a valid root index */
inline QModelIndex ensureRootIndex(const QModelIndex &root) {
    if (root.isValid()) return root;
    return model->index(QDir::homePath());
}

/* ------------------------------ Tree (list) view ------------------------------ */
inline QWidget* buildTreeWidget(const QModelIndex &rootIdx) {
    auto *view = new QTreeView();

    view->setModel(model);
    const QModelIndex root = ensureRootIndex(rootIdx);            /* added */
    view->setRootIndex(root);
    if (crumbs) crumbs->setPath(model->filePath(root));

    view->setHeaderHidden(false);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);

    /* added: force 32×32 rendering */
    view->setIconSize(kIconSize);
    view->setItemDelegate(new FixedIconDelegate(view));

    currentView = view;

    view->header()->setSectionResizeMode(QHeaderView::Interactive);
    view->header()->setStretchLastSection(false);
    view->setColumnWidth(0, 600);

    /* added: double-click navigation and preview */
    connect(view, &QTreeView::doubleClicked, this, [this, view](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            view->setRootIndex(idx);
            currentRoot = idx;
            if (crumbs) crumbs->setPath(model->filePath(idx));
        } else {
            previewFile(idx);
        }
    });

    return view;
}

/* ------------------------------ Column view ------------------------------ */
inline QWidget* buildColumnWidget(const QModelIndex &rootIdx) {
    auto *splitter = new QSplitter(Qt::Horizontal);

    auto *cv = new ColumnView32();                 /* uses definition from colfm.cpp */
    cv->setModel(model);
    const QModelIndex root = ensureRootIndex(rootIdx);            /* added */
    cv->setRootIndex(root);

    /* added: ensure 32×32 on the initial visible column */
    cv->setIconSize(kIconSize);
    cv->setItemDelegate(new FixedIconDelegate(cv));

    currentView = cv;

    /* right-side preview pane: keep label (Info panel can replace later) */
    QWidget *previewPane = new QWidget();
    QPalette pal = previewPane->palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 30));
    previewPane->setAutoFillBackground(true);
    previewPane->setPalette(pal);

    if (!previewLabel) previewLabel = new QLabel("Preview");
    previewLabel->setStyleSheet("QLabel { color: white; padding: 8px; }");
    auto *previewLayout = new QVBoxLayout(previewPane);
    previewLayout->setContentsMargins(0,0,0,0);
    previewLayout->addWidget(previewLabel);

    splitter->addWidget(cv);
    splitter->addWidget(previewPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    /* added: update preview + breadcrumbs on click */
    connect(cv, &QColumnView::clicked, this, [this, cv](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            cv->setCurrentIndex(idx);
            currentRoot = idx;
            if (crumbs) crumbs->setPath(model->filePath(idx));
        }
        previewFile(idx);
    });

    return splitter;
}

/* ------------------------------ Icon (grid) view ------------------------------ */
inline QWidget* buildIconWidget(const QModelIndex &rootIdx) {
    auto *view = new QListView();

    view->setViewMode(QListView::IconMode);
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setUniformItemSizes(false);

    view->setModel(model);
    const QModelIndex root = ensureRootIndex(rootIdx);            /* added */
    view->setRootIndex(root);
    if (crumbs) crumbs->setPath(model->filePath(root));

    /* added: enforce 32×32 */
    view->setIconSize(kIconSize);
    view->setItemDelegate(new FixedIconDelegate(view));

    currentView = view;

    /* added: double-click navigation and preview */
    connect(view, &QListView::doubleClicked, this, [this, view](const QModelIndex &idx){
        if (!idx.isValid()) return;
        if (model->isDir(idx)) {
            view->setRootIndex(idx);
            currentRoot = idx;
            if (crumbs) crumbs->setPath(model->filePath(idx));
        } else {
            previewFile(idx);
        }
    });

    return view;
}
