# Introduction

This page contains documentation on the communication with Synclair Vision's software DigiView.

## General workflow

The file msg_defs.hpp contains all necessary helper functions to pack messages that can then be sent over a network connection.
It is recommended to have msg_defs.hpp open while reading this documentation.

# Base message structure

|   **Name**   | **data type** | **Description** |
|:------------:|:-------------:|:---------------:|
|  timestamp   |   uint64_t    | Time of message received, must be set manually |
|   version    |    uint8_t    | Version of msg_defs.hpp |
| message_type | enum: uint8_t | Message type, see below |
|  param_type  | enum: uint8_t | Parameter type, see below |
|     data     |  \[uint8_t\]  | Arguments or data for specific parameter type |

#

## Message types

This table contains the possible values in the message_type field of a
message. The values are provided in the table, but are also available in
the MESSAGE_TYPE enum in the msg_def.hpp file with the corresponding
name for convenience.

| **Name** | **Value** | **Description** |
|:--------------------|:-----|:-----------------------------------------|
| EMPTY | 0 | Indicates that the message is empty |
| QUIT | 1 | Instructs DigiView to stop execution |
| GET_PARAMETERS | 2 | Indicates that the message is a data request |
| SET_PARAMETERS | 3 | Indicates that the message has data for DigiView to set |
| CURRENT_PARAMETERS | 4 | Response flag set by DigiView indicating that the message contains current parameters used by the camera |
| ACKNOWLEDGEMENT | 5 | Indicates a succesful SET request |
| CHECKSUM_ERROR | 6 | The check sum of the previous message did not match the check sum that was sent |
| DATA_ERROR | 7 | DigiView could not interpret the data received |
| FORBIDDEN | 8 | The sender does not have permission to perform the requested action(s) |
| UNKNOWN | 9 | The parameter or message type supplied is unknown to DigiView |

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
| VIDEO_OUTPUT | 1 | GET & SET | Information regarding the video output |
| CAPTURE* | 2 | GET & SET | Message for controlling system recording |
| DETECTION | 3 | GET & SET | Message containing AI post processing parameters |
| DETECTED_ROI | 4 | GET | Retrieve information of specific detections |
| LENS | 5 | GET & SET | Control for which lens parameters to use when doing undistortion calculations |
| CAM_EULER | 6 | GET & SET | Handles user-controlled cameras' direction in euler angles relative to the world |
| CAM_ZOOM | 7 | GET & SET | Handles user-controlled cameras' zoom |
| CAM_LOCK_FLAGS | 8 | GET & SET | Handles user-controlled cameras' locking settings |
| CAM_CONTROL_MODE | 9 | GET & SET | Handles user-controlled cameras' control mode |
| CAM_CROP_MODE | 10 | GET & SET | Handles user-controlled cameras' way of cropping its output image |
| CAM_OFFSET | 11 | GET & SET | Handles user-controlled cameras' direction in euler angles relative to center of picture |
| CAM_FOV | 12 | GET* & SET | Handles field of view of the user-controlled cameras |
| CAM_TARGET | 13 | GET & SET | Handles user-controlled cameras' direction in the geographic coordinate system lat/lon + altitude |
| CAM_SENSOR | 14 | GET & SET | Control for the image sensor's settings |
|  |  |  |  |

# Messages

