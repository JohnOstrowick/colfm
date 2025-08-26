proposed code for capturing keystrokes:
/* === colfm.cpp — paste EXACTLY these snippets === */

/* 1) Add these includes near your other Qt includes (once) */
#include <QKeyEvent>        /* added */
#include <QInputDialog>     /* added */
#include <QFile>            /* added */
#include <QTextStream>      /* added */

/* 2) Add these declarations inside class ColFM (public or private, your call) */
bool handleShortcut(QKeyEvent *ke);            /* added */
void goBack();                                  /* added */

/* 3) In ColFM::eventFilter(...) REPLACE the body with this minimal dispatcher */
inline bool ColFM::eventFilter(QObject *obj, QEvent *ev) {
    if (ev->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(ev);
        if (handleShortcut(ke)) return true;                /* added */
    }
    return QMainWindow::eventFilter(obj, ev);
}

/* 4) Paste these DEFINITIONS anywhere after the class definition (e.g. below setViewMode) */
void ColFM::goBack() {                                     /* added */
    if (backStack.isEmpty()) return;
    const QString prev = backStack.takeFirst();
    if (prev.isEmpty()) return;
    currentRoot = model->index(prev);
    setViewMode(mode);
}

/* helper: create a new folder named “New Folder”, “New Folder 2”, ... in currentRoot */
static QString createNewFolder(QFileSystemModel *model, const QModelIndex &rootIdx) { /* added */
    QDir d(model->filePath(rootIdx));
    QString base("New Folder");
    QString name = base;
    int n = 2;
    while (d.exists(name)) name = QString("%1 %2").arg(base).arg(n++);
    if (!d.mkdir(name)) return QString();
    return d.absoluteFilePath(name);
}

