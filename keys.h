#ifndef KEYS_H
#define KEYS_H

#include <QKeyEvent>
#include <QApplication>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QColumnView>

class ColFM; // forward declaration only

// ---- Small helper so you see something happen when a command has no real slot yet
inline void stub(QWidget *parent, const char *msg) {
    QMessageBox::information(parent, "ColFM", QString::fromLatin1(msg));
}

// ---- Helpers for the “if nothing is selected, pick first/last” behaviour (all views)

inline QAbstractItemView* findFocusedView(ColFM* win) {
    QWidget *fw = QApplication::focusWidget();
    if (auto *v = qobject_cast<QAbstractItemView*>(fw)) return v;
    // Fallback: try to find any item view in the window
    return win ? win->findChild<QAbstractItemView*>() : nullptr;
}

inline bool selectFirst(QAbstractItemView* v) {
    if (!v) return false;
    QAbstractItemModel *m = v->model();
    if (!m) return false;
    const QModelIndex root = v->rootIndex();
    const int rows = m->rowCount(root);
    if (rows <= 0) return true; // nothing to select, but handled
    const QModelIndex idx = m->index(0, 0, root);
    if (auto *sel = v->selectionModel())
        sel->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    v->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    return true;
}

inline bool selectLast(QAbstractItemView* v) {
    if (!v) return false;
    QAbstractItemModel *m = v->model();
    if (!m) return false;
    const QModelIndex root = v->rootIndex();
    const int rows = m->rowCount(root);
    if (rows <= 0) return true; // nothing to select, but handled
    const QModelIndex idx = m->index(rows - 1, 0, root);
    if (auto *sel = v->selectionModel())
        sel->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    v->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    return true;
}

// Returns true if it consumed the key by selecting first/last (only when nothing is selected)
inline bool handleArrowIfNoneSelected(ColFM* win, int key) {
    QAbstractItemView *v = findFocusedView(win);
    if (!v) return false;
    if (v->currentIndex().isValid()) return false; // let the view’s default behaviour run

    switch (key) {
        case Qt::Key_Up:   return selectFirst(v);
        case Qt::Key_Down: return selectLast(v);
        case Qt::Key_Left:
            if (qobject_cast<QColumnView*>(v)) { // column view: go up one level
                // call existing “Up” action if present
                // NOTE: direct call to slot to avoid meta invoke; stub if missing
                // You’ve implemented onUp() in toolbars.h
                //reinterpret_cast<QWidget*>(win); // keep the cast pattern consistent
		QWidget *dummy = reinterpret_cast<QWidget*>(win);
(void)dummy; // explicitly mark as unused
                // call through if available
                // We can’t include headers; just assume it exists:
                win->onUp();
                return true;
            }
            // icon/list fallback = go to first item
            return selectFirst(v);
        case Qt::Key_Right:
            if (qobject_cast<QColumnView*>(v)) {
                // column view: select first item
                return selectFirst(v);
            }
            // icon/list fallback = go to last item
            return selectLast(v);
        default:
            return false;
    }
}

// ---- Main dispatcher. Return true if handled.
inline bool handleKeyEvent(ColFM *win, QKeyEvent *ev) {
    if (!win || !ev) return false;

    const int key = ev->key();
    const Qt::KeyboardModifiers mods = ev->modifiers();
    const bool ctrl  = mods.testFlag(Qt::ControlModifier);
    const bool shift = mods.testFlag(Qt::ShiftModifier);
    const bool alt   = mods.testFlag(Qt::AltModifier);

    // --- Special: tilde for Home (Qt has no Key_Tilde; rely on text)
    if (!ctrl && !alt && ev->text() == "~") {
        // If you have onHome(), call it; otherwise stub
        // win->onHome();
        stub(reinterpret_cast<QWidget*>(win), "Home");
        return true;
    }

    // --- Ctrl-based shortcuts (only a few wired to real slots you already have)
    if (ctrl) {
        switch (key) {
        case Qt::Key_I:     win->onInfo(); return true;            // works (you confirmed)
        case Qt::Key_O:     win->onOpen(); return true;            // wired to open selection
        case Qt::Key_W: {                                          // close active modal, else window
            if (QWidget *w = QApplication::activeModalWidget()) { w->close(); return true; break;}
            reinterpret_cast<QWidget*>(win)->close(); return true;
        }
        case Qt::Key_N:     // Ctrl+N = new window (stub unless implemented)
            stub(reinterpret_cast<QWidget*>(win), "New Window"); return true;
        case Qt::Key_F:     stub(reinterpret_cast<QWidget*>(win), "Find / Search"); return true;
        case Qt::Key_G:     stub(reinterpret_cast<QWidget*>(win), "Goto (Breadcrumb focus)"); return true;
        case Qt::Key_C:     stub(reinterpret_cast<QWidget*>(win), "Copy"); return true;
        case Qt::Key_X:     stub(reinterpret_cast<QWidget*>(win), "Cut"); return true;
        case Qt::Key_V:     stub(reinterpret_cast<QWidget*>(win), "Paste"); return true;
        case Qt::Key_A:     stub(reinterpret_cast<QWidget*>(win), "Select All"); return true;
        case Qt::Key_U:     // Up one level
            win->onUp(); return true;
        case Qt::Key_T:     stub(reinterpret_cast<QWidget*>(win), "New Tab"); return true;
        case Qt::Key_Comma: stub(reinterpret_cast<QWidget*>(win), "Preferences"); return true;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            stub(reinterpret_cast<QWidget*>(win), "Move to Trash (no prompt)"); return true;
        case Qt::Key_Q:     qApp->quit(); return true;
        default: break;
        }
    }

    // Shift+Enter => Rename (stubbed unless you have onRename())
    if ((key == Qt::Key_Return || key == Qt::Key_Enter) && shift) {
        if ((ev->key()==Qt::Key_Return || ev->key()==Qt::Key_Enter) && (ev->modifiers() & Qt::ShiftModifier)) { win->onRename(); return true; }
        return true;
    }

    // --- Plain keys
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:   win->onOpen(); return true; // you wired Enter earlier
    case Qt::Key_Slash:   stub(reinterpret_cast<QWidget*>(win), "Go to Root"); return true;
    case Qt::Key_Backslash: stub(reinterpret_cast<QWidget*>(win), "Goto (Breadcrumb focus)"); return true;
    case Qt::Key_Question:  stub(reinterpret_cast<QWidget*>(win), "Help — Keyboard Shortcuts"); return true;
    case Qt::Key_Delete:    stub(reinterpret_cast<QWidget*>(win), "Move to Trash (confirm)"); return true;

    // Arrow keys: only intercept when nothing is selected, otherwise let view handle it
    case Qt::Key_Up:       if (handleArrowIfNoneSelected(win, Qt::Key_Up))   return true; break;
    case Qt::Key_Down:     if (handleArrowIfNoneSelected(win, Qt::Key_Down)) return true; break;
    case Qt::Key_Left:     if (handleArrowIfNoneSelected(win, Qt::Key_Left)) return true; break;
    case Qt::Key_Right:    if (handleArrowIfNoneSelected(win, Qt::Key_Right))return true; break;

    default: break;
    }

    // Back/Forward with Alt+Left / Alt+Right (optional – stub)
    if (alt && key == Qt::Key_Left)  { stub(reinterpret_cast<QWidget*>(win), "Back"); return true; }
    if (alt && key == Qt::Key_Right) { stub(reinterpret_cast<QWidget*>(win), "Forward"); return true; }

    return false; // not handled
}

#endif // KEYS_H
