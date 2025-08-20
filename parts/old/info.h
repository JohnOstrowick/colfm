#pragma once
// info.h — Reusable "Get Info" widget and dialog for ColFM (Qt 6)

#include <QWidget>
#include <QLocale>
#include <QLabel>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QImageReader>
#include <QProcess>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDialog>
#include <QBuffer>
#include <QTemporaryDir>

namespace colfm {

// ---- small helpers ---------------------------------------------------------
inline QString humanSize(qint64 bytes) {
    static const char *u[] = {"B","KB","MB","GB","TB"};
    double sz = double(bytes); int i = 0;
    while (sz >= 1024.0 && i < 4) { sz /= 1024.0; ++i; }
    return QString::number(sz, 'f', i==0?0:1) + " " + u[i];
}
inline QString permsToString(QFile::Permissions p) {
    auto bit = [p](QFile::Permission perm, QChar c){ return (p & perm) ? c : QChar('-'); };
    return QString()
        + bit(QFile::ReadOwner,  'r') + bit(QFile::WriteOwner, 'w') + bit(QFile::ExeOwner,  'x')
        + bit(QFile::ReadGroup,  'r') + bit(QFile::WriteGroup, 'w') + bit(QFile::ExeGroup,  'x')
        + bit(QFile::ReadOther,  'r') + bit(QFile::WriteOther, 'w') + bit(QFile::ExeOther,  'x');
}

// ---- InfoWidget ------------------------------------------------------------
class InfoWidget : public QWidget {
public:
    explicit InfoWidget(QWidget *parent=nullptr) : QWidget(parent) {
        preview = new QLabel();
        preview->setAlignment(Qt::AlignCenter);
        preview->setMinimumHeight(160);

        info = new QTextBrowser();
        info->setOpenLinks(false);
        info->setReadOnly(true);

        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(8,8,8,8);
        lay->addWidget(info, 2);
        lay->addWidget(preview, 3);
        setLayout(lay);
    }

    void setFile(const QString &path) {
        lastPath = path;
        QFileInfo fi(path);
        QMimeDatabase db; QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
        const QString extra = buildExtraHtml(fi, mt);
        info->setHtml(buildInfoHtml(fi, mt, extra));
        renderPreview(fi, mt);
    }
    QString currentPath() const { return lastPath; }

    static QString buildInfoHtml(const QFileInfo &fi, const QMimeType &mt, const QString &extraHtml) {
        const QString name = fi.fileName().toHtmlEscaped();
        const QString type = (mt.isValid()? mt.name() : "unknown").toHtmlEscaped();
        const QString size = fi.isDir()? "-" : humanSize(fi.size());
	const QString mod  = QLocale().toString(fi.lastModified(), QLocale::ShortFormat);
	const QString created = fi.birthTime().isValid()
    ? QLocale().toString(fi.birthTime(), QLocale::ShortFormat)
    : "-";
        const QString perms = permsToString(fi.permissions());
        const QString owner = fi.owner().toHtmlEscaped();
        const QString group = fi.group().toHtmlEscaped();
        const QString path  = fi.absoluteFilePath().toHtmlEscaped();

        const QString css = R"(
            <style>
              body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Arial, sans-serif; font-size:12px; color:#222; }
              .title { font-weight:700; font-size:14px; margin-bottom:8px; }
              .section { border-top:1px solid #e0e0e0; padding:8px 0; }
              .row { display:grid; grid-template-columns:130px 1fr; gap:8px; padding:2px 0; }
              .k { font-weight:600; white-space:nowrap; }
              .v { overflow-wrap:anywhere; }
              .mono { font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace; }
              .hint { color:#666; font-size:11px; }
            </style>
        )";

        auto row = [](const QString &k, const QString &v){
            return QString("<div class='row'><div class='k'>%1</div><div class='v'>%2</div></div>").arg(k, v);
        };

        QString html;
        html += "<html><head>"+css+"</head><body>";
        html += "<div class='title'>" + name + "</div>";
        html += "<div class='section'>";
        html += row("Kind", type);
        html += row("Size", size);
        html += row("Where", path);
        html += row("Created", created);
        html += row("Modified", mod);
        html += row("Owner", owner);
        html += row("Group", group);
        html += row("Permissions", "<span class='mono'>"+perms+"</span>");
        html += "</div>";

        html += "<div class='section'>";
        html += row("Name & Extension", name);
        html += row("", "<span class='hint'>Extension hiding not yet implemented</span>");
        html += "</div>";

        html += "<div class='section'>" + row("Comments", "<span class='hint'>Not implemented</span>") + "</div>";
        html += "<div class='section'>" + row("Open with", type) + "</div>";

        if (!extraHtml.isEmpty())
            html += "<div class='section'>" + extraHtml + "</div>";

        html += "<div class='section'><div class='k'>Preview</div><div class='v hint'>Shown below</div></div>";
        html += "</body></html>";
        return html;
    }

private:
    QLabel *preview{};
    QTextBrowser *info{};
    QString lastPath;

    QString buildExtraHtml(const QFileInfo &fi, const QMimeType &mt) {
        if (fi.isFile() && (mt.name().startsWith("text/") || mt.name()=="application/json" || mt.name()=="application/xml")) {
            if (fi.size() > 256*1024) return {};
            QFile f(fi.absoluteFilePath());
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString s = QString::fromUtf8(f.read(4096));
                s.replace('&',"&amp;").replace('<',"&lt;").replace('>',"&gt;");
                return "<div><div class='k'>Snippet</div><pre class='mono'>"+s+"</pre></div>";
            }
        }
        return {};
    }

