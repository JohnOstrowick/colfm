#ifndef HOME_H
#define HOME_H

#include <QDir>
#include <QFileSystemModel>

inline void ColFM::onGoHome() {
    QString home = QDir::homePath();
    currentRoot = model->setRootPath(home);
    if (crumbs) crumbs->setPath(home);
    onRefresh();
}

#endif // HOME_H
