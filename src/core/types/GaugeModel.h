#ifndef GRADUATOR_MANOMETER_H
#define GRADUATOR_MANOMETER_H
#include <vector>
#include <QString>
class GaugeModel final {
public:
    GaugeModel() {
    }
    explicit GaugeModel(const std::vector<double> &pressureValues) : m_pressureValues(pressureValues) {
    }
    void setPressureValues(const std::vector<double> &values) { m_pressureValues = values; }
    const std::vector<double> &pressureValues() const { return m_pressureValues; }
    void setName(const QString &name) { m_name = name; }
    const QString &name() const { return m_name; }
    void setPrintingTemplate(const QString &printingTemplate) { m_printingTemplate = printingTemplate; }
    const QString &printingTemplate() const { return m_printingTemplate; }
private:
    std::vector<double> m_pressureValues;
    QString m_name;
    QString m_printingTemplate;
};
#endif //GRADUATOR_MANOMETER_H