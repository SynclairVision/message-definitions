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

static constexpr uint32_t STREAM_NAME_SIZE      = 8;

/*
------------------------------------------------------------------------------------------------------------------------
    TYPES

    There is one enumeration for each parameter type and one for each message type.
------------------------------------------------------------------------------------------------------------------------
*/

enum PARAM_TYPE : uint8_t {
    SYSTEM_STATUS,
    GENERAL_SETTINGS,
    MODEL,
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
    CAM_SENSOR,
    CAM_DEPTH_ESTIMATION,
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


/*
------------------------------------------------------------------------------------------------------------------------
    STRUCTS

    One struct for the base message, and one for each parameter type. There is also a convenience struct for bounding
    boxes.
------------------------------------------------------------------------------------------------------------------------
*/
struct message {
    uint64_t timestamp;
    uint8_t  version;
    uint8_t  message_type;
    uint8_t  param_type;
    uint8_t  data[PARAMCOUNT];
    uint8_t checksum;
};

struct bounding_box {
    uint16_t x, y, w, h;
};

// Add comment for each parameter?
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
    uint8_t  run_ai;
    char     crop_model_name[16];
    char     var_model_name[16];
};

struct model_parameters {
    char model_name[16];
};

struct video_output_parameters {
    char         stream_name[STREAM_NAME_SIZE];
    uint16_t     width;
    uint16_t     height;
    uint8_t      fps;
    uint8_t      layout_mode;
    uint8_t      detection_overlay_mode;
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
    float   x;
    float   y;
    float   yaw_abs;
    float   pitch_abs;
    float   yaw_rel;
    float   pitch_rel;
};

struct cam_fov_parameters {
    uint8_t cam_id;
    float   fov;
};

struct cam_target_parameters {
    uint8_t cam_id;
    float   x;
    float   y;
    float   t_latitude;
    float   t_longitude;
    float   t_altitude;
};

struct cam_sensor_parameters {
    uint8_t ae; // automatic exposure
    uint8_t ag; // automatic gain
    uint8_t target_brightness;
    uint32_t exposure_value;
    uint32_t gain_value;
};

struct cam_depth_estimation_parameters {
    uint8_t cam_id;
    uint8_t depth_estimation_mode;
    float depth;
};

/*
------------------------------------------------------------------------------------------------------------------------
    SERIALIZATION

    Used when sending/receiving messages over the network.
------------------------------------------------------------------------------------------------------------------------
*/
inline char *serialize_message(const message &msg) {
    char *buffer = new char[sizeof(msg)];
    memcpy(buffer, &msg, sizeof(msg));
    return buffer;
}

inline message deserialize_message(char *buffer) {
    message msg;
    memcpy(&msg, buffer, sizeof(msg));
    return msg;
}

/*
------------------------------------------------------------------------------------------------------------------------
    PACKING FUNCTIONS
    
    For each parameter type there is one pack function.
------------------------------------------------------------------------------------------------------------------------
*/
inline void pack_system_status_parameters(message &msg, uint8_t status, uint8_t error) {
    msg.param_type = SYSTEM_STATUS;
    msg.data[0]    = status;
    msg.data[1]    = error;
}

inline void pack_general_settings_parameters(
    message &msg, uint16_t camera_width, uint16_t camera_height, uint16_t roi_width, uint16_t roi_height, uint8_t camera_fps,
    uint8_t run_ai, const char *crop_model_name, const char *var_model_name) {

    msg.param_type = GENERAL_SETTINGS;
    uint8_t offset = 0;
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
    memcpy((void *)&msg.data[offset], &run_ai, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], crop_model_name, 16);
    offset += 16;
    memcpy((void *)&msg.data[offset], var_model_name, 16);
}

inline void pack_model_parameters(message &msg, const char *model_name) {
    msg.param_type = MODEL;
    memcpy((void *)&msg.data[0], model_name, 16);
}

inline void pack_video_output_parameters(
    message &msg, char *stream_name, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode, uint8_t detection_overlay_mode,
    bounding_box *views = nullptr, bounding_box detection_overlay_box = {}, uint16_t single_detection_size = 0) {

    msg.param_type = VIDEO_OUTPUT;
    uint16_t offset = 0;
    memcpy((void *)&msg.data[offset], stream_name, STREAM_NAME_SIZE);
    offset += STREAM_NAME_SIZE;
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
        if (num_views > 4) num_views = 4;
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
    uint16_t offset     = 0;
    uint8_t cap_flags  = 0x0;
    cap_flags         |= static_cast<uint8_t>(pic ? CAP_FLAG_SINGLE_IMAGE : 0);
    cap_flags         |= static_cast<uint8_t>(vid ? CAP_FLAG_VIDEO : 0);
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
    uint16_t offset           = 0;
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
    uint16_t offset = 0;
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
    uint16_t offset = 0;
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
    message &msg, uint8_t cam, float x, float y, float yaw_abs = 0, float pitch_abs = 0, float yaw_rel = 0, float pitch_rel = 0) {
    msg.param_type = CAM_OFFSET;
    uint16_t offset = 0;
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
}

