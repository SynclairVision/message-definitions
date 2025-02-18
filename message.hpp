#pragma once

enum MessageType {
    SYSTEM_STATUS,
    GENERAL_SETTINGS,
    VIDEO_OUTPUT,
    CAPTURE,
    DETECTION,
    SET_CAM_WHITE_BALANCE,
    CAM_LOCK_FLAGS,
    CAM_CONTROL_MODE,
    CAM_CROP_MODE,
    CAM_FOV,
    CAM_TARGET,
    CAM_SENSOR,
    CURRENT_PARAMETERS,
    CAM_EULER,
    CAM_ZOOM,
    QUIT,
    CAM_OFFSET
};

struct message {
    uint16_t message_type;
    uint16_t version;
    uint16_t command;
    uint16_t length;
    uint16_t param_type;
    char data[256];  // Exempel på storlek, justera efter behov
};

