#include <QtWidgets>
#include <QtCore>
#include <QtGui>
#include "contextmenu.h"
#include "drag.h"
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

#include "labels.h"

static QPixmap boxIcon(const QPixmap &src, int box) {
    QPixmap out(box, box);
    out.fill(Qt::transparent);
    if (src.isNull()) return out;

    // scale up/down into the box while keeping aspect
    QPixmap scaled = src.scaled(box, box, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const int x = (box - scaled.width()) / 2;
    const int y = (box - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);
    return out;
}

static QIcon boxedIconFromImage(QImage img, int box) {
    // ensure final is exactly box×box centered
    QPixmap pm = QPixmap::fromImage(img);
    return QIcon(boxIcon(pm, box));
}

static QIcon tintedIconFor(const QFileInfo &fi, int box /*48, 32, ...*/) {
    QPixmap base = QFileIconProvider().icon(fi).pixmap(box);
    if (base.isNull())
        return QIcon(QPixmap(box, box)); // empty box

    QImage img = base.toImage();

    // Symlink: teal buff
    if (fi.isSymLink()) {
        for (int y=0; y<img.height(); ++y)
            for (int x=0; x<img.width(); ++x) {
                QColor c = img.pixelColor(x,y);
                if (c.alpha() > 0)
                    c.setRgb((c.red()+0)/2, (c.green()+180)/2, (c.blue()+180)/2, c.alpha());
                img.setPixelColor(x,y,c);
            }
        return boxedIconFromImage(img, box);
    }

    // Executable (non-dir): subtle green buff
    if (fi.isExecutable() && !fi.isDir()) {
        for (int y=0; y<img.height(); ++y)
            for (int x=0; x<img.width(); ++x) {
                QColor c = img.pixelColor(x,y);
                if (c.alpha() > 0)
                    c.setRgb((c.red()+128)/2, (c.green()+255)/2, (c.blue()+128)/2, c.alpha());
                img.setPixelColor(x,y,c);
            }
        return boxedIconFromImage(img, box);
    }

    // No tint → just box to exact size
    return QIcon(boxIcon(QPixmap::fromImage(img), box));
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
        LabelManager::readLabelFile(desktopDir());

        QString p = desktopDir();
        QDir d(p);
        d.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable); // no Hidden
        d.setSorting(QDir::Name | QDir::IgnoreCase);

        QFileIconProvider prov;
        QFileInfoList list = d.entryInfoList();
        QList<QStandardItem*> rows;
        rows.reserve(list.size());

        for (const QFileInfo &fi : list) {
	    QIcon ico = tintedIconFor(fi, 48);  // or 32 if you want tighter icons
	    // Label tint (hex from .labelcolor): blend like the symlink buff
		QString lab = LabelManager::getLabel(fi.fileName());
			if (!lab.isEmpty()) {
		    QColor qc(lab);
		    if (qc.isValid()) {
			QImage im = ico.pixmap(48,48).toImage();
			LabelManager::tintImage(im, qc);                  // same 50/50 blend
			ico = QIcon(QPixmap::fromImage(im));
		    }
		}

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

// main app
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
	Drag::enableOn(view);

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
