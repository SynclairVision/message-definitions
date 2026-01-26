# Introduction

This page contains documentation on the communication with Synclair Vision's software DigiView.

## General workflow

The file msg_defs.hpp contains all necessary helper functions to pack messages that can then be sent over a network connection. It is recommended to have msg_defs.hpp open while reading this documentation.

# Base message structure

|   **Name**   | **data type** | **Description** |
|:------------:|:-------------:|:---------------:|
|  timestamp   |   uint64_t    | Time of message received, must be set manually |
|   version    |    uint8_t    | Version of msg_defs.hpp |
| message_type | enum: uint8_t | Message type, see below |
|  param_type  | enum: uint8_t | Parameter type, see below |
|     data     |  \[uint8_t\]  | Arguments or data for specific parameter type |
| checksum     |  \[uint8_t\]  | Checksum of the message |

## Message types

This table contains the possible values in the message_type field of a
message. The values are provided in the table, but are also available in
the MESSAGE_TYPE enum in the msg_defs.hpp file with the corresponding
name for convenience.

| **Name** | **Value** | **Description** |
|:--------------------|:-----|:-----------------------------------------|
| EMPTY | 0 | Indicates that the message is empty |
| GET_PARAMETERS | 1 | Indicates that the message is a data request |
| SET_PARAMETERS | 2 | Indicates that the message has data for DigiView to set |
| CURRENT_PARAMETERS | 3 | Response flag set by DigiView indicating that the message contains current parameters used by the camera |
| GET_INTERVAL | 4 | Indicates that the message is a request to set up periodic sending of a specific parameter type |
| ACKNOWLEDGEMENT | 5 | Indicates a succesful SET request |
| CHECKSUM_ERROR | 6 | The check sum of the previous message did not match the check sum that was sent |
| DATA_ERROR | 7 | DigiView could not interpret the data received |
| FORBIDDEN | 8 | The sender does not have permission to perform the requested action(s) |
| UNKNOWN | 9 | The parameter or message type supplied is unknown to DigiView |
| QUIT | 255 | Instructs DigiView to close the connection |

## Parameter types

This table contains the possible parameter types in the parameter_type
field. The values are provided in the table, but are also available in
the PARAM_TYPE enum in the msg_defs.hpp file with the corresponding
name. For more information about the exact nature, behavior and
available data fields, check the corresponding message's section under [Messages](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#messages).

An asterisk means a specific operation is not yet implemented.

| **Name** | **Value** | **Message type** | **Description** |
|:-------------------|:-----|:-----------|:---------------------------|
| SYSTEM_STATUS | 0 | GET | Returns whether the system is currently running |
| AI | 1 | GET & SET | AI processing parameters |
| MODEL | 2 | GET | Model information |
| VIDEO_OUTPUT | 3 | GET & SET | Information regarding the video output |
| CAPTURE | 4 | GET & SET | Message for controlling system recording |
| DETECTION | 5 | GET & SET | Message containing AI post processing parameters |
| DETECTED_ROI | 6 | GET | Retrieve information of specific detections |
| CAM_TARGETING | 7 | GET & SET | Handles user-controlled cameras targeting including euler angles and coordinates |
| CAM_OPTICS_AND_CONTROL | 8 | GET & SET | Handles user-controlled cameras' zoom, FOV, and crop mode |
| CAM_OFFSET | 9 | GET | Retrieve directional info relative to the center of the picture |
| SENSOR | 10 | GET & SET | Control for the image sensor's settings |
| CAM_DEPTH_ESTIMATION | 11 | GET & SET | Control for the depth estimation unit |
| SINGLE_TARGET_TRACKING | 12 | GET & SET | Single target tracking parameters |
|  |  |  |  |

# Messages

