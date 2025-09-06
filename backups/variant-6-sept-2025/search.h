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


// External helpers (must exist in your project somewhere)

namespace Search {

inline QStringList runPlocateDialogAndSearch(QWidget *parent,
                                             QString *outScopeRoot = nullptr,
                                             bool *outShowHidden = nullptr) {
    // --- dialog ---
    QDialog dlg(parent);
    dlg.setWindowTitle("Search with plocate");

    auto *edit  = new QLineEdit(&dlg); edit->setPlaceholderText("Search term…");
    auto *scope = new QComboBox(&dlg);
    auto *showHidden = new QCheckBox("Show hidden files", &dlg); showHidden->setChecked(false);

    const QString home = QDir::homePath();
    scope->addItem("My Home", home);       // default
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
            volumes << selRoot; // specific volume
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
    if (!dbArgs.isEmpty()) args << dbArgs;  // use per-volume DBs when available

    QProcess proc;
    proc.start("plocate", args);
    proc.waitForFinished(-1);

    QStringList all = QString::fromUtf8(proc.readAllStandardOutput())
                          .split('\n', Qt::SkipEmptyParts);

    // --- scope filter (if not using custom dbs) ---
    QStringList scoped;
    if (!dbArgs.isEmpty()) {
        scoped = all; // already scoped by -d
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
        QRegularExpression hiddenRe("(^|/)\\.[^/]+"); // any dot-starting path component
        QStringList filtered;
        for (const QString &p : scoped) {
            if (hiddenRe.match(p).hasMatch()) continue;
            filtered << p;
        }
        scoped = filtered;
    }

    return scoped;
}

// --- wrapped search function ---
inline void doSearch(ColFM *fm) {
    QString scopeRoot; bool showHidden = false;
    //QStringList results = runPlocateDialogAndSearch(nullptr, &scopeRoot, &showHidden);
    QStringList results = runPlocateDialogAndSearch(fm, &scopeRoot, &showHidden);
	//QMessageBox::information(nullptr, "Search Results",
     //                    results.join("\n"));
    // truncate to 1000 results max
    if (results.size() > 1000) {
        results = results.mid(0, 1000);
    }
    if (results.isEmpty()) {
        return;
    }
    // show results in main window
	auto *lv = new QListView(fm);
	auto *listModel = new QStringListModel(results, lv);
	lv->setModel(listModel);
	lv->setUniformItemSizes(true);
	lv->setSelectionMode(QAbstractItemView::SingleSelection);

	fm->currentView = lv;
	fm->setCentralWidget(lv);

	QObject::connect(lv, &QListView::doubleClicked, fm, [fm, listModel](const QModelIndex &i){
	    if (!i.isValid()) return;
	    const QString path = listModel->data(i, Qt::DisplayRole).toString();
	    QFileInfo fi(path);
	    if (fi.isDir()) {
		fm->currentRoot = fm->model->index(fi.absoluteFilePath());
		fm->setViewMode(fm->mode);
		if (fm->crumbs) fm->crumbs->setPath(fi.absoluteFilePath());
	    } else {
		const QString parent = fi.absolutePath();
		fm->currentRoot = fm->model->index(parent);
		fm->setViewMode(fm->mode);
		if (fm->crumbs) fm->crumbs->setPath(parent);
		const QModelIndex fileIdx = fm->model->index(path);
		if (fileIdx.isValid()) fm->previewFile(fileIdx);
	    }
	});
    // Your original code that manipulates ColFM stays where it was before,
    // now you can call Search::doSearch() from colfm.cpp.
}

} // namespace Search
