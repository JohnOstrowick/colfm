// keys.h
#ifndef KEYS_H
#define KEYS_H

#include <QKeyEvent>
#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QVariant>
#include <QPointer>

class ColFM; // forward declaration

// Helper: try to call a ColFM slot by name. Returns true if invoked, false otherwise.
inline bool tryInvoke(ColFM *win, const char *slot)
{
    if (!win || !slot) return false;
    // Attempt queued first; if not found, fall back to direct (Qt checks anyway)
    bool ok = QMetaObject::invokeMethod(win, slot, Qt::AutoConnection);
    return ok;
}

// Helper: small visible stub if an action/slot isn’t present yet.
inline void stub(QWidget *parent, const char *label)
{
    QMessageBox::information(parent, "ColFM", QString::fromLatin1(label));
}

// Helper: navigate breadcrumbs text field, if exposed via a slot.
// We call generic “focusBreadcrumbs” first; if absent, fall back stubs.
inline bool focusBreadcrumbs(ColFM *win)
{
    if (tryInvoke(win, "focusBreadcrumbs")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Goto breadcrumb (focus)");
    return true;
}

// Helper: focus search dialog (plocate/find). Prefer “focusSearch” slot.
inline bool focusSearch(ColFM *win)
{
    if (tryInvoke(win, "focusSearch")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Find dialog");
    return true;
}

// Helper: open info panel on current selection/path.
// Prefer a stable slot name “onGetInfo” (or “showGetInfoDialog”).
inline bool showInfo(ColFM *win)
{
    if (tryInvoke(win, "onInfo")) return true;
    if (tryInvoke(win, "showGetInfoDialog")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Get Info");
    return true;
}

// Helper: open selected file/folder.
inline bool doOpen(ColFM *win)
{
    if (tryInvoke(win, "onOpen")) return true;
    if (tryInvoke(win, "openSelection")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Open");
    return true;
}

// Helper: rename selected item.
inline bool doRename(ColFM *win)
{
    if (tryInvoke(win, "onRename")) return true;
    if (tryInvoke(win, "renameSelection")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Rename");
    return true;
}

// Helper: new window/tab/folder.
inline bool newWindow(ColFM *win)
{
    if (tryInvoke(win, "onNewWindow")) return true;
    stub(reinterpret_cast<QWidget*>(win), "New Window");
    return true;
}
inline bool newTab(ColFM *win)
{
    if (tryInvoke(win, "onNewTab")) return true;
    stub(reinterpret_cast<QWidget*>(win), "New Tab");
    return true;
}
inline bool newFolder(ColFM *win)
{
    if (tryInvoke(win, "onNewFolder")) return true;
    if (tryInvoke(win, "makeNewFolder")) return true;
    stub(reinterpret_cast<QWidget*>(win), "New Folder");
    return true;
}

// Helper: place into new folder.
inline bool placeIntoNewFolder(ColFM *win)
{
    if (tryInvoke(win, "onPlaceIntoNewFolder")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Place into New Folder");
    return true;
}

// Helper: clipboard and edit.
inline bool doCopy(ColFM *win) { if (tryInvoke(win, "onCopy")) return true; stub(reinterpret_cast<QWidget*>(win), "Copy"); return true; }
inline bool doCut (ColFM *win) { if (tryInvoke(win, "onCut"))  return true; stub(reinterpret_cast<QWidget*>(win), "Cut");  return true; }
inline bool doPaste(ColFM *win) { if (tryInvoke(win, "onPaste"))return true; stub(reinterpret_cast<QWidget*>(win), "Paste");return true; }
inline bool doUndo(ColFM *win) { if (tryInvoke(win, "onUndo")) return true; stub(reinterpret_cast<QWidget*>(win), "Undo"); return true; }
inline bool doRedo(ColFM *win) { if (tryInvoke(win, "onRedo")) return true; stub(reinterpret_cast<QWidget*>(win), "Redo"); return true; }
inline bool selectAll(ColFM *win){ if (tryInvoke(win, "onSelectAll")) return true; stub(reinterpret_cast<QWidget*>(win), "Select All"); return true; }

// Helper: navigation.
inline bool goHome(ColFM *win)  { if (tryInvoke(win, "onHome"))   return true; stub(reinterpret_cast<QWidget*>(win), "Home");   return true; }
inline bool goUp(ColFM *win)    { if (tryInvoke(win, "onUp"))     return true; stub(reinterpret_cast<QWidget*>(win), "Up");     return true; }
inline bool goBack(ColFM *win)  { if (tryInvoke(win, "onBack"))   return true; stub(reinterpret_cast<QWidget*>(win), "Back");   return true; }
inline bool goForward(ColFM *win){if (tryInvoke(win, "onForward"))return true; stub(reinterpret_cast<QWidget*>(win), "Forward");return true; }
inline bool goDesktop(ColFM *win){if (tryInvoke(win, "onDesktop"))return true; stub(reinterpret_cast<QWidget*>(win), "Desktop");return true; }
inline bool goRoot(ColFM *win)  { if (tryInvoke(win, "onRoot"))   return true; stub(reinterpret_cast<QWidget*>(win), "Root");   return true; }

// Helper: eject / terminal / print / prefs / linkfile / move / find original.
inline bool doEject(ColFM *win) { if (tryInvoke(win, "onEject")) return true; stub(reinterpret_cast<QWidget*>(win), "Eject"); return true; }
inline bool newTerminalHere(ColFM *win){ if (tryInvoke(win, "onNewTerminalHere")) return true; stub(reinterpret_cast<QWidget*>(win), "New Terminal Here"); return true; }
inline bool doPrint(ColFM *win) { if (tryInvoke(win, "onPrint")) return true; stub(reinterpret_cast<QWidget*>(win), "Print"); return true; }
inline bool showPrefs(ColFM *win){ if (tryInvoke(win, "onPreferences")) return true; stub(reinterpret_cast<QWidget*>(win), "Preferences"); return true; }
inline bool makeLink(ColFM *win){ if (tryInvoke(win, "onMakeLink")) return true; stub(reinterpret_cast<QWidget*>(win), "Make Linkfile"); return true; }
inline bool doMove(ColFM *win)  { if (tryInvoke(win, "onMove")) return true; stub(reinterpret_cast<QWidget*>(win), "Move"); return true; }
inline bool findOriginal(ColFM *win){ if (tryInvoke(win, "onFindOriginal")) return true; stub(reinterpret_cast<QWidget*>(win), "Find Original"); return true; }
inline bool saveViewForFolder(ColFM *win){ if (tryInvoke(win, "onSaveViewForFolder")) return true; stub(reinterpret_cast<QWidget*>(win), "View saved for this folder"); return true; }

// Helper: delete/trash variants.
inline bool trashNoPrompt(ColFM *win)
{
    if (tryInvoke(win, "onTrashNoPrompt")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Move to Trash (no prompt)");
    return true;
}
inline bool trashWithPrompt(ColFM *win)
{
    if (tryInvoke(win, "onTrashWithPrompt")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Move to Trash (confirm)");
    return true;
}

// Arrow-key behaviours.
// Icon view: Up = cd .. ; Right/Down = cd into selected; Left = back
// Column/List: Up/Down as selection; Left = back; Right = open/into
inline bool arrowUp(ColFM *win)
{
    if (tryInvoke(win, "onArrowUp")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Arrow Up");
    return true;
}
inline bool arrowDown(ColFM *win)
{
    if (tryInvoke(win, "onArrowDown")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Arrow Down");
    return true;
}
inline bool arrowLeft(ColFM *win)
{
    if (tryInvoke(win, "onArrowLeft")) return true;
    if (tryInvoke(win, "onBack")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Arrow Left / Back");
    return true;
}
inline bool arrowRight(ColFM *win)
{
    if (tryInvoke(win, "onArrowRight")) return true;
    if (tryInvoke(win, "onOpen")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Arrow Right / Open");
    return true;
}

// Help dialog for listing keys (hook to a central help view later).
inline bool showHelp(ColFM *win)
{
    if (tryInvoke(win, "onShowKeyboardHelp")) return true;
    stub(reinterpret_cast<QWidget*>(win), "Help — Keyboard Shortcuts");
    return true;
}

// Main handler — returns true if the key was handled.
inline bool handleKeyEvent(ColFM *win, QKeyEvent *ev)
{
    if (!win || !ev) return false;

    const int key = ev->key();
    const Qt::KeyboardModifiers mods = ev->modifiers();
    const bool ctrl  = mods & Qt::ControlModifier;
    const bool shift = mods & Qt::ShiftModifier;
    const bool alt   = mods & Qt::AltModifier;

    // ----- Ctrl-based shortcuts -----
    if (ctrl) {
        switch (key) {
        case Qt::Key_A: return selectAll(win);
        case Qt::Key_B: return placeIntoNewFolder(win) ? true : (makeLink(win), true); // B = Bookmark (repurposed to Place into New Folder per spec)
        case Qt::Key_C: return doCopy(win);
        case Qt::Key_D: return goDesktop(win);
        case Qt::Key_E: return doEject(win);
        case Qt::Key_F: return focusSearch(win);
        case Qt::Key_G: return focusBreadcrumbs(win);
        case Qt::Key_H: { // Minimise window
            QWidget *w = reinterpret_cast<QWidget*>(win);
            if (w) w->showMinimized();
            return true;
        }
        case Qt::Key_I: win->onInfo(); return true;
        case Qt::Key_J: return saveViewForFolder(win);
        case Qt::Key_L: return makeLink(win);
        case Qt::Key_M: return doMove(win);
        case Qt::Key_N: return newWindow(win);
	case Qt::Key_O: win->onOpen(); return true;
        case Qt::Key_P: return doPrint(win);
        case Qt::Key_Q: qApp->quit(); return true;
        case Qt::Key_R: return findOriginal(win);
        case Qt::Key_T: return newTab(win);
        case Qt::Key_U: return goUp(win);
        case Qt::Key_V: return doPaste(win);
	case Qt::Key_W: { QWidget *w = QApplication::activeModalWidget(); if (w) w->close(); else reinterpret_cast<QWidget*>(win)->close(); return true; }
        case Qt::Key_X: return doCut(win);
        case Qt::Key_Y: return doRedo(win);
        case Qt::Key_Z: return doUndo(win);
        case Qt::Key_Comma: return showPrefs(win);   // Ctrl+, => Preferences
        case Qt::Key_Backspace: return trashNoPrompt(win); // Ctrl+Backspace => Trash (no prompt)
        case Qt::Key_Delete:    return trashNoPrompt(win); // Ctrl+Delete    => Trash (no prompt)
        default: break;
        }
    }

    // Special combos
    if (ctrl && shift && key == Qt::Key_N) return newFolder(win); // Ctrl+Shift+N => New Folder
    if (ctrl && alt   && key == Qt::Key_N) return newFolder(win); // Ctrl+Alt+N   => New Folder

    // ----- Non-ctrl keys -----
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:   return doOpen(win);             // Enter => Open
    case Qt::Key_Slash:   return goRoot(win);             // / => Root
    case Qt::Key_Backslash: return focusBreadcrumbs(win); // \ => Breadcrumb focus
    case Qt::Key_Question:  return showHelp(win);         // ? => Help
    case Qt::Key_Delete:    return trashWithPrompt(win);  // Delete => Trash (prompt)
    if (ev->text() == "~") return goHome(win); // tilde/home
    case Qt::Key_Up:        return arrowUp(win);
    case Qt::Key_Down:      return arrowDown(win);
    case Qt::Key_Left:      return arrowLeft(win);
    case Qt::Key_Right:     return arrowRight(win);
    default: break;
    }

    // Shift+Enter => Rename
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter) {
        if (shift) return doRename(win);
    }

    // Back/Forward with Alt+Left / Alt+Right (common on Linux)
    if (alt && key == Qt::Key_Left)  return goBack(win);
    if (alt && key == Qt::Key_Right) return goForward(win);

    return false; // not handled
}

#endif // KEYS_H
