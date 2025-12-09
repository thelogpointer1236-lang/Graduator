#include "GraduationTableModel.h"
#include <iostream>
#include <QtMath>
#include "core/types/GaugeModel.h"
#include "core/services/ServiceLocator.h"
#include "defines.h"

namespace {
    const GaugeModel *gaugeModel() {
        const int idx = ServiceLocator::instance().configManager()->get<int>(CFG_KEY_CURRENT_GAUGE_MODEL, -1);
        return ServiceLocator::instance().gaugeCatalog()->findByIdx(idx);
    }
}

GraduationTableModel::GraduationTableModel(QObject *parent) : QAbstractTableModel(parent) {
    m_gaugeModel = gaugeModel();
    connect(ServiceLocator::instance().configManager(), &ConfigManager::valueChanged,
            [this](const QString &keyPath, const QJsonValue &) {
                if (keyPath == CFG_KEY_CURRENT_GAUGE_MODEL) {
                    beginResetModel();
                    m_gaugeModel = gaugeModel();
                    endResetModel();
                } else if (keyPath == CFG_KEY_CURRENT_CAMERA_STR) {
                    beginResetModel();
                    m_cameraStr = ServiceLocator::instance().cameraProcessor()->cameraStr();
                    endResetModel();
                }
            });
    m_updateTimer.setInterval(100);
    m_updateTimer.start();
    connect(&m_updateTimer, &QTimer::timeout, this, &GraduationTableModel::updateIndicators);
    connect( ServiceLocator::instance().graduationService(), &GraduationService::tableUpdateRequired,
            this, &GraduationTableModel::updateScale);
}

void printVector2D(const std::vector<std::vector<double>>& vec) {
    for (const auto& innerVec : vec) {
        for (const auto& value : innerVec) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

int GraduationTableModel::rowCount(const QModelIndex &parent) const {
    if (!m_gaugeModel) return 0 + 4;
    const int cnt = m_gaugeModel->pressureValues().size();
    return cnt + 4;
}

int GraduationTableModel::columnCount(const QModelIndex &parent) const {
    return m_cameraStr.size() * 2;
}

QVariant GraduationTableModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole || !m_gaugeModel)
        return {};
    const int pp_size = m_gaugeModel->pressureValues().size();
    const int row = index.row();
    const int col = index.column();
    const int camIdx = m_cameraStr[(col) / 2].digitValue() - 1;
    if (camIdx >= 8) return QVariant();
    const bool isForward = (col) % 2 == 0;

    // Graduation data
    if (row < pp_size) {
        const auto &data = isForward ? m_partyResult.forward : m_partyResult.backward;
        if (camIdx >= data.size() || row >= data[camIdx].size())
            return {};
        auto &val = data[camIdx][row];
        return qFuzzyIsNull(val.angle) ? QVariant() : QVariant::fromValue(val.angle);
    }

    // Additional data
    if (row < rowCount()) {
        const int infoRow = row - pp_size;
        switch (infoRow) {
            case 0: {
                const auto &data = isForward ? m_partyResult.forward : m_partyResult.backward;
                if (camIdx < data.size() && data[camIdx].size() == m_gaugeModel->pressureValues().size())
                return QVariant::fromValue(data[camIdx].back().angle - data[camIdx].front().angle);
                break;
            }
            case 1: {
                const auto &data = isForward ? m_partyResult.nolinForward : m_partyResult.nolinBackward;
                if (camIdx < data.size())
                return QVariant::fromValue(data[camIdx]);
                break;
            }
            case 2:
                return QVariant::fromValue(ServiceLocator::instance().graduationService()->angleMeasCountForCamera(camIdx, isForward));
            case 3:
                return QVariant::fromValue(ServiceLocator::instance().cameraProcessor()->lastAngleForCamera(camIdx));
            default:
                return QVariant();

        }
    }
    return QVariant();
}

QVariant GraduationTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return {};
    static const QString infoRows[] = {
        tr("Common"),
        tr("Nonlinear"),
        tr("Quantity"),
        tr("Current angle")
    };
    if (orientation == Qt::Horizontal) {
        if ((section) / 2 >= m_cameraStr.size()) return {};
        const int camIdx = m_cameraStr[(section) / 2].digitValue();
        const bool isForward = (section) % 2 == 0;
        return QString("%1%2").arg(isForward ? L"⤒" : L"⤓").arg(camIdx);
    } else {
        const auto &values = m_gaugeModel->pressureValues();
        if (section < 0) return {};
        if (section < values.size()) return QString::number(values[section]);
        if (section - values.size() < std::size(infoRows)) return infoRows[section - values.size()];
    }
    return {};
}
bool GraduationTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return QAbstractTableModel::setData(index, value, role);
}

Qt::ItemFlags GraduationTableModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index);
}

void GraduationTableModel::updateIndicators() {
    QModelIndex topLeft = index(m_gaugeModel->pressureValues().size(), 0);
    QModelIndex bottomRight = index(m_gaugeModel->pressureValues().size() + 4 - 1, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}

void GraduationTableModel::updateScale() {
    m_partyResult = ServiceLocator::instance().graduationService()->getPartyResult();
    QModelIndex topLeft = index(0, 0);
    QModelIndex bottomRight = index(m_gaugeModel->pressureValues().size() - 1, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}
