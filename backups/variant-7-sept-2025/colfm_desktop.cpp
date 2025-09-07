#include <QtWidgets>
#include <QtCore>
#include <QtGui>
#include "helpers/contextmenu.h"
#include "helpers/drag.h"
#include "helpers/iconview_factory.h"
#include "helpers/icons_qicon.h"
#if __has_include(<X11/Xlib.h>)
  #define HAVE_X11 1
  #include <X11/Xlib.h>
  #include <X11/Xatom.h>
#endif

// ---------- helpers ----------

static QString detectWallpaperFromOS() {
    // Try GNOME via gsettings
    QProcess p;
    p.start("gsettings", {"get", "org.gnome.desktop.background", "picture-uri"});
    if (p.waitForFinished(200) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
        QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        // Example: 'file:///home/john/Pictures/wall.jpg' or '""'
        out.remove('\'').remove('\"');
        if (out.startsWith("file://")) out = QUrl(out).toLocalFile();
        if (QFile::exists(out)) return out;

        // GNOME dark variant fallback
        p.start("gsettings", {"get", "org.gnome.desktop.background", "picture-uri-dark"});
        if (p.waitForFinished(200) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
            out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
            out.remove('\'').remove('\"');
            if (out.startsWith("file://")) out = QUrl(out).toLocalFile();
            if (QFile::exists(out)) return out;
        }
    }

    // Try KDE Plasma config
    const QString plasmaCfg = QDir::homePath() + "/.config/plasma-org.kde.plasma.desktop-appletsrc";
    if (QFile::exists(plasmaCfg)) {
        QFile f(plasmaCfg);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString cfg = QString::fromUtf8(f.readAll());
            // Prefer the last Image= entry (active containment)
            QRegularExpression re(R"(Image=(.+))");
            QRegularExpressionMatchIterator it = re.globalMatch(cfg);
            QString imagePath;
            while (it.hasNext()) {
                const auto m = it.next();
                imagePath = m.captured(1).trimmed();
            }
            if (!imagePath.isEmpty()) {
                if (imagePath.startsWith("file://")) imagePath = QUrl(imagePath).toLocalFile();
                if (QFile::exists(imagePath)) return imagePath;
            }
        }
    }

    return QString(); // unknown
}

static QPixmap loadBackground(const QRect &screen) {
    // Ask the OS/compositor for the configured wallpaper file, not a screenshot.
    const QString wall = detectWallpaperFromOS();
    QPixmap pm(screen.size());
    pm.fill(Qt::black); // fallback

    if (!wall.isEmpty() && QFile::exists(wall)) {
        QImage img(wall);
        if (!img.isNull()) {
            // Scale to fill, centre-cropped, like most desktop environments
            QImage scaled = img.scaled(screen.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            QPainter p(&pm);
            const int dx = (pm.width()  - scaled.width())  / 2;
            const int dy = (pm.height() - scaled.height()) / 2;
            p.drawImage(dx, dy, scaled);
        }
    }
    return pm;
}

static inline QString desktopDir() {
    return QDir::homePath() + "/Desktop";
}

static inline QString trashDir() {
    return QDir::homePath() + "/.local/share/Trash/files";
}

#include "helpers/labels.h"

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
    }

    // Label tint (hex from .labelcolor)
    QString lab = LabelManager::getLabel(fi.fileName());
    if (!lab.isEmpty()) {
        QColor qc(lab);
        if (qc.isValid()) {
            for (int y=0; y<img.height(); ++y)
                for (int x=0; x<img.width(); ++x) {
                    QColor c = img.pixelColor(x,y);
                    if (c.alpha() > 0)
                        c.setRgb((c.red()+qc.red())/2, (c.green()+qc.green())/2, (c.blue()+qc.blue())/2, c.alpha());
                    img.setPixelColor(x,y,c);
                }
        }
    }

    return boxedIconFromImage(img, box);
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
            rows << it;
        }

        // Trash pseudo-item
        //QImage trashImg(":/icons/trash.png");
        QImage trashImg;
trashImg.loadFromData(IconsData::bytesMap().value("trash.png"), "PNG");
        if (!trashImg.isNull()) {
            auto *trash = new QStandardItem(boxedIconFromImage(trashImg, 48), "Trash");
            trash->setEditable(false);
            trash->setToolTip(trashDir());
            trash->setData(trashDir(), PathRole);
            trash->setData(QStringLiteral("trash"), KindRole);
            rows << trash;
        }

        for (QStandardItem *it : rows) appendRow(it);
    }
};


// ---------- main ----------

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    QWidget w;
    w.setWindowTitle("ColFM Desktop");
    w.setWindowFlag(Qt::FramelessWindowHint, true);
    w.setWindowFlag(Qt::WindowStaysOnBottomHint, true);
    w.setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);
    w.setAutoFillBackground(true);

    const QRect g = QGuiApplication::primaryScreen()->geometry();
    QPalette pal = w.palette();
pal.setBrush(QPalette::Window, QBrush(::loadBackground(g)));
    w.setPalette(pal);

    const QRect work = QGuiApplication::primaryScreen()->availableGeometry();
    w.setGeometry(work);
    w.show();
    //w.raise();

#if HAVE_X11
    // Make window behave like a desktop layer (skip taskbar, always behind)
    Display *d = XOpenDisplay(nullptr);
    if (d) {
        WId xw = w.winId();
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
    view->setLayoutDirection(Qt::LeftToRight);
    view->setSelectionBehavior(QAbstractItemView::SelectItems);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setIconSize(QSize(48,48));
    view->setWordWrap(true);
    view->setTextElideMode(Qt::ElideNone);
    // grid: columnised label beneath icon; allow ~3 lines
    const int textLines = 3;
    const int padW = 60;
    const int padH = 24;
    view->setGridSize(QSize(view->iconSize().width() + padW,
                            view->iconSize().height() + view->fontMetrics().lineSpacing()*textLines + padH));
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setWrapping(true);
    view->setSpacing(8);
    view->setUniformItemSizes(false);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    Drag::enableOn(view);

    view->setFlow(QListView::TopToBottom);
    view->setLayoutDirection(Qt::LeftToRight);
    view->viewport()->setAutoFillBackground(false);
    view->setStyleSheet("QListView{background:transparent;} QScrollBar{background:transparent;}");

    // position to fill the work area with a small inset
    view->setGeometry(w.rect().adjusted(16,16,-80,-16)); // leave space for dock on the right (~64-80px)
view->show();
view->raise();

    // keep background reactive to screen resize (multi-monitor, dock changes, etc.)
    auto *tick = new QTimer(&w);
    tick->setInterval(5000);
    QObject::connect(tick, &QTimer::timeout, &w, [&w](){
        const QRect g = QGuiApplication::primaryScreen()->geometry();
        QPalette pal = w.palette();
        pal.setBrush(QPalette::Window, QBrush(::loadBackground(g)));
        w.setPalette(pal);
    });
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
