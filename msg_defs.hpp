/*
################################
DigiView message definitions for DigiView version 0.2
################################
msg_defs.hpp version 0.2.1
################################
*/
#pragma once

#ifndef MSG_DEFS_HPP
#define MSG_DEFS_HPP

#include <cstring>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

static constexpr uint32_t PARAMCOUNT = 64;
static constexpr uint32_t VERSION = 0x00;

static constexpr float U16_MAX_F = 65535.0f;

enum PARAM_TYPE: uint8_t  {
    VIDEO_OUTPUT,
    CAM_LENS,
    CAM_EULER,
    CAM_ZOOM,
    CAM_LOCK_FLAGS,
    CAM_CONTROL_MODE,
    CAM_CROP_MODE,
};

enum MESSAGE_TYPE: uint8_t {
    EMPTY,
    QUIT,
    GET_PARAMETERS,
    SET_PARAMETERS,
    CURRENT_PARAMETERS,
    ACKNOWLEDGEMENT,
    CHECKSUM_ERROR,
    DATA_ERROR,
    FORBIDDEN,
    UNKNOWN,
};

/* BASE MESSAGE */
struct message {
    uint8_t version;
    uint8_t message_type;
    uint8_t param_type;
    uint8_t data[PARAMCOUNT];
};

/* PARAMETER TYPES 
    There is one struct for each group of parameters in the system.
*/
struct view_parameters {
    uint16_t x, y, w, h;
};

struct video_output_parameters {
    uint16_t width;
    uint16_t height;
    uint8_t fps;
    uint8_t layout_mode;
    //layout info, ignored for SET_PARAMETERS
    view_parameters views[4];
};

struct cam_lens_parameters {
    uint8_t lens_id;
};

struct cam_euler_parameters {
    uint8_t cam_id;
    uint8_t is_delta;
    float yaw;
    float pitch;
    float roll;
};

struct cam_zoom_parameters {
    uint8_t cam_id;
    int8_t zoom;
};

struct cam_lock_flags_parameters {
    uint8_t cam_id;
    uint8_t flags;
};

struct cam_control_mode_parameters {
    uint8_t cam_id;
    uint8_t mode;
};

struct cam_crop_mode_parameters {
    uint8_t cam_id;
    uint8_t mode;
};


/* PARAMETER PACKING
*/
inline void pack_video_output_parameters(message &msg, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode, view_parameters *views = nullptr) {
    msg.param_type = VIDEO_OUTPUT;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &width, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &height, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &fps, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &layout_mode, sizeof(uint8_t));
    if(views != nullptr) {
        offset += sizeof(uint8_t);
        uint8_t num_views = (layout_mode & 0xf0) >> 4;
        for(uint8_t i = 0; i < num_views; i++) {
            memcpy((void *)&msg.data[offset], &views[i].x, sizeof(uint16_t));
            offset += sizeof(uint16_t);
            memcpy((void *)&msg.data[offset], &views[i].y, sizeof(uint16_t));
            offset += sizeof(uint16_t);
            memcpy((void *)&msg.data[offset], &views[i].w, sizeof(uint16_t));
            offset += sizeof(uint16_t);
            memcpy((void *)&msg.data[offset], &views[i].h, sizeof(uint16_t));
            offset += sizeof(uint16_t);
        }
    }
}

inline void pack_cam_lens_parameters(message &msg, uint8_t lens_id) {
    msg.param_type = CAM_LENS;
    memcpy((void *)&msg.data[0], &lens_id, sizeof(uint8_t));
}

