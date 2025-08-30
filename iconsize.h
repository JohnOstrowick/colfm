#ifndef ICONSIZE_H
#define ICONSIZE_H

#include <QToolButton>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QAbstractItemView>
#include <QStatusBar>
#include <QMainWindow>
#include <QSize>
#include <QString>
#include <QFileSystemModel>
#include <functional>

inline void addIconSizePopup(QMainWindow *win, QToolBar *tb, QAction *toggleHiddenBtn, QFileSystemModel *model, std::function<void()> onRefresh) {
    QAction *insertBefore = nullptr;
    const auto acts = tb->actions();
    int pos = acts.indexOf(toggleHiddenBtn);  // <- FIX: use the QAction* directly
    if (pos >= 0 && pos + 1 < acts.size()) insertBefore = acts.at(pos + 1);

    QToolButton *btn = new QToolButton(tb);
    btn->setToolTip("Icon size");
    btn->setIcon(QIcon("icons/iconsize.png"));
	btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
	btn->setIconSize(tb->iconSize());
    btn->setPopupMode(QToolButton::InstantPopup);

    QMenu *m = new QMenu(btn);
    btn->setMenu(m);

    auto addSizeAction = [&](int s){
        QAction *a = m->addAction(QString::number(s));
        a->setCheckable(true);
        if (s == 32) a->setChecked(true);
        QObject::connect(a, &QAction::triggered, win, [=]{
            for (QAction *x : m->actions()) x->setChecked(false);
            for (QAction *x : m->actions()) if (x->text().toInt() == s) { x->setChecked(true); break; }

            const QSize newSz(s, s);
            if (auto f = dynamic_cast<FixedFSModel*>(model)) {
                f->setIconSize(newSz);
            }

            onRefresh();

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

            if (auto root = win->centralWidget()) {
                const auto views = root->findChildren<QAbstractItemView*>();
                for (QAbstractItemView *v : views) {
                    v->setIconSize(newSz);
                    v->setItemDelegate(new LocalAdjDelegate(newSz, v));
                    v->viewport()->update();
                }
            }

            if (win->statusBar()) win->statusBar()->showMessage(QString("Icon size: %1").arg(s), 1200);
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
#endif // ICONSIZE_H
