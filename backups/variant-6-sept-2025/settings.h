#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QDir>

inline void ColFM::writePrefs(int folderMB, int iconSize, int viewMode) {
    QString configFile = QDir::homePath() + "/.colfm";
    QSettings settings(configFile, QSettings::IniFormat);
    settings.setValue("minFolderSizeMB", folderMB);
    settings.setValue("defaultIconSize", iconSize);
    settings.setValue("defaultView", viewMode);
}

inline void ColFM::readPrefs(int &folderMB, int &iconSize, int &viewMode) {
    QString configFile = QDir::homePath() + "/.colfm";
    QSettings settings(configFile, QSettings::IniFormat);
    folderMB = settings.value("minFolderSizeMB", 30).toInt();
    iconSize = settings.value("defaultIconSize", 32).toInt();
    viewMode = settings.value("defaultView", 1).toInt();  // 1 = Columns
}

inline void ColFM::onSettings() {
    QDialog dlg(this);
    dlg.setWindowTitle("Settings");

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    QFormLayout *form = new QFormLayout();

	QComboBox *folderSizeBox = new QComboBox();
	QList<int> folderSizes = {10,20,30,40,50,60,70,80,90,100,150,200,250,300};
	for (int size : folderSizes)
	    folderSizeBox->addItem(QString::number(size) + " MB", size);

	QComboBox *iconSizeBox = new QComboBox();
	QList<int> iconSizes = {16, 24, 32, 48, 64, 128};
	for (int size : iconSizes)
	    iconSizeBox->addItem(QString::number(size) + " px", size);

    QComboBox *viewModeBox = new QComboBox();
    viewModeBox->addItems({"List", "Columns", "Icons"});

    form->addRow("Min folder size (progress):", folderSizeBox);
    form->addRow("Default icon size:", iconSizeBox);
    form->addRow("Default view mode:", viewModeBox);

    mainLayout->addLayout(form);

    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(btns);

    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    int folderMB, iconSize, viewMode;
    readPrefs(folderMB, iconSize, viewMode);
    folderSizeBox->setCurrentIndex(folderSizes.indexOf(folderMB));
    iconSizeBox->setCurrentIndex(iconSizes.indexOf(iconSize));
    viewModeBox->setCurrentIndex(viewMode);

    if (dlg.exec() == QDialog::Accepted) {
	writePrefs(folderSizeBox->currentData().toInt(), iconSizeBox->currentData().toInt(), viewModeBox->currentIndex());
	statusBar()->showMessage("Settings saved", 2000);
    }
}

#endif // SETTINGS_H
