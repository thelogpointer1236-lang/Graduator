#include "StatusBarView.h"

StatusBarView::StatusBarView(QWidget *parent)
    : QAbstractItemView(parent)
{
    grid_ = new QGridLayout(this);
    grid_->setContentsMargins(8, 4, 8, 4);
    grid_->setSpacing(6);

    setLayout(grid_);
}


void StatusBarView::setModel(QAbstractItemModel *m) {
    QAbstractItemView::setModel(m);

    if (!m) {
        labels_.clear();
        return;
    }

    connect(m, &QAbstractItemModel::modelReset,
            this, &StatusBarView::onModelChanged);

    connect(m, &QAbstractItemModel::layoutChanged,
            this, &StatusBarView::onModelChanged);

    connect(m, &QAbstractItemModel::dataChanged,
            this, &StatusBarView::onDataChanged);

    connect(m, &QAbstractItemModel::columnsInserted,
            this, &StatusBarView::onModelChanged);

    connect(m, &QAbstractItemModel::columnsRemoved,
            this, &StatusBarView::onModelChanged);

    // Первичное построение
    rebuild();
}


void StatusBarView::onModelChanged() {
    rebuild();
}


void StatusBarView::onDataChanged(const QModelIndex &tl, const QModelIndex &br) {
    if (!model()) return;

    int left = tl.column();
    int right = br.column();

    for (int col = left; col <= right; ++col) {
        updateCell(col);
    }
}


void StatusBarView::rebuild() {
    if (!model()) return;

    // удалить старые виджеты
    QLayoutItem *child;
    while ((child = grid_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    labels_.clear();

    int cols = model()->columnCount();
    labels_.reserve(cols);

    for (int col = 0; col < cols; ++col) {
        auto *lbl = new QLabel("—", this);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        // ВАЖНО:
        lbl->setAutoFillBackground(true);
        lbl->setAttribute(Qt::WA_StyledBackground, true);

        labels_.append(lbl);
        grid_->addWidget(lbl, 0, col);
    }

    // загрузить данные
    for (int col = 0; col < cols; ++col) {
        updateCell(col);
    }
}


void StatusBarView::updateCell(int col) {
    if (!model()) return;
    if (col < 0 || col >= labels_.size()) return;

    QLabel *lbl = labels_[col];
    QModelIndex idx = model()->index(0, col);
    if (!idx.isValid()) return;

    QString style;

    // TEXT
    lbl->setText(idx.data(Qt::DisplayRole).toString());

    // FOREGROUND COLOR
    {
        QVariant v = idx.data(Qt::ForegroundRole);
        if (v.isValid()) {
            QColor c = v.value<QColor>();
            style += QString("color:%1;").arg(c.name());
        }
    }

    // BACKGROUND COLOR
    {
        QVariant v = idx.data(Qt::BackgroundRole);
        if (v.isValid()) {
            QColor c = v.value<QColor>();
            lbl->setAutoFillBackground(true);
            lbl->setAttribute(Qt::WA_StyledBackground, true);
            style += QString("background-color:%1;border-radius:6px;padding:2px;")
                         .arg(c.name());
        } else {
            lbl->setAutoFillBackground(false);
        }
    }

    // APPLY STYLE
    lbl->setStyleSheet(style);

    // FONT — используем только то, что даёт модель
    {
        QVariant v = idx.data(Qt::FontRole);
        if (v.isValid()) {
            lbl->setFont(v.value<QFont>());
        }
    }

    // ALIGNMENT
    {
        QVariant v = idx.data(Qt::TextAlignmentRole);
        if (v.isValid()) {
            lbl->setAlignment(Qt::Alignment(v.toInt()));
        }
    }

    // TOOLTIP
    {
        QVariant v = idx.data(Qt::ToolTipRole);
        if (v.isValid()) {
            lbl->setToolTip(v.toString());
        }
    }
}