inline void pack_cam_fov_parameters(message &msg, uint8_t cam, float fov) {
    msg.param_type = CAM_FOV;
    int16_t mrad;
    uint16_t offset = 0;
    memcpy((void *)&msg.data[offset], &cam, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mrad    = static_cast<int16_t>(fov * 1000.0f);
    memcpy((void *)&msg.data[offset], &mrad, sizeof(int16_t));
}

inline void pack_cam_target_parameters(message &msg, uint8_t cam, float x, float y, float t_lat, float t_lon, float t_alt) {
    msg.param_type = CAM_TARGET;
    uint16_t offset = 0;
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


inline void pack_cam_sensor_parameters(message &msg, uint8_t ae, uint8_t ag, uint8_t target_brightness, uint32_t exposure_value, uint32_t gain_value) {
    msg.param_type = CAM_SENSOR;
    uint8_t offset = 0;
    memcpy((void *)&msg.data[offset], &ae, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &ag, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &target_brightness, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &exposure_value, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy((void *)&msg.data[offset], &gain_value, sizeof(uint32_t));
}

inline void pack_cam_depth_estimation_parameters(message &msg, uint8_t cam_id, uint8_t depth_estimation_mode, float depth) {
    msg.param_type = CAM_DEPTH_ESTIMATION;
    uint8_t offset = 0;
    int32_t mm;
    memcpy((void *)&msg.data[offset], &cam_id, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&msg.data[offset], &depth_estimation_mode, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mm = static_cast<int32_t>(depth * 1000.0f);
    memcpy((void *)&msg.data[offset], &mm, sizeof(int32_t));
}

/*
------------------------------------------------------------------------------------------------------------------------
    GET PACKING FUNCTIONS
------------------------------------------------------------------------------------------------------------------------
*/

/*
    Generic function for getting parameters. Specify the parameter type and in some cases the camera index.
*/
inline void pack_get_parameters(message &msg, uint8_t param_type, uint8_t cam_index = 255) {
    msg.version      = VERSION;
    msg.message_type = GET_PARAMETERS;
    msg.param_type   = param_type;
    if (cam_index != 255)
        msg.data[0] = cam_index;
}

/*
    Getter for video output, specifies the stream name.
*/
inline void pack_get_video_output_parameters(message &msg, char *stream_name) {
    pack_get_parameters(msg, VIDEO_OUTPUT);
    memcpy((void *)&msg.data[0], stream_name, STREAM_NAME_SIZE);
}

/*
    Convenience function for DETECTED_ROI. Specify the index of the detection to get.
*/
inline void pack_get_detected_roi(message &msg, uint8_t index) {
    pack_get_parameters(msg, DETECTED_ROI);
    msg.data[0] = index;
}

/*
    Convenience function for DETECTED_ROI. Get all detections that are visible on screen.
*/
inline void pack_get_detected_roi_visible(message &msg) {
    pack_get_parameters(msg, DETECTED_ROI);
    msg.data[0] = 254;
}

/*
    Convenience function for DETECTED_ROI. Get all detections.
*/
inline void pack_get_detected_roi_all(message &msg) {
    pack_get_parameters(msg, DETECTED_ROI);
    msg.data[0] = 255;
}

/*
    Convenience function for CAM_OFFSET. Specify the camera index and offset from center.
*/
inline void pack_get_cam_offset_parameters(message &msg, uint8_t cam, float x, float y) {
    pack_get_parameters(msg, CAM_OFFSET);
    pack_cam_offset_parameters(msg, cam, x, y);
}

/*
    Convenience function for CAM_TARGET. Specify the camera index and offset from center.
*/
inline void pack_get_cam_target_parameters(message &msg, uint8_t cam, float x, float y) {
    pack_get_parameters(msg, CAM_TARGET);
    pack_cam_target_parameters(msg, cam, x, y, 0.0f, 0.0f, 0.0f);
}

/*
------------------------------------------------------------------------------------------------------------------------
    SET PACKING FUNCTIONS

    For each parameter type there is one pack function.
------------------------------------------------------------------------------------------------------------------------
*/
inline void pack_set_general_settings_parameters(
    message &msg, uint16_t camera_width, uint16_t camera_height, uint16_t roi_width, uint16_t roi_height, uint8_t camera_fps,
    uint8_t run_ai, const char *crop_model_name, const char *var_model_name) {

    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_general_settings_parameters(
        msg, camera_width, camera_height, roi_width, roi_height, camera_fps, run_ai,
        crop_model_name, var_model_name);
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
    message &msg, char *stream_name, uint16_t width, uint16_t height, uint8_t fps, uint8_t layout_mode, uint8_t detection_overlay_mode) {
    msg.version      = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_video_output_parameters(msg, stream_name, width, height, fps, layout_mode, detection_overlay_mode);
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

inline void pack_set_cam_sensor_parameters(message &msg, uint8_t ae, uint8_t ag, uint8_t target_brightness, uint32_t exposure_value, uint32_t gain_value) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_sensor_parameters(msg, ae, ag, target_brightness, exposure_value, gain_value);
}

inline void pack_set_cam_depth_estimation_parameters(message &msg, uint8_t cam_id, uint8_t depth_estimation_mode) {
    msg.version = VERSION;
    msg.message_type = SET_PARAMETERS;
    pack_cam_depth_estimation_parameters(msg, cam_id, depth_estimation_mode, 0.0f);
}


/*
------------------------------------------------------------------------------------------------------------------------
    PARAMETER UNPACKING

    For each parameter type there is one unpack function.
------------------------------------------------------------------------------------------------------------------------
*/
inline void unpack_system_status_parameters(message &raw_msg, system_status_parameters &params) {
    memcpy((void *)&params.status, (void *)&raw_msg.data[0], sizeof(uint8_t));
    memcpy((void *)&params.error, (void *)&raw_msg.data[1], sizeof(uint8_t));
}

inline void unpack_general_settings_parameters(message &raw_msg, general_settings_parameters &params) {
    uint8_t offset = 0;
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
    memcpy((void *)&params.run_ai, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.crop_model_name, (void *)&raw_msg.data[offset], 16);
    offset += 16;
    memcpy((void *)&params.var_model_name, (void *)&raw_msg.data[offset], 16);
}

inline void unpack_model_parameters(message &raw_msg, model_parameters &params) {
    memcpy((void *)&params.model_name, (void *)&raw_msg.data[0], 16);
}

inline void unpack_video_output_parameters(message &raw_msg, video_output_parameters &params) {
    uint16_t offset = 0;
    memcpy((void *)&params.stream_name, (void *)&raw_msg.data[offset], STREAM_NAME_SIZE);
    offset += STREAM_NAME_SIZE;
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
    if (num_views > 4) num_views = 4;
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
    uint16_t offset    = 0;
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
    uint16_t offset = 0;
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
    uint16_t offset = 0;
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
    uint16_t offset = 0;
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
    int32_t mrad;
    uint16_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[0], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&x, (void *)&raw_msg.data[offset], sizeof(int16_t));
    offset += sizeof(int16_t);
    memcpy((void *)&y, (void *)&raw_msg.data[offset], sizeof(int16_t));
    offset += sizeof(int16_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.yaw_abs    = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.pitch_abs  = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.yaw_rel    = static_cast<float>(mrad) / 1000.0f;
    offset += sizeof(int32_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.pitch_rel  = static_cast<float>(mrad) / 1000.0f;
    params.x      = static_cast<float>(x) / S16_MAX_F;
    params.y      = static_cast<float>(y) / S16_MAX_F;
}

inline void unpack_cam_fov_parameters(message &raw_msg, cam_fov_parameters &params) {
    int16_t mrad;
    uint16_t offset = 0;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&mrad, (void *)&raw_msg.data[offset], sizeof(uint16_t));
    params.fov = static_cast<float>(mrad) / 1000.0f;
}

inline void unpack_cam_target_parameters(message &raw_msg, cam_target_parameters &params) {
    int32_t mrad;
    int32_t mm;
    uint16_t offset = 0;
    int16_t offs;
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
    memcpy((void *)&params.ag, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.target_brightness, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.exposure_value, (void *)&raw_msg.data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy((void *)&params.gain_value, (void *)&raw_msg.data[offset], sizeof(uint32_t));
}

inline void unpack_cam_depth_estimation_parameters(message &raw_msg, cam_depth_estimation_parameters &params) {
    uint8_t offset = 0;
    int32_t mm;
    memcpy((void *)&params.cam_id, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&params.depth_estimation_mode, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy((void *)&mm, (void *)&raw_msg.data[offset], sizeof(int32_t));
    params.depth = static_cast<float>(mm) / 1000.0f;
}

// CHECK_SUM stuff

enum CRC8TYPE{
    AUTOSAR,
    BLUETOOTH,
    CDMA2000,
    DARC,
    DVB_S2,
    GSM_A,
    GSM_B,
    HITAG,
    I_432_1,
    I_CODE,
    LTE,
    MAXIN_DOW,
    MIFARE_MAD,
    NRSC_5,
    OPENSAFETY,
    ROHC,
    SAE_J1850,
    SMBUS,
    TECH_3250,
    WCDMA
};

struct crc8 {
    crc8() : polynomial(0x07), init_val(0x00), final_xor(0x00), reflect_in(false), reflect_out(false) {}
    crc8(uint8_t key, uint8_t initial_value = 0x00, uint8_t final_xor_value = 0x00, bool b_reflection_in = false, bool b_reflection_out = false) : 
                                        polynomial(key), init_val(initial_value), final_xor(final_xor_value), reflect_in(b_reflection_in), reflect_out(b_reflection_out){}
    crc8(int preset){
        switch (preset)
        {
            case CRC8TYPE::AUTOSAR:
            {
                this->polynomial = 0x2F;
                this->init_val = 0xFF;
                this->final_xor = 0xFF;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::BLUETOOTH:
            {
                this->polynomial = 0xA7;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = true;
                this->reflect_out = true;
                break;
            }
            case CRC8TYPE::CDMA2000:
            {
                this->polynomial = 0x9B;
                this->init_val = 0xFF;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::DARC:
            {
                this->polynomial = 0x39;
                this->init_val = 0xFF;
                this->final_xor = 0x00;
                this->reflect_in = true;
                this->reflect_out = true;            
                break;
            }
            case CRC8TYPE::DVB_S2:
            {
                this->polynomial = 0xD5;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::GSM_A:
            {
                this->polynomial = 0x1D;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::GSM_B:
            {
                this->polynomial = 0x49;
                this->init_val = 0x00;
                this->final_xor = 0xFF;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::HITAG:
            {
                this->polynomial = 0x1D;
                this->init_val = 0xFF;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::I_432_1:
            {
                this->polynomial = 0x07;
                this->init_val = 0x00;
                this->final_xor = 0x55;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::I_CODE:
            {
                this->polynomial = 0x1D;
                this->init_val = 0xFD;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;    
                break;
            }
            case CRC8TYPE::LTE:
            {
                this->polynomial = 0x9B;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::MAXIN_DOW:
            {
                this->polynomial = 0x31;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = true;
                this->reflect_out = true;
                break;
            }
            case CRC8TYPE::MIFARE_MAD:
            {
                this->polynomial = 0x1D;
                this->init_val = 0xC7;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::NRSC_5:
            {
                this->polynomial = 0x31;
                this->init_val = 0xFF;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::OPENSAFETY:
            {
                this->polynomial = 0X2F;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::ROHC:
            {
                this->polynomial = 0x07;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = true;
                this->reflect_out = true;
                break;
            }
            case CRC8TYPE::SAE_J1850:
            {
                this->polynomial = 0x1D;
                this->init_val = 0xFF;
                this->final_xor = 0xFF;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::SMBUS:
            {
                this->polynomial = 0x07;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = false;
                this->reflect_out = false;
                break;
            }
            case CRC8TYPE::TECH_3250:
            {
                this->polynomial = 0x1D;
                this->init_val = 0xFF;
                this->final_xor = 0x00;
                this->reflect_in = true;
                this->reflect_out = true;
                break;
            }
            case CRC8TYPE::WCDMA:
            {
                this->polynomial = 0x9B;
                this->init_val = 0x00;
                this->final_xor = 0x00;
                this->reflect_in = true;
                this->reflect_out = true;
                break;
            }
        }
    }
    uint8_t reflect(uint8_t data) {
        uint8_t reflection = 0x00;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (data & (1 << bit)) {
                reflection |= (1 << (7 - bit));
            }
        }
        return reflection;
    }

    uint8_t crc(uint8_t* data, uint32_t n_bytes) {
        uint8_t crc_ret = init_val;

        for (uint32_t byte = 0; byte < n_bytes; ++byte) {
            uint8_t current_byte = reflect_in ? reflect(data[byte]) : data[byte];
            crc_ret ^= current_byte;
            for (uint8_t bit = 0; bit < 8; ++bit) {
                if (crc_ret & 0x80) {
                    crc_ret = (crc_ret << 1) ^ polynomial;
                } else {
                    crc_ret <<= 1;
                }
            }
        }
        crc_ret = reflect_out ? reflect(crc_ret) : crc_ret;
        return crc_ret ^ final_xor;
    }
    uint8_t final_xor = 0x00;
    uint8_t init_val = 0x00;
    uint8_t filler = 0x00;
    uint8_t polynomial = 0x07;
    bool reflect_in = false;
    bool reflect_out = false;
};
#endif // MSG_DEFS_HPP
