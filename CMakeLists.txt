cmake_minimum_required(VERSION 3.5)

project(Graduator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64")
#set(CMAKE_PREFIX_PATH "S:/Qt/Qt-5.15.17/win32-msvc")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party)

add_definitions(-DUSE_STUB_IMPLEMENTATIONS)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        Concurrent
        Sql
        Network
        LinguistTools
        REQUIRED)

set(RES_FILES res.qrc)

set(SRC_FILES
        src/main.cpp
        src/core/services/ConfigManager.cpp
        src/core/services/GaugeCatalog.cpp
        src/core/services/Logger.cpp
        src/core/services/ServiceLocator.cpp
        src/core/services/UserDialogService.cpp
        src/core/services/pressureController/PressureControllerBase.cpp
        src/core/services/pressureController/drivers/G540Driver.cpp
        src/core/services/pressureController/drivers/LPTDriver.cpp
        src/core/services/pressureController/impl/PressureControllerStand5.cpp
        src/core/services/pressureController/impl/PressureControllerStand4.cpp
        src/core/services/pressureController/impl/utils.cpp
        src/gui/widgets/cameraGrid/AspectRatioLabel.cpp
        src/gui/widgets/cameraGrid/CameraGridWidget.cpp
        src/core/services/cameraProcessor/CameraProcessor.cpp
        src/core/services/cameraProcessor/dshow/image/RGBImage.cpp
        src/core/services/cameraProcessor/dshow/FrameGrabberCB.cpp
        src/core/services/cameraProcessor/dshow/VideoCaptureProcessor.cpp
        src/gui/widgets/MainWidget.cpp
        src/gui/widgets/LogAndConfigTab.cpp
        src/gui/windows/MainWindow.cpp
        src/gui/widgets/SettingsWidget.cpp
        src/gui/widgets/control/ControlWidget.cpp
        src/gui/widgets/control/AutomaticWidget.cpp
        src/gui/widgets/control/ManualWidget.cpp
        src/gui/widgets/GraduationTableWidget.cpp
        src/gui/widgets/CameraSettingsTab.cpp
        src/gui/models/GraduationTableModel.cpp
        src/gui/widgets/control/PartyWidget.cpp
        src/core/services/PressureSensor.cpp
        src/core/services/graduationService/GraduationService.cpp
        src/core/services/graduationService/Graduator.cpp
        src/core/services/cameraProcessor/anglemeter/cast_anglemeter.cpp
        src/gui/views/StatusBarView.cpp
        src/gui/models/StatusBarModel.cpp
        src/ApplicationBuilder.cpp
        src/core/services/stubs/PressureSensor.cpp
        src/core/services/graduationService/PressureAngleGraduator.cpp
        src/core/services/graduationService/BatchGraduator.cpp
        src/core/services/PartyManager.cpp
        src/core/services/cameraProcessor/anglemeter/AnglemeterProcessor.cpp
        src/core/services/cameraProcessor/anglemeter/AnglemeterProcessor.cpp
        src/core/services/cameraProcessor/CameraSettings.cpp
        src/core/services/cameraProcessor/Camera.cpp
        src/gui/widgets/notifications/NotificationArea.cpp
        src/gui/widgets/notifications/NotificationWidget.cpp
        src/core/utils/Derivator.cpp
        src/core/utils/Mediator.cpp
        src/core/services/TelemetryLogger.cpp
        src/core/types/PartyResult.cpp
)

set(HEADER_FILES
        src/core/types/Pressure.h
        src/core/types/GaugeModel.h
        src/core/types/PressureUnit.h
        src/core/services/ConfigManager.h
        src/core/services/GaugeCatalog.h
        src/core/services/Logger.h
        src/core/services/ServiceLocator.h
        src/core/services/UserDialogService.h
        src/core/services/pressureController/PressureControllerBase.h
        src/core/services/pressureController/drivers/G540Driver.h
        src/core/services/pressureController/drivers/LPTDriver.h
        src/core/services/pressureController/impl/PressureControllerStand5.h
        src/core/services/pressureController/impl/PressureControllerStand4.h
        src/core/services/pressureController/impl/utils.h
        src/gui/widgets/cameraGrid/AspectRatioLabel.h
        src/gui/widgets/cameraGrid/CameraGridWidget.h
        src/core/services/cameraProcessor/CameraProcessor.h
        src/core/services/cameraProcessor/dshow/VideoCaptureProcessor.h
        src/core/services/cameraProcessor/dshow/FrameGrabberCB.h
        src/gui/widgets/MainWidget.h
        src/gui/widgets/LogAndConfigTab.h
        src/gui/windows/MainWindow.h
        src/gui/views/StatusBarView.h
        src/gui/widgets/SettingsWidget.h
        src/gui/widgets/control/ControlWidget.h
        src/gui/widgets/control/AutomaticWidget.h
        src/gui/widgets/control/ManualWidget.h
        src/gui/widgets/GraduationTableWidget.h
        src/gui/widgets/CameraSettingsTab.h
        src/gui/models/GraduationTableModel.h
        src/gui/widgets/control/PartyWidget.h
        src/core/services/PressureSensor.h
        src/core/services/graduationService/Graduator.h
        src/core/services/graduationService/GraduationService.h
        src/gui/models/StatusBarModel.h
        src/core/types/PartyResult.h
        src/ApplicationBuilder.h
        src/core/services/PartyManager.h
        src/core/types/Printer.h
        src/core/types/Displacement.h
        src/core/types/Grad.h
        src/core/services/cameraProcessor/Camera.h
        src/core/services/cameraProcessor/CameraSettings.h
        src/core/services/cameraProcessor/anglemeter/AnglemeterProcessor.h
        src/gui/widgets/notifications/NotificationArea.h
        src/gui/widgets/notifications/NotificationWidget.h
        src/core/services/TelemetryLogger.h
        src/core/utils/Derivator.h
        src/core/utils/Mediator.h
        src/defines.h
)


qt5_wrap_cpp(SRC_MOC_FILES
        ${HEADER_FILES}
)

add_executable(Graduator
        ${SRC_FILES}
        ${SRC_MOC_FILES}
        ${RES_FILES}
)

target_link_libraries(Graduator
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Concurrent
    Qt5::Network
    Qt5::Sql)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Concurrent Sql Network)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
    if (EXISTS "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
    endif ()
    #    if (EXISTS "${CMAKE_SOURCE_DIR}/config.json")
    #        add_custom_command(
    #                TARGET Graduator POST_BUILD
    #                COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #                ${CMAKE_SOURCE_DIR}/config.json
    #                $<TARGET_FILE_DIR:Graduator>/config.json
    #        )
    #    endif()
    if (EXISTS "${CMAKE_SOURCE_DIR}/gauges")
        add_custom_command(
                TARGET Graduator POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/gauges
                $<TARGET_FILE_DIR:Graduator>/gauges
        )
    endif()
    if (EXISTS "${CMAKE_SOURCE_DIR}/third-party/WinRing0")
        add_custom_command(
                TARGET Graduator POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/third-party/WinRing0
                $<TARGET_FILE_DIR:Graduator>
        )
    endif()
endif ()



