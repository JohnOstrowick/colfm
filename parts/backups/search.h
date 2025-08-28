#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QListView>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QFileInfo>

namespace Search {
// External helpers (must exist in your project somewhere)
extern bool _ensureVolDb(const QString &volPath);
extern QString _volDbPath(const QString &volPath);

inline QStringList runPlocateDialogAndSearch(QWidget *parent,
                                             QString *outScopeRoot = nullptr,
                                             bool *outShowHidden = nullptr)
{
    // --- dialog ---
    QDialog dlg(parent);
    dlg.setWindowTitle("Search with plocate");

    auto *edit  = new QLineEdit(&dlg); edit->setPlaceholderText("Search term…");
    auto *scope = new QComboBox(&dlg);
    auto *showHidden = new QCheckBox("Show hidden files", &dlg); showHidden->setChecked(false);

    const QString home = QDir::homePath();
    scope->addItem("My Home", home);
    scope->addItem("Entire System", "/");
    scope->addItem("/var", "/var");

    const QString user = qEnvironmentVariable("USER");
    const QString mediaRoot = QString("/media/%1").arg(user);
    scope->addItem("USB Drives", mediaRoot);

    QDir mediaDir(mediaRoot);
    if (mediaDir.exists()) {
        for (const QString &vol : mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString volPath = mediaDir.absoluteFilePath(vol);
            scope->addItem(QString("Media: %1").arg(vol), volPath);
        }
    }

    auto *form = new QFormLayout();
    form->addRow("Search term:", edit);
    form->addRow("Scope:", scope);
    form->addRow("", showHidden);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addWidget(btns);

    edit->setFocus();
    if (dlg.exec() != QDialog::Accepted) return {};

    const QString query = edit->text().trimmed();
    if (query.isEmpty()) return {};

    const QString selRoot = scope->currentData().toString();
    if (outScopeRoot) *outScopeRoot = selRoot;
    if (outShowHidden) *outShowHidden = showHidden->isChecked();

    // --- build per-volume DB(s) if scope is under /media/$USER ---
    QStringList dbArgs;
    if (selRoot.startsWith(mediaRoot)) {
        QStringList volumes;
        if (selRoot == mediaRoot) {
            for (const QString &vol : mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                volumes << mediaDir.absoluteFilePath(vol);
            }
        } else {
            volumes << selRoot;
        }
        for (const QString &volPath : volumes) {
            if (_ensureVolDb(volPath)) {
                dbArgs << "-d" << _volDbPath(volPath);
            }
        }
    }

    // --- run plocate ---
    QStringList args;
    args << "-i" << "--basename" << query;
    if (!dbArgs.isEmpty()) args << dbArgs;

    QProcess proc;
    proc.start("plocate", args);
    proc.waitForFinished(-1);

    QStringList all = QString::fromUtf8(proc.readAllStandardOutput())
                          .split('\n', Qt::SkipEmptyParts);

    // --- scope filter (if not using custom dbs) ---
    QStringList scoped;
    if (!dbArgs.isEmpty()) {
        scoped = all;
    } else if (selRoot == "/") {
        scoped = all;
    } else {
        auto inScope = [&](const QString &p){
            return p == selRoot || p.startsWith(selRoot + "/");
        };
        for (const auto &p : all) if (inScope(p)) scoped << p;
    }

    // --- hidden filter ---
    if (!showHidden->isChecked()) {
        QRegularExpression hiddenRe("(^|/)\\.[^/]+");
        QStringList filtered;
        for (const QString &p : scoped) {
            if (hiddenRe.match(p).hasMatch()) continue;
            filtered << p;
        }
        scoped = filtered;
    }

    return scoped;
}

// --- Wire ColFM to the helper ---

inline void onSearchPlocate() {
    QString scopeRoot; bool showHidden = false;
    const QStringList results = runPlocateDialogAndSearch(this, &scopeRoot, &showHidden);
    if (results.isEmpty()) {
        statusBar()->showMessage("No results found", 2000);
        return;
    }

    auto *lv = new QListView();
    auto *listModel = new QStringListModel(results, lv);
    lv->setModel(listModel);
    lv->setUniformItemSizes(true);
    lv->setSelectionMode(QAbstractItemView::SingleSelection);
    currentView = lv;
    setCentralWidget(lv);

    QObject::connect(lv, &QListView::doubleClicked, this, [this, listModel](const QModelIndex &i){
        if (!i.isValid()) return;
        const QString path = listModel->data(i, Qt::DisplayRole).toString();
        QFileInfo fi(path);
        if (fi.isDir()) {
            currentRoot = model->index(fi.absoluteFilePath());
            setViewMode(mode);
            if (crumbs) crumbs->setPath(fi.absoluteFilePath());
        } else {
            const QString parent = fi.absolutePath();
            currentRoot = model->index(parent);
            setViewMode(mode);
            if (crumbs) crumbs->setPath(parent);
            const QModelIndex fileIdx = model->index(path);
            if (fileIdx.isValid()) previewFile(fileIdx);
        }
    });
}
}
