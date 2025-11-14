#ifndef GRADUATOR_STATUSBARVIEW_H
#define GRADUATOR_STATUSBARVIEW_H

#include <QAbstractItemView>
#include <QGridLayout>
#include <QLabel>
#include <QVector>

class StatusBarView final : public QAbstractItemView {
    Q_OBJECT

public:
    explicit StatusBarView(QWidget *parent = nullptr);
    ~StatusBarView() override = default;

    void setModel(QAbstractItemModel *model) override;

    QModelIndex indexAt(const QPoint &) const override { return {}; }
    void scrollTo(const QModelIndex &, ScrollHint) override {}
    QRect visualRect(const QModelIndex &) const override { return {}; }

    QModelIndex moveCursor(CursorAction, Qt::KeyboardModifiers) override { return {}; }

    int horizontalOffset() const override { return 0; }
    int verticalOffset() const override { return 0; }
    bool isIndexHidden(const QModelIndex &) const override { return false; }

    void setSelection(const QRect &, QItemSelectionModel::SelectionFlags) override {}
    QRegion visualRegionForSelection(const QItemSelection &) const override { return {}; }

private slots:
    void onModelChanged();
    void onDataChanged(const QModelIndex &tl, const QModelIndex &br);

private:
    void rebuild();
    void updateCell(int col);

private:
    QGridLayout *grid_{};
    QVector<QLabel*> labels_;
};

#endif // GRADUATOR_STATUSBARVIEW_H
