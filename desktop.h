#pragma once
// desktop.h — draws desktop icons (48 px), right-aligned, alpha-sorted.
// Depends: Qt >= 5.12 (QtGui, QtWidgets, QtCore)

#include <QListView>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QDir>
#include <QIcon>
#include <QTimer>
#include <QFileInfo>
#include <QPixmap>

class DesktopIconsWidget : public QListView {
    Q_OBJECT
public:
    explicit DesktopIconsWidget(QWidget *parent = nullptr)
        : QListView(parent), model(new QStandardItemModel(this)) {

        setViewMode(QListView::IconMode);
        setIconSize(QSize(48, 48));
        setGridSize(QSize(96, 96));                 // generous label space
        setResizeMode(QListView::Adjust);
        setMovement(QListView::Static);
        setWrapping(true);
        setSpacing(8);
        setUniformItemSizes(true);
        setEditTriggers(QAbstractItemView::NoEditTriggers);

        // Right-aligned grid: use RTL layout; items will pack from the right.
        setLayoutDirection(Qt::RightToLeft);
        setTextElideMode(Qt::ElideRight);

        setModel(model);
        refresh();

        // light auto-refresh (mounts, files changing)
        auto *t = new QTimer(this);
        t->setInterval(2500);
        connect(t, &QTimer::timeout, this, &DesktopIconsWidget::maybeRefresh);
        t->start();

        connect(this, &QListView::doubleClicked, this, [this](const QModelIndex &idx){
            const QString path = idx.data(Qt::UserRole + 1).toString();
            const QString kind = idx.data(Qt::UserRole + 2).toString();
            emit activatedPath(path, kind);
        });
    }

signals:
    // kind: "file", "dir", "trash", "mount"
    void activatedPath(const QString &path, const QString &kind);

public slots:
    void refresh() {
        QStringList names;
        struct Item { QString name, path, kind; QIcon icon; };
        QVector<Item> items; items.reserve(128);

        // 1) Desktop contents (files + folders)
        const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QDir d(desktopPath);
        d.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden);
        d.setSorting(QDir::Name | QDir::IgnoreCase);
        const auto entries = d.entryInfoList();
        for (const QFileInfo &fi : entries) {
            Item it;
            it.name = fi.fileName();
            it.path = fi.absoluteFilePath();
            it.kind = fi.isDir() ? "dir" : "file";
            it.icon = iconForFile(fi);
            items.push_back(std::move(it));
        }

        // 2) Trash
        {
            Item it;
            it.name = QStringLiteral("Trash");
            it.kind = QStringLiteral("trash");
            it.path = trashPath(); // logical target
            it.icon = QIcon::fromTheme(QStringLiteral("user-trash"));
            if (it.icon.isNull()) it.icon = QIcon::fromTheme(QStringLiteral("edit-delete"));
            items.push_back(std::move(it));
        }

        // 3) Mounted drives (removable + user mounts)
        for (const QStorageInfo &st : QStorageInfo::mountedVolumes()) {
            if (!st.isValid() || !st.isReady()) continue;
            const QString mp = st.rootPath();
            // Heuristic: show /media/$USER/*, /run/media/$USER/*, and removable
            if (st.isReadOnly() && !st.isRemovable()) continue;
            if (!(st.isRemovable() || mp.startsWith("/media/") || mp.startsWith("/run/media/")))
                continue;

            Item it;
            it.name = st.displayName().isEmpty() ? mp : st.displayName();
            it.path = mp;
            it.kind = QStringLiteral("mount");
            it.icon = st.isRemovable()
                    ? QIcon::fromTheme(QStringLiteral("drive-removable-media"))
                    : QIcon::fromTheme(QStringLiteral("drive-harddisk"));
            if (it.icon.isNull()) it.icon = QIcon::fromTheme(QStringLiteral("folder"));
            items.push_back(std::move(it));
        }

        // Alpha-sort by visible name (case-insensitive)
        std::sort(items.begin(), items.end(), [](const Item &a, const Item &b){
            return QString::localeAwareCompare(a.name, b.name) < 0;
        });

        // Rebuild model
        model->clear();
        for (const Item &it : items) {
            auto *row = new QStandardItem(it.icon, it.name);
            row->setEditable(false);
            row->setToolTip(it.path);
            row->setData(it.path, Qt::UserRole + 1);
            row->setData(it.kind, Qt::UserRole + 2);
            row->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
            model->appendRow(row);
        }
    }

private:
    QStandardItemModel *model;
    QByteArray lastHash;

    static QIcon iconForFile(const QFileInfo &fi) {
        // Prefer theme icons; fall back to generic folder/file icons
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
        // XDG trash spec location (logical target; you can handle it upstream)
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

    void maybeRefresh() {
        const QByteArray h = snapshotHash();
        if (h != lastHash) {
            lastHash = h;
            refresh();
        }
    }

    QByteArray snapshotHash() const {
        QByteArray sum;
        auto hashOneDir = [&sum](const QString &p){
            QDir d(p);
            d.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
            d.setSorting(QDir::Name | QDir::IgnoreCase);
            for (const QFileInfo &fi : d.entryInfoList()) {
                sum.append(fi.fileName().toUtf8());
                sum.append(char(fi.isDir()));
                sum.append(char(fi.size() & 0xFF));
            }
        };
        hashOneDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        for (const QStorageInfo &st : QStorageInfo::mountedVolumes()) {
            if (!st.isValid() || !st.isReady()) continue;
            const QString mp = st.rootPath();
            if (st.isRemovable() || mp.startsWith("/media/") || mp.startsWith("/run/media/"))
                sum.append(mp.toUtf8());
        }
        return qHash(sum).toHex();
    }
};

// Helper to build and return the widget
inline QWidget* buildDesktopIcons(QWidget *parent = nullptr) {
    return new DesktopIconsWidget(parent);
}
