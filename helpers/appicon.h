#ifndef APPICON_H
#define APPICON_H

#include <QMessageBox>

inline void ColFM::onAppIcon(const QString &msg) {
    QMessageBox::information(this, "Application Menu", "You selected: " + msg);
}

inline void ColFM::lockSession() {
    // Try common Linux session lock commands
    if (system("xdg-screensaver lock") == 0) return;
    if (system("gnome-screensaver-command -l") == 0) return;
    if (system("loginctl lock-session") == 0) return;
    if (system("dm-tool lock") == 0) return;
    if (system("xlock") == 0) return;
    if (system("slock") == 0) return;

    // If none worked, show a fallback message
    QMessageBox::warning(this, "Lock Failed", "Could not lock session: no known locker succeeded.");
}

inline void ColFM::shutdownNow() {
    QProcess::startDetached("shutdown -h now");
}

inline void ColFM::rebootNow() {
    QProcess::startDetached("shutdown -r now");
}

inline void ColFM::forceQuitApp() {
    QProcess::startDetached("xkill");
}

inline void ColFM::openProcessManager() {
    QProcess::startDetached("gnome-system-monitor");
}


#endif // APPICON_H