    bool renderPdfFirstPageToPng(const QString &pdfPath, QString *outPngPath) {
        // Uses poppler-utils (pdftoppm) if present.
        const QString base = QDir::tempPath() + "/colfm_pdf_"
            + QString::fromLatin1(QCryptographicHash::hash(pdfPath.toUtf8(), QCryptographicHash::Md5).toHex());
        const QString pngPath = base + ".png";

        QProcess p;
        p.start("pdftoppm", {"-png","-singlefile","-f","1","-l","1", pdfPath, base});
        if (!p.waitForFinished(3000)) { p.kill(); return false; }
        if (QFile::exists(pngPath)) { if (outPngPath) *outPngPath = pngPath; return true; }
        return false;
    }

    void renderPreview(const QFileInfo &fi, const QMimeType &mt) {
        preview->clear();

        if (fi.isDir()) { preview->setText("Folder"); return; }

        if (mt.name().startsWith("image/")) {
            QImageReader r(fi.absoluteFilePath());
            r.setAutoTransform(true);
            const QImage img = r.read();
            if (!img.isNull()) {
                QPixmap pm = QPixmap::fromImage(img);
                preview->setPixmap(pm.scaled(preview->size()*devicePixelRatioF(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                preview->setScaledContents(false);
                return;
            }
        }

        if (mt.name()=="application/pdf") {
            QString out;
            if (renderPdfFirstPageToPng(fi.absoluteFilePath(), &out)) {
                QPixmap pm(out);
                if (!pm.isNull()) {
                    preview->setPixmap(pm.scaled(preview->size()*devicePixelRatioF(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    preview->setScaledContents(false);
                    return;
                }
            }
            preview->setText("PDF preview unavailable");
            return;
        }

        if (mt.name().startsWith("text/") || mt.name()=="application/json" || mt.name()=="application/xml") {
            preview->setText("Text file");
            return;
        }

        preview->setText(fi.fileName());
    }
};

// ---- convenience: non‑modal dialog wrapper ---------------------------------
inline QDialog* showInfoDialog(QWidget *parent, const QString &path) {
    auto *dlg = new QDialog(parent);
    dlg->setWindowTitle(QString("Info — %1").arg(QFileInfo(path).fileName()));
    dlg->setLayout(new QVBoxLayout());
    auto *w = new InfoWidget(dlg);
    dlg->layout()->addWidget(w);
    w->setFile(path);
    dlg->resize(420, 560);
    dlg->setModal(false);
    dlg->show();
    return dlg;
}

} // namespace colfm
