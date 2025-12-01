#ifndef GRADUATOR_SERVICELOCATOR_H
#define GRADUATOR_SERVICELOCATOR_H
#include <QObject>
#include <QPointer>
#include "Logger.h"
#include "GaugeCatalog.h"
#include "ConfigManager.h"
#include "PartyManager.h"
#include "pressureController/PressureControllerBase.h"
#include "cameraProcessor/CameraProcessor.h"
#include "PressureSensor.h"
#include "TelemetryLogger.h"
#include "graduationService/GraduationService.h"


// servicelocator.h
class ServiceLocator final : public QObject {
    Q_OBJECT
public:
    static ServiceLocator &instance();
    void setLogger(Logger *logger);
    Logger *logger() const noexcept;
    void setGaugeCatalog(GaugeCatalog *catalog);
    GaugeCatalog *gaugeCatalog() const noexcept;
    void setConfigManager(ConfigManager *configManager);
    ConfigManager *configManager() const noexcept;
    void setPartyManager(PartyManager *partyManager);
    PartyManager *partyManager() const noexcept;
    void setPressureController(PressureControllerBase *pressureController);
    PressureControllerBase *pressureController() const noexcept;
    void setCameraProcessor(CameraProcessor *cameraProcessor);
    CameraProcessor *cameraProcessor() const noexcept;
    void setPressureSensor(PressureSensor *pressureSensor);
    PressureSensor *pressureSensor() const noexcept;
    void setGraduationService(GraduationService *graduationService);
    GraduationService *graduationService() const noexcept;
    void setTelemetryLogger(TelemetryLogger *telemetryLogger);
    TelemetryLogger *telemetryLogger() const noexcept;

private:
    QPointer<Logger> m_logger;
    QPointer<GaugeCatalog> m_gaugeCatalog;
    QPointer<ConfigManager> m_configManager;
    QPointer<PartyManager> m_partyManager;
    QPointer<PressureControllerBase> m_pressureController;
    QPointer<CameraProcessor> m_cameraProcessor;
    QPointer<PressureSensor> m_pressureSensor;
    QPointer<GraduationService> m_graduationService;
    QPointer<TelemetryLogger> m_telemetryLogger;
};
#endif //GRADUATOR_SERVICELOCATOR_H