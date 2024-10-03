# Introduction

# Base message structure

|   **Name**   | **data type** | **Description** |
|:------------:|:-------------:|:---------------:|
|  timestamp   |   uint64_t    |                 |
|   version    |    uint8_t    |                 |
| message_type | enum: uint8_t |                 |
|  param_type  | enum: uint8_t |                 |
|     data     |  \[uint8_t\]  |                 |

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
| SET_PARAMETERS | 3 | Indicates that the message has data |
| CURRENT_PARAMETERS | 4 | Response flag set by DigiView indicating that the message contains current parameters used by the camera |
| ACKNOWLEDGEMENT | 5 | Indicates that the message is an ACK and does not contain any data |
| <span style="color: red">CHECKSUM_ERROR</span> | 6 | The check sum of the previous message did not match the check sum that was sent |
| DATA_ERROR | 7 | DigiView could not interpret the data received |
| <span style="color: red">FORBIDDEN</span> | 8 | The sender does not have permission to perform the requested action(s) |
| UNKNOWN | 9 | The parameter type supplied is unknown to DigiView |

## Parameter types

This table contains the possible parameter types in the parameter_type
field. The values are provided in the table, but are also available in
the PARAM_TYPE enum in the msg_defs.hpp file with the corresponding
name. For more information about the exact nature, behavior and
available data fields, check the corresponding message’s section in
chapter <a href="#cha:msg" data-reference-type="ref"
data-reference="cha:msg">3</a>.

| **Name** | **Value** | **Message type** | **Description** |
|:-------------------|:-----|:-----------|:---------------------------|
| SYSTEM_STATUS | 0 | GET | Returns whether the system is currently running |
| VIDEO_OUTPUT | 1 | GET & SET | Information regarding the video output |
| <span style="color: red">CAPTURE</span> | 2 | GET & SET | Message for controlling system recording |
| DETECTION | 3 | GET & SET | Message containing AI post processing parameters |
| DETECTED_ROI | 4 | GET | Retrieve detections based on output image |
| LENS | 5 | GET & SET | Control for which lens parameters to use when doing undistortion calculations |
| CAM_EULER | 6 | GET & SET | Handles user controlled cameras direction in euler angles relative to the world |
| CAM_ZOOM | 7 | GET & SET | Handles user controlled cameras’ zoom |
| CAM_LOCK_FLAGS | 8 | GET & SET | Handles user-controlled cameras’ locking settings |
| CAM_CONTROL_MODE | 9 | GET & SET | Handles user-controlled cameras’ control mode |
| CAM_CROP_MODE | 10 | GET & SET | Handles user-controlled cameras’ way of cropping its output image |
| CAM_OFFSET | 11 | GET & SET | Handles user-controlled cameras direction in euler angles relative to the drone |
| CAM_FOV | 12 | <span style="color: red">GET</span> & SET | Handles field of view of the user-controlled cameras |
| <span style="color: red">CAM_TARGET</span> | 13 | GET & SET | Handles user-controlled cameras direction in the geographic coordinate system lat/lon + altitude |
| CAM_SENSOR | 14 | GET & SET | Control for the image sensor’s settings |
|  |  |  |  |

The message types available.

# Messages

This section will describe each message listed in table
<a href="#tab:messages" data-reference-type="ref"
data-reference="tab:messages">2.1</a> in detail. Each subsection is for
a separate message where all data-fields and, if any exists, any
unexpected behaviors are explained. Each subsection is named after the
message’s corresponding enum name in section
<a href="#sec:param_types" data-reference-type="ref"
data-reference="sec:param_types">2.2</a>. If the enum is unavailable to
you, please refer back to the table in that section for the message’s
message_type value.

## SYSTEM_STATUS

### Data field

The system status messages contains the following fields.

| **Field name** | **Datatype** | **Valid arguments** |
|:--------------:|:------------:|:-------------------:|
|     status     |   uint8_t    |       \[0,3\]       |
|     error      |   uint8_t    |          0          |

### Behavior

The returned values indicate the systems current operational status
indicated by the value in the *status* field. The possibilities are
listed below.

| **Value** |         **Meaning**         |
|:---------:|:---------------------------:|
|     0     |       System is idle        |
|     1     |      System is running      |
|     2     |  System finished execution  |
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
| fps | uint8_t | \[5,30\] | \[5,30\] |
| layout_mode | uint8_t | \[0,6\] | \[0,6\] |
| detection_overlay_mode | uint8_t | \[0,5\] | \[0,5\] |
| views | \[bounding_box\] | n/a | See table <a href="#tab:bbox" data-reference-type="ref"
data-reference="tab:bbox">3.1</a> |
| detection_overlay_box | bounding_box | n/a | See table <a href="#tab:bbox" data-reference-type="ref"
data-reference="tab:bbox">3.1</a> |
| single_detection_size | uint16_t | n/a | See table <a href="#tab:bbox" data-reference-type="ref"
data-reference="tab:bbox">3.1</a> |

