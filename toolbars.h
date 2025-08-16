#pragma once
#include <QToolBar>
#include <QLineEdit>
#include <QSizePolicy>
#include <QDir>
#include <functional>

class Breadcrumbs : public QToolBar {
public:
    explicit Breadcrumbs(const QString &title, QWidget *parent=nullptr)
        : QToolBar(title, parent) {
        setMovable(true);
        edit = new QLineEdit(this);
        edit->setPlaceholderText("Path…");
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        addWidget(edit);

        QObject::connect(edit, &QLineEdit::returnPressed, this, [this]{
            if (onPathChosen) onPathChosen(edit->text());
        });
    }

    void setOnPathChosen(std::function<void(const QString&)> cb) { onPathChosen = std::move(cb); }
    QLineEdit* editField() const { return edit; }

    void setPath(const QString &path) {
        edit->setText(QDir::cleanPath(path));
    }

private:
    QLineEdit *edit{};
    std::function<void(const QString&)> onPathChosen;
};
