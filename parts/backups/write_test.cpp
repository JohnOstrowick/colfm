#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>

int main() {
    QString path = QDir::homePath() + "/.labelcolor";
    QFile f(path);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << "\"testfile\",\"yellow\"\n";
        f.close();
        qDebug() << "Write successful to" << path;
    } else {
        qDebug() << "FAILED to open:" << path << f.errorString();
    }
    return 0;
}
