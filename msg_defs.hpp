#pragma once

#include "message.hpp"
#include <cstring>  // Inkludera cstring för memcpy

// Definiera strukturerna endast en gång
struct cam_white_balance_parameters {
    uint16_t cam_id;
    float white_balance;
};

struct cam_fov_parameters {
    uint16_t cam_id;
    float fov;
};

struct cam_target_parameters {
    uint16_t cam_id;
    float x, y, t_latitude, t_longitude, t_altitude;
};

struct cam_sensor_parameters {
    uint16_t cam_id;
    uint16_t ae;
    uint16_t target_brightness;
    uint16_t exposure_value;
    uint16_t gain_value;
};

struct detected_roi_parameters {
    // Definiera fälten som behövs här
};

struct cam_euler_parameters {
    uint16_t cam_id;
    uint16_t is_delta;
    float yaw, pitch, roll;
};

struct cam_zoom_parameters {
    uint16_t cam_id;
    int16_t zoom;
};

struct cam_lock_flags_parameters {
    uint16_t cam_id;
    uint16_t flags;
};

struct cam_control_mode_parameters {
    uint16_t cam_id;
    uint16_t mode;
};

struct cam_crop_mode_parameters {
    uint16_t cam_id;
    uint16_t mode;
};

struct cam_offset_parameters {
    uint16_t cam_id;
    float x, y, yaw_abs, pitch_abs, yaw_rel, pitch_rel;
};

// Definiera funktionerna endast en gång
void pack_set_cam_white_balance(message &msg, uint16_t cam_id, float white_balance) {
    msg.message_type = SET_CAM_WHITE_BALANCE;
    cam_white_balance_parameters params = {cam_id, white_balance};
    memcpy(msg.data, &params, sizeof(params));
}

void pack_get_cam_offset_parameters(message &msg, uint16_t cam_id, float x, float y) {
    msg.message_type = CAM_OFFSET;
    cam_offset_parameters params = {cam_id, x, y, 0.0f, 0.0f, 0.0f, 0.0f}; // Exempelvärden
    memcpy(msg.data, &params, sizeof(params));
}

void pack_get_parameters(message &msg, MessageType type, uint16_t cam_id = 0) {
    msg.message_type = type;
    msg.param_type = cam_id;
    // Lägg till ytterligare packningslogik här om nödvändigt
}

void pack_get_detected_roi_all(message &msg) {
    msg.message_type = DETECTION;
    // Lägg till kod för att packa detected_roi_all här
}

void unpack_cam_fov_parameters(const message &msg, cam_fov_parameters &params) {
    // Lägg till kod för att packa upp cam_fov_parameters här
}

void unpack_cam_target_parameters(const message &msg, cam_target_parameters &params) {
    // Lägg till kod för att packa upp cam_target_parameters här
}

void unpack_cam_sensor_parameters(const message &msg, cam_sensor_parameters &params) {
    // Lägg till kod för att packa upp cam_sensor_parameters här
}

void unpack_detected_roi_parameters(const message &msg, detected_roi_parameters &params) {
    // Lägg till kod för att packa upp detected_roi_parameters här
}

void unpack_cam_euler_parameters(const message &msg, cam_euler_parameters &params) {
    // Lägg till kod för att packa upp cam_euler_parameters här
}

void unpack_cam_zoom_parameters(const message &msg, cam_zoom_parameters &params) {
    // Lägg till kod för att packa upp cam_zoom_parameters här
}

void unpack_cam_lock_flags_parameters(const message &msg, cam_lock_flags_parameters &params) {
    // Lägg till kod för att packa upp cam_lock_flags_parameters här
}

void unpack_cam_control_mode_parameters(const message &msg, cam_control_mode_parameters &params) {
    // Lägg till kod för att packa upp cam_control_mode_parameters här
}

void unpack_cam_crop_mode_parameters(const message &msg, cam_crop_mode_parameters &params) {
    // Lägg till kod för att packa upp cam_crop_mode_parameters här
}

void unpack_cam_offset_parameters(const message &msg, cam_offset_parameters &params) {
    // Lägg till kod för att packa upp cam_offset_parameters här
}
