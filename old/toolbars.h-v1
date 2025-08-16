#pragma once
#include <QToolBar>
#include <QLineEdit>
#include <QWidgetAction>
#include <QSizePolicy>
#include <QDir>
#include <QAction>
#include <QList>
#include <QString>
#include <QStringList>
#include <functional>

class Breadcrumbs : public QToolBar {
public:
    explicit Breadcrumbs(const QString &title, QWidget *parent=nullptr)
        : QToolBar(title, parent) {
        setMovable(true);
        spacerAct = addWidget(makeSpacer());
        edit = new QLineEdit(this);
        edit->setPlaceholderText("Path…");
        editAct = addWidget(edit);
        QObject::connect(edit, &QLineEdit::returnPressed, this, [this]{
            if (onPathChosen) onPathChosen(edit->text());
        });
    }

    void setOnPathChosen(std::function<void(const QString&)> cb) { onPathChosen = std::move(cb); }
    QLineEdit* editField() const { return edit; }

    void setPath(const QString &path) {
        for (QAction *a : segActs) { removeAction(a); delete a; }
        segActs.clear();

        const QString sep = QDir::separator();
        QString clean = QDir::cleanPath(path);
        edit->setText(clean);

        bool absolute = clean.startsWith(sep);
        QStringList parts = clean.split(sep, Qt::SkipEmptyParts);

        if (absolute) addSegment("/", sep);

        QString accum = absolute ? sep : QString();
        for (int i = 0; i < parts.size(); ++i) {
            if (accum.isEmpty() || accum == sep) accum += parts[i];
            else accum += sep + parts[i];
            addSegment(parts[i], accum);
        }
    }

private:
    QLineEdit *edit{};
    QAction *spacerAct{};
    QAction *editAct{};
    QList<QAction*> segActs;
    std::function<void(const QString&)> onPathChosen;

    QWidget* makeSpacer() {
        QWidget *sp = new QWidget(this);
        sp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        return sp;
    }

    void addSegment(const QString &label, const QString &fullPath) {
        QAction *seg = new QAction(label, this);
        insertAction(spacerAct, seg);
        segActs.push_back(seg);
        QObject::connect(seg, &QAction::triggered, this, [this, fullPath]{
            if (onPathChosen) onPathChosen(fullPath);
            if (edit) edit->setText(fullPath);
        });
    }
};