inline void pack_cam_euler_parameters(message &msg, uint8_t cam, uint8_t is_delta, float yaw, float pitch, float roll) {
    msg.param_type = CAM_EULER;
    int32_t mrad;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &is_delta, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad = static_cast<int32_t>(yaw * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad = static_cast<int32_t>(pitch * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad = static_cast<int32_t>(roll * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
}

inline void pack_cam_zoom_parameters(message &msg, uint8_t cam, int8_t zoom) {
    msg.param_type = CAM_ZOOM;
    memcpy((void *)&msg.data[0], &cam, sizeof(uint8_t));
    memcpy((void *)&msg.data[1], &zoom, sizeof(int8_t));
}

inline void pack_cam_lock_flags_parameters(message &msg, uint8_t cam, uint8_t flags) {
    msg.param_type = CAM_LOCK_FLAGS;
    memcpy((void *)&msg.data[0], &cam, sizeof(uint8_t));
    memcpy((void *)&msg.data[1], &flags, sizeof(uint8_t));
}

inline void pack_cam_control_mode_parameters(message &msg, uint8_t cam, uint8_t mode) {
    msg.param_type = CAM_CONTROL_MODE;
    memcpy((void *)&msg.data[0], &cam, sizeof(uint8_t));
    memcpy((void *)&msg.data[1], &mode, sizeof(uint8_t));
}

inline void pack_cam_crop_mode_parameters(message &msg, uint8_t cam, uint8_t mode) {
    msg.param_type = CAM_CROP_MODE;
    memcpy((void *)&msg.data[0], &cam, sizeof(uint8_t));
    memcpy((void *)&msg.data[1], &mode, sizeof(uint8_t));
}


/* MESSAGE PACKING
    For each parameter and info there is one set_parameter_type
    Only one function for get parameters is needed.
*/
inline void pack_get_parameters(message &msg, uint8_t param_type) {
    msg.version = VERSION;
    msg.message_type = GET_PARAMETERS;
    msg.param_type = param_type;
}

inline void pack_set_video_output_parameters(message &msg, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_video_output_parameters(msg, width, height, fps, layout_mode);
}

inline void pack_set_cam_lens_parameters(message &msg, uint8_t lens_id) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_lens_parameters(msg, lens_id);
}

inline void pack_set_cam_euler_parameters(message &msg, uint8_t cam, uint8_t is_delta, float yaw, float pitch, float roll) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_euler_parameters(msg, cam, is_delta, yaw, pitch, roll);
}

inline void pack_set_cam_zoom_parameters(message &msg, uint8_t cam, int8_t zoom) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_zoom_parameters(msg, cam, zoom);
}

inline void pack_set_cam_lock_flags_parameters(message &msg, uint8_t cam, uint8_t flags) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_lock_flags_parameters(msg, cam, flags);
}

inline void pack_set_cam_control_mode_parameters(message &msg, uint8_t cam, uint8_t mode) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_control_mode_parameters(msg, cam, mode);
}

inline void pack_set_cam_crop_mode_parameters(message &msg, uint8_t cam, uint8_t mode) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_crop_mode_parameters(msg, cam, mode);
}

/* PARAMETER UNPACKING
*/
inline void unpack_video_output_parameters(message &raw_msg, video_output_parameters &params) {
    uint8_t offset = 0;
    memcpy((void *)&params.width, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&params.height, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&params.fps, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.layout_mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    uint8_t num_views = (params.layout_mode & 0xf0) >> 4;
    for(uint8_t i = 0; i < num_views; i++) {
        memcpy((void *)&params.views[i].x, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy((void *)&params.views[i].y, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy((void *)&params.views[i].w, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy((void *)&params.views[i].h, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
    }
}

inline void unpack_cam_lens_parameters(message &raw_msg, cam_lens_parameters &params) {
    memcpy((void *)&params.lens_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
}

inline void unpack_cam_euler_parameters(message &raw_msg, cam_euler_parameters &params) {
    int32_t mrad;
    uint8_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.is_delta, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.yaw = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.pitch = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.roll = static_cast<float>(mrad) / 1000.0f;
}

inline void unpack_cam_zoom_parameters(message &raw_msg, cam_zoom_parameters &params) {
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
    memcpy((void *)&params.zoom, (void *)&raw_msg.data[1], sizeof(int8_t));
}

inline void unpack_cam_lock_flags_parameters(message &raw_msg, cam_lock_flags_parameters &params) {
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
    memcpy((void *)&params.flags, (void *)&raw_msg.data[1], sizeof(uint8_t));
}

inline void unpack_cam_control_mode_parameters(message &raw_msg, cam_control_mode_parameters &params) {
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
    memcpy((void *)&params.mode, (void *)&raw_msg.data[1], sizeof(uint8_t));
}

inline void unpack_cam_crop_mode_parameters(message &raw_msg, cam_crop_mode_parameters &params) {
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
    memcpy((void *)&params.mode, (void *)&raw_msg.data[1], sizeof(uint8_t));
}

#endif // MSG_DEFS_HPP