bool ColFM::handleShortcut(QKeyEvent *ke) {                 /* added */
    const bool ctrl  = ke->modifiers() & Qt::ControlModifier;
    const bool shift = ke->modifiers() & Qt::ShiftModifier;
    const bool alt   = ke->modifiers() & Qt::AltModifier;
    const int  key   = ke->key();

    auto focusBreadcrumb = [this]{ if (crumbs && crumbs->editField()) { crumbs->editField()->setFocus(); crumbs->editField()->selectAll(); } };
    auto gotoPath = [this](const QString &p){ if (!p.isEmpty()) { pushHistory(); currentRoot = model->index(p); setViewMode(mode); } };

    // --- non-modified arrows + misc ---
    if (!ctrl && !shift && !alt) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) { onOpen(); return true; }         // Enter → Open
        if (key == Qt::Key_Question) { QMessageBox::information(this,"Help","See shortcuts in README."); return true; } // ? help stub
        if (key == Qt::Key_Slash)    { gotoPath("/"); return true; }                           // / → root
        if (key == Qt::Key_Backslash){ focusBreadcrumb(); return true; }                      // \ → Goto (breadcrumb)
        if (key == Qt::Key_AsciiTilde) { gotoPath(QDir::homePath()); return true; }          // ~ → home

        if (key == Qt::Key_Left)  { goBack(); return true; }                                  // ← back
        if (mode == ViewMode::Icon) {
            if (key == Qt::Key_Up)    { onUp(); return true; }                                // ↑ up a level in icon view
            if (key == Qt::Key_Right || key == Qt::Key_Down) {                                // → / ↓ cd into selected dir
                QModelIndex idx = currentIndex(); if (idx.isValid() && model->isDir(idx)) { pushHistory(); currentRoot = idx; setViewMode(mode); return true; }
            }
        } else {
            // list/column: views already handle up/down selection; ensure preview updates
            if (key == Qt::Key_Up || key == Qt::Key_Down) {
                QModelIndex cur = currentIndex(); if (cur.isValid()) previewFile(cur);
            }
        }

        // Backspace/Delete (prompt), Ctrl+Backspace/Ctrl+Delete (no prompt) handled below with modifiers
    }

    // --- Shift combos ---
    if (shift && !ctrl && !alt) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) { onRename(); return true; }       // Shift+Enter → Rename
    }

    // --- Ctrl only (primary map) ---
    if (ctrl && !shift && !alt) {
        switch (key) {
            case Qt::Key_A: if (currentView) { currentView->selectAll(); return true; } break;              // Select All
            case Qt::Key_B: { // Bookmark current path
                QFile f(QDir::homePath()+"/.colfm.bookmarks"); if (f.open(QIODevice::Append|QIODevice::Text)) { QTextStream ts(&f); ts << model->filePath(currentRoot) << "\n"; }
                statusBar()->showMessage("Bookmarked", 1200); return true;
            }
            case Qt::Key_D: gotoPath(QDir::homePath()+"/Desktop"); return true;                             // Desktop
            case Qt::Key_C: statusBar()->showMessage("Copy (TODO)", 1200); return true;                     // Copy
            case Qt::Key_E: statusBar()->showMessage("Eject (TODO)", 1200); return true;                    // Eject (sidebar button handles per-drive)
            case Qt::Key_F: onSearchPlocate(); return true;                                                // Find
            case Qt::Key_G: focusBreadcrumb(); return true;                                                // Goto
            case Qt::Key_H: showMinimized(); return true;                                                  // Minimise
            case Qt::Key_I: onInfo(); return true;                                                         // Info
            case Qt::Key_L: onCreateSoftlink(); return true;                                               // Make linkfile
            case Qt::Key_M: onMove(); return true;                                                         // Move
            case Qt::Key_N: { // New window
                QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
                return true;
            }
            case Qt::Key_O: onOpen(); return true;                                                         // Open
            case Qt::Key_P: statusBar()->showMessage("Print (TODO)", 1200); return true;                   // Print
            case Qt::Key_Q: QCoreApplication::quit(); return true;                                         // Quit
            case Qt::Key_R: { // Find original of symlink
                QModelIndex idx = currentIndex();
                if (idx.isValid()) {
                    QFileInfo fi(model->filePath(idx));
                    if (fi.isSymLink()) { gotoPath(fi.symLinkTarget()); return true; }
                }
                statusBar()->showMessage("Not a link", 1000); return true;
            }
            case Qt::Key_T: statusBar()->showMessage("Tabs (TODO)", 1200); return true;                    // New tab
            case Qt::Key_U: onUp(); return true;                                                           // Up a level
            case Qt::Key_V: statusBar()->showMessage("Paste (TODO)", 1200); return true;                   // Paste
            case Qt::Key_W: close(); return true;                                                          // Close window
            case Qt::Key_X: statusBar()->showMessage("Cut (TODO)", 1200); return true;                     // Cut
            case Qt::Key_Y: statusBar()->showMessage("Redo (TODO)", 1200); return true;                    // Redo
            case Qt::Key_Z: statusBar()->showMessage("Undo (TODO)", 1200); return true;                    // Undo
            case Qt::Key_Comma: QMessageBox::information(this,"Preferences","Preferences (stub)"); return true; // Prefs
            case Qt::Key_J: { // Save view prefs for this folder (stub write)
                QFile f(model->filePath(currentRoot)+"/.colfm.view");
                if (f.open(QIODevice::WriteOnly|QIODevice::Text)) {
                    QTextStream ts(&f);
                    QString vm = (mode==ViewMode::Column? "column": mode==ViewMode::Tree? "tree":"icon");
                    ts << "view=" << vm << "\n";
                }
                QMessageBox::information(this,"ColFM","View saved for this folder");
                return true;
            }
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                onMoveToTrash(); return true;                                                              // Ctrl+Backspace/Delete → no prompt
        }
    }

    // --- Ctrl+Shift ---
    if (ctrl && shift && !alt) {
        if (key == Qt::Key_N) { // New folder in place + offer rename
            const QString path = createNewFolder(model, currentRoot);
            if (!path.isEmpty()) {
                QModelIndex ni = model->index(path);
                if (ni.isValid() && currentView) {
                    currentView->setCurrentIndex(ni);
                    onRename();
                }
            }
            return true;
        }
    }

    // --- Ctrl+Alt ---
    if (ctrl && alt && !shift) {
        if (key == Qt::Key_N) { // same as Ctrl+Shift+N
            const QString path = createNewFolder(model, currentRoot);
            if (!path.isEmpty()) {
                QModelIndex ni = model->index(path);
                if (ni.isValid() && currentView) {
                    currentView->setCurrentIndex(ni);
                    onRename();
                }
            }
            return true;
        }
    }

    // --- Plain Backspace/Delete with prompt ---
    if (!ctrl && !alt && (key == Qt::Key_Backspace || key == Qt::Key_Delete)) {
        if (QMessageBox::question(this,"Move to Trash","Move selection to Trash?")==QMessageBox::Yes) {
            onMoveToTrash();
        }
        return true;
    }

    // Spacebar → Get Info (mac-like)
    if (!ctrl && !alt && key == Qt::Key_Space) { onInfo(); return true; }

    return false; // not handled
}
