#ifndef GRADUATOR_TELEMETRYLOGGER_H
#define GRADUATOR_TELEMETRYLOGGER_H

#include "core/types/Pressure.h"
#include "core/types/PartyResult.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <map>
#include <vector>

/**
 * @brief TelemetryLogger
 *
 * Класс отвечает за логирование телеметрии процесса градуировки:
 *  - при старте градуировки создаёт новую директорию и начинает приём данных;
 *  - периодически (по таймеру) сбрасывает накопленные измерения в CSV-файлы;
 *  - при завершении градуировки сбрасывает остаток данных и сохраняет PartyResult;
 *  - при прерывании градуировки сбрасывает остаток данных, но PartyResult не сохраняет.
 */
class TelemetryLogger final : public QObject {
    Q_OBJECT
public:
    explicit TelemetryLogger(const QString &logFolder, QObject *parent = nullptr);
    ~TelemetryLogger() override;

    /// Задать единицы измерения давления (будут применяться при логировании)
    void setPressureUnit(PressureUnit unit);

    /// Активировать логгер (подписаться на события GraduationService)
    void begin();

    /// Деактивировать логгер и освободить ресурсы
    void end();

private slots:
    /// Обработчик начала градуировки
    void onGraduationStarted();

    /// Обработчик успешного завершения градуировки
    void onGraduationEnded();

    /// Обработчик прерывания градуировки
    void onGraduationInterrupted();

    /// Пришло новое измерение давления
    void onPressureMeasured(qreal t, Pressure p);

    /// Пришло новое измерение угла (канал i)
    void onAngleMeasured(qint32 i, qreal t, qreal a);

    /// Периодический сброс телеметрии на диск
    void saveTelemetryData();

    /// Сохранение PartyResult после завершения градуировки
    void savePartyResult(const PartyResult& result);

private:
    /// Создаёт подкаталог с уникальным именем для текущего запуска градуировки
    QString makeDailyFolder() const;

    /// Убедиться, что файлы телеметрии открыты (давление + все уже известные каналы угла)
    void ensureFilesOpened();

    /// Открыть файл для давления
    void openPressureFile();

    /// Открыть файл для угла указанного канала
    void openAngleFile(int idx);

    /// Запуск новой "сессии" логирования (подписка на датчики, старт таймера)
    void startLoggingSession();

    /// Завершение текущей "сессии" логирования
    /// @param savePartyResultFlag - если true, дополнительно сохраняется PartyResult
    void finishLoggingSession(bool savePartyResultFlag);

    /// Отключение от датчиков (давление, камера)
    void disconnectSensors();

    /// Закрыть все открытые файлы телеметрии
    void closeFiles();

    /// Очистить накопленные измерения из памяти
    void clearBuffers();

private:
    /// Корневая директория для логов (приходит извне)
    QString m_logFolderRoot;

    /// Путь к директории текущей сессии (создаётся при старте градуировки)
    QString m_dailyFolder;

    /// Таймер периодического сохранения данных
    QTimer m_savingTimer;

    /// Текущая единица измерения давления
    PressureUnit m_pressureUnit{PressureUnit::Pa};

    /// Файлы для углов по каналам
    std::map<int, QFile*> m_angleFiles;

    /// Файл для давления
    QFile* m_pressureFile{nullptr};

    /// Накопленные измерения угла: [канал -> вектор (t, angle)]
    std::map<int, std::vector<std::pair<qreal, qreal>>> m_angleMeasurements;

    /// Накопленные измерения давления: вектор (t, pressure)
    std::vector<std::pair<qreal, qreal>> m_pressureMeasurements;

    /// Флаг: идёт ли сейчас активная сессия градуировки (логирование включено)
    bool m_isLoggingActive{false};
};

#endif // GRADUATOR_TELEMETRYLOGGER_H
