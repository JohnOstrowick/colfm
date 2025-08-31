#ifndef MOVE_H
#define MOVE_H

#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>

class MoveDialog : public QDialog {
public:
    explicit MoveDialog(const QString &srcPath, QWidget *parent = nullptr)
        : QDialog(parent), sourcePath(srcPath) 
    {
        setWindowTitle("Move");

        QVBoxLayout *layout = new QVBoxLayout(this);

        // Folder list
        folderList = new QListWidget(this);
        layout->addWidget(new QLabel("Select destination folder:"));
        layout->addWidget(folderList);

        // Populate folders
        QDir dir = QFileInfo(sourcePath).absoluteDir();
        QString cwd = dir.absolutePath();
        QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        folderList->addItem(".. (Up one level)");
        for (const QString &f : folders) {
            folderList->addItem(f);
        }

        // Elsewhere input
        elsewhereEdit = new QLineEdit(this);
        elsewhereEdit->setPlaceholderText("Elsewhere: enter full path");
        layout->addWidget(elsewhereEdit);

        // Buttons
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::accepted, this, &MoveDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &MoveDialog::reject);
        layout->addWidget(buttons);
    }

    QString destination() const {
        QString manual = elsewhereEdit->text().trimmed();
        if (!manual.isEmpty()) return manual;

        QListWidgetItem *sel = folderList->currentItem();
        if (!sel) return QString();

        QString text = sel->text();
        if (text.startsWith("..")) {
            QDir dir = QFileInfo(sourcePath).absoluteDir();
            dir.cdUp();
            return dir.absolutePath();
        } else {
            QDir dir = QFileInfo(sourcePath).absoluteDir();
            return dir.absoluteFilePath(text);
        }
    }

    QString sourcePath;
    QListWidget *folderList;
    QLineEdit *elsewhereEdit;
};

namespace Move {
    inline void doMove(QWidget *parent, const QString &filePath) {
        MoveDialog dlg(filePath, parent);
        if (dlg.exec() == QDialog::Accepted) {
            QString dest = dlg.destination();
            if (dest.isEmpty()) return;

            QFileInfo srcInfo(filePath);
            QString finalDest = dest;
            if (QDir(dest).exists()) {
                // If dest is folder, preserve filename
                finalDest = QDir(dest).filePath(srcInfo.fileName());
            }

            if (QFile::rename(filePath, finalDest)) {
                QMessageBox::information(parent, "Move", "Moved to:\n" + finalDest);
            } else {
                QMessageBox::warning(parent, "Move", "Failed to move file.");
            }
        }
    }
}

inline void ColFM::onMoveButton() {
    const QStringList items = selectItems();
    if (items.isEmpty()) {
        if (statusBar()) statusBar()->showMessage("No item selected", 2000);
        return;
    }
    Move::doMove(this, items.first());
}

#endif // MOVE_H
