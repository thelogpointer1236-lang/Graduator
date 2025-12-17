#include "GraduationTableModel.h"

#include <QtMath>
#include <QColor>
#include <iostream>

#include "core/types/GaugeModel.h"
#include "core/services/ServiceLocator.h"
#include "defines.h"

namespace {

// -----------------------------------------------------------------------------
// Константы и утилиты
// -----------------------------------------------------------------------------

constexpr int kInfoRowCount = 4;
constexpr int kUpdateIntervalMs = 1000/30;  // Обновление индикаторов ~30 раз в секунду

    // Цвета ошибок — Background

    // Некорректное значение — тёплый красный
    const QColor kInvalidValueErrorBg(200, 50, 50);

    // Диапазон угла — синий
    const QColor kAngleRangeErrorBg(60, 100, 200);

    // Гистерезис — тёплый оранжевый
    const QColor kHysteresisErrorBg(230, 140, 40);


    // Цвета ошибок — Foreground

    // Красный и синий требуют максимального контраста
    const QColor kInvalidValueErrorFg(Qt::white);
    const QColor kAngleRangeErrorFg(Qt::white);

    // Оранжевый лучше читается с почти чёрным
    const QColor kHysteresisErrorFg(30, 30, 30);

const GaugeModel* currentGaugeModel() {
    const int idx = ServiceLocator::instance().configManager()
                        ->get<int>(CFG_KEY_CURRENT_GAUGE_MODEL, -1);
    return ServiceLocator::instance().gaugeCatalog()->findByIdx(idx);
}

// Отладочная печать 2D-вектора — возможно, стоит убрать, если не используется
void printVector2D(const std::vector<std::vector<double>>& vec) {
    for (const auto &row : vec) {
        for (double v : row)
            std::cout << v << " ";
        std::cout << std::endl;
    }
}

} // namespace

// -----------------------------------------------------------------------------
// Конструктор
// -----------------------------------------------------------------------------