This section will describe each message listed under [Parameter types](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#parameter-types) in detail. Each subsection is for a separate message where all data-fields and, if any exists, any unexpected behaviors are explained. Each subsection is named after the message's corresponding parameter name.

## SYSTEM_STATUS

### Data field

The system status messages contains the following fields:

| **Field name** | **Datatype** | **Valid arguments** |
|:--------------:|:------------:|:-------------------:|
|     status     |   uint8_t    |       \[0,3\]       |
|     error      |   uint8_t    |          0          |

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

## VIDEO_OUTPUT

### Data field

The video output message has two separate versions, one for get
operations and one for set operations.

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--:|:--:|:--:|:--:|
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
| 0 | 1 square camera |
| 1 | 2 square cameras stacked on top of each other |
| 2 | 2 panoramic cameras stacked on top of each other |
| 3 | 2 square cameras side by side on top of one panoramic camera |
| 4 | 4 square cameras |
| 5 | 3 square cameras on top of one panoramic camera |
| 6 | Debug camera, downscaled full view of the physical camera. There are no input controls available in this mode. |

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

Each detection camera is 120x120 pixels, so to calculate the number of
detections shown the following formula is applied.

$$n\_{detections} = \lfloor \frac{\mathrm{limiting \quad pixel \quad length}}{120} \rfloor$$

The limiting pixel length is the output resolution width (height) for
row (column) layouts.

### Get behavior

##### width and height

Returns the current <b>total</b> output resolution, including all user-controllable cameras and detection cameras.

##### fps

Unused.

##### layout

Contains the current layout mode and number of active user-controlled cameras, see table under [layout](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#layout). The mode value will be located in the 4 least significant bits, and the number of active user-controlled cameras currently in use will be located in the 4 most significant bits.

To extract number of views and layout mode:
```
video_output_parameters p = ...;
int mode = p.layout_mode & 0x0f;
int num_views = (p.layout_mode & 0xf0) >> 4;
```

##### detection overlay mode

Contains the current detection overlay, see table under [detection overlay mode](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#detection-overlay-mode).

##### views

Is an array of length 4 containing the bounding boxes of each of the user-controllable cameras in pixels. The positions are given by their top-left corner and their width and height. If there are less than 4 user-controllable cameras available the unused array index/indices will still contain information, albeit either undefined or outdated. The number of active cameras can be extracted from the layout, see [layout](https://github.com/SynclairVision/message-definitions/blob/main/docs.md#layout-1).

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

Currently not implemented

### Data field

n/a

### Behavior

n/a

## DETECTION

The detection message contains parameters modifying the internal
handling of detecting objects and how detections found by the system are
presented.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--:|:--:|:--:|:--:|
| mode | uint8_t | \[0,255\] | \[0,255\] |
| sorting_mode | uint8_t | \[0,2\] | \[0,2\] |
| crop_confidence_threshold | float | \[0.0,0.99) | \[0.0,0.99) |
| var_confidence_threshold | float | \[0.0,0.99) | \[0.0,0.99) |
| crop_box_limit | uint16_t | \[10,500\] | \[10,500\] |
| var_box_limit | uint16_t | \[10,500\] | \[10,500\] |
| crop_box_overlap | float | \[0.0,0.99) | \[0.0,0.99) |
| var_box_overlap | float | \[0.0,0.99) | \[0.0,0.99) |
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

##### crop confidence threshold

Sets the threshold for considering detections while tracking a detected
object. If outside the valid range, the previous value is kept.

##### var confidence threshold

Sets the threshold for considering detections when scanning for new
objects. If outside the valid range, the previous value is kept.

##### crop box limit

Deprecated.

##### var box limit

Deprecated.

##### crop box overlap

Sets the max allowed overlap between detection bounding boxes while
tracking an object. If outside the valid range the previous value is
kept.

##### var box overlap

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
|     yaw_abs      |    float     |           n/a           |
|    pitch_abs     |    float     |           n/a           |
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

##### yaw (absolute)

The yaw euler angle in radians (using Tait-Bryan formalism) in relation to true
north.

##### pitch (absolute)

The pitch euler angle in radians (using Tait-Bryan formalism) in relation to true
north.

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

## LENS

Controls which lens calibration data to use.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|    lens_id     |   uint8_t    |         \[0,2\]         |         \[0,2\]         |

### Behavior

Controls which calibration data from the lens.cal file to use when
removing distortion caused by the lens. The index to lens mapping can be
found here.

|                 |                           |
|:---------------:|:-------------------------:|
| **index Value** | **Lens name in lens.cal** |
|        0        |          CIL216           |
|        1        |          CIL018           |
|        2        |           LN007           |

## CAM_EULER

Message for user-controllable cameras' positioning relative to true
north.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|    is_delta    |   uint8_t    |           0,1           |           0,1           |
|      yaw       |    float     |     \[-3.14,3.14\]      |     \[-3.14,3.14\]      |
|     pitch      |    float     |     \[-3.14,3.14\]      |     \[-3.14,3.14\]      |
|      roll      |    float     |     \[-1.57,1.57\]      |     \[-1.57,1.57\]      |

### Set behavior

##### cam id

Dictates which user-controlled camera is updated with the message.

##### is delta

Boolean which decides if the angle sent is to be interpreted as a global
direction or a change to the current aim. 0 indicates that the angle
should be taken as a global direction. While any non-zero value will
resolve to interpreting the data as a change, it is recommended to stick
to 1 translating to true.

##### yaw

The updated yaw in radians.

##### pitch

The updated pitch in radians.

##### roll

The updated roll in radians (currently ignored).

## CAM_ZOOM

Controls the (digital) zoom of a user-controlled camera by modifying the
FOV captured by the camera.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|      zoom      |    int8_t    |      \[-127,127\]       |           n/a           |

### SET behavior

##### cam id

The id of the user-controlled camera to update.

##### zoom

Unitless value to zoom in or out. Positive values zoom in, negative zoom out.
Larger values will result in a larger zoom factor.

### GET behavior

##### cam id

The id of the user-controlled camera to get the zoom for.

##### zoom

Undefined, use CAM_FOV to get the current zoom level.

## CAM_LOCK_FLAGS

Controls which degrees of freedom are locked in place.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|     flags      |   uint8_t    |         \[0,4\]         |         \[0,4\]         |

### Set behavior

##### cam id

The id of the user-controlled camera to set the locked direction for.

##### flags

Which degrees of freedom to lock in place. If a direction is not locked
the camera will follow the system in that direction. Only the first 3
bits are relevant, if the bit is 1 the direction is locked. For example,
if only roll and pitch is set to 1 the camera will look in the flight
direction of the drone.

| **Bit number** | **Locked direction** |
|:--------------:|:--------------------:|
|       1        |         roll         |
|       2        |        pitch         |
|       3        |         yaw          |


### Get behavior

When getting the cam flags the cam_id field needs to contain a valid id,
otherwise the flags returned will be undefined.

## CAM_CONTROL_MODE

Decides how the software gimbal for a user-controlled camera behaves.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|      mode      |   uint8_t    |         \[0,3\]         |         \[0,3\]         |

### Set behavior

##### cam id

Sets which user-controlled camera to change control mode for.

##### mode

Sets the camera to be controlled in one of the following 4 ways:

| **mode value** |        **Control type**         |
|:--------------:|:-------------------------------:|
|       0        | Pilot view, not yet implemented |
|       1        |       Directional control       |
|       2        |        Coordial control         |
|       3        |              Pass               |

Camera control modes

### Get behavior

When getting the control mode the cam_id field needs to contain a valid
id, otherwise the control mode returned will be undefined.

## CAM_CROP_MODE

Handles the way the cropping of a user-controlled camera is performed.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|      mode      |   uint8_t    |         \[0,4\]         |         \[0,4\]         |

### Set behavior

##### cam id

Sets which user-controlled camera the message is aimed at.

##### mode

Which cropping mode to use. See table below for available modes.

| **mode value** | **Type of cropping** |
|:--:|:--:|
| 0 | Deprecated. Crops a rectangular section of the image. |
| 1 | Crops a cartesian sector of the image, resulting in a square output. Default. |
| 2 | Crops a panoramic view. Default. |
| 3 | Deprecated. Undistorts the cropped portion. |
| 4 | Debug view, downscales the full 4K image to the output resolution. |

If the mode is not one of the values found in the table above the resulting output image for
the user-controlled camera with id cam_id will be full black.

### Get behavior

When getting the cropping mode the cam_id field needs to contain a valid
id, otherwise the crop mode returned will be undefined.

## CAM_OFFSET

Utility for getting the euler angle (in Tait-Bryan formalism) for a
pixel in a user-controlled camera that is not the center. In the set
version the cameras direction is also updated to the offset of that
pixel.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|       x        |    float     |      \[-1.0,1.0\]       |      \[-1.0,1.0\]       |
|       y        |    float     |      \[-1.0,1.0\]       |      \[-1.0,1.0\]       |
|   frame_rel    |   uint8_t    |           n/a           |           0,1           |
|      yaw       |    float     |           n/a           |     \[-3.14,3.14\]      |
|     pitch      |    float     |           n/a           |     \[-3.14,3.14\]      |

### Set behavior

Using this message as a setter will move the view of the camera with id
cam_id to the pixel requested. Note that x & y are NOT the pixel values
in the cameras frame but rather the normalized coordinates in the x-y
plane of the frame. -1, -1 is the top left corner, 1, 1 is the bottom right.

##### cam id

Sets the user-controllable camera's id to set offset for. If cam id is
outside the valid set arguments the message will be ignored.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior.

### Get behavior

This message's get version will return the euler angle of the requested
pixel, either in relation to the frame or in relation to the global
coordinate system.

##### cam id

Id of the user-controlled camera to receive data from.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior.

##### frame_rel

Flag for whether to return the angles in the cameras coordinate system
or the global coordinate system. 0 for the global coordinate system, 1
for the system's coordinate system.

##### yaw

Return value of the yaw euler angle (in Tait-Bryan formalism), either in
the cameras coordinates or the global coordinates. See frame_rel.

##### pitch

Return value of the pitch euler angle (in Tait-Bryan formalism), either
in the cameras coordinates or the global coordinates. See frame_rel.

## CAM_FOV

Data relating to the field of view of a user-controllable camera.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** |
|:--------------:|:------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |
|      fov       |    float     |      \[0.1,3.14\]       |

### Set behavior

##### cam id

Sets the user-controllable camera's id to set fov for. If cam id is
outside the valid set arguments the message will be ignored.

##### fov

The new fov to set in radians. Note that the fov passed in this message
directly changes the horizontal fov, the vertical fov is dynamically
calculated to maintain the aspect ratio of the camera. While it is
possible to send fovs outside the valid interval and they will be
accepted, doing so might result in undefined behavior.

## CAM_TARGET

Message for GPS targeting. If the system is unable to
acquire its own GPS location or altitude, the results of this message
will be undefined.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|       x        |    float     |      \[-1.0,1.0\]       |      \[-1.0,1.0\]       |
|       y        |    float     |      \[-1.0,1.0\]       |      \[-1.0,1.0\]       |
|   t_latitude   |    float     |     \[-90.0,90.0\]      |     \[-90.0,90.0\]      |
|  t_longitude   |    float     |    \[-180.0,180.0\]     |    \[-180.0,180.0\]     |
|   t_altitude   |    float     |           any           |           any           |

### Set behavior

When using this message to set, one can either set x and y or latitude,
longitude, and altitude. If x and y are set, the system will calculate
the corresponding latitude, longitude, and altitude of the pixel in the
user-controlled camera's frame and update the camera's target to that
coordinate. If latitude, longitude, and altitude are set, the system will
update the camera's target to that coordinate.

##### cam id

Sets the user-controllable camera's id to set the target for. If cam id
is outside the valid set arguments the message will be ignored.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior. -1 is the left edge of the frame, 1 is
the right edge.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior. -1 is the top edge of the frame, 1 is
the bottom edge.

##### latitude

The targeted latitude IFF both x and y are 0.

##### longitude

The targeted longitude IFF both x and y are 0.

##### altitude

The targeted altitude IFF both x and y are 0.

### Get behavior

When using this message to get the system will return the targeted
latitude, longitude, and altitude, calculated based of the pixel
supplied in the x and y arguments. IFF both the x and y arguments are
set to 0 the message will instead return the current targeted coodinates
of the camera. If the camera is not currently tracking a coordinate the
returned values will be either out of date or undefined.

##### cam id

Sets the user-controllable camera's id to set the target for. If cam id
is outside the valid set arguments the message will be ignored.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior. -1 is the left edge of the frame, 1 is
the right edge.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera's frame) will work, using the message this way is
technically undefined behavior. -1 is the top edge of the frame, 1 is
the bottom edge.

##### latitude

The targeted latitude IFF both x and y are 0. Out of date or undefined
if the camera is not currently locked to a coordinate.

##### longitude

The targeted longitude IFF both x and y are 0. Out of date or undefined
if the camera is not currently locked to a coordinate.

##### altitude

The targeted altitude IFF both x and y are 0. Out of date or undefined
if the camera is not currently locked to a coordinate.

## CAM_SENSOR

Message for control of the camera hardware.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--:|:--:|:--:|:--:|
| ae | uint8_t | 0,1 | 0,1 |
| target_brightness | uint8_t | (0,255) | (0,255) |
| exposure_value | uint32_t | (0,40000\] | (0,40000\] |
| gain_value | uint32_t | (0,50000\] | (0,50000\] |

### Set behavior

If automatic exposure is set to true when sending this message the
system will automatically modify the exposure which might result in the
change made becoming undone.

##### automatic exposure (ae)

Sets whether the system should automatically adjust the exposure of the
camera, with 0 being false. While any non-zero value will resolve to
interpreting the data as true, it is recommended to stick to 1 meaning
true.

##### target brightness

The targeted brightness of the image, used for automatic exposure
calculations.

##### exposure value

The value to the the exposure to. 40000 is set as the limit in this
documentation, but the actual max depends on the fps. The exposure is
measured in μs and should not exceed the frame time. If the fps is
set to 30 that translates to a max exposure of
$\frac{1}{30} \cdot 10^6 = 33333$.

##### gain value

The gain to set for the camera sensor. While 50000 is set as the maximum
in this documentation the actual max depends heavily on the brightness
of the image.

### Get behavior

Using this message's getter will simply return the values currently used
by the system. See the set behavior for each field's meaning.

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
