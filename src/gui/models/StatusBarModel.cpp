#include "StatusBarModel.h"
#include "core/services/ServiceLocator.h"

StatusBarModel::StatusBarModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_elapsed.start();

    m_updateTimer.setInterval(100);
    m_updateTimer.start();

    connect(&m_updateTimer, &QTimer::timeout, this, [this]() {
        emit dataChanged(index(0, 0, {}), index(0, ColumnCount - 1, {}));
    });
}

QModelIndex StatusBarModel::index(int row, int col, const QModelIndex &) const {
    if (row != 0 || col < 0 || col >= ColumnCount)
        return {};
    return createIndex(row, col);
}

QModelIndex StatusBarModel::parent(const QModelIndex &) const { return {}; }

int StatusBarModel::rowCount(const QModelIndex &) const { return 1; }
int StatusBarModel::columnCount(const QModelIndex &) const { return ColumnCount; }


QString StatusBarModel::formatTime(qint64 ms) const {
    int h = ms / 3600000;
    int m = (ms / 60000) % 60;
    int s = (ms / 1000) % 60;
    return QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
}

namespace {
    static QFont boldFont()
    {
        QFont f;
        f.setBold(true);
        f.setPointSize(9);
        return f;
    }

    static QFont normalFont()
    {
        QFont f;
        f.setPointSize(9);
        return f;
    }

    static QVariant fg(const QColor &c) { return c; }
    static QVariant bg(const QColor &c) { return c; }

    static const QColor COLOR_OK("#2e7d32");        // зелёный
    static const QColor COLOR_WARN("#f9a825");      // желтый (если понадобится)
    static const QColor COLOR_ERR("#c62828");       // красный
    static const QColor COLOR_TEXT(Qt::black);      // текст по умолчанию
    static const QColor COLOR_TEXT_INV(Qt::white);  // инвертированный
    static const QColor COLOR_NEUTRAL("#616161");   // серый
}

QVariant StatusBarModel::data(const QModelIndex &idx, int role) const {
    if (!idx.isValid()) return {};

    auto &service = *ServiceLocator::instance().graduationService();
    auto &pc = *ServiceLocator::instance().pressureController();
    auto &ps = *ServiceLocator::instance().pressureSensor();

    const int col = idx.column();

    //
    // COLUMN 0 — SYSTEM STATE
    //
    if (col == 0) {
        bool running = service.isRunning();
        QString text;
        QColor back = COLOR_NEUTRAL;
        QColor front = COLOR_TEXT_INV;

        if (running) {
            text = "Идет градуировка";
            back = COLOR_OK;
        } else {
            QString err;
            bool ready = service.isReadyToRun(err);
            text = ready ? "Система готова к работе" : err;
            back = ready ? COLOR_OK : COLOR_ERR;
        }

        if (role == Qt::DisplayRole) return text;
        if (role == Qt::BackgroundRole) return bg(back);
        if (role == Qt::ForegroundRole) return fg(front);
        if (role == Qt::FontRole) return boldFont();

        return {};
    }


    //
    // COLUMN 1 — TIME
    //
    if (col == 1) {
        if (role == Qt::DisplayRole) {
            qint64 elapsed = m_elapsed.elapsed();
            qint64 sinceStart = service.getElapsedTimeSeconds() * 1000;

            return QString("%1 / %2")
                    .arg(formatTime(elapsed))
                    .arg(formatTime(sinceStart));
        }
        return {};
    }


    //
    // COLUMN 2 — PRESSURE
    //
    if (col == 2) {
        double p1 = ps.getLastPressure().toKgf();
        double p2 = pc.getTargetPressure();

        if (role == Qt::DisplayRole)
            return QString("%1 / %2 кгс")
                    .arg(p1, 0, 'f', 2)
                    .arg(p2, 0, 'f', 2);

        if (role == Qt::ForegroundRole)
            return fg(p1 > p2 ? COLOR_ERR : COLOR_TEXT);

        if (role == Qt::FontRole)
            return boldFont();

        return {};
    }


    //
    // COLUMN 3 — PRESSURE SPEED
    //
    if (col == 3) {
        double s1 = pc.getCurrentPressureVelocity();
        double s2 = pc.getTargetPressureVelocity();

        if (role == Qt::DisplayRole)
            return QString("%1 / %2 кгс/с")
                    .arg(s1, 0, 'f', 2)
                    .arg(s2, 0, 'f', 2);

        if (role == Qt::ForegroundRole)
            return fg(s1 > s2 ? COLOR_ERR : COLOR_TEXT);

        if (role == Qt::FontRole)
            return boldFont();

        return {};
    }


    //
    // COLUMN 4 — IMPULSES
    //
    if (col == 4) {
        if (role == Qt::DisplayRole)
            return QString("%1 имп").arg(pc.getImpulsesCount());
        return {};
    }


    //
    // COLUMN 5 — FREQUENCY
    //
    if (col == 5) {
        if (role == Qt::DisplayRole)
            return QString("%1 Гц").arg(pc.getImpulsesFreq());
        return {};
    }


    //
    // COLUMN 6 — LIMIT SWITCHES
    //
    if (col == 6) {
        bool start = pc.isStartLimitTriggered();
        bool end = pc.isEndLimitTriggered();

        if (role == Qt::DisplayRole)
            return QString("Старт: %1 / Конец: %2")
                    .arg(start ? "да" : "нет")
                    .arg(end ? "да" : "нет");

        if (role == Qt::ForegroundRole)
            return fg(start || end ? COLOR_ERR : COLOR_OK);

        return {};
    }


    //
    // COLUMN 7 — VALVES
    //
    if (col == 7) {
        auto st = pc.g540Driver()->flapsState();
        QString text;
        QColor color = COLOR_NEUTRAL;

        switch (st) {
            case G540FlapsState::Unknown:
                text = "Неизв. сост. клапанов";
                color = COLOR_NEUTRAL;
                break;
            case G540FlapsState::CloseBoth:
                text = "Оба закрыты";
                color = COLOR_ERR;
                break;
            case G540FlapsState::OpenInput:
            case G540FlapsState::OpenOutput:
                text = "Клапан открыт";
                color = COLOR_OK;
                break;
        }

        if (role == Qt::DisplayRole) return text;
        if (role == Qt::ForegroundRole) return fg(color);
        if (role == Qt::FontRole) return boldFont();

        return {};
    }

    return {};
}