This section will describe each message listed under [Parameter types](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#parameter-types) in detail. Each subsection is for a separate message where all data-fields and, if any exists, any unexpected behaviors are explained. Each subsection is named after the message's corresponding parameter name.

## Common behavior

### stream_name

In messages where a stream_name field is present, the field is used to
specify which video stream the message is referring to. The stream_name is a
string of max length 8 characters. If the stream_name does not match an existing
stream, the operation is ignored. Each stream carries its own set of
parameters. So, if you are watching the output of stream "stream" and you wish to know the size of that stream, you must send a GET VIDEO_OUTPUT request with stream_name set to "stream".

### cam_id

In messages where a cam_id field is present, the field is used to
specify which user controllable camera the message is referring to. It is usually paired with a stream_name field to identify the camera uniquely. The cam_id is a uint8_t and valid values are
\[0, num_user_views - 1\] where num_user_views is the number of user controllable cameras
which can be read from the VIDEO_OUTPUT message. So, if you are watching the output of stream "stream" and you wish to know the zoom level of the top left camera on that stream, you must send a GET CAM_OPTICS_AND_CONTROL request with stream_name set to "stream" and cam_id set to 0.

### GET_INTERVAL

The GET_INTERVAL message can be used to set up periodic sending of a specific parameter type. The interval is specified in milliseconds and the minimum allowed interval is 100 ms. If a GET_INTERVAL message is sent with an interval lower than 100 ms, the request is ignored. If a GET_INTERVAL message is sent with an interval of 0 ms, any previously set periodic sending of that parameter type is cancelled.

When a GET_INTERVAL message is sent, DigiView will reply with the requested parameter type at the specified interval until a new GET_INTERVAL message is sent with the same parameter type and an interval of 0 ms.

Currently work in progress.

## SYSTEM_STATUS

Message for retrieving the current system status.

### Data field

The system status messages contains the following fields:

| **Field name** | **Datatype** | **Valid arguments** |
|:--------------:|:------------:|:-------------------:|
|     status     |   uint8_t    |       \[0,3\]       |
|     error      |   uint8_t    |          0          |
|  jetson_temp   |    float     |      \[0,105\]      |

### Behavior

The returned values indicate the systems current operational status
indicated by the value in the *status* field. The possibilities are
listed below.

| **Value** |         **Meaning**         |
|:---------:|:----------------------------|
|     0     | System is idle              |
|     1     | System is running           |
|     2     | System finished execution   |
|     3     | System encountered an error |

The error field is currently unused and will always return 0.

## AI

Message for high-level control of the AI processing.

### Data field

The AI message contains the following fields:
| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
| ai_enabled | bool | true, false | true, false |
| track_model_name | char[16] | n/a | n/a |
| scan_model_name | char[16] | n/a | n/a |

### Set behavior
The ai_enabled field can be set to true or false to enable or disable
AI processing. If the field is set to true, but no models are
specified, the system will not be able to perform any AI processing.

Scan model is the model used for initial detection or "scanning" of objects, while scan model is the model used for verification or "tracking" of objects once they have been detected.

The names of the models must match exactly with the names returned by the MODEL message.

### Get behavior
The ai_enabled field will return whether AI processing is currently enabled or disabled. The track_model_name and scan_model_name fields will return the names of the currently set models.

## MODEL

Message carrying information about available models.

### Data field

The model message contains the following field:

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
| model_name | char[16] | n/a | n/a |

### Set behavior

Ignored.

### Get behavior

The model message will return one message for each model available. The model name
is a string containing the name of the model. This can then be used in the AI message to set the models used for detection and verification.

## VIDEO_OUTPUT

Message for controlling the video output resolution and layout.

### Data field

The video output message has two separate versions, one for get
operations and one for set operations.

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--:|:--:|:--:|:--:|
| stream_name | char[8] | see below | see below |
| width | uint16_t | \[640,1920\] | \[640,1920\] |
| height | uint16_t | \[480,1080\] | \[480,1080\] |
| fps* | uint8_t | ignored | ignored |
| layout_mode | uint8_t | \[0,6\] |  \[1,4\](4 MSB), \[0,6\](4 LSB) |
| detection_overlay_mode | uint8_t | \[0,5\] | \[0,5\] |
| views | \[bounding_box\] | n/a | See table below |
| detection_overlay_box | bounding_box | n/a | See table below |
| single_detection_size | uint16_t | n/a | See table below |

Some of the fields use the non-fundamental data type *bounding_box*
which is a struct defined in the header file used to hold information
regarding the different cameras. Its fields are shown below.

| **Name** | **Data type** | **Valid values** |
|:--------:|:-------------:|:----------------:|
|    x     |   uint16_t    |   \[0,65535\]    |
|    y     |   uint16_t    |   \[0,65535\]    |
|    w     |   uint16_t    |   \[0,65535\]    |
|    h     |   uint16_t    |   \[0,65535\]    |

### Set behavior

##### stream_name

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### width and height

If a width \> 1920 or height \> 1080 is set the arguments are clamped to
1920 and 1080 respectively. If they are set lower than 640 or 480 for
width and height respectively, the operation is ignored.

##### fps

Unused

##### layout

The layout mode controls which type of user-controllable camera layout
is used. If the argument is larger than the max overlay mode (currently
6) no change is made. The currently available modes are listed below.

| **layout_mode value** | **User camera layout** |
|:--:|:---|
| 0 | 1 camera |
| 1 | 2 cameras stacked on top of each other |
| 2 | 2 cameras side by side |
| 3 | 3 cameras, 2 on top, 1 below|
| 4 | 4 cameras in a 2x2 grid |
| 5 | 4 cameras, 3 on top, 1 below |
| 6 | debug view, shows source frame from sensor |

##### detection overlay mode

The detection overlay mode controls the layout of the detection cameras
that are fixed on objects detected by the system. If the argument is
larger than the max detection overlay mode (currently 5) no change is
made. The currently available modes are listed below.

| **detection_overlay_mode value** | **Detection camera layout** |
|:--:|:---|
| 0 | No overlay |
| 1 | 1 detection is shown in the top right, covering a small part of the user-controllable camera as a picture-in-picture. |
| 2 | Shows a column of detections to the right, does not cover user-controllable cameras. Number of detections shown depend on the output resolution. |
| 3 | Shows a column of detections to the left. |
| 4 | Shows a row of detections on the top of the frame, does not cover user-controllable cameras. Number of detections shown depend on the output resolution. |
| 5 | Shows a row of detections on the bottom of the frame. |

Each detection camera is square, and the side length in pixels can be
found in the single_detection_size field. The number of detection cameras
shown can be calculated using the following formula:

$$n\_{detections} = \lfloor \frac{\mathrm{limiting \quad pixel \quad length}}{\mathrm{single\_detection\_size}} \rfloor$$

The limiting pixel length is the output resolution width (height) for
row (column) layouts.

### Get behavior

##### width and height

Returns the current <b>total</b> output resolution, including all user-controllable cameras and detection cameras.

##### fps

Unused.

##### layout

Contains the current layout mode, see table under [layout](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#layout).

##### detection overlay mode

Contains the current detection overlay, see table under [detection overlay mode](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#detection-overlay-mode).

##### num_user_views

Indicates the number of user-controllable camera views currently active. This value is derived from the layout mode, see [layout](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#layout-1).

##### views

Is an array of length 4 containing the bounding boxes of each of the user-controllable cameras in pixels. The positions are given by their top-left corner and their width and height. If there are less than 4 user-controllable cameras available the unused array index/indices will still contain information, albeit either undefined or outdated.

##### detection overlay box

A single bounding box for the position of the entire
detection overlay. It does NOT contain information about the position of
each individual detection cameras. The indexing for these cameras are
left to the receiver.

##### single detection size

The side length in pixels of a single detection camera. The detection
cameras are square, as such this value can be used in conjunction with
the detection overlay box to calculate a specific detection cameras
index.

## CAPTURE

The capture message contains parameters modifying the internal
handling of capturing images and video streams.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--:|:--:|:--:|:--:|
| stream_name | char[8] | see below | see below |
| cap_single_image | bool | true, false | true, false |
| record_video | bool | true, false | true, false |
| images_captured | uint16_t | n/a | n/a |
| videos_captured | uint16_t | n/a | n/a |

### Set behavior

##### stream_name

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### cap single image

If set to true, the system will capture a single image when the capture message is received. If set to false, no image will be captured.

##### record video

If set to true, the system will start recording video from the specified stream. If set to false, the system will stop recording video from the specified stream. If the system is already recording video from the specified stream when a request to start recording is received, no action is taken.

### Get behavior

The get version of the message will return with all fields containing
the current values used by the system.

## DETECTION

The detection message contains parameters modifying the internal
handling of detecting objects and how detections found by the system are
presented.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--:|:--:|:--:|:--:|
| mode | uint8_t | \[0,255\] | \[0,255\] |
| sorting_mode | uint8_t | \[0,2\] | \[0,2\] |
| track_confidence_threshold | float | \[0.0,0.99\) | \[0.0,0.99\) |
| scan_confidence_threshold | float | \[0.0,0.99\) | \[0.0,0.99\) |
| track_box_limit | uint16_t | \[10,500\] | \[10,500\] |
| scan_box_limit | uint16_t | \[10,500\] | \[10,500\] |
| track_box_overlap | float | \[0.0,0.99\) | \[0.0,0.99\) |
| scan_box_overlap | float | \[0.0,0.99\) | \[0.0,0.99\) |
| creation_score_scale | uint8_t | \[0,254\],255 | \[0,254\] |
| bonus_detection_scale | uint8_t | \[0,254\],255 | \[0,254\] |
| bonus_redetection_scale | uint8_t | \[0,254\],255 | \[0,254\] |
| missed_detection_penalty | uint8_t | \[0,254\],255 | \[0,254\] |
| missed_redetection_penalty | uint8_t | \[0,254\],255 | \[0,254\] |

### Set behavior

##### mode

Unused parameter and is ignored when received by the system.

##### sorting mode

Dictates how the detection cameras are sorted, and by extension which
detections are shown in the output. The possible sorting modes are found below. If the set sorting mode is larger than the maximum (currently 2) the sorting mode is not changed.

| **sorting_mode value** | **Sorting of detection cameras** |
|:--:|:---|
| 0 | Sort by detection score, highest to lowest. |
| 1 | Sort by detection age, highest to lowest. |
| 2 | Sort by score, highest to lowest but once a detection is assigned a detection camera it is locked to that camera for the lifetime of the object, independent of new detection with a higher score. |

##### scan confidence threshold

Sets the threshold for considering detections while tracking a detected
object. If outside the valid range, the previous value is kept.

##### scan confidence threshold

Sets the threshold for considering detections when scanning for new
objects. If outside the valid range, the previous value is kept.

##### scan box limit

Deprecated.

##### scan box limit

Deprecated.

##### scan box overlap
Sets the max allowed overlap between detection bounding boxes while
tracking an object. If outside the valid range the previous value is
kept.

##### scan box overlap

Sets the max allowed overlap between detection bounding boxes while
scanning for new object. If outside the valid range the previous value
is kept.

##### creaction score scale

Sets the initial score of new objects found when scanning. If set to 255
the previous value is kept.

##### bonus detection scale

Sets the score bonus of objects that are found again when scanning for
new objects. If set to 255 the previous value is kept.

##### bonus redetection scale

Sets the score bonus of objects that are found again when tracking the
object. If set to 255 the previous value is kept.

##### missed detection penalty

Sets the score penalty of objects that are not found when scanning for
new objects, despite being expected to be found. If set to 255 the
previous value is kept.

##### missed redetection penalty

Sets the score penalty of objects that are not found when tracking the
object. If set to 255 the previous value is kept.

### Get behavior

The get version of the message will return with all fields containing
the current values used by the system.

## DETECTED_ROI

Detected ROI contains information specific to one detection.

### Data field

|  **Field name**  | **Datatype** | **Valid SET arguments** |
|:----------------:|:------------:|:-----------------------:|
|      index       |   uint8_t    |   \[0,253\], 254, 255   |
|      score       |   uint8_t    |           n/a           |
| total_detections |   uint8_t    |           n/a           |
|     yaw_global      |    float     |           n/a           |
|    pitch_global     |    float     |           n/a           |
| rel_frame_of_reference |   uint8_t    |        0, 1, 2         |
|     yaw_rel      |    float     |           n/a           |
|    pitch_rel     |    float     |           n/a           |
|     latitude     |    float     |           n/a           |
|    longitude     |    float     |           n/a           |
|     altitude     |    float     |           n/a           |
|     distance     |    float     |           n/a           |

### Behavior

When sending the message only the index field will be read, all other
parameters can be set to 0 or left uninitialized. If multiple detections
are requested (see text about index below) each detections information
will be sent in a separate message. The number of messages sent can be
found in the total detections field.

##### index

Dictates which detection(s) to receive information about. There are 2
special indicies reserved for multi-detection requests. These are as
follows:

1.  index=254: The system will reply with all detections currently
    visible in the output overlay. The information regarding each
    detection will be sent as a separate message.

2.  index=255: The system will reply with all currently known
    detections. The information regarding each detection will be sent as
    a separate message.

In the returned message the index field will contain the index (between
0 and total_detections) of the message's detection.

##### score

The assigned score of the object.

##### total detections

The number detections for which information has been sent.

##### yaw (global)

The yaw euler angle in radians (using Tait-Bryan formalism) in relation to true
north.

##### pitch (global)

The pitch euler angle in radians (using Tait-Bryan formalism) in relation to true
north.

##### rel frame of reference

Indicates the frame of reference for the relative yaw and pitch
angles. The possible values are listed below.

| **Value** |       **Frame of reference**        |
|:---------:|:----------------------------------:|
|     0     | Global frame, same as yaw_global and pitch_global   |
|     1     | Autopilot heading frame, relative to the system's current heading |
|     2     | Camera frame, relative to the camera's center axis   |

##### yaw (relative)

The yaw euler angle in radians in relation to the center axis of the camera.

##### pitch (relative)

The pitch euler angle in radians in relation to the center axis of the camera.

##### latitude

The latitude of the object within the range \[-90.0, 90.0\].
Currently not implemented and always returns 0.

##### longitude

The longitude of the object \[-180.0, 180.0). Currently not
implemented and always returns 0.

##### altitude

The altitude of the object in meters above sea level. Currently not
implemented and always returns 0.

##### distance

The distance to the object from the camera (drone) in meters. Currently
not implemented and always returns 0.

## CAM_TARGETING

Message for user-controllable cameras' targeting information.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
| stream_name | char[8] | see below | see below |
| cam_id | uint8_t | \[0,3\] | \[0,3\] |
| targeting_mode | uint8_t | \[0,2\] | \[0,2\] |
| euler_delta | bool | true, false | true, false |
| yaw | float | \[-3.14,3.14\] | \[-3.14,3.14\] |
| pitch | float | \[-1.57,1.57\] | \[-1.57,1.57\] |
| roll | float | \[-1.57,1.57\] | \[-1.57,1.57\] |
| lock_flags | uint8_t | \[0,4\] | \[0,4\] |
| x_offset | float | \[-1.0,1.0\] | \[-1.0,1.0\] |
| y_offset | float | \[-1.0,1.0\] | \[-1.0,1.0\] |
| target_latitude | float | \[-90.0,90.0\] | | \[-90.0,90.0\] |
| target_longitude | float | \[-180.0,180.0\) | | \[-180.0,180.0\) |
| target_altitude | float | \[-1000.0,10000.0\] | \[-1000.0,10000.0\] | \[-1000.0,10000.0\] |

### Set behavior

##### stream name

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### cam id

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### targeting mode

Sets how the camera is to be targeted. The possible modes are listed
below. If the set mode is larger than the maximum (currently 3) the mode is not changed.
| **targeting_mode value** | **Targeting mode** |
|:------------------------:|:------------------:|
| 0 | Directional, the camera is aimed using global euler angles. |
| 1 | Coordinal, the camera is aimed using global coordinates. |
| 2 | Detection, the camera is aimed at a specific detection. |
| 3 | Single target tracking, the camera is aimed at the object being tracked by the single target tracking system. See [SINGLE_TARGET_TRACKING](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#single-target-tracking). |

##### euler delta

If true, the yaw, pitch, and roll values are treated as deltas (changes) rather than absolute positions.
##### yaw, pitch, roll

The euler angles in radians (using Tait-Bryan formalism) used to aim the camera. Roll is currently ignored.

##### lock flags

Controls which degrees of freedom are locked in place. If a direction is
not locked the camera will follow the system in that direction. 3 LSB
are relevant, control locking with yaw, pitch and roll in that order.
For example, if the lock_flags field is set to 0b00000011, the camera
will stabilize pitch and roll, but look in the system's forward direction.

##### x offset, y offset

Instructs the camera to offset its aim by a fraction of the view. The
values are unitless and in the range \[-1.0,1.0\]. For example, setting
x_offset to 0.5 will move the camera's aim to the right by half the
view's width, while setting y_offset to -0.5 will move the aim down by
half the view's height.

##### target latitude, target longitude, target altitude

The global coordinates used to aim the camera. Latitude is in the range
\[-90.0,90.0\], longitude is in the range \[-180.0,180.0\) and
altitude is the altitude of the target relative to the system in meters.
These fields are only used if the targeting mode is set to coordinal.

### Get behavior

When getting the CAM_TARGETING message all fields will be filled with
the current values used by the stream and camera specified by the
stream_name and cam_id fields.

## CAM_OPTICS_AND_CONTROL

Message for user-controllable cameras' optics and control information.

### Data fields
| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
| stream_name | char[8] | see below | see below |
| cam_id | uint8_t | \[0,3\] | \[0,3\] |
| zoom | int8_t | \[-127,127\] | n/a |
| fov | float | \[0.0,180.0\] | n/a |
| crop_mode | uint8_t | \[0,1\] | n/a |

### Set behavior

##### stream name

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### cam id

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### zoom

Sets the zoom level of the camera. The valid range is \[-127,127\], where 0 is no zoom, positive values zoom in, and negative values zoom out.

##### fov

Sets the field of view (FOV) of the camera in radians. The valid range is \[0.1, 3.14\] radians. Values outside this range are ignored.

##### crop mode

Sets the crop mode of the camera. The valid values are:
| **crop_mode value** | **Crop mode** |
|:-------------------:|:------------------:|
| 1 | Cartesian, the output is rectified (no distortion) |
| 2 | Panoramic, the output is in a panoramic format |

### Get behavior

When getting the CAM_OPTICS_AND_CONTROL message all fields will be filled with
the current values used by the stream and camera specified by the
stream_name and cam_id fields.

## CAM_OFFSET

Message useful for getting directions to objects seen in the user-controllable
cameras.

### Data fields

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|   stream_name  |   char[8]   |         see below       |         see below       |
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|       x        |    float     |     \[-1.0,1.0\]       |     \[-1.0,1.0\]       |
|       y        |    float     |     \[-1.0,1.0\]       |     \[-1.0,1.0\]       |
|    yaw_global     |    float     |     \[-3.14,3.14\]      |     \[-3.14,3.14\]      |
|   pitch_global    |    float     |     \[-3.14,3.14\]      |     \[-3.14,3.14\]      |
|    yaw_rel     |    float     |     \[-3.14,3.14\]      |     \[-3.14,3.14\]      |
|   pitch_rel    |    float     |     \[-3.14,3.14\]      |     \[-3.14,3.14\]      |

### Set behavior

Ignored.

### Get behavior

Specifying an offset in the range \[-1.0,1.0\] for x and y will return
the corresponding yaw and pitch angles in both absolute (in relation to true north)
and relative (in relation to the center axis of the camera) terms. The x
and y offsets are unitless and represent a fraction of the view. For example,
an x offset of 0.5 would indicate that the object is located halfway across
the camera's view in the horizontal direction. The returned yaw and pitch
angles are in radians.

## SENSOR

Message for controlling the image sensor's settings. Currently disabled.

## CAM_DEPTH_ESTIMATION

Message for controlling the depth estimation unit. Currently disabled.

## SINGLE_TARGET_TRACKING

Message for controlling the single target tracking unit.

### Data fields

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
| command | uint8_t | \[0,4\] | \[0,4\] |
| stream_name | char[8] | see below | see below |
| cam_id | uint8_t | \[0,3\] | \[0,3\] |
| x_offset | float | \[-1.0,1.0\] | \[-1.0,1.0\] |
| y_offset | float | \[-1.0,1.0\] | \[-1.0,1.0\] |
| detection_id | uint8_t | \[0,255\] | \[0,255\] |
| zoom_level | uint16_t | \[0,65535\] | \[0,65535\] |
| yaw_global | float | \[-3.14,3.14\] | \[-3.14,3.14\] |
| pitch_global | float | \[-1.57,1.57\] | \[-1.57,1.57\] |
| rel_frame_of_reference | uint8_t | 0,1,2 | 0,1,2 |
| yaw_rel | float | \[-3.14,3.14\] | \[-3.14,3.14\] |
| pitch_rel | float | \[-1.57,1.57\] | \[-1.57,1.57\] |

### Set behavior

##### command

Sets the command for the single target tracking system. The possible commands are listed below. If the set command is larger than the maximum (currently 4) the command is not changed.
| **command value** | **Command** |
|:-----------------:|:-----------:|
| 0 | Disable single target tracking. |
| 1 | Set target to specific direction |
| 2 | Set target to specific detection id. |
| 3 | Nudge the target by a small amount. |
| 4 | No operation |

##### stream name

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### cam id

See [Common behavior](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#common-behavior).

##### x offset, y offset

When command is set to 1, these fields are used to specify the target direction. The values are unitless and in the range \[-1.0,1.0\]. For example, setting x_offset to 0.5 will initiate tracking of an object located to the right by half the view's width, while setting y_offset to -0.5 will initiate tracking of an object located down by half the view's height.

When command is set to 3, these fields are used to nudge the current target direction by a small amount. The values are unitless and in the range \[-1.0,1.0\]. For example, setting x_offset to 0.1 will nudge the target direction to the right by 10% of the view's width, while setting y_offset to -0.1 will nudge the target direction down by 10% of the view's height.

##### detection id

When command is set to 2, this field is used to specify the detection id to track. The valid range is \[0,255\]. If the specified detection does not exist, the command is ignored.

##### zoom level

Sets the zoom level of the camera used for single target tracking. The valid range is \[0,10\].
A zoom of 0 makes the system use the biggest possible tracking box, while a zoom of 10 makes the system use the smallest possible tracking box.

##### yaw, pitch (global)

The yaw and pitch euler angles in radians (using Tait-Bryan formalism) in relation to true north.
Used when command is set to 1 and designates the target direction.

##### rel frame of reference

Indicates the frame of reference for the relative yaw and pitch
angles. The possible values are listed below.

| **Value** |       **Frame of reference**        |
|:---------:|:----------------------------------:|
|     0     | Global frame, same as yaw_global and pitch_global   |
|     1     | Autopilot heading frame, relative to the system's current heading |
|     2     | Camera frame, relative to the camera's center axis   |

##### yaw, pitch (relative)

Unused for set.

### Get behavior

When getting the SINGLE_TARGET_TRACKING message all fields will be filled with the current tracking state, as well as the current target direction.

Yaw and pitch (global and relative) indicate the current target direction. Absolute values are in relation to true north, while relative values are set according to the frame of reference.

# Supported MAVLINK messages
Both PX4 and Ardupilot are currently supported.

When configured to be run with MAVLink, the software listens to port 14550 (local IP address) for incoming MAVLink messages. Make sure to forward any MAVLink communication to this port.

## [Gimbal Protocol v2](https://mavlink.io/en/services/gimbal_v2.html)
DigiView is configured to follow the standard of MAVLink's Gimbal Protocol v2. DigiView will emit one heartbeat per virtual camera device and emit GIMBAL_MANAGER_STATUS and GIMBAL_DEVICE_STATUS. They can be controlled through gimbal messages, as well as camera messages for capture (not yet supported).

## Messages read by DigiView

DigiView needs a high rate of attitude messages to work properly. Make sure that the autopilot emits ATTITUDE or ATTITUDE_QUATERNION at a minimum of 100 Hz. Note: if you're working with the development kit, which includes internal sensors, you can ignore any ATTITUDE-messages.

| Number | Type | Name |
|---|---|---|
| 30  | Message | [ATTITUDE](https://mavlink.io/en/messages/common.html#ATTITUDE) |
| 31  | Message | [ATTITUDE_QUATERNION](https://mavlink.io/en/messages/common.html#ATTITUDE_QUATERNION) |
| 33  | Message | [GLOBAL_POSITION_INT](https://mavlink.io/en/messages/common.html#GLOBAL_POSITION_INT) |

### Gimbal messages

| Number | Type | Name |
|---|---|---|
| 195  | Command | [SET_ROI_LOCATION](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_SET_ROI_LOCATION) |
| 197  | Command | [SET_ROI_NONE](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_SET_ROI_NONE) |
| 282  | Message | [GIMBAL_MANAGER_SET_ATTITUDE](https://mavlink.io/en/messages/common.html#GIMBAL_MANAGER_SET_ATTITUDE) |
| 287  | Message | [GIMBAL_MANAGER_SET_PITCHYAW](https://mavlink.io/en/messages/common.html#GIMBAL_MANAGER_SET_PITCHYAW) |
| 1000 | Command | [GIMBAL_MANAGER_PITCHYAW](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW) |
| 1001 | Command | [GIMBAL_MANAGER_CONFIGURE](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE) |

### Camera messages

| Number | Type | Name |
|---|---|---|
| 200  | Command | [CONTROL_VIDEO](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_CONTROL_VIDEO)* |
| 203  | Command | [DIGICAM_CONTROL](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_DIGICAM_CONTROL)* |
| 214  | Command | [SET_CAM_TRIGG_INTERVAL](https://mavlink.io/en/messages/common.html#MAV_CMD_DO_SET_CAM_TRIGG_INTERVAL)* |
| 530  | Command | [CAMERA_MODE](https://mavlink.io/en/messages/common.html#MAV_CMD_SET_CAMERA_MODE)* |
| 531  | Command | [CAMERA_ZOOM](https://mavlink.io/en/messages/common.html#MAV_CMD_SET_CAMERA_ZOOM) |
| 2000 | Command | [IMAGE_START_CAPTURE](https://mavlink.io/en/messages/common.html#MAV_CMD_IMAGE_START_CAPTURE)* |
| 2001 | Command | [IMAGE_STOP_CAPTURE](https://mavlink.io/en/messages/common.html#MAV_CMD_IMAGE_STOP_CAPTURE)* |

### Synclair Vision MAVLink Dialect

Coming soon.