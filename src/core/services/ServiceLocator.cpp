#include "ServiceLocator.h"
ServiceLocator &ServiceLocator::instance() {
    static ServiceLocator instance;
    return instance;
}
void ServiceLocator::setLogger(Logger *logger) {
    m_logger = logger;
}
Logger *ServiceLocator::logger() const noexcept {
    return m_logger;
}
void ServiceLocator::setGaugeCatalog(GaugeCatalog *catalog) {
    m_gaugeCatalog = catalog;
}
GaugeCatalog *ServiceLocator::gaugeCatalog() const noexcept {
    return m_gaugeCatalog;
}
void ServiceLocator::setConfigManager(ConfigManager *configManager) {
    m_configManager = configManager;
}
ConfigManager *ServiceLocator::configManager() const noexcept {
    return m_configManager;
}
void ServiceLocator::setPartyManager(PartyManager *partyManager) {
    m_partyManager = partyManager;
}
PartyManager *ServiceLocator::partyManager() const noexcept {
    return m_partyManager;
}
void ServiceLocator::setPressureController(PressureControllerBase *pressureController) {
    m_pressureController = pressureController;
}
PressureControllerBase *ServiceLocator::pressureController() const noexcept {
    return m_pressureController;
}
void ServiceLocator::setCameraProcessor(CameraProcessor *cameraProcessor) {
    m_cameraProcessor = cameraProcessor;
}
CameraProcessor *ServiceLocator::cameraProcessor() const noexcept {
    return m_cameraProcessor;
}
void ServiceLocator::setPressureSensor(PressureSensor *pressureSensor) {
    m_pressureSensor = pressureSensor;
}
PressureSensor *ServiceLocator::pressureSensor() const noexcept {
    return m_pressureSensor;
}
void ServiceLocator::setGraduationService(GraduationService *graduationService) {
    m_graduationService = graduationService;
}
GraduationService *ServiceLocator::graduationService() const noexcept {
    return m_graduationService;
}