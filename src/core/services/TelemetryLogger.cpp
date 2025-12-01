#include "TelemetryLogger.h"
#include "core/services/ServiceLocator.h"

#include <QDate>
#include <QFile>
#include <QTextStream>
#include <QDir>

TelemetryLogger::TelemetryLogger(const QString &logFolder, QObject *parent)
    : QObject(parent)
    , m_logFolderRoot(logFolder)
{
}

TelemetryLogger::~TelemetryLogger()
{
    end();
    if (m_pressureFile) {
        m_pressureFile->close();
        delete m_pressureFile;
    }
    for (auto &kv : m_angleFiles) {
        if (kv.second) {
            kv.second->close();
            delete kv.second;
        }
    }
}

void TelemetryLogger::setPressureUnit(PressureUnit unit)
{
    m_pressureUnit = unit;
}

QString TelemetryLogger::makeDailyFolder() const
{
    QString dateStr = QDate::currentDate().toString("dd.MM.yyyy_HH-mm-ss");
    QDir root(m_logFolderRoot);
    root.mkpath(dateStr);
    return root.filePath(dateStr);
}

void TelemetryLogger::begin()
{
    m_dailyFolder = makeDailyFolder();

    connect(ServiceLocator::instance().pressureSensor(), &PressureSensor::pressureMeasured,
            this, &TelemetryLogger::onPressureMeasured, Qt::QueuedConnection);

    connect(ServiceLocator::instance().cameraProcessor(), &CameraProcessor::angleMeasured,
            this, &TelemetryLogger::onAngleMeasured, Qt::QueuedConnection);

    connect(&m_savingTimer, &QTimer::timeout,
            this, &TelemetryLogger::saveTelemetryData);

    m_savingTimer.setInterval(2000);
    m_savingTimer.start();
}

void TelemetryLogger::end()
{
    disconnect(&m_savingTimer, nullptr, this, nullptr);
    disconnect(ServiceLocator::instance().pressureSensor(), nullptr, this, nullptr);
    disconnect(ServiceLocator::instance().cameraProcessor(), nullptr, this, nullptr);
    m_savingTimer.stop();
    saveTelemetryData();
}

void TelemetryLogger::onPressureMeasured(qreal t, Pressure p)
{
    m_pressureMeasurements.emplace_back(t, p.getValue(m_pressureUnit));
}

void TelemetryLogger::onAngleMeasured(qint32 i, qreal t, qreal a)
{
    m_angleMeasurements[i].emplace_back(t, a);
}

void TelemetryLogger::ensureFilesOpened()
{
    if (!m_pressureFile)
        openPressureFile();

    // angle files — лениво при первом сохранении конкретного канала
}

void TelemetryLogger::openPressureFile()
{
    QString path = QDir(m_dailyFolder).filePath("tp.csv");

    m_pressureFile = new QFile(path);
    bool existed = m_pressureFile->exists();

    if (!m_pressureFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete m_pressureFile;
        m_pressureFile = nullptr;
        return;
    }

    if (!existed) {
        QTextStream ts(m_pressureFile);
        ts << "t,pressure\n";
    }
}

void TelemetryLogger::openAngleFile(int idx)
{
    if (m_angleFiles.count(idx) && m_angleFiles[idx])
        return;

    QString filename = QString("ta%1.csv").arg(idx);
    QString path = QDir(m_dailyFolder).filePath(filename);

    QFile *file = new QFile(path);
    bool existed = file->exists();

    if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete file;
        return;
    }

    if (!existed) {
        QTextStream ts(file);
        ts << "t,angle\n";
    }

    m_angleFiles[idx] = file;
}

void TelemetryLogger::saveTelemetryData()
{
    if (m_pressureMeasurements.empty() && m_angleMeasurements.empty())
        return;

    ensureFilesOpened();

    // Pressure
    if (m_pressureFile && !m_pressureMeasurements.empty()) {
        QTextStream ts(m_pressureFile);
        for (auto &p : m_pressureMeasurements)
            ts << p.first << "," << p.second << "\n";
    }

    // Angles
    for (auto &kv : m_angleMeasurements) {
        int idx = kv.first;
        auto &vec = kv.second;
        if (vec.empty())
            continue;

        if (!m_angleFiles.count(idx) || !m_angleFiles[idx])
            openAngleFile(idx);

        QFile *file = m_angleFiles[idx];
        if (!file)
            continue;

        QTextStream ts(file);
        for (auto &a : vec)
            ts << a.first << "," << a.second << "\n";
    }

    // Clear buffers after flushing
    m_pressureMeasurements.clear();
    m_angleMeasurements.clear();
}
