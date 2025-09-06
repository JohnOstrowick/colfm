#ifndef MOVE_H
#define MOVE_H

// ==== Qt headers (UI + FS + IPC) ====
#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

// ==== STL ====
#include <functional>
#include <memory>

// If your getCWD() is a free function in new.h, forward declare here.
// If it lives as ColFM::getCWD(), change the lambda used in startDnDServer() call-site accordingly.
QString getCWD();

//
// ──────────────────────────────────────────────────────────────────────────────
//   ORIGINAL MOVE DIALOG (preserved)
// ──────────────────────────────────────────────────────────────────────────────
//
class MoveDialog : public QDialog {
public:
    explicit MoveDialog(const QString &currentDir, QWidget *parent = nullptr)
        : QDialog(parent), currentDir_(currentDir)
    {
        setWindowTitle(QStringLiteral("Move to…"));

        auto *layout = new QVBoxLayout(this);

        // Current directory label
        auto *dirLabel = new QLabel(this);
        dirLabel->setText(QStringLiteral("Current directory: %1").arg(currentDir_));
        layout->addWidget(dirLabel);

        // Folder list
        folderList_ = new QListWidget(this);
        layout->addWidget(folderList_);

        populateFolderList();

        // Elsewhere input
        elsewhereEdit_ = new QLineEdit(this);
        elsewhereEdit_->setPlaceholderText(QStringLiteral("Elsewhere: enter full path"));
        layout->addWidget(elsewhereEdit_);

        // Buttons
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &MoveDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &MoveDialog::reject);

        // Double-click behaviour on folder list to accept quickly
        connect(folderList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *){
            accept();
        });
    }

    QString destination() const {
        // Priority: Elsewhere edit if set
        const QString typed = elsewhereEdit_->text().trimmed();
        if (!typed.isEmpty()) return typed;

        // Else a selection from the list
        auto *it = folderList_->currentItem();
        if (!it) return QString();

        const QString choice = it->text();
        if (choice.startsWith("..")) {
            // Up one level
            QDir d(currentDir_);
            d.cdUp();
            return d.absolutePath();
        }
        return QDir(currentDir_).filePath(choice);
    }

private:
    void populateFolderList() {
        folderList_->clear();

        QDir d(currentDir_);
        folderList_->addItem(QStringLiteral(".. (Up one level)"));

        const QStringList folders = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &f : folders) {
            folderList_->addItem(f);
        }
    }

    QString currentDir_;
    QListWidget *folderList_ = nullptr;
    QLineEdit *elsewhereEdit_ = nullptr;
};

