#include "TelemetryLogger.h"
#include "core/services/ServiceLocator.h"

#include <QDateTime>
#include <QTextStream>
#include <QDir>

// Предполагается наличие следующих типов и методов:
//   ServiceLocator::instance().pressureSensor()
//   ServiceLocator::instance().cameraProcessor()
//   ServiceLocator::instance().graduationService()
//   GraduationService::getPartyResult()
// и т.п.

TelemetryLogger::TelemetryLogger(const QString &logFolder, QObject *parent)
    : QObject(parent)
    , m_logFolderRoot(logFolder)
{
    // Конфигурируем таймер
    m_savingTimer.setInterval(2000);

    connect(&m_savingTimer, &QTimer::timeout,
            this, &TelemetryLogger::saveTelemetryData);

    // Логгер сам подписывается на GraduationService
    begin();
}

TelemetryLogger::~TelemetryLogger()
{
    // Аккуратно завершить логирование и освободить ресурсы
    end();
}

void TelemetryLogger::setPressureUnit(PressureUnit unit)
{
    m_pressureUnit = unit;
}

QString TelemetryLogger::makeDailyFolder() const
{
    // Создаём уникальное имя подкаталога по текущей дате/времени
    // Пример: "05.12.2025_14-23-01"
    const QString dateStr = QDateTime::currentDateTime().toString("dd.MM.yyyy_HH-mm-ss");
    QDir root(m_logFolderRoot);
    root.mkpath(dateStr);                // создаём подкаталог (если не удалось — всё равно вернём путь)
    return root.filePath(dateStr);
}

void TelemetryLogger::begin()
{
    // Подписываемся на события сервиса градуировки.
    // Сами датчики будут подключены только при старте градуировки.
    auto &locator = ServiceLocator::instance();

    connect(locator.graduationService(), &GraduationService::started,
            this, &TelemetryLogger::onGraduationStarted, Qt::QueuedConnection);

    connect(locator.graduationService(), &GraduationService::ended,
            this, &TelemetryLogger::onGraduationEnded, Qt::QueuedConnection);

    connect(locator.graduationService(), &GraduationService::interrupted,
            this, &TelemetryLogger::onGraduationInterrupted, Qt::QueuedConnection);
}

void TelemetryLogger::end()
{
    // Завершаем текущую сессию логирования (если она есть).
    // Здесь мы НЕ сохраняем PartyResult — он уже сохраняется в onGraduationEnded.
    finishLoggingSession(false);

    // Отписываемся от всех сигналов GraduationService и таймера.
    auto &locator = ServiceLocator::instance();
    disconnect(locator.graduationService(), nullptr, this, nullptr);
    disconnect(&m_savingTimer, nullptr, this, nullptr);
}

void TelemetryLogger::onGraduationStarted()
{
    // Начало новой градуировки:
    // - создаём новую директорию для логов
    // - очищаем буферы
    // - закрываем предыдущие файлы (на всякий случай)
    // - подключаем датчики и запускаем таймер

    m_dailyFolder = makeDailyFolder();
    clearBuffers();
    closeFiles();

    startLoggingSession();
}

void TelemetryLogger::onGraduationEnded()
{
    // Успешное завершение градуировки:
    // - фиксируем оставшиеся в памяти измерения,
    // - сохраняем PartyResult,
    // - закрываем файлы и отключаем датчики.

    finishLoggingSession(true);
}

void TelemetryLogger::onGraduationInterrupted()
{
    // Прерывание градуировки:
    // - фиксируем оставшиеся измерения,
    // - НЕ сохраняем PartyResult,
    // - закрываем файлы и отключаем датчики.

    finishLoggingSession(false);
}

void TelemetryLogger::onPressureMeasured(qreal t, Pressure p)
{
    if (!m_isLoggingActive)
        return;

    auto* pc = ServiceLocator::instance().pressureController();

    m_pressureMeasurements.push_back(
        QString("%1,%2,%3,%4,%5")
                .arg(t)
                .arg(p.getValue(m_pressureUnit))
                .arg(pc->getCurrentPressureVelocity())
                .arg(pc->getImpulsesFreq())
                .arg(pc->getImpulsesCount())
    );
}

void TelemetryLogger::onAngleMeasured(qint32 i, qreal t, qreal a)
{
    if (!m_isLoggingActive)
        return;

    m_angleMeasurements[i].push_back(QString("%1,%2").arg(t).arg(a));
}

