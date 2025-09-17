#ifndef COLFM_MINIMAL_H
#define COLFM_MINIMAL_H

#include <QMainWindow>
#include <QStringList>

class ColFM : public QMainWindow {
public:
    ColFM(QWidget *parent=nullptr);

    QStringList selectItems();
    void drawButtons();
    void onAppIcon(const QString &msg);
    void onFolderize();
    void onRefresh();

    void shutdownNow();
    void rebootNow();
    void forceQuitApp();
    void openProcessManager();
    void lockSession();

    void onMoveToTrash();
    void onEmptyTrash();
    void onOpenTrash();
    void onRestoreFromTrash();

    void onCut(const QString &path);
    void onCopy(const QString &path);
    void onPaste(const QString &targetDir);

    void onUp();
    void onChDir(const QString&);
    void onOpen();
    void openWith(const QString &filePath);
    void onMoveButton();
    void onBack();
    void onNewWindow();
    void onNewFolder();
};

#endif // COLFM_MINIMAL_H