GraduationTableModel::GraduationTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      m_gaugeModel(currentGaugeModel())
{
    auto configMgr = ServiceLocator::instance().configManager();

    // Обновление gaugeModel или cameraStr при изменении настроек
    connect(configMgr, &ConfigManager::valueChanged,
            this, [this](const QString &key, const QJsonValue &) {

        if (key == CFG_KEY_CURRENT_GAUGE_MODEL) {
            beginResetModel();
            m_gaugeModel = currentGaugeModel();
            endResetModel();
        }
        else if (key == CFG_KEY_CURRENT_CAMERA_STR) {
            beginResetModel();
            m_cameraStr = ServiceLocator::instance().cameraProcessor()->cameraStr();
            endResetModel();
        }
    });

    // Периодическое обновление индикаторов
    m_updateTimer.setInterval(kUpdateIntervalMs);
    m_updateTimer.start();

    connect(&m_updateTimer, &QTimer::timeout,
            this, &GraduationTableModel::updateIndicators);

    // Получение данных результатов градуировки
    connect(ServiceLocator::instance().graduationService(),
            &GraduationService::tableUpdateRequired,
            this, &GraduationTableModel::updateScale);
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

int GraduationTableModel::pressureRowCount() const {
    return m_gaugeModel ? m_gaugeModel->pressureValues().size() : 0;
}

int GraduationTableModel::infoRowCount() const {
    return kInfoRowCount;
}

bool GraduationTableModel::isForwardColumn(int col) const {
    return (col % 2) == 0;
}

int GraduationTableModel::cameraIndexFromColumn(int col) const {
    const int idx = col / 2;
    if (idx >= m_cameraStr.size())
        return -1;
    return m_cameraStr[idx].digitValue() - 1;  // 1-based → 0-based
}

// -----------------------------------------------------------------------------
// Модельные методы
// -----------------------------------------------------------------------------

int GraduationTableModel::rowCount(const QModelIndex &) const {
    return pressureRowCount() + infoRowCount();
}

int GraduationTableModel::columnCount(const QModelIndex &) const {
    return m_cameraStr.size() * 2;
}

QVariant GraduationTableModel::data(const QModelIndex &index, int role) const {
    if (!m_gaugeModel)
        return {};

    const int row = index.row();
    const int col = index.column();
    const int camIdx = cameraIndexFromColumn(col);
    if (camIdx < 0 || camIdx >= 8)
        return {};

    const bool forward = isForwardColumn(col);

    if (const auto *issue = m_validationResult.issueFor(camIdx, forward, row)) {

        // Подсветка ошибок/предупреждений — фон
        if (role == Qt::BackgroundRole) {
            // Подсветка диапазона угла — спокойный синий
            switch (issue->error) {
                case PartyValidationIssue::Error::Ok:
                    return {};
                case PartyValidationIssue::Error::AngleRange:
                    return kAngleRangeErrorBg;
                case PartyValidationIssue::Error::Hysteresis:
                    return kHysteresisErrorBg;
                case PartyValidationIssue::Error::InvalidValue:
                    return kInvalidValueErrorBg;
                default:
                    return {};
            }
        }

        // Цвет текста
        if (role == Qt::ForegroundRole) {
            switch (issue->error) {
                case PartyValidationIssue::Error::Ok:
                    return {};
                case PartyValidationIssue::Error::AngleRange:
                    return kAngleRangeErrorFg;
                case PartyValidationIssue::Error::Hysteresis:
                    return kHysteresisErrorFg;
                case PartyValidationIssue::Error::InvalidValue:
                    return kInvalidValueErrorFg;
                default:
                    return {};
            }
        }
    }

    if (role != Qt::DisplayRole)
        return {};

    const int pRows = pressureRowCount();

    // -----------------------
    // Основные значения углов
    // -----------------------
    if (row < pRows) {
        const auto &vec = forward ? m_partyResult.forward : m_partyResult.backward;
        if (camIdx >= vec.size() || row >= vec[camIdx].size())
            return {};

        const auto &val = vec[camIdx][row];
        return qFuzzyIsNull(val.angle) ? QVariant() : QVariant::fromValue(val.angle);
    }

    // -----------------------
    // Информационные строки
    // -----------------------
    const int infoRow = row - pRows;

    switch (infoRow) {

        // Разница "последний − первый"
        case 0: {
            const auto &vec = forward ? m_partyResult.forward : m_partyResult.backward;
            if (camIdx < vec.size() && vec[camIdx].size() == pRows) {
                const auto &v = vec[camIdx];
                return v.back().angle - v.front().angle;
            }
            break;
        }

        // Нелинейность
        case 1: {
            const auto &vec = forward ? m_partyResult.nolinForward : m_partyResult.nolinBackward;
            if (camIdx < vec.size())
                return vec[camIdx];
            break;
        }

        // Количество измерений
        case 2:
            return ServiceLocator::instance().graduationService()
                    ->angleMeasCountForCamera(camIdx, forward);

        // Последний измеренный угол камеры
        case 3:
            return ServiceLocator::instance().cameraProcessor()
                    ->lastAngleForCamera(camIdx);
    }

    return {};
}

QVariant GraduationTableModel::headerData(int section,
                                          Qt::Orientation orient,
                                          int role) const {
    if (role != Qt::DisplayRole)
        return {};

    static const QString infoTitles[kInfoRowCount] = {
        tr("Common"),
        tr("Nonlinear"),
        tr("Quantity"),
        tr("Current angle")
    };

    if (orient == Qt::Horizontal) {
        const int camDigitIdx = section / 2;
        if (camDigitIdx >= m_cameraStr.size())
            return {};

        const int camNum = m_cameraStr[camDigitIdx].digitValue();
        const bool forward = isForwardColumn(section);

        return QString("%1%2")
                .arg(forward ? tr("⤒") : tr("⤓"))
                .arg(camNum);
    }

    // Vertical
    const int pRows = pressureRowCount();

    if (section < pRows)
        return QString::number(m_gaugeModel->pressureValues()[section]);

    const int infoRow = section - pRows;
    if (infoRow >= 0 && infoRow < kInfoRowCount)
        return infoTitles[infoRow];

    return {};
}

bool GraduationTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return QAbstractTableModel::setData(index, value, role);
}

Qt::ItemFlags GraduationTableModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index);
}

// -----------------------------------------------------------------------------
// Slots
// -----------------------------------------------------------------------------

void GraduationTableModel::updateIndicators() {
    const int pRows = pressureRowCount();

    QModelIndex tl = index(pRows, 0);
    QModelIndex br = index(pRows + kInfoRowCount - 1, columnCount() - 1);

    emit dataChanged(tl, br, {Qt::DisplayRole, Qt::BackgroundRole});
}

void GraduationTableModel::updateScale() {
    auto svc = ServiceLocator::instance().graduationService();

    m_partyResult = svc->getPartyResult();
    m_validationResult = m_partyResult.validate();

    QModelIndex tl = index(0, 0);
    QModelIndex br = index(rowCount() - 1, columnCount() - 1);

    emit dataChanged(tl, br, {Qt::DisplayRole, Qt::BackgroundRole});
}
