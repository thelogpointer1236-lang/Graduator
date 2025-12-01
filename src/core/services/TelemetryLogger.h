#ifndef GRADUATOR_TELEMETRYLOGGER_H
#define GRADUATOR_TELEMETRYLOGGER_H

#include "core/types/Pressure.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QDir>
#include <map>
#include <vector>

class TelemetryLogger final : public QObject {
    Q_OBJECT
public:
    explicit TelemetryLogger(const QString &logFolder, QObject *parent = nullptr);
    ~TelemetryLogger() override;

    void setPressureUnit(PressureUnit unit);

    void begin();
    void end();

private slots:
    void onPressureMeasured(qreal t, Pressure p);
    void onAngleMeasured(qint32 i, qreal t, qreal a);
    void saveTelemetryData();

private:
    QString makeDailyFolder() const;
    void ensureFilesOpened();
    void openPressureFile();
    void openAngleFile(int idx);

private:
    QString m_logFolderRoot;
    QString m_dailyFolder;

    QTimer m_savingTimer;
    PressureUnit m_pressureUnit{PressureUnit::Pa};

    std::map<int, QFile*> m_angleFiles;
    QFile* m_pressureFile{nullptr};

    std::map<int, std::vector<std::pair<qreal, qreal>>> m_angleMeasurements;
    std::vector<std::pair<qreal, qreal>> m_pressureMeasurements;
};

#endif // GRADUATOR_TELEMETRYLOGGER_H
