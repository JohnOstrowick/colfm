#ifndef LABELS_H
#define LABELS_H

#include <QMap>
#include <QString>
#include <QColor>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QPainter>
#include <QIcon>
#include <QLabel>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QApplication>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <functional>

/*
 * LabelManager:
 * -------------
 * Handles reading/writing .labelcolor files,
 * tinting icons, and drawing swatch rows for sidebar/toolbar.
 */
class LabelManager {
public:
    // Map of filename -> colour hex string
    static QMap<QString, QString> labelMap;

    // Read .labelcolor file in directory
    static void readLabelFile(const QString &dirPath) {
        labelMap.clear();
        QFile f(dirPath + "/.labelcolor");
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            QStringList parts = line.split("\",\"");
            if (parts.size() != 2) continue;
            QString fname = parts[0];
            fname.remove(0, 1);                     // strip leading quote
            QString col = parts[1];
            col.chop(1);                            // strip trailing quote
            labelMap[fname] = col;
        }
    }

    // Return colour string (hex) for a file if labelled
    static QString getLabel(const QString &fileName) {
        return labelMap.value(fileName, "");
    }

    // Tint a pixmap with the given QColor
    static QIcon tintIcon(const QIcon &base, const QColor &c, int size=32) {
        QPixmap pix = base.pixmap(size, size);
        QImage img = pix.toImage();
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor orig = img.pixelColor(x, y);
                if (orig.alpha() > 0) {
                    int r = (orig.red()   + c.red())   / 2;
                    int g = (orig.green() + c.green()) / 2;
                    int b = (orig.blue()  + c.blue())  / 2;
                    img.setPixelColor(x, y, QColor(r,g,b, orig.alpha()));
                }
            }
        }
        return QIcon(QPixmap::fromImage(img));
    }

    // Apply tint directly to an existing QPixmap (for custom drawing)
    static void applyLabelColor(QPixmap &pixmap, const QColor &c) {
        QPainter p(&pixmap);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(pixmap.width() - 10, pixmap.height() - 10, 8, 8);
        p.end();
    }

	static void tintImage(QImage &img, const QColor &c) {
	    for (int y = 0; y < img.height(); ++y) {
		for (int x = 0; x < img.width(); ++x) {
		    QColor orig = img.pixelColor(x, y);
		    if (orig.alpha() > 0) {
			int r = (orig.red()   + c.red())   / 2;
			int g = (orig.green() + c.green()) / 2;
			int b = (orig.blue()  + c.blue())  / 2;
			img.setPixelColor(x, y, QColor(r,g,b, orig.alpha()));
		    }
		}
	    }
	}

	static QColor colourFromName(const QString &name) {
	    QColor c(name);              // try interpret as hex or color string
	    if (c.isValid()) return c;

	    if (name == "red")    return QColor("#e74c3c");
	    if (name == "orange") return QColor("#f39c12");
	    if (name == "yellow") return QColor("#f1c40f");
	    if (name == "green")  return QColor("#2ecc71");
	    if (name == "blue")   return QColor("#3498db");
	    if (name == "violet") return QColor("#8e44ad");
	    if (name == "black")  return QColor("#000000");
	    if (name == "white")  return QColor("#ffffff");
	    if (name == "grey")   return QColor("#7f8c8d");

	    return QColor(); // invalid
	}

    // Build colour swatches row (e.g. in sidebar)
    static void drawSwatches(QTreeWidget *sidebar, QWidget *parent, QAbstractItemModel *model, std::function<QString()> getCWD) {
        QTreeWidgetItem *swRow = new QTreeWidgetItem(sidebar);
        swRow->setFlags(Qt::NoItemFlags);
        QWidget *row = new QWidget(sidebar);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(8, 8, 8, 8);
        h->setSpacing(12);

        auto addSwatch = [&](const QString &name, const QString &hex) {
            QToolButton *tb = new QToolButton(row);
            tb->setAutoRaise(true);
            tb->setCursor(Qt::PointingHandCursor);
            tb->setFixedSize(24,24);
            tb->setToolTip(name);
            tb->setStyleSheet(QString("QToolButton { border: 2px solid black; background-color: %1; border-radius: 5px; padding: 5px; }"
                                      "QToolButton:pressed { border-style: inset; }").arg(hex));

            QObject::connect(tb, &QToolButton::clicked, parent, [parent, model, name, getCWD] {
                QStringList sel;
                if (auto *av = qobject_cast<QAbstractItemView*>(QApplication::focusWidget())) {
                    if (av->selectionModel()) {
                        const auto idxs = av->selectionModel()->selectedIndexes();
                        for (const QModelIndex &ix : idxs)
                            if (ix.column() == 0)
                                sel << model->data(ix).toString();
                    }
                }
                QStringList filesToWrite = sel.isEmpty() ? QStringList{ getCWD() } : sel;

                for (const QString &file : filesToWrite) {
                    QString dir = QFileInfo(file).isDir() ? file : QFileInfo(file).absolutePath();
                    QString base = QFileInfo(file).fileName();
                    QString path = dir + "/.labelcolor";
                    QFile f(path);
                    if (f.open(QIODevice::Append | QIODevice::Text)) {
                        QTextStream out(&f);
                        out << "\"" << base << "\",\"" << name << "\"\n";
                        f.close();
                    } else {
                        QMessageBox::warning(parent, "Write failed", "Could not open:\n" + path);
                    }
                }
            });

            h->addWidget(tb);
        };

        // Colours
        addSwatch("red",    "#e74c3c");
        addSwatch("orange", "#f39c12");
        addSwatch("yellow", "#f1c40f");
        addSwatch("green",  "#2ecc71");
        addSwatch("blue",   "#3498db");
        addSwatch("violet", "#8e44ad");
        addSwatch("black",  "#000000");
        addSwatch("white",  "#ffffff");
        addSwatch("grey",   "#7f8c8d");

        sidebar->setItemWidget(swRow, 0, row);
        swRow->setSizeHint(0, QSize(100, row->sizeHint().height() + 4));
    }
};

// static map definition
inline QMap<QString, QString> LabelManager::labelMap;

#endif // LABELS_H
