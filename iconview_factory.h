// iconview_factory.h  (standalone, no app-specific symbols)
#pragma once
#include <QListView>
#include <QAbstractItemModel>

inline QListView* makeIconView(QWidget *parent, QAbstractItemModel *model) {
    auto *view = new QListView(parent);
    view->setModel(model);
    view->setViewMode(QListView::IconMode);
    view->setLayoutDirection(Qt::LeftToRight);
    view->setSelectionBehavior(QAbstractItemView::SelectItems);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setIconSize(QSize(48,48));
    view->setWordWrap(true);
    view->setTextElideMode(Qt::ElideNone);
    // columnised label under icon (~3 lines)
    const int textLines = 3, padW = 60, padH = 24;
    view->setGridSize(QSize(view->iconSize().width() + padW,
                            view->iconSize().height() + view->fontMetrics().lineSpacing()*textLines + padH));
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setWrapping(true);
    view->setSpacing(8);
    view->setUniformItemSizes(false);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    Drag::enableOn(view);
    return view;
}
