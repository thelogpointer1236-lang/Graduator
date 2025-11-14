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
        bool isRunning = service.isRunning();

        QString text;
        bool okColor = false;

        if (isRunning) {
            okColor = true;
            text = "Идет градуировка";
        } else {
            QString err;
            bool ready = service.isReadyToRun(err);
            okColor = ready;
            text = ready ? "Система готова к работе"
                         : err;
        }

        if (role == Qt::DisplayRole) return text;

        if (role == Qt::BackgroundRole)
            return okColor ? QColor("#4CAF50") : QColor("#c62828");

        if (role == Qt::ForegroundRole)
            return QColor(Qt::white);

        if (role == Qt::FontRole) {
            QFont f;
            f.setBold(true);
            f.setPointSize(9);
            return f;
        }

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
            return QColor(p1 > p2 ? "#c62828" : "#000000");

        if (role == Qt::FontRole) {
            QFont f; f.setBold(true); return f;
        }

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
            return QColor(s1 > s2 ? "#c62828" : "#000000");

        if (role == Qt::FontRole) {
            QFont f; f.setBold(true); return f;
        }

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
        bool limit = pc.isAnyLimitTriggered();

        if (role == Qt::DisplayRole)
            return QString("Старт / Конец");

        if (role == Qt::ForegroundRole)
            return QColor(limit ? "#c62828" : "#2e7d32");

        if (role == Qt::FontRole) {
            QFont f; f.setBold(true); return f;
        }

        return {};
    }


    //
    // COLUMN 7 — VALVES
    //
    if (col == 7) {
        auto s = pc.g540Driver()->flapsState();

        QString text;
        QString color;

        switch (s) {
            case G540FlapsState::Unknown:
                text = "Неизв. сост. клапанов";
                color = "#616161";
                break;

            case G540FlapsState::CloseBoth:
                text = "Оба закрыты";
                color = "#c62828";
                break;

            case G540FlapsState::OpenInput:
                text = "Открыт входной";
                color = "#2e7d32";
                break;

            case G540FlapsState::OpenOutput:
                text = "Открыт выходной";
                color = "#2e7d32";
                break;
        }

        if (role == Qt::DisplayRole)
            return text;

        if (role == Qt::ForegroundRole)
            return QColor(color);

        if (role == Qt::FontRole) {
            QFont f; f.setBold(true); return f;
        }

        return {};
    }

    return {};
}
