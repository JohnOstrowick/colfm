#pragma once
// desktop.h — Qt6, header-only, no Q_OBJECT/moc needed.
// Fullscreen desktop behind ColFM with 48px right-aligned icons.
// Shows Desktop items, Trash, and mounted drives (/media/*, /run/media/*).
// Background = average colour of GNOME wallpaper (gsettings picture-uri).

#include <QtWidgets>
#include <QtGui>
#include <QtCore>

class DesktopIconsWidget : public QListView {
public:
    QStandardItemModel *model = nullptr;

    explicit DesktopIconsWidget(QWidget *parent = nullptr)
        : QListView(parent), model(new QStandardItemModel(this)) {

        setViewMode(QListView::IconMode);
        setIconSize(QSize(48, 48));
        setGridSize(QSize(96, 96));
        setResizeMode(QListView::Adjust);
        setMovement(QListView::Static);
        setWrapping(true);
        setSpacing(8);
        setUniformItemSizes(true);
        setEditTriggers(QAbstractItemView::NoEditTriggers);

        // Right-aligned packing
        setLayoutDirection(Qt::RightToLeft);
        setTextElideMode(Qt::ElideRight);

        setModel(model);
        refresh();

        auto *t = new QTimer(this);
        t->setInterval(2500);
        QObject::connect(t, &QTimer::timeout, this, [this]{ refresh(); });
        t->start();

        QObject::connect(this, &QListView::doubleClicked, this, [this](const QModelIndex &idx){
            const QString path = idx.data(Qt::UserRole + 1).toString();
            // Default action: open with system handler (you can swap this to your opener)
            if (!path.isEmpty())
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
    }

    void refresh() {
        struct Item { QString name, path, kind; QIcon icon; };
        QVector<Item> items; items.reserve(128);
        // Load label colours for this desktop dir
LabelManager::readLabelFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

        // Desktop contents
        const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QDir d(desktopPath);
        d.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden);
        d.setSorting(QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &fi : d.entryInfoList()) {
            Item it;
            it.name = fi.fileName();
            it.path = fi.absoluteFilePath();
            it.kind = fi.isDir() ? "dir" : "file";
            it.icon = iconForFile(fi);
            items.push_back(std::move(it));
        }

        // Trash
        {
            Item it;
            it.name = QStringLiteral("Trash");
            it.kind = QStringLiteral("trash");
            it.path  = trashPath();
            it.icon  = QIcon::fromTheme(QStringLiteral("user-trash"));
            if (it.icon.isNull()) it.icon = QIcon::fromTheme(QStringLiteral("edit-delete"));
            items.push_back(std::move(it));
        }

        // Mounted drives (heuristic)
        for (const QStorageInfo &st : QStorageInfo::mountedVolumes()) {
            if (!st.isValid() || !st.isReady()) continue;
            const QString mp = st.rootPath();
            if (!(mp.startsWith("/media/") || mp.startsWith("/run/media/"))) continue;

            Item it;
            it.name = st.displayName().isEmpty() ? mp : st.displayName();
            it.path = mp;
            it.kind = QStringLiteral("mount");
            it.icon = QIcon::fromTheme(QStringLiteral("drive-removable-media"));
            if (it.icon.isNull()) it.icon = QIcon::fromTheme(QStringLiteral("drive-harddisk"));
            if (it.icon.isNull()) it.icon = QIcon::fromTheme(QStringLiteral("folder"));
            items.push_back(std::move(it));
        }

        // Alpha-sort
        std::sort(items.begin(), items.end(), [](const Item &a, const Item &b){
            return QString::localeAwareCompare(a.name, b.name) < 0;
        });

        model->clear();
        for (const Item &it : items) {
            auto *row = new QStandardItem(it.icon, it.name);
            row->setEditable(false);
            row->setToolTip(it.path);
            row->setData(it.path, Qt::UserRole + 1);
            row->setData(it.kind, Qt::UserRole + 2);
            row->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
	// Apply label colour if exists
	QString lbl = LabelManager::getLabel(it.name);
	if (!lbl.isEmpty()) {
	    row->setData(QBrush(QColor(lbl)), Qt::BackgroundRole);
	    QFont f = row->font(); 
	    f.setBold(true); 
	    row->setFont(f);
	}
            model->appendRow(row);
        }
    }

    static QIcon iconForFile(const QFileInfo &fi) {
        if (fi.isDir()) {
            QIcon ic = QIcon::fromTheme(QStringLiteral("folder"));
            if (!ic.isNull()) return ic;
        } else {
            const QString mimeGuess = mimeFromSuffix(fi.suffix());
            QIcon ic = QIcon::fromTheme(mimeIconName(mimeGuess));
            if (!ic.isNull()) return ic;
        }
        return QIcon::fromTheme(fi.isDir() ? QStringLiteral("folder") : QStringLiteral("text-x-generic"));
    }

    static QString trashPath() {
        const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        return home + QStringLiteral("/.local/share/Trash");
    }

    static QString mimeFromSuffix(const QString &suf) {
        const QString s = suf.toLower();
        if (s=="png"||s=="jpg"||s=="jpeg"||s=="gif"||s=="bmp"||s=="webp") return "image";
        if (s=="mp4"||s=="mov"||s=="mkv"||s=="avi") return "video";
        if (s=="mp3"||s=="wav"||s=="flac"||s=="ogg") return "audio";
        if (s=="pdf") return "application-pdf";
        if (s=="txt"||s=="md"||s=="rtf") return "text";
        return "application";
    }

    static QString mimeIconName(const QString &mimeHint) {
        if (mimeHint == "image") return "image-x-generic";
        if (mimeHint == "video") return "video-x-generic";
        if (mimeHint == "audio") return "audio-x-generic";
        if (mimeHint == "text")  return "text-x-generic";
        if (mimeHint == "application-pdf") return "application-pdf";
        return "application-octet-stream";
    }
};

class DesktopWindow : public QWidget {
public:
    QTimer *bgTimer = nullptr;