//
// ──────────────────────────────────────────────────────────────────────────────
//   MOVE API (header-only; minimal hooks elsewhere)
// ──────────────────────────────────────────────────────────────────────────────
//
namespace Move {

enum class Mode { None, Copy, Cut };

struct Clipboard {
    inline static QStringList paths;
    inline static Mode mode = Mode::None;
};

// ── Internal helpers ──────────────────────────────────────────────────────────

inline QString humanError(const QString &path, const QString &why) {
    return QStringLiteral("%1: %2").arg(path, why);
}

inline bool ensureDirExists(const QString &dirPath, QString *err = nullptr) {
    QDir d(dirPath);
    if (d.exists()) return true;
    if (d.mkpath(QStringLiteral("."))) return true;
    if (err) *err = humanError(dirPath, QStringLiteral("cannot create directory"));
    return false;
}

inline QString uniqueTargetName(const QString &targetDir, const QString &baseName) {
    // If no conflict, return as-is.
    QString candidate = QDir(targetDir).filePath(baseName);
    if (!QFileInfo::exists(candidate)) return candidate;

    // Split name and extension
    const QFileInfo fi(baseName);
    const QString stem = fi.completeBaseName();
    const QString ext  = fi.suffix().isEmpty() ? QString() : QStringLiteral(".") + fi.suffix();

    int n = 1;
    while (true) {
        const QString alt = QDir(targetDir).filePath(QStringLiteral("%1 (copy %2)%3").arg(stem).arg(n).arg(ext));
        if (!QFileInfo::exists(alt)) return alt;
        ++n;
    }
}

inline bool copyFilePreserve(const QString &src, const QString &dst, QString *err = nullptr) {
    // Ensure parent exists
    if (!ensureDirExists(QFileInfo(dst).absolutePath(), err)) return false;

    // Don’t overwrite; pick a unique target if dst exists
    QString actualDst = dst;
    if (QFileInfo::exists(actualDst)) {
        actualDst = uniqueTargetName(QFileInfo(dst).absolutePath(), QFileInfo(dst).fileName());
    }

    QFile in(src);
    if (!in.exists()) { if (err) *err = humanError(src, QStringLiteral("source missing")); return false; }
    if (!in.copy(actualDst)) { if (err) *err = humanError(actualDst, QStringLiteral("copy failed")); return false; }

    // Preserve permissions if possible
    QFile(actualDst).setPermissions(in.permissions());
    return true;
}

inline bool copyDirectoryRecursively(const QString &srcDir, const QString &dstDir, QString *err = nullptr) {
    QDir src(srcDir);
    if (!src.exists()) { if (err) *err = humanError(srcDir, QStringLiteral("source dir missing")); return false; }
    if (!ensureDirExists(dstDir, err)) return false;

    // Iterate entries (skip . and ..)
    QDirIterator it(srcDir, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        const QString rel = QDir(srcDir).relativeFilePath(info.absoluteFilePath());
        const QString outPath = QDir(dstDir).filePath(rel);

        if (info.isDir()) {
            if (!ensureDirExists(outPath, err)) return false;
        } else {
            if (!copyFilePreserve(info.absoluteFilePath(), outPath, err)) return false;
        }
    }
    return true;
}

inline bool removeRecursively(const QString &path, QString *err = nullptr) {
    QFileInfo fi(path);
    if (!fi.exists()) return true;
    if (fi.isDir()) {
        QDir d(path);
        if (!d.removeRecursively()) { if (err) *err = humanError(path, QStringLiteral("removeRecursively failed")); return false; }
        return true;
    }
    if (!QFile::remove(path)) { if (err) *err = humanError(path, QStringLiteral("remove failed")); return false; }
    return true;
}

inline bool moveEntry(const QString &srcPath, const QString &dstDir, QString *err = nullptr) {
    QFileInfo sfi(srcPath);
    if (!sfi.exists()) { if (err) *err = humanError(srcPath, QStringLiteral("source missing")); return false; }
    if (!ensureDirExists(dstDir, err)) return false;

    const QString dstCandidate = QDir(dstDir).filePath(sfi.fileName());

    // Try fast rename first
    if (QFile::rename(srcPath, dstCandidate)) {
        return true;
    }

    // On failure (e.g., cross-device), fall back to copy then delete
    if (sfi.isDir()) {
        if (!copyDirectoryRecursively(srcPath, dstCandidate, err)) return false;
        if (!removeRecursively(srcPath, err)) return false;
        return true;
    } else {
        if (!copyFilePreserve(srcPath, dstCandidate, err)) return false;
        if (!QFile::remove(srcPath)) { if (err) *err = humanError(srcPath, QStringLiteral("remove after copy failed")); return false; }
        return true;
    }
}

inline bool copyEntry(const QString &srcPath, const QString &dstDir, QString *err = nullptr) {
    QFileInfo sfi(srcPath);
    if (!sfi.exists()) { if (err) *err = humanError(srcPath, QStringLiteral("source missing")); return false; }
    if (!ensureDirExists(dstDir, err)) return false;

    const QString dstCandidate = QDir(dstDir).filePath(sfi.fileName());
    if (sfi.isDir()) {
        return copyDirectoryRecursively(srcPath, dstCandidate, err);
    } else {
        return copyFilePreserve(srcPath, dstCandidate, err);
    }
}

// ── Registry for multiple instances (temp JSON file) ──────────────────────────

inline QString registryFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    return QDir(dir).filePath(QStringLiteral("colfm_dnd_registry.json"));
}