void TelemetryLogger::ensureFilesOpened()
{
    if (m_dailyFolder.isEmpty())
        return; // нет активной директории — логировать некуда

    if (!m_pressureFile)
        openPressureFile();

    // Файлы для углов открываются лениво в openAngleFile(int),
    // когда появляются данные для конкретного канала.
}

void TelemetryLogger::openPressureFile()
{
    const QString path = QDir(m_dailyFolder).filePath("tp.csv");

    m_pressureFile = new QFile(path);
    const bool existed = m_pressureFile->exists();

    if (!m_pressureFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete m_pressureFile;
        m_pressureFile = nullptr;
        return;
    }

    // Если файл создаётся впервые — пишем заголовок CSV
    if (!existed) {
        QTextStream ts(m_pressureFile);
        ts << "t,p,dp,fimp,cimp\n";
    }
}

void TelemetryLogger::openAngleFile(int idx)
{
    if (m_angleFiles.count(idx) && m_angleFiles[idx])
        return; // файл уже открыт

    const QString filename = QString("ta%1.csv").arg(idx);
    const QString path = QDir(m_dailyFolder).filePath(filename);

    QFile *file = new QFile(path);
    const bool existed = file->exists();

    if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete file;
        return;
    }

    // Если файл создаётся впервые — пишем заголовок CSV
    if (!existed) {
        QTextStream ts(file);
        ts << "t,a\n";
    }

    m_angleFiles[idx] = file;
}

void TelemetryLogger::saveTelemetryData()
{
    // Срабатывает по таймеру.
    // Выгружаем накопленные данные на диск и очищаем буферы.

    if (!m_isLoggingActive)
        return;

    if (m_pressureMeasurements.empty() && m_angleMeasurements.empty())
        return;

    ensureFilesOpened();

    // --- Давление ---
    if (m_pressureFile && !m_pressureMeasurements.empty()) {
        QTextStream ts(m_pressureFile);
        for (const auto &str : m_pressureMeasurements)
            ts << str << "\n";
    }

    // --- Углы ---
    for (auto &kv : m_angleMeasurements) {
        const int idx = kv.first;
        auto &vec = kv.second;
        if (vec.empty())
            continue;

        if (!m_angleFiles.count(idx) || !m_angleFiles[idx])
            openAngleFile(idx);

        QFile *file = m_angleFiles[idx];
        if (!file)
            continue;

        QTextStream ts(file);
        for (const auto &str : vec)
            ts << str << "\n";
    }

    // После успешной записи очищаем буферы, чтобы не накапливать лишнее в памяти
    clearBuffers();
}

