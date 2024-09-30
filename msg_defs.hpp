#pragma once

#ifndef MSG_DEFS_HPP
#define MSG_DEFS_HPP

#include <cstring>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

static constexpr uint32_t PARAMCOUNT            = 64;
static constexpr uint32_t VERSION               = 0x00;

static constexpr float    U16_MAX_F             = 65535.0f;
static constexpr float    S16_MAX_F             = 32767.0f;

static constexpr uint8_t  CAP_FLAG_SINGLE_IMAGE = 0x01;
static constexpr uint8_t  CAP_FLAG_VIDEO        = 0x02;

enum PARAM_TYPE : uint8_t {
    SYSTEM_STATUS,
    GENERAL_SETTINGS,
    VIDEO_OUTPUT,
    CAPTURE,
    DETECTION,
    DETECTED_ROI,
    LENS,
    CAM_EULER,
    CAM_ZOOM,
    CAM_LOCK_FLAGS,
    CAM_CONTROL_MODE,
    CAM_CROP_MODE,
    CAM_OFFSET,
    CAM_FOV,
    CAM_TARGET,
    CAM_SENSOR
};

enum MESSAGE_TYPE : uint8_t {
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
    uint64_t timestamp; // Timestamp for when the message was received
    uint8_t  version;
    uint8_t  message_type;
    uint8_t  param_type;
    uint8_t  data[PARAMCOUNT];
};

/* PARAMETER TYPES
    There is one struct for each group of parameters in the system.
*/
struct bounding_box {
    uint16_t x, y, w, h;
};

struct system_status_parameters {
    uint8_t status;
    uint8_t error;
};

struct general_settings_parameters {
    uint16_t camera_width;
    uint16_t camera_height;
    uint16_t roi_width;
    uint16_t roi_height;
    uint8_t  camera_fps;
    float    mount_yaw;
    float    mount_pitch;
    float    mount_roll;
    uint8_t  run_ai;
};

struct video_output_parameters {
    uint16_t     width;
    uint16_t     height;
    uint8_t      fps;
    uint8_t      layout_mode;
    uint8_t      detection_overlay_mode;
    // layout info, ignored for SET_PARAMETERS
    bounding_box views[4];
    bounding_box detection_overlay_box;
    uint16_t     single_detection_size;
};

struct capture_parameters {
    bool     cap_single_image;
    bool     record_video;
    uint16_t images_captured;
    uint16_t videos_captured;
};

struct detection_parameters {
    uint8_t  mode;
    uint8_t  sorting_mode;
    float    crop_confidence_threshold;
    float    var_confidence_threshold;
    uint16_t crop_box_limit;
    uint16_t var_box_limit;
    float    crop_box_overlap;
    float    var_box_overlap;
    uint8_t  creation_score_scale;
    uint8_t  bonus_detection_scale;
    uint8_t  bonus_redetection_scale;
    uint8_t  missed_detection_penalty;
    uint8_t  missed_redetection_penalty;

};

struct detected_roi_parameters {
    uint8_t index;
    uint8_t score;
    uint8_t total_detections;
    float   yaw_abs;
    float   pitch_abs;
    float   yaw_rel;
    float   pitch_rel;
    float   latitude;
    float   longitude;
    float   altitude;
    float   distance;
};

struct lens_parameters {
    uint8_t lens_id;
};

struct cam_euler_parameters {
    uint8_t cam_id;
    uint8_t is_delta;
    float   yaw;
    float   pitch;
    float   roll;
};

struct cam_zoom_parameters {
    uint8_t cam_id;
    int8_t  zoom;
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
    float   x; // -1 < x < 1
    float   y; // -1 < y < 1
    uint8_t frame_rel;
    float   yaw;
    float   pitch;
};

struct cam_fov_parameters {
    uint8_t cam_id;
    float   fov;
};

struct cam_target_parameters {
    uint8_t cam_id;
    float   x; // -1 < x < 1
    float   y; // -1 < y < 1
    float   t_latitude;
    float   t_longitude;
    float   t_altitude;
};

struct cam_sensor_parameters {
    uint8_t ae;
    uint8_t target_brightness;
    uint32_t exposure_value;
    uint32_t gain_value;
};

/* PARAMETER PACKING
 */
