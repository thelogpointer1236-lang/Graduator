#include "GraduationTableModel.h"
#include <iostream>
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
    m_updateTimer.setInterval(5000);
    m_updateTimer.start();
    connect(&m_updateTimer, &QTimer::timeout, this, &GraduationTableModel::onUpdateTimer);
}

void printVector2D(const std::vector<std::vector<double>>& vec) {
    for (const auto& innerVec : vec) {
        for (const auto& value : innerVec) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

void GraduationTableModel::onUpdateTimer() {
    beginResetModel();
    m_forwardData = ServiceLocator::instance().graduationService()->graduateForward();
    m_backwardData = ServiceLocator::instance().graduationService()->graduateBackward();
    printVector2D(m_forwardData);
    std::cout << std::endl;
    printVector2D(m_backwardData);
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    endResetModel();
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
    const int row = index.row();
    const int col = index.column();
    const int camIdx = m_cameraStr[(col) / 2].digitValue();
    if (row < m_gaugeModel->pressureValues().size() && camIdx < 8) {
        const bool isForward = (col) % 2 == 0;
        const auto &data = isForward ? m_forwardData : m_backwardData;
        if (row >= data.size() || camIdx >= data[row].size())
            return {};
        auto &val = data[row][camIdx];
        return QVariant::fromValue(data[row][camIdx]);
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