inline QString makeServerName() {
    return QStringLiteral("colfm_dnd_%1").arg(QCoreApplication::applicationPid());
}

inline void writeRegistry(const QStringList &names) {
    QJsonArray arr;
    for (const QString &n : names) arr.append(n);
    QJsonObject obj{{"updated", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}, {"servers", arr}};
    QFile f(registryFile());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

inline QStringList readRegistry() {
    QFile f(registryFile());
    if (!f.open(QIODevice::ReadOnly)) return {};
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return {};
    const auto arr = doc.object().value(QStringLiteral("servers")).toArray();
    QStringList out;
    for (const auto &v : arr) out << v.toString();
    return out;
}

inline void registryAdd(const QString &name) {
    auto list = readRegistry();
    if (!list.contains(name)) {
        list << name;
        writeRegistry(list);
    }
}

inline void registryRemove(const QString &name) {
    auto list = readRegistry();
    list.removeAll(name);
    writeRegistry(list);
}

// ── IPC server (answers “WHAT_IS_YOUR_CWD?”) ──────────────────────────────────

inline void startDnDServer(std::function<QString()> getCWDProvider) {
    static std::unique_ptr<QLocalServer> server;
    static QString myName;
    static bool installedQuitHook = false;

    if (server) return; // already running

    myName = makeServerName();
    // Clean up any stale socket with same name
    QLocalServer::removeServer(myName);

    server = std::make_unique<QLocalServer>();
    if (!server->listen(myName)) {
        // If we fail to listen (very unlikely with PID in name), just return; no IPC for this instance.
        server.reset();
        return;
    }

    registryAdd(myName);

    QObject::connect(server.get(), &QLocalServer::newConnection, server.get(), [getCWDProvider, srv = server.get()](){
        while (QLocalSocket *sock = srv->nextPendingConnection()) {
            QObject::connect(sock, &QLocalSocket::readyRead, sock, [sock, getCWDProvider]() {
                const QByteArray req = sock->readAll();
                if (req.startsWith("WHAT_IS_YOUR_CWD")) {
                    const QString cwd = getCWDProvider ? getCWDProvider() : QString();
                    const QByteArray reply = cwd.toUtf8();
                    sock->write(reply);
                    sock->flush();
                }
                sock->disconnectFromServer();
            });
            QObject::connect(sock, &QLocalSocket::disconnected, sock, [sock](){ sock->deleteLater(); });
        }
    });

    if (!installedQuitHook) {
        installedQuitHook = true;
	const QString nameCopy = myName;
	QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [nameCopy](){ registryRemove(nameCopy); });
    }
}

// Tries all known servers except ourselves, returns the first CWD we get back.
inline QString requestTargetCWD(int timeoutMs = 500) {
    const QString myName = makeServerName();
    const QStringList servers = readRegistry();

    for (const QString &name : servers) {
        if (name == myName) continue; // don't ask ourselves

        QLocalSocket sock;
        sock.connectToServer(name);
        if (!sock.waitForConnected(timeoutMs)) continue;

        sock.write("WHAT_IS_YOUR_CWD");
        sock.flush();

        if (!sock.waitForReadyRead(timeoutMs)) { sock.abort(); continue; }
        const QByteArray reply = sock.readAll();
        sock.disconnectFromServer();

        const QString cwd = QString::fromUtf8(reply).trimmed();
        if (!cwd.isEmpty()) return cwd;
    }
    return QString();
}