void TelemetryLogger::savePartyResult(const PartyResult &result)
{
    if (m_dailyFolder.isEmpty())
        return;

    QDir dir(m_dailyFolder);

    // --- 1. Краткое резюме PartyResult ---
    {
        QFile file(dir.filePath("party_summary.txt"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&file);
            ts << "durationSeconds: " << result.durationSeconds << "\n";
            ts << "forward_paths: " << static_cast<int>(result.forward.size()) << "\n";
            ts << "backward_paths: " << static_cast<int>(result.backward.size()) << "\n";
            ts << "debug_forward_records: " << static_cast<int>(result.debugDataForward.size()) << "\n";
            ts << "debug_backward_records: " << static_cast<int>(result.debugDataBackward.size()) << "\n";
            // TODO: при необходимости добавить сериализацию GaugeModel,
            //       когда будет известен его интерфейс.
        }
    }

    // --- 2. Forward-результаты по узлам ---
    {
        QFile file(dir.filePath("party_forward.csv"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&file);
            ts << "step,nodeIndex,pressure,angle,valid\n";

            for (std::size_t step = 0; step < result.forward.size(); ++step) {
                const auto &nodes = result.forward[step];
                for (std::size_t nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx) {
                    const NodeResult &node = nodes[nodeIdx];
                    ts << step << "," << nodeIdx << ","
                       << node.pressure << "," << node.angle << ","
                       << (node.valid ? 1 : 0) << "\n";
                }
            }
        }
    }

    // --- 3. Backward-результаты по узлам ---
    {
        QFile file(dir.filePath("party_backward.csv"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&file);
            ts << "step,nodeIndex,pressure,angle,valid\n";

            for (std::size_t step = 0; step < result.backward.size(); ++step) {
                const auto &nodes = result.backward[step];
                for (std::size_t nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx) {
                    const NodeResult &node = nodes[nodeIdx];
                    ts << step << "," << nodeIdx << ","
                       << node.pressure << "," << node.angle << ","
                       << (node.valid ? 1 : 0) << "\n";
                }
            }
        }
    }

    // Вспомогательный лямбда-хелпер для сохранения DebugData
    auto saveDebugVector = [&dir](const QString &prefix,
                                  const std::vector<const DebugData*> &vec) {
        for (std::size_t i = 0; i < vec.size(); ++i) {
            const DebugData *dbg = vec[i];
            if (!dbg)
                continue;

            const QString fileName = QString("%1_%2.csv").arg(prefix).arg(i);
            QFile file(dir.filePath(fileName));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                continue;

            QTextStream ts(&file);
            ts << "windowIndex,nodePressure,localPressure[],localAngle[]\n";

            for (std::size_t w = 0; w < dbg->windows.size(); ++w) {
                const auto &win = dbg->windows[w];
                ts << w << "," << win.nodePressure << ",";

                // localPressure — запишем через ';'
                for (std::size_t lp = 0; lp < win.localPressure.size(); ++lp) {
                    if (lp > 0) ts << ";";
                    ts << win.localPressure[lp];
                }

                ts << ",";

                // localAngle — тоже через ';'
                for (std::size_t la = 0; la < win.localAngle.size(); ++la) {
                    if (la > 0) ts << ";";
                    ts << win.localAngle[la];
                }

                ts << "\n";
            }
        }
    };

    // --- 4. Debug-данные (forward/backward) ---
    saveDebugVector("party_debug_forward", result.debugDataForward);
    saveDebugVector("party_debug_backward", result.debugDataBackward);
}

void TelemetryLogger::startLoggingSession()
{
    if (m_isLoggingActive)
        return;

    auto &locator = ServiceLocator::instance();

    setPressureUnit(static_cast<PressureUnit>(
        locator.configManager()->get<int>("current.pressureUnit", static_cast<int>(PressureUnit::Pa))
    ));

    // Подключаемся к датчикам только на время градуировки
    connect(locator.pressureSensor(), &PressureSensor::pressureMeasured,
            this, &TelemetryLogger::onPressureMeasured, Qt::QueuedConnection);

    connect(locator.cameraProcessor(), &CameraProcessor::angleMeasured,
            this, &TelemetryLogger::onAngleMeasured, Qt::QueuedConnection);

    m_isLoggingActive = true;
    m_savingTimer.start();
}

void TelemetryLogger::finishLoggingSession(bool savePartyResultFlag)
{
    if (!m_isLoggingActive)
        return;

    // Останавливаем таймер и сбрасываем остаток телеметрии
    m_savingTimer.stop();
    saveTelemetryData();

    // Отключаемся от датчиков
    disconnectSensors();

    // При успешном завершении градуировки — сохраняем PartyResult
    if (savePartyResultFlag) {
        auto &locator = ServiceLocator::instance();
        auto graduationService = locator.graduationService();
        if (graduationService) {
            // PartyResult result = graduationService->getPartyResult();
            // if (result.isValid()) {
            //     savePartyResult(result);
            // }
        }
    }

    // Закрываем файлы и очищаем состояние
    closeFiles();
    clearBuffers();
    m_isLoggingActive = false;
    m_dailyFolder.clear();
}

void TelemetryLogger::disconnectSensors()
{
    auto &locator = ServiceLocator::instance();

    disconnect(locator.pressureSensor(), nullptr, this, nullptr);
    disconnect(locator.cameraProcessor(), nullptr, this, nullptr);
}

void TelemetryLogger::closeFiles()
{
    if (m_pressureFile) {
        m_pressureFile->close();
        delete m_pressureFile;
        m_pressureFile = nullptr;
    }

    for (auto &kv : m_angleFiles) {
        QFile *file = kv.second;
        if (!file)
            continue;
        file->close();
        delete file;
    }
    m_angleFiles.clear();
}

void TelemetryLogger::clearBuffers()
{
    m_pressureMeasurements.clear();
    m_angleMeasurements.clear();
}