    explicit DesktopWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("DesktopWindow");
        setWindowTitle("ColFM Desktop");
        setWindowFlag(Qt::FramelessWindowHint, true);
        setWindowFlag(Qt::Tool, true);                 // keep off taskbar
        setWindowFlag(Qt::WindowStaysOnBottomHint, true);
#ifdef Q_OS_X11
        setAttribute(Qt::WA_X11NetWmWindowTypeDesktop, true);
#endif
        setAttribute(Qt::WA_TranslucentBackground, false);

        const QRect geom = QGuiApplication::primaryScreen()->geometry();
        setGeometry(geom);

        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(16,16,16,16);
        lay->setSpacing(0);

        auto *icons = new DesktopIconsWidget(this);
        lay->addWidget(icons);

        setBackgroundToDominantWallpaperColour();

        bgTimer = new QTimer(this);
        bgTimer->setInterval(5000);
        QObject::connect(bgTimer, &QTimer::timeout, this, [this]{ setBackgroundToDominantWallpaperColour(); });
        bgTimer->start();

        showFullScreen();
        lower();
    }

    void setBackgroundToDominantWallpaperColour() {
        const QString path = currentWallpaperPath();
        QColor bg = QColor(QStringLiteral("#2b2b2b"));
        if (!path.isEmpty()) {
            QImage img(path);
            if (!img.isNull()) bg = dominantColour(img);
        }
        QPalette pal = palette();
        pal.setColor(QPalette::Window, bg);
        setAutoFillBackground(true);
        setPalette(pal);
        lower();
    }

    static QString currentWallpaperPath() {
        QProcess p;
        p.start(QStringLiteral("gsettings"),
                {QStringLiteral("get"), QStringLiteral("org.gnome.desktop.background"), QStringLiteral("picture-uri")});
        p.waitForFinished(400);
        QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        out.remove('\'');
        if (out.startsWith(QStringLiteral("file://")))
            out = QUrl(out).toLocalFile();
        return QFileInfo(out).exists() ? out : QString();
    }

    static QColor dominantColour(const QImage &img) {
        if (img.isNull()) return QColor(QStringLiteral("#2b2b2b"));
        const QImage im = img.convertToFormat(QImage::Format_ARGB32_Premultiplied)
                               .scaled(160, 90, Qt::KeepAspectRatio, Qt::FastTransformation);
        quint64 r=0,g=0,b=0,c=0;
        for (int y=0;y<im.height();++y){
            const QRgb *row = reinterpret_cast<const QRgb*>(im.constScanLine(y));
            for (int x=0;x<im.width();++x){
                const QRgb px = row[x];
                r += qRed(px); g += qGreen(px); b += qBlue(px); ++c;
            }
        }
        if (!c) return QColor(QStringLiteral("#2b2b2b"));
        return QColor(int(r/c), int(g/c), int(b/c));
    }
};

// Create once after showing the main ColFM window.
inline DesktopWindow* ensureDesktopWindow() {
    static DesktopWindow *dw = nullptr;
    if (!dw) {
        dw = new DesktopWindow();
        QObject::connect(qApp, &QGuiApplication::primaryScreenChanged, dw,
                         [](QScreen *s){
                             if (!s) return;
                             auto *w = static_cast<DesktopWindow*>(dw);
                             w->setGeometry(s->geometry());
                             w->lower();
                         });
    }
    return dw;
}