inline QStringList requestAllCWDs(int timeoutMs = 500) {
    QStringList out;
    const QString myName = makeServerName();
    const QStringList servers = readRegistry();

    for (const QString &name : servers) {
        if (name == myName) continue;

        QLocalSocket sock;
        sock.connectToServer(name);
        if (!sock.waitForConnected(timeoutMs)) continue;
        sock.write("WHAT_IS_YOUR_CWD");
        sock.flush();
        if (!sock.waitForReadyRead(timeoutMs)) { sock.abort(); continue; }
        const QString cwd = QString::fromUtf8(sock.readAll()).trimmed();
        sock.disconnectFromServer();
        if (!cwd.isEmpty()) out << cwd;
    }
    return out;
}

// ── Public API: clipboard & operations ────────────────────────────────────────

inline void setClipboard(const QStringList &paths, Mode m) {
    Clipboard::paths = paths;
    Clipboard::mode  = m;
}

inline void clearClipboard() {
    Clipboard::paths.clear();
    Clipboard::mode = Mode::None;
}

// Paste into target directory (returns true if all succeeded)
// Shows QMessageBox on partial failure; also returns false.
inline bool pasteTo(const QString &targetDir, QWidget *parent = nullptr) {
    if (Clipboard::mode == Mode::None || Clipboard::paths.isEmpty()) {
        QMessageBox::information(parent, QStringLiteral("Paste"), QStringLiteral("Nothing to paste."));
        return false;
    }

    QString err;
    bool allOk = true;

    for (const QString &p : Clipboard::paths) {
        const bool ok = (Clipboard::mode == Mode::Copy)
                ? copyEntry(p, targetDir, &err)
                : moveEntry(p, targetDir, &err);
        if (!ok) {
            allOk = false;
            QMessageBox::warning(parent, QStringLiteral("Operation failed"), err);
        }
    }

    // If CUT (move) succeeded for all, clear clipboard; if partial failure, we keep it.
    if (Clipboard::mode == Mode::Cut && allOk) clearClipboard();
    return allOk;
}

// Dialog-based “Move…” using the preserved MoveDialog.
inline bool promptAndMove(const QStringList &paths, QWidget *parent = nullptr) {
    if (paths.isEmpty()) {
        QMessageBox::information(parent, QStringLiteral("Move"), QStringLiteral("No items selected."));
        return false;
    }

    // Use the first item’s directory as the initial directory
    const QFileInfo fi(paths.first());
    const QString initialDir = fi.dir().absolutePath();

    MoveDialog dlg(initialDir, parent);
    if (dlg.exec() != QDialog::Accepted) return false;

    const QString target = dlg.destination();
    if (target.isEmpty()) return false;

    QString err;
    bool allOk = true;
    for (const QString &p : paths) {
        if (!moveEntry(p, target, &err)) {
            allOk = false;
            QMessageBox::warning(parent, QStringLiteral("Move failed"), err);
        }
    }
    return allOk;
}

// Convenience shim to mirror possible existing call-sites
inline bool doMove(QWidget *parent, const QString &singlePath) {
    return promptAndMove(QStringList{singlePath}, parent);
}

// ── Drag-and-drop “probe” (initial step: just show where it would go) ─────────

inline void probeDropTargetAndShow(QWidget *parent = nullptr) {
    // Try to find any other instance(s) and show their CWD(s)
    const QStringList targets = requestAllCWDs();
    if (targets.isEmpty()) {
        QMessageBox::information(parent, QStringLiteral("Drag-and-Drop"), QStringLiteral("No target ColFM instance responded."));
        return;
    }
    if (targets.size() == 1) {
        QMessageBox::information(parent, QStringLiteral("Drag-and-Drop target"), QStringLiteral("Target would be:\n%1").arg(targets.first()));
        return;
    }
    // Multiple targets: show a compact list
    QString msg = QStringLiteral("Targets detected:\n");
    for (const QString &t : targets) msg += QStringLiteral(" • ") + t + QStringLiteral("\n");
    msg += QStringLiteral("\n(Next step: we’ll let you choose one and paste directly.)");
    QMessageBox::information(parent, QStringLiteral("Drag-and-Drop targets"), msg);
}

} // namespace Move

#endif // MOVE_H