Some of the fields use the non-fundamental data type *bounding_box*
which is a struct defined in the header file used to hold information
regarding the different cameras. Its fields can be found here.

| **Name** | **Data type** | **Valid values** |
|:--------:|:-------------:|:----------------:|
|    x     |   uint16_t    |   \[0,65535\]    |
|    y     |   uint16_t    |   \[0,65535\]    |
|    w     |   uint16_t    |   \[0,65535\]    |
|    h     |   uint16_t    |   \[0,65535\]    |

The bounding_box struct

### Set behavior

##### width and height

If a width \> 1920 or height \> 1080 is set the arguments are clamped to
1920 and 1080 respectively. If they are set lower than 640 or 480 for
width and height respectively, the operation is ignored.

##### fps

The fps argument is clamped between 5 and 30 fps. The value set using
this message only alters the frame rate of the RTSP stream and NOT the
frame rate of the camera. The fps set is multiplied by 2 due to RTSP
lag. Better systems controls directly interfacing with the camera
settings will be available in the future.

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

The different user camera layouts

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

The available detection camera overlays

Each detection camera is 120x120 pixels, so to calculate the number of
detections shown the following formula is applied.
$$n\_{detections} = \lfloor \frac{\mathrm{limiting \quad pixel \quad length}}{120} \rfloor$$
The limiting pixel length is the output resolution width (height) for
row (column) layouts.

### Get behavior

##### width and height

Simply returns the current output resolution.

##### fps

Currently unused. In the future the hardware fps will be returned.

##### layout

Contains the current layout mode, see table
<a href="#tab:layouts" data-reference-type="ref"
data-reference="tab:layouts">3.2</a>.

##### detection overlay mode

Contains the current detection overlay, see table
<a href="#tab:det_layout" data-reference-type="ref"
data-reference="tab:det_layout">3.3</a>.

##### views

Is an array of length 4 containing the bounding boxes
\[<a href="#tab:bbox" data-reference-type="ref"
data-reference="tab:bbox">3.1</a>\] of each of the user-controllable
cameras in pixels. The positions are given by their top-left corner and
their width and height. If there are less than 4 user-controllable
cameras available the unused array index/indices will still contain
information, albeit either undefined or outdated. The number of valid
cameras can be extracted from the layout, see table
<a href="#tab:layouts" data-reference-type="ref"
data-reference="tab:layouts">3.2</a>.

##### detection overlay box

A single bounding box \[<a href="#tab:bbox" data-reference-type="ref"
data-reference="tab:bbox">3.1</a>\] for the position of the entire
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
detections are shown in the output. The possible sorting modes are found
in table <a href="#tab:det_sort" data-reference-type="ref"
data-reference="tab:det_sort">3.4</a>. If the set sorting mode is larger
than the maximum (currently 2) the sorting mode is not changed.

| **sorting_mode value** | **Sorting of detection cameras** |
|:--:|:---|
| 0 | Sort by detection score, highest to lowest. |
| 1 | Sort by detection age, highest to lowest. |
| 2 | Sort by score, highest to lowest but once a detection is assigned a detection camera it is locked to that camera for the lifetime of the object, independent of new detection with a higher score. |

Sorting modes for detection cameras

##### crop confidence threshold

Sets the threshold for considering detections while tracking a detected
object. If outside the valid range, the previous value is kept.

##### var confidence threshold

Sets the threshold for considering detections when scanning for new
objects. If outside the valid range, the previous value is kept.

##### crop box limit

Deprecated. If the value is outside the valid range it is clamped.

##### var box limit

Deprecated. If the value is outside the valid range it is clamped.

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

|  **Field name**  | **Datatype** | **Valid GET arguments** |
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
0 and total_detections) of the message’s detection.

##### score

The assigned score of the object. See
<a href="#sec:det" data-reference-type="ref"
data-reference="sec:det">3.4</a> for more information regarding the
scoring.

##### total detections

The number detections for which information has been sent.

##### yaw (absolute)

The yaw euler angle (using Tait-Bryan formalism) in relation to true
north.

##### pitch (absolute)

The pitch euler angle (using Tait-Bryan formalism) in relation to true
north.

##### yaw (relative)

The yaw euler angle in relation to the center axis of the camera.

##### pitch (relative)

The pitch euler angle in relation to the center axis of the camera.

##### latitude

The latitude of the object in degrees within the range \[-90.0, 90.0\].
Currently not implemented and always returns 0.

##### longitude

The longitude of the object in degrees \[-180.0, 180.0). Currently not
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

Message for user-controllable cameras’ positioning relative to true
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

The updated roll in radians.

## CAM_ZOOM

Controls the (digital) zoom of a user-controlled camera by modifying the
fov captured by the camera.

