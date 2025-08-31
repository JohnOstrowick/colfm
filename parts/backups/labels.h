#ifndef LABELS_H
#define LABELS_H

#include <QMap>
#include <QString>
#include <QFileSystemModel>
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
#include <QMenu>
#include <QToolBar>
#include <QMainWindow>
#include <QWidgetAction>
#include <QTimer>

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

	// Build the horizontal swatch row once; reuse everywhere (sidebar, menus, etc.)
	static QWidget* buildSwatchRow(QWidget *parent,
				       QAbstractItemModel *model,
				       std::function<QString()> getCWD) {
	    QWidget *row = new QWidget(parent);
	    auto *h = new QHBoxLayout(row);
	    h->setContentsMargins(8, 8, 8, 8);
	    h->setSpacing(12);

	    auto addSwatch = [&](const QString &name, const QString &hex) {
		QToolButton *tb = new QToolButton(row);
		tb->setAutoRaise(true);
		tb->setCursor(Qt::PointingHandCursor);
		tb->setFixedSize(24,24);
		tb->setToolTip(name);
		tb->setAttribute(Qt::WA_Hover, true);
		tb->setMouseTracking(true);

		/* button styles for label swatches */
		const QString pressedHex = QColor(hex).darker(125).name();  // ~25% darker on click
		tb->setStyleSheet(QString(
			"QToolButton { background-color: %1; border: 3px solid transparent; border-radius: 6px; padding: 5px; }"
			"QToolButton:hover { border-color: #888888; }"            /* medium grey on hover */
			"QToolButton:pressed { background-color: %2; border-color: #333333; }" /* dark fill + border on click */
		).arg(hex, pressedHex));


		QObject::connect(tb, &QToolButton::pressed, tb, [parent, model, name, getCWD, tb] {
		// selection -> files, else use getCWD()
		tb->setDown(true);                   // force visible press
		tb->update();
		QApplication::processEvents();       // paint it now
		QStringList sel;
		if (auto *av = qobject_cast<QAbstractItemView*>(QApplication::focusWidget())) {
		if (av->selectionModel()) {
			const auto idxs = av->selectionModel()->selectedIndexes();
			for (const QModelIndex &ix : idxs) {
			if (ix.column() != 0) continue;
			QString p;
			if (auto *fsm = qobject_cast<QFileSystemModel*>(model)) {
				p = fsm->filePath(ix);
			} else {
				// Try absolute path via UserRole+1, else fall back to CWD + display text
				p = model->data(ix, Qt::UserRole + 1).toString();
				if (p.isEmpty()) {
				const QString disp = model->data(ix, Qt::DisplayRole).toString();
				const QString baseDir = QFileInfo(getCWD()).isDir()
							? getCWD()
							: QFileInfo(getCWD()).absolutePath();
				p = baseDir + QLatin1Char('/') + disp;
				}
			}
			if (!p.isEmpty()) sel << p;
			}
		}
		    }
		    const QStringList filesToWrite = sel.isEmpty() ? QStringList{ getCWD() } : sel;

		    for (const QString &file : filesToWrite) {
			const QString dir  = QFileInfo(file).isDir() ? file : QFileInfo(file).absolutePath();
			const QString base = QFileInfo(file).fileName();
			const QString path = dir + "/.labelcolor";
			QFile f(path);
			if (f.open(QIODevice::Append | QIODevice::Text)) {
			    QTextStream out(&f);
			    out << "\"" << base << "\",\"" << name << "\"\n";
			} else {
			    QMessageBox::warning(parent, "Write failed", "Could not open:\n" + path);
			}
		    }
		});
         // Close the owning QMenu (more robust: use the window() which is the menu window)
         if (auto *m = qobject_cast<QMenu*>(tb->window()))
             QTimer::singleShot(0, m, &QMenu::close);

         // Ask main window to refresh, if it has onRefresh()
         if (QWidget *win = tb->window())
             QMetaObject::invokeMethod(win, "onRefresh", Qt::QueuedConnection);


		h->addWidget(tb);
	    };

		auto addClearSwatch = [&]() {
        QToolButton *tb = new QToolButton(row);
        tb->setAutoRaise(true);
        tb->setCursor(Qt::PointingHandCursor);
        tb->setFixedSize(24,24);
        tb->setToolTip(QStringLiteral("clear label"));
        tb->setText(QString::fromUtf8("×"));
        tb->setStyleSheet(
            "QToolButton { background-color: #7f8c8d; color: white; border: 3px solid transparent; border-radius: 6px; padding: 0; font-weight: bold; }"
            "QToolButton:hover { border-color: #888888; }"
            "QToolButton:pressed { background-color: #5e696a; border-color: #333333; }"
        );
        tb->setAttribute(Qt::WA_Hover, true);
        tb->setMouseTracking(true);

        QObject::connect(tb, &QToolButton::pressed, tb, [parent, model, getCWD, tb] {
            // Gather targets: selected items (column 0) or fall back to CWD
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
                                p = baseDir + QLatin1Char('/') + disp;
                            }
                        }
                        if (!p.isEmpty()) targets << p;
                    }
                }
            }
            if (targets.isEmpty()) targets << getCWD();

            // Rewrite each directory's .labelcolor without the clicked items
            for (const QString &p : targets) {
                const QFileInfo fi(p);
                const QString dir  = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
                const QString item = fi.fileName().isEmpty() ? QStringLiteral(".") : fi.fileName();
                const QString labelPath = dir + QStringLiteral("/.labelcolor");

                QFile f(labelPath);
                if (!f.exists()) continue;

                QStringList linesOut;
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    while (!f.atEnd()) {
                        const QString line = QString::fromUtf8(f.readLine()).trimmed();
                        if (line.isEmpty()) continue;
                        // Our CSV line format is: "name","colour" — drop any matching 'name'
                        if (line.startsWith(QStringLiteral("\"") + item + QStringLiteral("\""))) {
                            continue;
                        }
                        linesOut << line;
                    }
                    f.close();
                }
                if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    QTextStream out(&f);
                    for (const QString &ln : linesOut) out << ln << "\n";
                    f.close();
                }
            }

            // Close menu if running inside a QMenu and refresh the window
            if (auto *m = qobject_cast<QMenu*>(tb->window()))
                QTimer::singleShot(0, m, &QMenu::close);
            if (QWidget *win = tb->window())
                QMetaObject::invokeMethod(win, "onRefresh", Qt::QueuedConnection);
        });

        h->addWidget(tb);
    };
	
	    // Colours (single source of truth)
	    addSwatch("red",    "#e74c3c");
	    addSwatch("orange", "#f39c12");
	    addSwatch("yellow", "#f1c40f");
	    addSwatch("green",  "#2ecc71");
	    addSwatch("blue",   "#3498db");
	    addSwatch("violet", "#8e44ad");
	    addSwatch("black",  "#000000");
	    addSwatch("grey",   "#7f8c8d");
	    addSwatch("white",  "#ffffff");
	    addClearSwatch();

	    return row;
	}

        h->addWidget(tb);
    };

    // Build colour swatches row (e.g. in sidebar)
	// Build colour swatches row (e.g. in sidebar)
static void drawSwatches(QTreeWidget *sidebar, QWidget *parent,
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

};

	// static map definition
	inline QMap<QString, QString> LabelManager::labelMap;

#endif // LABELS_H
