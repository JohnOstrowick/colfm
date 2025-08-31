#ifndef LABELS_H
#define LABELS_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHBoxLayout>
#include <QToolButton>
#include <QApplication>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QIODevice>
#include <QMenu>
#include <QTimer>
#include <QColor>
#include <QMainWindow>
#include <QToolBar>
#include <QWidgetAction>
#include <QAction>
#include <QMetaObject>
#include <QPixmap>
#include <QImage>
#include <QIcon>
#include <QMap>
#include <QSize>
#include <functional>

class LabelManager {
public:
    // Single source-of-truth label map (filename -> colour name/hex)
    inline static QMap<QString, QString> labelMap;
    inline static std::function<void()> refreshHook;

    // Read .labelcolor into labelMap for a directory
    static inline void readLabelFile(const QString &dirPath) {
        labelMap.clear();
        QFile f(dirPath + QStringLiteral("/.labelcolor"));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            // Expect CSV: "name","colour"
            if (!line.startsWith('"')) continue;
            int sep = line.indexOf("\",\"");
            if (sep <= 0) continue;
            QString fname = line.mid(1, sep - 1);
            QString col = line.mid(sep + 3);
            if (col.endsWith('"')) col.chop(1);
            if (!fname.isEmpty() && !col.isEmpty()) {
                labelMap[fname] = col;
            }
        }
    }

    // Lookup colour string (hex/name) for a file
    static inline QString getLabel(const QString &fileName) {
        return labelMap.value(fileName, QString());
    }

    // Colour mapper (accepts hex or known names)
    static inline QColor colourFromName(const QString &name) {
        QColor c(name); // try hex or named colour string
        if (c.isValid()) return c;

        if (name == QStringLiteral("red"))    return QColor("#e74c3c");
        if (name == QStringLiteral("orange")) return QColor("#f39c12");
        if (name == QStringLiteral("yellow")) return QColor("#f1c40f");
        if (name == QStringLiteral("green"))  return QColor("#2ecc71");
        if (name == QStringLiteral("blue"))   return QColor("#3498db");
        if (name == QStringLiteral("violet")) return QColor("#8e44ad");
        if (name == QStringLiteral("black"))  return QColor("#000000");
        if (name == QStringLiteral("white"))  return QColor("#ffffff");
        if (name == QStringLiteral("grey"))   return QColor("#7f8c8d");
        // "clear" intentionally returns invalid colour (no tint)
        return QColor();
    }

    // Tint an image in-place
    static inline void tintImage(QImage &img, const QColor &c) {
        if (!c.isValid()) return;
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor orig = img.pixelColor(x, y);
                if (orig.alpha() > 0) {
                    int r = (orig.red()   + c.red())   / 2;
                    int g = (orig.green() + c.green()) / 2;
                    int b = (orig.blue()  + c.blue())  / 2;
                    img.setPixelColor(x, y, QColor(r, g, b, orig.alpha()));
                }
            }
        }
    }

    // Return a tinted icon from a base icon
    static inline QIcon tintIcon(const QIcon &base, const QColor &c, int size = 32) {
        if (!c.isValid()) return base;
        QPixmap pix = base.pixmap(size, size);
        QImage img = pix.toImage();
        tintImage(img, c);
        return QIcon(QPixmap::fromImage(img));
    }

    // Build the horizontal swatch row once; reuse everywhere (sidebar, toolbar menu, context menu).
    static inline QWidget* buildSwatchRow(QWidget *parent,
                                          QAbstractItemModel *model,
                                          std::function<QString()> getCWD)
    {
        QWidget *row = new QWidget(parent);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(8,4,8,4);
        h->setSpacing(6);

        auto addSwatch = [&](const QString &name, const QString &hex) {
            QToolButton *tb = new QToolButton(row);
            tb->setAutoRaise(true);
            tb->setCursor(Qt::PointingHandCursor);
            tb->setFixedSize(24,24);
            tb->setToolTip(name);
            tb->setAttribute(Qt::WA_Hover, true);
            tb->setMouseTracking(true);

            if (name == QStringLiteral("clear")) {
                // Grey "X" button that removes label entries
                tb->setText(QString::fromUtf8("×"));
                tb->setStyleSheet(
                    "QToolButton { background-color:#7f8c8d; color:white; border:2px solid transparent; border-radius:6px; padding:5px; }"
                    "QToolButton:hover { border-color:#888888; }"
                    "QToolButton:pressed { background-color:#5e696a; border-color:#333333; }"
                );
            } else {
                const QString pressedHex = QColor(hex).darker(125).name();  // ~25% darker on press
                tb->setStyleSheet(QString(
                    "QToolButton { background-color:%1; border:2px solid transparent; border-radius:6px; padding:5px; }"
                    "QToolButton:hover { border-color:#888888; }"
                    "QToolButton:pressed { background-color:%2; border-color:#333333; }"
                ).arg(hex, pressedHex));
            }

            QObject::connect(tb, &QToolButton::pressed, tb,
                             [parent, model, name, getCWD, tb] {
                // Visual acknowledge
                tb->setDown(true);
                tb->update();
                QApplication::processEvents();

                // Collect target paths (selected items, else CWD)
                QStringList targets;
                if (auto *av = qobject_cast<QAbstractItemView*>(QApplication::focusWidget())) {
                    if (av->selectionModel()) {
                        const auto idxs = av->selectionModel()->selectedIndexes();
                        for (const QModelIndex &ix : idxs) {
                            if (ix.column() != 0) continue;
                            QString p;
                            if (auto *fsm = qobject_cast<QFileSystemModel*>(model)) {
                                p = fsm->filePath(ix);
                            } else {
                                p = model->data(ix, Qt::UserRole + 1).toString();
                                if (p.isEmpty()) {
                                    const QString disp = model->data(ix, Qt::DisplayRole).toString();
                                    const QString baseDir = QFileInfo(getCWD()).isDir()
                                                            ? getCWD()
                                                            : QFileInfo(getCWD()).absolutePath();
                                    p = QDir(baseDir).filePath(disp);
                                }
                            }
                            if (!p.isEmpty()) targets << p;
                        }
                    }
                }
                if (targets.isEmpty()) targets << getCWD();

                if (name == QStringLiteral("clear")) {
                    // Rewrite .labelcolor without the selected items
                    for (const QString &path : targets) {
                        const QFileInfo fi(path);
                        const QString dir  = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
                        const QString item = fi.fileName().isEmpty() ? QStringLiteral(".") : fi.fileName();
                        const QString labelPath = dir + QStringLiteral("/.labelcolor");

                        QFile f(labelPath);
                        if (!f.exists()) continue;

                        QStringList outLines;
                        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            while (!f.atEnd()) {
                                const QString line = QString::fromUtf8(f.readLine()).trimmed();
                                if (line.isEmpty()) continue;
                                // CSV: "name","colour" — drop matching name
                                if (line.startsWith(QStringLiteral("\"") + item + QStringLiteral("\"")))
                                    continue;
                                outLines << line;
                            }
                            f.close();
                        }
                        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                            QTextStream out(&f);
                            for (const QString &ln : outLines) out << ln << "\n";
                            f.close();
                        }
                    }
                } else {
                    // Append label entries
                    for (const QString &path : targets) {
                        const QFileInfo fi(path);
                        const QString dir  = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
                        const QString item = fi.fileName().isEmpty() ? QStringLiteral(".") : fi.fileName();
                        const QString labelPath = dir + QStringLiteral("/.labelcolor");

                        QFile f(labelPath);
                        if (f.open(QIODevice::Append | QIODevice::Text)) {
                            QTextStream out(&f);
                            out << "\"" << item << "\",\"" << name << "\"\n";
                            f.close();
                        } else {
                            QMessageBox::warning(parent, QStringLiteral("Write failed"),
                                                 QStringLiteral("Could not open:\n") + labelPath);
                        }
                    }
                }

            });

            h->addWidget(tb);
        };

        // Colours (single source of truth)
        addSwatch(QStringLiteral("red"),    QStringLiteral("#e74c3c"));
        addSwatch(QStringLiteral("orange"), QStringLiteral("#f39c12"));
        addSwatch(QStringLiteral("yellow"), QStringLiteral("#f1c40f"));
        addSwatch(QStringLiteral("green"),  QStringLiteral("#2ecc71"));
        addSwatch(QStringLiteral("blue"),   QStringLiteral("#3498db"));
        addSwatch(QStringLiteral("violet"), QStringLiteral("#8e44ad"));
        addSwatch(QStringLiteral("black"),  QStringLiteral("#000000"));
	addSwatch(QStringLiteral("grey"),   QStringLiteral("#7f8c8d"));
        addSwatch(QStringLiteral("white"),  QStringLiteral("#ffffff"));
        addSwatch(QStringLiteral("clear"),  QStringLiteral("#7f8c8d")); // grey X to clear labels

        return row;
    }

    // Mount the swatch row into the sidebar as a single, non-selectable item.
    static inline void drawSwatches(QTreeWidget *sidebar, QWidget *parent,
                                    QAbstractItemModel *model,
                                    std::function<QString()> getCWD)
    {
        Q_UNUSED(parent);
        QTreeWidgetItem *swRow = new QTreeWidgetItem(sidebar);
        swRow->setFlags(Qt::NoItemFlags);
        QWidget *row = buildSwatchRow(sidebar, model, getCWD);
        sidebar->setItemWidget(swRow, 0, row);
        swRow->setSizeHint(0, QSize(100, row->sizeHint().height() + 4));
    }

    // Optional: add a "Label" popup to a toolbar (next to Size).
    static inline void addLabelsPopup(QMainWindow *win, QToolBar *tb,
                                      QAbstractItemModel *model,
                                      std::function<QString()> getCWD,
                                      QAction *insertBefore = nullptr)
    {
        QToolButton *btn = new QToolButton(tb);
        btn->setToolTip(QStringLiteral("Set label colour"));
        btn->setText(QStringLiteral("Label"));
        btn->setPopupMode(QToolButton::InstantPopup);

        QMenu *menu = new QMenu(btn);
        QWidgetAction *wa = new QWidgetAction(menu);
        wa->setDefaultWidget(buildSwatchRow(btn, model, getCWD));
        menu->addAction(wa);
        btn->setMenu(menu);

        if (insertBefore) tb->insertWidget(insertBefore, btn);
        else              tb->addWidget(btn);

        Q_UNUSED(win);
    }
};

#endif // LABELS_H