### Data field

| **Field name** | **Datatype** | **Valid SET arguments** | **Valid GET arguments** |
|:--------------:|:------------:|:-----------------------:|:-----------------------:|
|     cam_id     |   uint8_t    |         \[0,3\]         |         \[0,3\]         |
|      zoom      |    int8_t    |      \[-127,127\]       |           n/a           |

### SET behavior

##### cam id

The id of the user-controlled camera to update.

##### zoom

The zoom value to update the camera with. If the new fov is smaller than
0.1 radians or larger than 2*π* the fov is clamped.

### GET behavior

##### cam id

The id of the user-controlled camera to get the zoom for.

##### zoom

Deprecated, use cam fov <a href="#sec:cam_fov" data-reference-type="ref"
data-reference="sec:cam_fov">3.13</a>. The zoom of the camera.

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
bits are relevant, if the bit is 1 the direction is locked.

| **Bit number** | **Locked direction** |
|:--------------:|:--------------------:|
|       1        |         roll         |
|       2        |        pitch         |
|       3        |         yaw          |

Lock flags for user-controllable cameras

### Get behavior

When getting the cam flags the cam_id field needs to contain a valid id,
otherwise the flags returned will be undefined. s.

## CAM_CONTROL_MODE

Decides how the software gimbal is controlled.

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

Which cropping mode to use. See table
<a href="#tab:crop_mode" data-reference-type="ref"
data-reference="tab:crop_mode">3.7</a>.

| **mode value** | **Type of cropping** |
|:--:|:--:|
| 0 | Deprecated. Crops a rectangular section of the image. |
| 1 | Crops a cartesian sector of the image, resulting in a square output. |
| 2 | Crops a panoramic view. |
| 3 | Deprecated. Undistorts the cropped portion. |
| 4 | Debug view, downscales the full 4K image to the output resolution. |

The different cropping modes available.

If the mode is not one of the values found in table
<a href="#tab:crop_mode" data-reference-type="ref"
data-reference="tab:crop_mode">3.7</a> the resulting output image for
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
plane of the frame.

##### cam id

Sets the user-controllable camera’s id to set offset for. If cam id is
outside the valid set arguments the message will be ignored.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

### Get behavior

This message’s get version will return the euler angle of the requested
pixel, either in relation to the frame or in relation to the global
coordinate system.

##### cam id

Id of the user-controlled camera to receive data from.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

##### frame_rel

Flag for whether to return the angles in the cameras coordinate system
or the global coordinate system.

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

Sets the user-controllable camera’s id to set fov for. If cam id is
outside the valid set arguments the message will be ignored.

##### fov

The new fov to set in radians. Note that the fov passed in this message
directly changes the horizontal fov, the vertical fov is dynamically
calculated to maintain the aspect ratio of the camera. While it is
possible to send fovs outside the valid interval and they will be
accepted, doing so might result in undefined behavior.

## CAM_TARGET

Message for converting target pixels to geographic coordinates and
locking a camera towards those coordinates. If the system is unable to
acquire its own GPS location the results of this message will be
undefined.

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

When using this message to set, the latitude, longitude, and altitude of
the (x,y) pixel will be calculated and subsequently used as the target
for the camera. If the camera is translated in space after sending this
message the aim of the user-controllable camera with id cam_id will be
updated to the new rotation looking towards the lat/lon/alt set when
this message was sent. If both x and y are 0, the coordinates sent in
t_latitude, t_longitude, and t_altitude will be used as the target
coordinates.

##### cam id

Sets the user-controllable camera’s id to set the target for. If cam id
is outside the valid set arguments the message will be ignored.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

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

Sets the user-controllable camera’s id to set the target for. If cam id
is outside the valid set arguments the message will be ignored.

##### x

The normalized horizontal distance ((pixel x coord) - (image width / 2))
/ image width) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of yaw. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

##### y

The normalized vertical distance ((pixel y coord) - (image height / 2))
/ image height) from the center point of the user-controlled cameras
image, used to set the new aim vector in terms of pitch. While values
outside the valid set arguments (i.e. requesting a pixel outside the
user-controlled camera’s frame) will work, using the message this way is
technically undefined behavior.

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
to true.

##### target brightness

The targeted brightness of the image, used for automatic exposure
calculations.

##### exposure value

The value to the the exposure to. 40000 is set as the limit in this
documentation, but the actual max depends on the fps. The exposure is
measured in *μ**s* and should not exceed the frame time. If the fps is
set to 30 that translates to a max exposure of
$\frac{1}{30} \cdot 10^6 = 33333$.

##### gain value

The gain to set for the camera sensor. While 50000 is set as the maximum
in this documentation the actual max depends heavily on the brightness
of the image.

### Get behavior

Using this message’s getter will simply return the values currently used
by the system. See the set behavior for each field’s meaning.

# Supported MAVLINK messages
