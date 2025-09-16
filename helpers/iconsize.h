#pragma once
#include <QMenu>
#include <QActionGroup>
#include <QAbstractItemView>
#include <functional>
#include "icons_data.h"
#include "views.h"

//inline void addIconSizePopup(QMenu *viewMenu, QAbstractItemView *viewWidget, std::function<void()> onRefresh) {
inline void addIconSizePopup(QMenu *viewMenu, std::function<void()> onRefresh) {
    QMenu *iconSizeMenu = viewMenu->addMenu(IconsData::getIcon("iconsize.png"), "Icon Size");
(void)onRefresh;
    QActionGroup *iconSizeGroup = new QActionGroup(viewMenu);
    iconSizeGroup->setExclusive(true);

    QAction *smallIcons = iconSizeMenu->addAction("Small");
    smallIcons->setCheckable(true);
    smallIcons->setData(16);
    iconSizeGroup->addAction(smallIcons);

    QAction *mediumIcons = iconSizeMenu->addAction("Medium");
    mediumIcons->setCheckable(true);
    mediumIcons->setData(32);
    iconSizeGroup->addAction(mediumIcons);

    QAction *largeIcons = iconSizeMenu->addAction("Large");
    largeIcons->setCheckable(true);
    largeIcons->setData(48);
    iconSizeGroup->addAction(largeIcons);

    mediumIcons->setChecked(true);  // default

    /*QObject::connect(iconSizeGroup, &QActionGroup::triggered, viewMenu,
        [viewWidget, onRefresh](QAction *action) {
            int px = action->data().toInt();
            if (viewWidget)
                viewWidget->setIconSize(QSize(px, px));
            onRefresh();
        });
	*/
}
