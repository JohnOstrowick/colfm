#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QDialog>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QTimer>

inline void ColFM::onProgress(QObject *target, QColor colour, QColor background, int width, int height, bool bevel) {
    Q_UNUSED(target)
    Q_UNUSED(bevel)

    QDialog dlg(this);
    dlg.setWindowTitle("Copying...");
    dlg.setFixedSize(width, height);
    dlg.setStyleSheet(QString("background-color: %1;").arg(background.name()));

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QProgressBar *bar = new QProgressBar();
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(false);
    bar->setStyleSheet(QString(
        "QProgressBar::chunk { background-color: %1; }"
    ).arg(colour.name()));

    layout->addWidget(bar);

    dlg.show();

    // Simulate 3-second copy just for test
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        int v = bar->value();
        if (v >= 100) {
            timer.stop();
            dlg.accept();
        } else {
            bar->setValue(v + 5);
        }
    });
    timer.start(150);

    dlg.exec();
}

#endif // PROGRESSBAR_H
