#include "StatusBarModel.h"
#include "core/services/ServiceLocator.h"
#include "core/types/PressureUnit.h"

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

    static QString pressureUnitToString(PressureUnit unit)
    {
        switch (unit) {
            case PressureUnit::Pa: return QObject::tr("Pa");
            case PressureUnit::kPa: return QObject::tr("kPa");
            case PressureUnit::MPa: return QObject::tr("MPa");
            case PressureUnit::Bar: return QObject::tr("Bar");
            case PressureUnit::Kgf: return QObject::tr("kgf/cm");
            case PressureUnit::KgfM2: return QObject::tr("kgf/m");
            case PressureUnit::Atm: return QObject::tr("atm");
            case PressureUnit::mmHg: return QObject::tr("mmHg");
            case PressureUnit::mmH2O: return QObject::tr("mmH2O");
            case PressureUnit::Unknown: break;
        }

        return QObject::tr("kgf/cm");
    }
}

QVariant StatusBarModel::data(const QModelIndex &idx, int role) const {
    if (!idx.isValid()) return {};

    auto &service = *ServiceLocator::instance().graduationService();
    auto &pc = *ServiceLocator::instance().pressureController();
    auto &ps = *ServiceLocator::instance().pressureSensor();

    const int col = idx.column();

    //
    // COLUMN 0 — SYSTEM STATE (new FSM-based version)
    //
    if (col == 0) {

        const auto st = service.state();   // Новый метод
        QString text;
        QColor back = COLOR_NEUTRAL;
        QColor front = COLOR_TEXT_INV;

        switch (st) {

            case GraduationService::State::Running:
                text = tr("Calibration in progress");
                back = COLOR_OK;
                break;

            case GraduationService::State::Prepared:
                text = tr("System prepared to start");
                back = COLOR_OK;
                break;

            case GraduationService::State::Idle:
            default:
                text = tr("Idle");
                back = COLOR_NEUTRAL;
                break;
        }

        // Apply Qt roles
        if (role == Qt::DisplayRole)    return text;
        if (role == Qt::BackgroundRole) return bg(back);
        if (role == Qt::ForegroundRole) return fg(front);
        if (role == Qt::FontRole)       return boldFont();

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
        const PressureUnit unit = service.pressureUnit();
        const PressureUnit effectiveUnit = unit == PressureUnit::Unknown ? PressureUnit::Kgf : unit;
        const QString unitStr = pressureUnitToString(effectiveUnit);
        const double p1 = ps.getLastPressure().getValue(effectiveUnit);
        const double p2 = pc.getTargetPressure();

        if (role == Qt::DisplayRole)
            return tr("%1 / %2 %3")
                    .arg(p1, 0, 'f', 2)
                    .arg(p2, 0, 'f', 2)
                    .arg(unitStr);

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
        const PressureUnit unit = service.pressureUnit();
        const PressureUnit effectiveUnit = unit == PressureUnit::Unknown ? PressureUnit::Kgf : unit;
        const QString unitStr = pressureUnitToString(effectiveUnit);
        const double s1 = pc.getCurrentPressureVelocity();
        const double s2 = pc.getTargetPressureVelocity();

        if (role == Qt::DisplayRole)
            return tr("%1 / %2 %3/s")
                    .arg(s1, 0, 'f', 2)
                    .arg(s2, 0, 'f', 2)
                    .arg(unitStr);

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
            return tr("%1 imp").arg(pc.getImpulsesCount());
        return {};
    }


    //
    // COLUMN 5 — FREQUENCY
    //
    if (col == 5) {
        if (role == Qt::DisplayRole)
            return tr("%1 Hz").arg(pc.getImpulsesFreq());
        return {};
    }


    //
    // COLUMN 6 — LIMIT SWITCHES
    //
    if (col == 6) {
        bool start = pc.isStartLimitTriggered();
        bool end = pc.isEndLimitTriggered();

        if (role == Qt::DisplayRole)
            return tr("Start: %1 / End: %2")
                    .arg(start ? tr("yes") : tr("no"))
                    .arg(end ? tr("yes") : tr("no"));

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
                text = tr("Unknown valves state");
                color = COLOR_NEUTRAL;
                break;
            case G540FlapsState::CloseBoth:
                text = tr("Both valves closed");
                color = COLOR_ERR;
                break;
            case G540FlapsState::OpenInput:
            case G540FlapsState::OpenOutput:
                text = tr("Valve is open");
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
