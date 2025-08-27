#ifndef COLFM_SIDEBAR_H
#define COLFM_SIDEBAR_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QDir>
#include <QFileInfo>
#include <QStyle>

inline void populateSidebar() {
    if (!sidebar) return;

    sidebar->clear();
    sidebar->setColumnCount(1);
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

    // helper: one row widget per item with icon + text + (optional) eject button
    auto addItem = [&](const QString &label, const QString &path, bool ejectable, bool isDrive){
        auto *it = new QTreeWidgetItem(sidebar);
        it->setData(0, Qt::UserRole, path);
        it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        QWidget *row = new QWidget(sidebar);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(8, 6, 8, 6);
        h->setSpacing(6);

        // icon (16x16)
        QLabel *ic = new QLabel(row);
        QPixmap pm;
        if (isDrive) pm = QPixmap("icons/disk.png");
        else if (path.endsWith("/.local/share/Trash/files")) pm = QPixmap("icons/open_trash.png");
        else         pm = style()->standardIcon(QStyle::SP_DirIcon).pixmap(16,16);
        ic->setPixmap(pm.scaled(16,16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        h->addWidget(ic, 0, Qt::AlignVCenter);

        // label
        QLabel *lab = new QLabel(label, row);
        lab->setStyleSheet("color:#ddd; font-size:14px; letter-spacing:0.4px;");
        h->addWidget(lab, 1, Qt::AlignVCenter);

        if (ejectable) {
            QToolButton *ej = new QToolButton(row);
            ej->setIcon(QIcon("icons/eject.png"));
            ej->setIconSize(QSize(18,18));
            ej->setAutoRaise(true);
            ej->setStyleSheet("QToolButton { background:#000; border:none; padding:6px; }");
            ej->setToolTip("Eject");
            ej->setProperty("mount", path);
            h->addWidget(ej, 0, Qt::AlignRight | Qt::AlignVCenter);

            QObject::connect(ej, &QToolButton::clicked, this, [this, ej]{
                const QString mp = ej->property("mount").toString();
                statusBar()->showMessage(QString("Eject %1 (not implemented)").arg(mp), 1500);
            });
        }

        sidebar->setItemWidget(it, 0, row);
        it->setSizeHint(0, QSize(100, row->sizeHint().height() + 4));
        return it;
    };

    const QString home = QDir::homePath();
    // Always show Desktop, Downloads, Trash first
    addItem("Home",     home + "./", false, false);
    addItem("Desktop",  home + "/Desktop", false, false);
    addItem("Downloads",home + "/Downloads", false, false);

    // Trash
    QTreeWidgetItem *trashItem = addItem("Trash", home + "/.local/share/Trash/files", false, false);
    QObject::connect(sidebar, &QTreeWidget::itemClicked, this, [this, trashItem](QTreeWidgetItem *clicked, int){
        if (clicked == trashItem) onOpenTrash();
    });

    // Divider
    {
        auto *div = new QTreeWidgetItem(sidebar);
        div->setFlags(Qt::NoItemFlags);
        div->setFirstColumnSpanned(true);
        sidebar->setItemWidget(div, 0, new QLabel("────────", sidebar));
    }

    // Visible (non-dot) folders in ~
    QDir hd(home);
    for (const QFileInfo &fi : hd.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        if (fi.fileName().startsWith('.')) continue;
        if (fi.fileName() == "Desktop" || fi.fileName() == "Downloads") continue;
        addItem(fi.fileName(), fi.absoluteFilePath(), false, false);
    }

    // Divider
    {
        auto *div2 = new QTreeWidgetItem(sidebar);
        div2->setFlags(Qt::NoItemFlags);
        div2->setFirstColumnSpanned(true);
        sidebar->setItemWidget(div2, 0, new QLabel("────────", sidebar));
    }

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
        for (const QString &mp : drives)
            addItem(QFileInfo(mp).fileName(), mp, true, true);
    }
}

inline QWidget* buildWithSidebar(QWidget *center) {
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

#endif // COLFM_SIDEBAR_H
