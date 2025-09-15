#pragma once
// info.h — Reusable "Get Info" widget and dialog for ColFM (Qt 6)

#include <QWidget>
#include <QLabel>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QImageReader>
#include <QProcess>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDialog>
#include <QBuffer>
#include <QTemporaryDir>
#include <QKeyEvent>
#include <QAction>
#include <QMessageBox>
#include <QLocale>
#include <QDir>
#include <QMessageBox>
#include <QShortcut>

namespace colfm {

class CtrlWFilter : public QObject { public: using QObject::QObject; bool eventFilter(QObject* o, QEvent* e) override { const auto t=e->type(); if (t==QEvent::ShortcutOverride || t==QEvent::KeyPress) { auto *k=static_cast<QKeyEvent*>(e); if ((k->modifiers() & Qt::ControlModifier) && k->key()==Qt::Key_W) { if (auto *d=qobject_cast<QDialog*>(o)) d->close(); e->accept(); return true; } } return QObject::eventFilter(o,e); } };

// ---- small helpers ---------------------------------------------------------
inline QString humanSize(qint64 bytes) {
    static const char *u[] = {"B","KB","MB","GB","TB"};
    double sz = double(bytes); int i = 0;
    while (sz >= 1024.0 && i < 4) { sz /= 1024.0; ++i; }
    return QString::number(sz, 'f', i==0?0:1) + " " + u[i];
}
inline QString permsToString(QFile::Permissions p) {
    auto bit = [p](QFile::Permission perm, QChar c){ return (p & perm) ? c : QChar('-'); };
    QString s;
    s += bit(QFile::ReadOwner,  'r'); s += bit(QFile::WriteOwner, 'w'); s += bit(QFile::ExeOwner,  'x');
    s += bit(QFile::ReadGroup,  'r'); s += bit(QFile::WriteGroup, 'w'); s += bit(QFile::ExeGroup,  'x');
    s += bit(QFile::ReadOther,  'r'); s += bit(QFile::WriteOther, 'w'); s += bit(QFile::ExeOther,  'x');
    return s;
}

// ---- InfoWidget ------------------------------------------------------------
class InfoWidget : public QWidget {
public:
    explicit InfoWidget(QWidget *parent=nullptr) : QWidget(parent) {
        // Top: editable file name (rename)
        auto *topRow = new QHBoxLayout();
        auto *nameLbl = new QLabel("Name:");
        nameEdit = new QLineEdit();
        nameEdit->setClearButtonEnabled(true);
        topRow->addWidget(nameLbl);
        topRow->addWidget(nameEdit);

        // Info (HTML) + preview image/label
        info = new QTextBrowser();
        info->setOpenLinks(false);
        info->setReadOnly(true);
        info->setStyleSheet("QTextBrowser{background:transparent;}");

        preview = new QLabel();
        preview->setAlignment(Qt::AlignCenter);
        preview->setMinimumHeight(180);

        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(8,8,8,8);
        lay->setSpacing(8);
        lay->addLayout(topRow);
        lay->addWidget(info, 2);
        lay->addWidget(preview, 3);
        setLayout(lay);
        setMinimumWidth(460);
        setMinimumHeight(560);

        connect(nameEdit, &QLineEdit::editingFinished, this, [this]{ tryRename(); });
        connect(nameEdit, &QLineEdit::returnPressed,  this, [this]{ tryRename(); });
    }

