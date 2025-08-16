// handleopen.h
#pragma once
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QMimeDatabase>
#include <QImageReader>
#include <QPixmap>
#include <QDateTime>
#include <QItemSelectionModel>
#include <QPointer>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>

// ---- ColFM open/preview helpers ----

inline QModelIndex ColFM::currentIndex() const {
    if (!currentView) return QModelIndex();
    auto *sel = currentView->selectionModel();
    if (!sel) return QModelIndex();
    QModelIndex idx = sel->currentIndex();
    if (!idx.isValid()) {
        const auto rows = sel->selectedRows();
        if (!rows.isEmpty()) return rows.first();
    }
    return idx;
}

inline QString humanSize(qint64 bytes) {
    const char *units[] = {"B","KB","MB","GB","TB"};
    double sz = (double)bytes;
    int u = 0;
    while (sz >= 1024.0 && u < 4) { sz /= 1024.0; ++u; }
    return QString::number(sz, 'f', (u==0?0:1)) + " " + units[u];
}

inline QString permsToString(QFile::Permissions p) {
    auto bit = [p](QFile::Permission perm, QChar c){ return (p & perm) ? c : QChar('-'); };
    return QString() +
        bit(QFile::ReadOwner,  'r') + bit(QFile::WriteOwner, 'w') + bit(QFile::ExeOwner,  'x') +
        bit(QFile::ReadGroup,  'r') + bit(QFile::WriteGroup, 'w') + bit(QFile::ExeGroup,  'x') +
        bit(QFile::ReadOther,  'r') + bit(QFile::WriteOther, 'w') + bit(QFile::ExeOther,  'x');
}

inline bool ColFM::isImageFile(const QString &path) const {
    QMimeDatabase db;
    auto mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    return mt.name().startsWith("image/");
}

inline void showInfoPopup(QWidget *parent, const QPixmap &pm, const QString &html) {
    static QPointer<QDialog> dlg;
    static QPointer<QLabel>  lab;
    if (!dlg) {
        dlg = new QDialog(parent, Qt::Tool);
        dlg->setWindowTitle("Info");
        dlg->setAttribute(Qt::WA_DeleteOnClose, false);
        auto *lay = new QVBoxLayout(dlg);
        lab = new QLabel(dlg);
        lab->setTextFormat(Qt::RichText);
        lab->setWordWrap(true);
        lay->addWidget(lab);
    }
    if (!pm.isNull()) {
        lab->setPixmap(pm);
        lab->setText(QString());
    } else {
        lab->setPixmap(QPixmap());
        lab->setText(html);
    }
    dlg->adjustSize();
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

inline void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);
    QFileInfo fi(path);
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);

    // Build info HTML
    const QString htmlInfo = QString(
        "<b>%1</b><br/>"
        "Type: %2<br/>"
        "Size: %3<br/>"
        "Modified: %4<br/>"
        "Permissions: %5<br/>"
        "Path: <tt>%6</tt>")
        .arg(fi.fileName().toHtmlEscaped())
        .arg(mt.isValid() ? mt.name().toHtmlEscaped() : "unknown")
        .arg(fi.isDir() ? "-" : humanSize(fi.size()))
        .arg(fi.lastModified().toString(Qt::ISODate))
        .arg(permsToString(fi.permissions()))
        .arg(path.toHtmlEscaped());

    // IMAGE preview
    if (fi.isFile() && mt.name().startsWith("image/")) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage img = reader.read();
        if (!img.isNull()) {
            QPixmap pm = QPixmap::fromImage(img);
            if (pm.width() > 512) pm = pm.scaledToWidth(512, Qt::SmoothTransformation);
            if (previewLabel) {
                previewLabel->setPixmap(pm);
                previewLabel->setToolTip(path);
            } else {
                showInfoPopup(this, pm, QString());
            }
            return;
        }
    }

    // TEXT preview (small)
    QString html = htmlInfo;
    if (fi.isFile() && mt.name().startsWith("text/") && fi.size() <= 256*1024) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            QString sample = ts.read(4096);
            sample.replace('<', "&lt;").replace('>', "&gt;");
            html += "<br/><br/><b>Preview:</b><br/><pre style='white-space:pre-wrap;'>" + sample + "</pre>";
        }
    }

    if (previewLabel) {
        previewLabel->setPixmap(QPixmap());
        previewLabel->setText(html);
        previewLabel->setToolTip(path);
    } else {
        showInfoPopup(this, QPixmap(), html);
    }
}

inline void ColFM::openApp(const QString &path) {
    QProcess::startDetached(path);
}

inline void ColFM::openFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);
    QFileInfo fi(path);

    if (fi.isDir()) {
        currentRoot = idx;
        if (crumbs) crumbs->setPath(path);
        setViewMode(mode);
        return;
    }

    if (fi.isExecutable()) {
        openApp(path);
        return;
    }

    // Open with system default application
    const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    if (!ok) statusBar()->showMessage("Could not open", 2000);
}
