#ifndef GRADUATOR_GAUGECATALOGSERVICE_H
#define GRADUATOR_GAUGECATALOGSERVICE_H
#include "core/types/GaugeModel.h"
#include <QObject>
#include <QList>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
class GaugeCatalog final : public QObject {
    Q_OBJECT
public:
    explicit GaugeCatalog(QObject *parent = nullptr);
    bool loadFromDirectory(const QString &dirPath);
    const QList<GaugeModel> &all() const noexcept;
    const QStringList &allNames() const noexcept;
    const QStringList &allPressureUnits() const noexcept;
    const QStringList &allPrintingTemplates() const noexcept;
    const QStringList &allPrinters() const noexcept;
    const QStringList &allPrecisions() const noexcept;
    const GaugeModel *findByName(const QString &name) const noexcept;
    const GaugeModel *findByIdx(int idx) const noexcept;
private:
    bool loadSingleFile(const QString &filePath, GaugeModel &outModel);
    QList<GaugeModel> m_models;
};
#endif //GRADUATOR_GAUGECATALOGSERVICE_H