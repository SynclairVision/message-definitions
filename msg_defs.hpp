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
static constexpr float S16_MAX_F = U16_MAX_F / 2.0f;

enum PARAM_TYPE: uint8_t {
    VIDEO_OUTPUT,
    DETECTION,
    DETECTED_ROI,
    CAM_LENS,
    CAM_EULER,
    CAM_ZOOM,
    CAM_LOCK_FLAGS,
    CAM_CONTROL_MODE,
    CAM_CROP_MODE,
    CAM_OFFSET,
    CAM_FOV,
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
struct bounding_box {
    uint16_t x, y, w, h;
};

struct video_output_parameters {
    uint16_t width;
    uint16_t height;
    uint8_t fps;
    uint8_t layout_mode;
    //layout info, ignored for SET_PARAMETERS
    bounding_box views[4];
};

struct detection_parameters {
    uint8_t mode;
    uint8_t overlay_mode;
    uint8_t sorting_mode;
    uint8_t num_detections;
    bounding_box overlay_box;
    uint16_t overlay_roi_size;
};

struct detected_roi_parameters {
    uint8_t index;
    uint8_t score;
    float rel_heading;
    float rel_tilt;
    float latitude;
    float longitude;
    float altitude;
    float distance;
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

struct cam_offset_parameters {
    uint8_t cam_id;
    float x; // -1 < x < 1
    float y; // -1 < y < 1
    uint8_t frame_rel;
    float yaw;
    float pitch;
};

struct cam_fov_parameters {
    uint8_t cam_id;
    float fov;
};

/* PARAMETER PACKING
*/
inline void pack_video_output_parameters(message &msg, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode, bounding_box *views = nullptr) {
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

inline void pack_detection_parameters(message &msg, uint8_t mode, uint8_t overlay_mode, uint8_t sorting_mode, uint8_t num_detections, bounding_box overlay_box, uint16_t overlay_roi_size) {
    msg.param_type = DETECTION;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &overlay_mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &sorting_mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &num_detections, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &overlay_box, sizeof(bounding_box));
    offset += sizeof(bounding_box);
    memcpy((void *)&msg.data[offset], &overlay_roi_size, sizeof(uint16_t));
}

inline void pack_detected_roi_parameters(message &msg, uint8_t index, int8_t score, float rel_heading, float rel_tilt, float lat, float lon, float alt, float dist) {
    msg.param_type = DETECTED_ROI;
    uint8_t offset = 0;
    int32_t mrad;
    memcpy((void *)&msg.data[offset], &index, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &score, sizeof(int8_t));
    offset += sizeof(int8_t);
    mrad = static_cast<int32_t>(rel_heading * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad = static_cast<int32_t>(rel_tilt * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad = static_cast<int32_t>(lat * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad = static_cast<int32_t>(lon * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad = static_cast<int32_t>(alt * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad = static_cast<int32_t>(dist * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
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

inline void pack_cam_offset_parameters(message &msg, uint8_t cam, float x, float y, uint8_t frame_rel, float yaw = 0, float pitch = 0) {
    msg.param_type = CAM_OFFSET;
    uint8_t offset = 0;
    int16_t offs_int;
    int32_t mrad;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    offs_int = static_cast<int16_t>(x * S16_MAX_F);
    memcpy((void *)&msg.data[offset], &offs_int, sizeof(int16_t));
    offset += sizeof(int16_t);
    offs_int = static_cast<int16_t>(y * S16_MAX_F);
    memcpy((void *)&msg.data[offset], &offs_int, sizeof(int16_t));
    offset += sizeof(int16_t);
    memcpy((void *)&msg.data[offset], &frame_rel, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad = static_cast<int32_t>(yaw * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad = static_cast<int32_t>(pitch * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
}

inline void pack_cam_fov_parameters(message &msg, uint8_t cam, float fov) {
    msg.param_type = CAM_FOV;
    int16_t mrad;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad = static_cast<int16_t>(fov * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int16_t));
}

/* MESSAGE PACKING
    For each parameter and info there is one set_parameter_type
    Only one function for get parameters without arguments is needed.
*/
inline void pack_get_parameters(message &msg, uint8_t param_type) {
    msg.version = VERSION;
    msg.message_type = GET_PARAMETERS;
    msg.param_type = param_type;
}

inline void pack_get_detected_roi(message &msg, uint8_t index) {
    pack_get_parameters(msg, DETECTED_ROI);
    msg.data[0] = index;
}

inline void pack_get_detected_roi_visible(message &msg) {
    pack_get_parameters(msg, DETECTED_ROI);
    msg.data[0] = 254;
}

inline void pack_get_detected_roi_all(message &msg) {
    pack_get_parameters(msg, DETECTED_ROI);
    msg.data[0] = 255;
}

inline void pack_get_cam_offset_parameters(message msg, uint8_t cam, float x, float y, uint8_t frame_rel) {
    pack_get_parameters(msg, CAM_OFFSET);
    pack_cam_offset_parameters(msg, cam, x, y, frame_rel);
}

inline void pack_set_detection_parameters(message &msg, uint8_t mode, uint8_t overlay_mode, uint8_t sorting_mode) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_detection_parameters(msg, mode, overlay_mode, sorting_mode, 0, {}, 0);
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

inline void pack_set_cam_fov_parameters(message &msg, uint8_t cam, float fov) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_fov_parameters(msg, cam, fov);
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

inline void unpack_detection_parameters(message &raw_msg, detection_parameters &params) {
    uint8_t offset = 0;
    memcpy(&params.mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.overlay_mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.sorting_mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.num_detections, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.overlay_box, (void *)&raw_msg.data[offset], sizeof(bounding_box));
    offset += sizeof(bounding_box);
    memcpy(&params.overlay_roi_size, (void *)&raw_msg.data[offset], sizeof(uint16_t));
}

inline void unpack_detected_roi_parameters(message &raw_msg, detected_roi_parameters &params) {
    uint8_t offset = 0;
    int32_t mrad;
    memcpy(&params.index, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.score, (void *)&raw_msg.data[offset], sizeof(int8_t));
    offset += sizeof(int8_t);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.rel_heading = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.rel_tilt = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.latitude = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.longitude = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.altitude = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.distance = static_cast<float>(mrad) / 1000.0f;
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

inline void unpack_cam_offset_parameters(message &raw_msg, cam_offset_parameters &params) {
    int16_t x, y;
    int32_t yaw, pitch;
    uint8_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&x, (void *)&raw_msg.data[offset], sizeof(int16_t));
    offset += sizeof(int16_t);
    memcpy((void *)&y, (void *)&raw_msg.data[offset], sizeof(int16_t));
    offset += sizeof(int16_t);
    memcpy((void *)&params.frame_rel, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&yaw, (void *)&raw_msg.data[offset], sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy((void *)&pitch, (void *)&raw_msg.data[offset], sizeof(int32_t));
    offset += sizeof(int32_t);
    params.x = static_cast<float>(x) / S16_MAX_F;
    params.y = static_cast<float>(y) / S16_MAX_F;
    params.yaw = static_cast<float>(yaw) / 1000.0f;
    params.pitch = static_cast<float>(pitch) / 1000.0f;
}

inline void unpack_cam_fov_parameters(message &raw_msg, cam_fov_parameters &params) {
    int16_t mrad;
    uint8_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    params.fov = static_cast<float>(mrad) / 1000.0f;
}

#endif // MSG_DEFS_HPP