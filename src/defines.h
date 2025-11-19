#ifndef CONFIG_DEFINES_H
#define CONFIG_DEFINES_H
// G540Driver
#define CFG_KEY_G540_BIT_END_LIMIT_SWITCH        "G540Driver.bitEndLimitSwitch"
#define CFG_KEY_G540_BIT_START_LIMIT_SWITCH      "G540Driver.bitStartLimitSwitch"
#define CFG_KEY_G540_BYTE_CLOSE_BOTH_FLAPS       "G540Driver.byteCloseBothFlaps"
#define CFG_KEY_G540_BYTE_OPEN_INPUT_FLAP        "G540Driver.byteOpenInputFlap"
#define CFG_KEY_G540_BYTE_OPEN_OUTPUT_FLAP       "G540Driver.byteOpenOutputFlap"
#define CFG_KEY_G540_MAX_FREQUENCY               "G540Driver.maxFrequency"
#define CFG_KEY_G540_MIN_FREQUENCY               "G540Driver.minFrequency"
#define CFG_KEY_G540_PORT_ADDRESS                "G540Driver.portAddress"
// current
#define CFG_KEY_CURRENT_CAMERA_STR               "current.cameraStr"
#define CFG_KEY_CURRENT_GAUGE_MODEL              "current.gaugeModel"
#define CFG_KEY_CURRENT_PRESSURE_UNIT            "current.pressureUnit"
#define CFG_KEY_CURRENT_PRECISION_CLASS          "current.precisionClass"
#define CFG_KEY_CURRENT_PRINTER                  "current.printer"
#define CFG_KEY_CURRENT_DIAL_LAYOUT              "current.dialLayout"
// cameraProcessor
#define CFG_KEY_SYS_CAMERA_STR                   "cameraProcessor.sysCameraStr"
#define CFG_KEY_SYS_AUTOOPEN                     "cameraProcessor.autoopen"
#define CFG_KEY_DRAW_CROSSHAIR                   "cameraProcessor.drawCrosshair"
// pressureController
#define CFG_KEY_PRESSURE_MAX_VELOCITY_FACTOR     "pressureController.maxVelocityFactor"
#define CFG_KEY_PRESSURE_MIN_VELOCITY_FACTOR     "pressureController.minVelocityFactor"
#define CFG_KEY_PRESSURE_NOMINAL_DURATION_SEC    "pressureController.nominalDurationSec"
#define CFG_KEY_PRESSURE_PRELOAD_FACTOR          "pressureController.preloadFactor"
//pressureSensor
#define CFG_KEY_PRESSURE_SENSOR_COM              "pressureSensor.comPort"
// stand
#define CFG_KEY_STAND_NUMBER                     "stand.number"
#define CFG_KEY_PARTIES_FOLDER                   "stand.partiesFolder"
#define CFG_KEY_PRECISION_CLASSES                "stand.precisionClasses"
#define CFG_KEY_DISPLACEMENTS                    "stand.displacements"
#define CFG_KEY_PRINTERS                         "stand.printers"
#endif // CONFIG_DEFINES_H