inline void pack_system_status_parameters(message &msg, uint8_t status, uint8_t error) {
    msg.param_type = SYSTEM_STATUS;
    msg.data[0]    = status;
    msg.data[1]    = error;
}

inline void pack_general_settings_parameters(
    message &msg, uint16_t camera_width, uint16_t camera_height, uint16_t roi_width, uint16_t roi_height, uint8_t camera_fps,
    float mount_yaw, float mount_pitch, float mount_roll, uint8_t run_ai) {

    msg.param_type = GENERAL_SETTINGS;
    uint8_t offset = 0;
    int32_t mrad;
    memcpy((void *)&msg.data[offset], &camera_width, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &camera_height, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &roi_width, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &roi_height, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &camera_fps, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad = static_cast<int32_t>(mount_yaw * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad = static_cast<int32_t>(mount_pitch * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad = static_cast<int32_t>(mount_roll * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy((void *)&msg.data[offset], &run_ai, sizeof(uint8_t));
}

inline void pack_video_output_parameters(
    message &msg, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode, uint8_t detection_overlay_mode,
    bounding_box *views = nullptr, bounding_box detection_overlay_box = {}, uint16_t single_detection_size = 0) {

    msg.param_type = VIDEO_OUTPUT;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &width, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &height, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &fps, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &layout_mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &detection_overlay_mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    if (views != nullptr) {
        uint8_t num_views = (layout_mode & 0xf0) >> 4;
        for (uint8_t i = 0; i < num_views; i++) {
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
    memcpy((void *)&msg.data[offset], &detection_overlay_box, sizeof(bounding_box));
    offset += sizeof(bounding_box);
    memcpy((void *)&msg.data[offset], &single_detection_size, sizeof(uint16_t));
}

inline void pack_capture_parameters(message &msg, bool pic, bool vid, uint16_t num_pics = 0, uint16_t num_vids = 0) {
    msg.param_type     = CAPTURE;
    uint8_t offset     = 0;
    uint8_t cap_flags  = 0x0;
    cap_flags         |= pic ? CAP_FLAG_SINGLE_IMAGE : 0;
    cap_flags         |= vid ? CAP_FLAG_VIDEO : 0;
    memcpy((void *)&msg.data[offset], &cap_flags, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &num_pics, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &num_vids, sizeof(uint16_t));
}

inline void pack_detection_parameters(
    message &msg, uint8_t mode, uint8_t sorting_mode, float crop_confidence_threshold, float var_confidence_threshold,
    uint16_t crop_box_limit, uint16_t var_box_limit,
    float crop_box_overlap, float var_box_overlap, uint8_t creation_score_scale, uint8_t bonus_detection_scale,
    uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty, uint8_t missed_redetection_penalty) {

    msg.param_type           = DETECTION;
    uint8_t offset           = 0;
    uint8_t crop_conf_thresh = static_cast<uint8_t>(crop_confidence_threshold * 255.0f);
    uint8_t var_conf_thresh  = static_cast<uint8_t>(var_confidence_threshold * 255.0f);
    uint8_t crop_box_ovlp    = static_cast<uint8_t>(crop_box_overlap * 255.0f);
    uint8_t var_box_ovlp     = static_cast<uint8_t>(var_box_overlap * 255.0f);

    memcpy((void *)&msg.data[offset], &mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &sorting_mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &crop_conf_thresh, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &var_conf_thresh, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &crop_box_limit, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &var_box_limit, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&msg.data[offset], &crop_box_ovlp, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &var_box_ovlp, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &creation_score_scale, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &bonus_detection_scale, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &bonus_redetection_scale, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &missed_detection_penalty, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &missed_redetection_penalty, sizeof(uint8_t));
}

inline void pack_detected_roi_parameters(
    message &msg, uint8_t total_detections, uint8_t index, uint8_t score, float yaw_abs, float pitch_abs,
    float yaw_rel, float pitch_rel, float lat, float lon, float alt, float dist) {
    msg.param_type = DETECTED_ROI;
    uint8_t offset = 0;
    int32_t mrad;
    memcpy((void *)&msg.data[offset], &index, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &score, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &total_detections, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad    = static_cast<int32_t>(yaw_abs * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(pitch_abs * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(yaw_rel * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(pitch_rel * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad    = static_cast<int32_t>(lat * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad    = static_cast<int32_t>(lon * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad    = static_cast<int32_t>(alt * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(float);
    mrad    = static_cast<int32_t>(dist * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
}

inline void pack_lens_parameters(message &msg, uint8_t lens_id) {
    msg.param_type = LENS;
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
    mrad    = static_cast<int32_t>(yaw * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(pitch * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(roll * 1000.0f);
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

inline void pack_cam_offset_parameters(
    message &msg, uint8_t cam, float x, float y, uint8_t frame_rel, float yaw = 0, float pitch = 0) {
    msg.param_type = CAM_OFFSET;
    uint8_t offset = 0;
    int16_t offs_int;
    int32_t mrad;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset   += sizeof(uint8_t);
    offs_int  = static_cast<int16_t>(x * S16_MAX_F);
    memcpy((void *)&msg.data[offset], &offs_int, sizeof(int16_t));
    offset   += sizeof(int16_t);
    offs_int  = static_cast<int16_t>(y * S16_MAX_F);
    memcpy((void *)&msg.data[offset], &offs_int, sizeof(int16_t));
    offset += sizeof(int16_t);
    memcpy((void *)&msg.data[offset], &frame_rel, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad    = static_cast<int32_t>(yaw * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(pitch * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
}

inline void pack_cam_fov_parameters(message &msg, uint8_t cam, float fov) {
    msg.param_type = CAM_FOV;
    int16_t mrad;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad    = static_cast<int16_t>(fov * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int16_t));
}

inline void pack_cam_target_parameters(message &msg, uint8_t cam, float x, float y, float t_lat, float t_lon, float t_alt) {
    msg.param_type = CAM_TARGET;
    uint8_t offset = 0;
    int32_t mrad;
    int16_t offs;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    offs    = static_cast<int16_t>(x * S16_MAX_F);
    memcpy((void *)&msg.data[offset], &offs, sizeof(int16_t));
    offset += sizeof(int16_t);
    offs    = static_cast<int16_t>(y * S16_MAX_F);
    memcpy((void *)&msg.data[offset], &offs, sizeof(int16_t));
    offset += sizeof(int16_t);
    mrad    = static_cast<int32_t>(t_lat * 1000000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(t_lon * 1000000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
    offset += sizeof(int32_t);
    mrad    = static_cast<int32_t>(t_alt * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int32_t));
}


inline void pack_cam_sensor_parameters(message &msg, uint8_t ae, uint8_t target_brightness, uint32_t exposure_value, uint32_t gain_value) {
    msg.param_type = CAM_SENSOR;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &ae, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &target_brightness, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &exposure_value, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy((void *)&msg.data[offset], &gain_value, sizeof(uint32_t));
}

/* MESSAGE PACKING
    For each parameter and info there is one set_parameter_type
    Only one function for get parameters without arguments is needed.
*/
inline void pack_get_parameters(message &msg, uint8_t param_type, uint8_t cam_index = 255) {
    msg.version      = VERSION;
    msg.message_type = GET_PARAMETERS;
    msg.param_type   = param_type;
    if (cam_index != 255)
        msg.data[0] = cam_index;
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

inline void pack_get_cam_offset_parameters(message &msg, uint8_t cam, float x, float y, uint8_t frame_rel) {
    pack_get_parameters(msg, CAM_OFFSET);
    pack_cam_offset_parameters(msg, cam, x, y, frame_rel);
}

inline void pack_get_cam_target_parameters(message &msg, uint8_t cam, float x, float y) {
    pack_get_parameters(msg, CAM_TARGET);
    pack_cam_target_parameters(msg, cam, x, y, 0.0f, 0.0f, 0.0f);
}

inline void pack_set_general_settings_parameters(
    message &msg, uint16_t camera_width, uint16_t camera_height, uint16_t roi_width, uint16_t roi_height, uint8_t camera_fps,
    float mount_yaw, float mount_pitch, float mount_roll, uint8_t run_ai) {

    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_general_settings_parameters(
        msg, camera_width, camera_height, roi_width, roi_height, camera_fps, mount_yaw, mount_pitch, mount_roll, run_ai);
}

inline void pack_set_detection_parameters(
    message &msg, uint8_t mode, uint8_t sorting_mode, float crop_confidence_threshold, float var_confidence_threshold,
    uint16_t crop_box_limit, uint16_t var_box_limit, float crop_box_overlap, float var_box_overlap, 
    uint8_t creation_score_scale, uint8_t bonus_detection_scale,
    uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty, uint8_t missed_redetection_penalty) {

    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_detection_parameters(
        msg, mode, sorting_mode, crop_confidence_threshold, var_confidence_threshold, crop_box_limit, var_box_limit,
        crop_box_overlap, var_box_overlap, creation_score_scale, bonus_detection_scale,
        bonus_redetection_scale, missed_detection_penalty, missed_redetection_penalty);
}

inline void pack_set_video_output_parameters(
    message &msg, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode, uint8_t detection_overlay_mode) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_video_output_parameters(msg, width, height, fps, layout_mode, detection_overlay_mode);
}

inline void pack_set_capture_parameters(message &msg, bool pic, bool vid) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_capture_parameters(msg, pic, vid);
}

inline void pack_set_lens_parameters(message &msg, uint8_t lens_id) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_lens_parameters(msg, lens_id);
}

inline void pack_set_cam_euler_parameters(message &msg, uint8_t cam, uint8_t is_delta, float yaw, float pitch, float roll) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_euler_parameters(msg, cam, is_delta, yaw, pitch, roll);
}

inline void pack_set_cam_zoom_parameters(message &msg, uint8_t cam, int8_t zoom) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_zoom_parameters(msg, cam, zoom);
}

inline void pack_set_cam_lock_flags_parameters(message &msg, uint8_t cam, uint8_t flags) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_lock_flags_parameters(msg, cam, flags);
}

inline void pack_set_cam_control_mode_parameters(message &msg, uint8_t cam, uint8_t mode) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_control_mode_parameters(msg, cam, mode);
}

inline void pack_set_cam_crop_mode_parameters(message &msg, uint8_t cam, uint8_t mode) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_crop_mode_parameters(msg, cam, mode);
}

inline void pack_set_cam_fov_parameters(message &msg, uint8_t cam, float fov) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_fov_parameters(msg, cam, fov);
}

inline void pack_set_cam_target_parameters(message &msg, uint8_t cam, float x, float y, float t_lat, float t_lon, float t_alt) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_target_parameters(msg, cam, x, y, t_lat, t_lon, t_alt);
}

inline void pack_set_cam_sensor_parameters(message &msg, uint8_t ae, uint8_t target_brightness, uint32_t exposure_value, uint32_t gain_value) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_sensor_parameters(msg, ae, target_brightness, exposure_value, gain_value);
}


/* PARAMETER UNPACKING
 */
inline void unpack_system_status_parameters(message &raw_msg, system_status_parameters &params) {
    memcpy((void *)&params.status, (void *)&raw_msg.data[0], sizeof(uint8_t));
    memcpy((void *)&params.error, (void *)&raw_msg.data[1], sizeof(uint8_t));
}

inline void unpack_general_settings_parameters(message &raw_msg, general_settings_parameters &params) {
    uint8_t offset = 0;
    int32_t mrad;
    memcpy((void *)&params.camera_width, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&params.camera_height, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&params.roi_width, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&params.roi_height, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy((void *)&params.camera_fps, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad = 0;
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.mount_yaw = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    mrad = 0;
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.mount_pitch = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    mrad = 0;
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.mount_roll = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy((void *)&params.run_ai, (void *)&raw_msg.data[offset], sizeof(uint8_t));
}

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
    memcpy((void *)&params.detection_overlay_mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset            += sizeof(uint8_t);
    uint8_t num_views  = (params.layout_mode & 0xf0) >> 4;
    for (uint8_t i = 0; i < num_views; i++) {
        memcpy((void *)&params.views[i].x, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy((void *)&params.views[i].y, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy((void *)&params.views[i].w, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy((void *)&params.views[i].h, (void *)&raw_msg.data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
    }
    memcpy((void *)&params.detection_overlay_box, (void *)&raw_msg.data[offset], sizeof(bounding_box));
    offset += sizeof(bounding_box);
    memcpy((void *)&params.single_detection_size, (void *)&raw_msg.data[offset], sizeof(uint16_t));
}

inline void unpack_capture_parameters(message &raw_msg, capture_parameters &params) {
    uint8_t offset    = 0;
    uint8_t cap_flags = 0;
    memcpy(&cap_flags, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.images_captured, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint16_t);
    memcpy(&params.videos_captured, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    params.cap_single_image = cap_flags & CAP_FLAG_SINGLE_IMAGE;
    params.record_video     = cap_flags & CAP_FLAG_VIDEO;
}

inline void unpack_detection_parameters(message &raw_msg, detection_parameters &params) {
    uint8_t offset = 0;
    uint8_t crop_conf_thresh;
    uint8_t var_conf_thresh;
    uint8_t crop_box_ovlp;
    uint8_t var_box_ovlp;
    memcpy(&params.mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.sorting_mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&crop_conf_thresh, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    params.crop_confidence_threshold  = static_cast<float>(crop_conf_thresh) / 255.0f;
    offset += sizeof(uint8_t);
    memcpy(&var_conf_thresh, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    params.var_confidence_threshold  = static_cast<float>(var_conf_thresh) / 255.0f;
    offset += sizeof(uint8_t);
    memcpy(&params.crop_box_limit, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(&params.var_box_limit, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(&crop_box_ovlp, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    params.crop_box_overlap  = static_cast<float>(crop_box_ovlp) / 255.0f;
    offset += sizeof(uint8_t);
    memcpy(&var_box_ovlp, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    params.var_box_overlap  = static_cast<float>(var_box_ovlp) / 255.0f;
    offset += sizeof(uint8_t);
    memcpy(&params.creation_score_scale, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.bonus_detection_scale, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.bonus_redetection_scale, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.missed_detection_penalty, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.missed_redetection_penalty, (void *)&raw_msg.data[offset], sizeof(uint8_t));
}

inline void unpack_detected_roi_parameters(message &raw_msg, detected_roi_parameters &params) {
    uint8_t offset = 0;
    int32_t mrad;
    memcpy(&params.index, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.score, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&params.total_detections, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.yaw_abs  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.pitch_abs  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.yaw_rel  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.pitch_rel  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.latitude  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.longitude  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.altitude  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(float);
    memcpy(&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.distance = static_cast<float>(mrad) / 1000.0f;
}

inline void unpack_lens_parameters(message &raw_msg, lens_parameters &params) {
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
    params.yaw  = static_cast<float>(mrad) / 1000.0f;
    offset     += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.pitch  = static_cast<float>(mrad) / 1000.0f;
    offset       += sizeof(int32_t);
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
    offset       += sizeof(int32_t);
    params.x      = static_cast<float>(x) / S16_MAX_F;
    params.y      = static_cast<float>(y) / S16_MAX_F;
    params.yaw    = static_cast<float>(yaw) / 1000.0f;
    params.pitch  = static_cast<float>(pitch) / 1000.0f;
}

inline void unpack_cam_fov_parameters(message &raw_msg, cam_fov_parameters &params) {
    int16_t mrad;
    uint8_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    params.fov = static_cast<float>(mrad) / 1000.0f;
}

inline void unpack_cam_target_parameters(message &raw_msg, cam_target_parameters &params) {
    int32_t mrad;
    int32_t mm;
    int16_t offs;
    uint8_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&offs, (void *)&raw_msg.data[offset], sizeof(int16_t));
    params.x = static_cast<float>(offs) / S16_MAX_F;
    offset += sizeof(int16_t);
    memcpy((void *)&offs, (void *)&raw_msg.data[offset], sizeof(int16_t));
    params.y = static_cast<float>(offs) / S16_MAX_F;
    offset += sizeof(int16_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.t_latitude  = static_cast<float>(mrad) / 1000000.0f;
    offset            += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.t_longitude  = static_cast<float>(mrad) / 1000000.0f;
    offset             += sizeof(int32_t);
    memcpy((void *)&mm, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.t_altitude = static_cast<float>(mm) / 1000.0f;
}


inline void unpack_cam_sensor_parameters(message &raw_msg, cam_sensor_parameters &params) {
    uint8_t offset = 0;
    memcpy((void *)&params.ae, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.target_brightness, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.exposure_value, (void *)&raw_msg.data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy((void *)&params.gain_value, (void *)&raw_msg.data[offset], sizeof(uint32_t));
}

#endif // MSG_DEFS_HPP