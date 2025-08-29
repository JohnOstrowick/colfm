#include <QtWidgets>
#include <QtCore>
#include <QtGui>

#if __has_include(<X11/Xlib.h>)
  #define HAVE_X11 1
  #include <X11/Xlib.h>
  #include <X11/Xatom.h>
#endif

// ---------- helpers ----------
static inline QString desktopDir() {
    QString p = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QFileInfo fi(p);
    if (fi.isSymLink()) {
        const QString t = fi.symLinkTarget();
        if (!t.isEmpty()) return QDir::cleanPath(t);
    }
    return p;
}

static inline QString trashDir() {
    return QDir::homePath() + "/.local/share/Trash/files";
}

static QPixmap loadBackground(const QRect &screenGeom) {
    QString p = QCoreApplication::applicationDirPath() + "/wallpapers/default.jpg";
    QPixmap pm;
    if (QFileInfo::exists(p)) pm.load(p);
    if (pm.isNull()) pm.load("/usr/share/backgrounds/warty-final-ubuntu.png");
    if (pm.isNull()) {
        pm = QPixmap(screenGeom.size());
        pm.fill(QColor("#2b2b2b"));
        return pm;
    }
    return pm.scaled(screenGeom.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

// ---------- simple model ----------
class DesktopModel : public QStandardItemModel {
public:
    enum Roles { PathRole = Qt::UserRole + 1, KindRole };

    void rebuild() {
        clear();
        setColumnCount(1);

        QString p = desktopDir();
        QDir d(p);
        d.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable); // no Hidden
        d.setSorting(QDir::Name | QDir::IgnoreCase);

        QFileIconProvider prov;
        QFileInfoList list = d.entryInfoList();
        QList<QStandardItem*> rows;
        rows.reserve(list.size());

        for (const QFileInfo &fi : list) {
            QIcon ico = prov.icon(fi);
            auto *it = new QStandardItem(ico, fi.fileName());
            it->setEditable(false);
            it->setToolTip(fi.absoluteFilePath());
            it->setData(fi.absoluteFilePath(), PathRole);
            it->setData(fi.isDir() ? "dir" : "file", KindRole);
            it->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
            rows << it;
        }

        std::sort(rows.begin(), rows.end(), [](QStandardItem* a, QStandardItem* b){
            return QString::localeAwareCompare(a->text(), b->text()) < 0;
        });
        for (auto *it : rows) appendRow(it);

        QIcon trashIc = QIcon::fromTheme("user-trash");
        if (trashIc.isNull()) trashIc = QIcon::fromTheme("edit-delete");
        if (trashIc.isNull()) trashIc = QApplication::style()->standardIcon(QStyle::SP_TrashIcon);
        auto *trash = new QStandardItem(trashIc, QStringLiteral("Trash"));
        trash->setEditable(false);
        trash->setToolTip(trashDir());
        trash->setData(trashDir(), PathRole);
        trash->setData(QStringLiteral("trash"), KindRole);
        trash->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        appendRow(trash);
    }
};

// ---------- context menu ----------
static void showContextMenu(QWidget *parent, const QModelIndex &idx, QAbstractItemModel *model, const QPoint &globalPos) {
    const QString path = model->data(idx, DesktopModel::PathRole).toString();
    const QString kind = model->data(idx, DesktopModel::KindRole).toString();

    QMenu m(parent);
    QAction *open         = m.addAction("Open");
    QAction *openInColfm  = m.addAction("Open in ColFM");
    QAction *openWith     = m.addAction("Open With…");
    QAction *trashAct     = (kind == "trash") ? nullptr : m.addAction("Move to Trash");
    QAction *revealTerm   = m.addAction("Open Terminal Here");

    QAction *chosen = m.exec(globalPos);
    if (!chosen) return;

    if (chosen == open) {
        if (kind == "dir" || kind == "trash") {
            const QString exe = QCoreApplication::applicationDirPath() + "/colfm";
            QProcess::startDetached(exe, { path });
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
        return;
    }
    if (chosen == openInColfm) {
        const QString exe = QCoreApplication::applicationDirPath() + "/colfm";
        QProcess::startDetached(exe, { path });
        return;
    }
    if (chosen == openWith) {
        QProcess::startDetached("xdg-open", { path });
        return;
    }
    if (trashAct && chosen == trashAct) {
        QProcess::startDetached("gio", { "trash", path });
        return;
    }
    if (chosen == revealTerm) {
        const QString dir = (kind == "file") ? QFileInfo(path).absolutePath() : path;
        QProcess::startDetached("x-terminal-emulator", { "--working-directory", dir });
        return;
    }
}

// ---------- main ----------
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget w;
    w.setWindowTitle("ColFM Desktop");
    w.setWindowFlag(Qt::FramelessWindowHint, true);
    w.setWindowFlag(Qt::WindowStaysOnBottomHint, true);
    w.setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);
    w.setAutoFillBackground(true);

    const QRect g = QGuiApplication::primaryScreen()->geometry();
    QPalette pal = w.palette();
    pal.setBrush(QPalette::Window, QBrush(loadBackground(g)));
    w.setPalette(pal);

    const QRect work = QGuiApplication::primaryScreen()->availableGeometry();
    w.setGeometry(work);
    w.show();

#if HAVE_X11
    Display *d = nullptr;
    if (QGuiApplication::platformName() == "xcb" && (d = XOpenDisplay(nullptr))) {
        Window xw = w.winId();
        Atom net_wm_state = XInternAtom(d, "_NET_WM_STATE", False);
        Atom skip_taskbar = XInternAtom(d, "_NET_WM_STATE_SKIP_TASKBAR", False);
        Atom below        = XInternAtom(d, "_NET_WM_STATE_BELOW", False);

        XEvent e; memset(&e, 0, sizeof(e));
        e.xclient.type = ClientMessage;
        e.xclient.window = xw;
        e.xclient.message_type = net_wm_state;
        e.xclient.format = 32;
        e.xclient.data.l[0] = 1;
        e.xclient.data.l[1] = skip_taskbar;
        e.xclient.data.l[2] = below;
        e.xclient.data.l[3] = 1;
        e.xclient.data.l[4] = 0;
        XSendEvent(d, DefaultRootWindow(d), False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &e);
        XFlush(d);
        XCloseDisplay(d);
    }
#endif

    w.lower();

    auto *model = new DesktopModel();
    model->rebuild();

    auto *view = new QListView(&w);
    view->setModel(model);
    view->setViewMode(QListView::IconMode);
    view->setIconSize(QSize(48,48));
    view->setGridSize(QSize(96,96));
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setWrapping(true);
    view->setSpacing(8);
    view->setUniformItemSizes(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    view->setFlow(QListView::TopToBottom);
    view->setLayoutDirection(Qt::RightToLeft);

    view->setStyleSheet("QListView{background:transparent;} QScrollBar{background:transparent;}");
    view->viewport()->setAutoFillBackground(false);

    view->setGeometry(w.rect().adjusted(16,16,-16,-16));
    view->show();

    auto *tick = new QTimer(&w);
    tick->setInterval(2000);
    QObject::connect(tick, &QTimer::timeout, model, [model]{ model->rebuild(); });
    tick->start();

    QObject::connect(view, &QListView::doubleClicked, view, [model](const QModelIndex &idx){
        if (!idx.isValid()) return;
        const QString path = model->data(idx, DesktopModel::PathRole).toString();
        const QString kind = model->data(idx, DesktopModel::KindRole).toString();
        if (kind == "dir" || kind == "trash") {
            const QString exe = QCoreApplication::applicationDirPath() + "/colfm";
            QProcess::startDetached(exe, { path });
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(view, &QWidget::customContextMenuRequested, view, [view,model](const QPoint &pos){
        const QModelIndex idx = view->indexAt(pos);
        if (!idx.isValid()) return;
        showContextMenu(view, idx, model, view->viewport()->mapToGlobal(pos));
    });

    return app.exec();
}
