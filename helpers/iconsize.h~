#ifndef ICONSIZE_H
#define ICONSIZE_H

#include <QMenu>
#include <QActionGroup>
#include <QIcon>
#include <QSize>
#include <QDebug>
#include "icons_data.h"

inline void addIconSizePopup(QMenu *parentMenu, std::function<void()> onRefresh) {
    // icon size submenu
    QMenu *iconSizeMenu = viewMenu->addMenu(IconsData::getIcon("iconsize.png"), "Icon Size");

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

    mediumIcons->setChecked(true); // default selected

    QObject::connect(iconSizeGroup, &QActionGroup::triggered, viewMenu, [=](QAction *action) {
        int px = action->data().toInt();
        qDebug() << "[IconSize] Setting icon size to:" << px << "px";
        // You must implement `setIconSize(QSize)` on the icon view in the main class
        iconView->setIconSize(QSize(px, px));
    });
}

#endif // ICONSIZE_H
