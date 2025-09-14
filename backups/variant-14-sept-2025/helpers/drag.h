#ifndef DRAG_H
#define DRAG_H

#include <QObject>
#include <QAbstractItemView>
#include <QApplication>
#include <QMouseEvent>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QDrag>
#include <QPixmap>
#include <QIcon>
#include <QRect>
#include <QPointer>

namespace Drag {

// Per-view filter: starts a QDrag with a ghost pixmap for any QAbstractItemView
class GhostDragFilter : public QObject {
public:
    explicit GhostDragFilter(QAbstractItemView *view)
        : QObject(view), view_(view) {}

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (!view_ || obj != view_) return QObject::eventFilter(obj, ev);

        switch (ev->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent*>(ev);
            if (me->button() == Qt::LeftButton) pressPos_ = me->pos();
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent*>(ev);
            if (!(me->buttons() & Qt::LeftButton)) break;
            if ((me->pos() - pressPos_).manhattanLength() < QApplication::startDragDistance())
                break;

            if (!view_->model()) break;

            // Selection to drag (prefer selected rows, fallback to index under cursor)
            QModelIndexList indexes;
            if (view_->selectionModel())
                indexes = view_->selectionModel()->selectedIndexes();
            if (indexes.isEmpty()) {
                QModelIndex ci = view_->indexAt(me->pos());
                if (!ci.isValid()) break;
                indexes << ci;
            }

            QMimeData *mime = view_->model()->mimeData(indexes);
            if (!mime) break;

            // Build ghost pixmap (icon if available, else grabbed rect)
            QPixmap pm;
            const QModelIndex rep = indexes.first();
            const QVariant deco  = view_->model()->data(rep, Qt::DecorationRole);
            if (deco.canConvert<QIcon>()) {
                const QIcon ic = qvariant_cast<QIcon>(deco);
                const QSize sz = view_->iconSize().isValid() ? view_->iconSize() : QSize(64,64);
                pm = ic.pixmap(sz);
            }
            if (pm.isNull()) {
                QRect r = view_->visualRect(rep);
                if (r.isValid()) {
                    r.adjust(-4,-4,4,4);
                    pm = view_->viewport()->grab(r);
                }
            }

            // Start the drag
            QDrag *drag = new QDrag(view_);
            drag->setMimeData(mime);
            if (!pm.isNull()) {
                drag->setPixmap(pm);
                drag->setHotSpot(QPoint(pm.width()/2, pm.height()/2));
            }
            drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction);
            return true; // handled
        }
        default: break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QPointer<QAbstractItemView> view_;
    QPoint pressPos_;
};

// Enable DnD + ghost on a single view
inline void enableOn(QAbstractItemView *v) {
    if (!v) return;
    v->setDragEnabled(true);
    v->setAcceptDrops(true);
    v->setDropIndicatorShown(true);
    v->setDragDropMode(QAbstractItemView::DragDrop);
    v->setDefaultDropAction(Qt::MoveAction);
    v->installEventFilter(new GhostDragFilter(v));
}

// Enable DnD + ghost on ALL QAbstractItemView descendants under a widget
inline void enableRecursively(QWidget *root) {
    if (!root) return;
    for (QAbstractItemView *v : root->findChildren<QAbstractItemView*>())
        enableOn(v);
}

} // namespace Drag

#endif // DRAG_H
