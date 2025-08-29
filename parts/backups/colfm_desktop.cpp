#include <QtWidgets>
#if __has_include(<X11/Xlib.h>)
  #define HAVE_X11 1
  #include <X11/Xlib.h>
  #include <X11/Xatom.h>
#endif

#include <QtWidgets>
#include <QtCore>
#include <QtGui>
// just above where we set the pixmap/icon (inside DesktopModel::data)
#include "labels.h"
// … call LabelManager::readLabelFile(desktopDir()) and apply a dot tint

// ---- Small helpers ---------------------------------------------------------

static inline QString desktopDir() {
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

static inline QString trashDir() {
    // Freedesktop spec path (logical folder with files/info)
    return QDir::homePath() + "/.local/share/Trash/files";
}

static QPixmap loadBackground(const QRect &screenGeom) {
    // 1) app-local default
    QString p = QCoreApplication::applicationDirPath() + "/wallpapers/default.jpg";
    QPixmap pm;
    if (QFileInfo::exists(p)) pm.load(p);

    // 2) ubuntu fallback
    if (pm.isNull()) pm.load("/usr/share/backgrounds/warty-final-ubuntu.png");

    // 3) solid color fallback
    if (pm.isNull()) {
        pm = QPixmap(screenGeom.size());
        pm.fill(QColor("#2b2b2b"));
        return pm;
    }
    return pm.scaled(screenGeom.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

// ---- A tiny model wrapper to inject a "Trash" pseudo-item ------------------

class DesktopModel : public QAbstractListModel {
public:
    explicit DesktopModel(QObject *parent=nullptr)
        : QAbstractListModel(parent)
        , fs(new QFileSystemModel(this))
    {
        fs->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden);
        fs->setRootPath(desktopDir());
        root = fs->index(desktopDir());
    }

    enum Roles { PathRole = Qt::UserRole + 1, KindRole };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        // +1 for the Trash pseudo-item
        return fs->rowCount(root) + 1;
    }

    QVariant data(const QModelIndex &idx, int role) const override {
        if (!idx.isValid()) return {};

        const int n = fs->rowCount(root);
        const bool isTrash = (idx.row() == n);

        if (role == Qt::DecorationRole) {
            if (isTrash) {
                QIcon ic = QIcon::fromTheme("user-trash");
                if (ic.isNull()) ic = QIcon::fromTheme("edit-delete");
                if (ic.isNull()) ic = QApplication::style()->standardIcon(QStyle::SP_TrashIcon);
                return ic.pixmap(48,48);
            } else {
                const QModelIndex fi = fs->index(idx.row(), 0, root);
                return fs->fileIcon(fi).pixmap(48,48);
            }
        }
        if (role == Qt::DisplayRole) {
            if (isTrash) return QStringLiteral("Trash");
            const QModelIndex fi = fs->index(idx.row(), 0, root);
            return fs->fileName(fi);
        }
        if (role == PathRole) {
            if (isTrash) return trashDir();
            const QModelIndex fi = fs->index(idx.row(), 0, root);
            return fs->filePath(fi);
        }
        if (role == KindRole) {
            if (isTrash) return QStringLiteral("trash");
            const QModelIndex fi = fs->index(idx.row(), 0, root);
            return fs->isDir(fi) ? QStringLiteral("dir") : QStringLiteral("file");
        }
        if (role == Qt::ToolTipRole) {
            if (isTrash) return trashDir();
            const QModelIndex fi = fs->index(idx.row(), 0, root);
            return fs->filePath(fi);
        }
        return {};
    }

    // Alphabetical sort (locale-aware)
    void sort(int, Qt::SortOrder order = Qt::AscendingOrder) override {
        Q_UNUSED(order);
        // QFileSystemModel already sorts by name if we call setNameFilterDisables
        // but we’ll just rebuild an index order by querying and letting the view arrange.
        // The view will call data() by row, which maps to the fs row order.
        fs->sort(0, Qt::AscendingOrder);
        // nothing else to do; our rows mirror fs rows (+ 1 trash)
        emit dataChanged(index(0,0), index(rowCount()-1,0));
        emit layoutChanged();
    }

    QFileSystemModel *fs;
    QModelIndex root;
};

// ---- Context menu for items -------------------------------------------------

static void showContextMenu(QWidget *parent, const QModelIndex &idx, QAbstractItemModel *model, const QPoint &globalPos) {
    const QString path = model->data(idx, DesktopModel::PathRole).toString();
    const QString kind = model->data(idx, DesktopModel::KindRole).toString();

    QMenu m(parent);
    QAction *open         = m.addAction("Open");
    QAction *openInColfm  = m.addAction("Open in ColFM");
    QAction *openWith     = m.addAction("Open With…");
    QAction *trashAct     = nullptr;
    if (kind != "trash")  trashAct = m.addAction("Move to Trash");
    QAction *revealTerm   = m.addAction("Open Terminal Here");

    QAction *chosen = m.exec(globalPos);
    if (!chosen) return;

    if (chosen == open) {
        if (kind == "dir" || kind == "trash") {
            // open folder in ColFM (new window)
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
        // Let the desktop environment choose
        QProcess::startDetached("xdg-open", { path });
        return;
    }

    if (trashAct && chosen == trashAct) {
        // try gio trash; fallback to rm -rf only if gio absent (but do nothing destructive here)
        QProcess::startDetached("gio", { "trash", path });
        return;
    }

    if (chosen == revealTerm) {
        const QString dir = (kind == "file") ? QFileInfo(path).absolutePath() : path;
        QProcess::startDetached("x-terminal-emulator", { "--working-directory", dir });
        return;
    }
}

// ---- Main -------------------------------------------------------------------

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Desktop background window that does not cover the dock
    QWidget back;
    back.setWindowFlag(Qt::FramelessWindowHint, true);
    back.setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);
    back.setWindowFlag(Qt::WindowStaysOnBottomHint, true);
    back.setAutoFillBackground(true);

    const QRect work = QGuiApplication::primaryScreen()->availableGeometry();
    QPalette pal = back.palette();
    pal.setBrush(QPalette::Window, QBrush(loadBackground(work)));
    back.setPalette(pal);
    back.setGeometry(work);
    back.show();
    back.lower();

    // Icon view of ~/Desktop
    auto *model = new DesktopModel(&back);
    model->sort(0, Qt::AscendingOrder);

    auto *view = new QListView(&back);
    view->setModel(model);
    view->setViewMode(QListView::IconMode);
    view->setIconSize(QSize(48,48));
    view->setGridSize(QSize(96,96));            // space for label beneath
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setWrapping(true);
    view->setSpacing(8);
    view->setUniformItemSizes(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setLayoutDirection(Qt::RightToLeft);  // pack from right → left
    view->setTextElideMode(Qt::ElideRight);
    view->setGeometry(work.adjusted(16,16,-16,-16)); // margin inside background

    // Double-click behavior
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

    // Context menu
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(view, &QWidget::customContextMenuRequested, view, [view,model](const QPoint &pos){
        const QModelIndex idx = view->indexAt(pos);
        if (!idx.isValid()) return;
        showContextMenu(view, idx, model, view->viewport()->mapToGlobal(pos));
    });

    view->show();
    back.lower(); // keep behind other stuff

    return app.exec();
}



/*
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget w;                                // normal top-level (not Tool)
    w.setWindowTitle("ColFM Desktop");
    w.setWindowFlag(Qt::FramelessWindowHint, true);
    w.setWindowFlag(Qt::WindowStaysOnBottomHint, true);
    w.setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);
    w.setAutoFillBackground(true);

    const QRect g = QGuiApplication::primaryScreen()->geometry();
    QPalette pal = w.palette();
    pal.setBrush(QPalette::Window, QBrush(loadBackground(g)));
    w.setPalette(pal);

    // w.showFullScreen();                        // get a real window id
    const QRect work = QGuiApplication::primaryScreen()->availableGeometry(); // excludes reserved dock/panels (when they set struts)
    w.setGeometry(work);
    w.show();

#if HAVE_X11
    // On X11, ask WM to keep it BELOW and out of taskbar
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
        e.xclient.data.l[0] = 1;                    // _NET_WM_STATE_ADD
        e.xclient.data.l[1] = skip_taskbar;
        e.xclient.data.l[2] = below;
        e.xclient.data.l[3] = 1;                    // normal window
        e.xclient.data.l[4] = 0;
        XSendEvent(d, DefaultRootWindow(d), False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &e);
        XFlush(d);
        XCloseDisplay(d);
    }
#endif

    w.lower();                                   // final nudge
    return app.exec();
}
*/