    void setFile(const QString &path) {
        lastPath = path;
        QFileInfo fi(path);
        nameEdit->setText(fi.fileName());

        QMimeDatabase db; QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
        const QString extra = buildExtraHtml(fi, mt);
        info->setHtml(buildInfoHtml(fi, mt, extra));
        renderPreview(fi, mt);
    }
    QString currentPath() const { return lastPath; }


static QString buildInfoHtml(const QFileInfo &fi, const QMimeType &mt, const QString &extraHtml) {
    const QString type = (mt.isValid()? mt.name() : "unknown").toHtmlEscaped();
    const QString size = fi.isDir()? "-" : humanSize(fi.size());
    const QString mod  = QLocale().toString(fi.lastModified(), QLocale::ShortFormat);
    const QString created = fi.birthTime().isValid()
        ? QLocale().toString(fi.birthTime(), QLocale::ShortFormat) : "-";
    const QString perms = permsToString(fi.permissions());
    const QString owner = fi.owner().toHtmlEscaped();
    const QString group = fi.group().toHtmlEscaped();
    const QString path  = fi.absoluteFilePath().toHtmlEscaped();

    const QString css = R"(
        <style>
          body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,Arial,sans-serif;
               font-size:12px;color:#e6e6e6;background:transparent;margin:0}
          table{width:100%;border-collapse:collapse}
          td.k{font-weight:700;white-space:nowrap;color:#cccccc;padding:2px 8px 2px 0}
          td.v{color:#e6e6e6;word-break:break-word}
          .mono{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,"Liberation Mono",monospace}
          .rule{border-top:1px solid #3a3a3a;height:8px}
          .hint{color:#a0a0a0;font-size:11px}
          pre{margin:0}
        </style>
    )";

    auto row = [](const QString &k, const QString &v){
        return QString("<tr><td class='k'>%1:</td><td class='v'>%2</td></tr>").arg(k, v);
    };

    QString html;
    html += "<html><head>"+css+"</head><body>";
    html += "<table>";
    html += row("Kind", type);
    html += row("Size", size);
    html += row("Where", path);
    html += row("Created", created);
    html += row("Modified", mod);
    html += row("Owner", owner);
    html += row("Group", group);
    html += row("Permissions", "<span class='mono'>"+perms+"</span>");
    html += "</table>";
    html += "<div class='rule'></div>";

    if (!extraHtml.isEmpty()) {
        html += "<table>";
        html += row("Snippet", extraHtml);
        html += "</table>";
        html += "<div class='rule'></div>";
    }

    html += "<table>";
    html += row("Preview", "<span class='hint'>Shown below</span>");
    html += "</table>";
    html += "</body></html>";
    return html;
}

private:
    QLabel *preview{};
    QTextBrowser *info{};
    QLineEdit *nameEdit{};
    QString lastPath;

    void tryRename() {
        if (lastPath.isEmpty()) return;
        QFileInfo fi(lastPath);
        QString newName = nameEdit->text().trimmed();
        if (newName.isEmpty() || newName == fi.fileName()) return;
        if (newName.contains('/')) {
            QMessageBox::warning(this, "Rename", "Invalid name.");
            nameEdit->setText(fi.fileName());
            return;
        }
        QDir dir(fi.absolutePath());
        const QString newPath = dir.absoluteFilePath(newName);
        if (QFile::exists(newPath)) {
            QMessageBox::warning(this, "Rename", "A file with that name already exists.");
            nameEdit->setText(fi.fileName());
            return;
        }
        if (!dir.rename(fi.fileName(), newName)) {
            QMessageBox::warning(this, "Rename", "Rename failed. Check permissions.");
            nameEdit->setText(fi.fileName());
            return;
        }
        // Success: refresh to new path
        lastPath = newPath;
        setFile(lastPath);
    }

    QString buildExtraHtml(const QFileInfo &fi, const QMimeType &mt) {
        if (fi.isFile() && (mt.name().startsWith("text/") || mt.inherits("application/json") || mt.inherits("application/xml"))) {
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

// make a png preview of the file
    bool renderPdfFirstPageToPng(const QString &pdfPath, QString *outPngPath) {
        // Uses poppler-utils (pdftoppm) if available.
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

        if (mt.inherits("application/pdf")) {
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

        if (mt.name().startsWith("text/") || mt.inherits("application/json") || mt.inherits("application/xml")) {
            preview->setText("Text file");
            return;
        }

        preview->setText(fi.fileName());
    }
};

// ---- convenience: non-modal dialog wrapper ---------------------------------
inline QDialog* showInfoDialog(QWidget *parent, const QString &path) {
    auto *dlg = new QDialog(parent);
    dlg->setWindowTitle(QString("Info — %1").arg(QFileInfo(path).fileName()));
    dlg->setLayout(new QVBoxLayout());
    auto *w = new InfoWidget(dlg);
    dlg->layout()->addWidget(w);
    w->setFile(path);
    dlg->resize(480, 640);
auto *f = new CtrlWFilter(dlg); dlg->installEventFilter(f); for (auto *w : dlg->findChildren<QWidget*>()) w->installEventFilter(f);
    dlg->setModal(false);
    dlg->raise(); dlg->activateWindow();
    dlg->show();
    return dlg;
}

} // namespace colfm
