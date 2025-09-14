#ifndef APPICON_H
#define APPICON_H

#include <QMessageBox>

void ColFM::onAppIcon() {
QMessageBox::information(this, "Debug", "Clicked the app icon");	
}

#endif // APPICON_H
