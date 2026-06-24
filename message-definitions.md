# DigiView 0.6 Message Definitions

This document is a customer-facing reference for DigiView 0.6 communication. It covers both:

- the native DigiView message format from `msg_defs.hpp`
- the Synclair MAVLink dialect, which mirrors the same parameter groups

It focuses on what each message does and how to use it. It does not describe DigiView internals.

## Overview

DigiView exposes parameter-based messages. A client typically:

1. sends a `GET_PARAMETERS` message to read the current value of a parameter group
2. sends a `SET_PARAMETERS` message to change supported fields
3. receives `CURRENT_PARAMETERS` as the normal response containing the current values

Some parameter groups are read-only, some are writable, and some exist in the message format but are only partly active in DigiView 0.6.

## Base message structure

The native DigiView message contains these top-level fields:

Current native protocol version is `0x00`.
This version uses `stream_name` as `char[16]` and `data[72]`.

| Name | Type | Description |
|---|---|---|
| `timestamp` | `uint64_t` | Message timestamp |
| `version` | `uint8_t` | Message definition version |
| `message_type` | `uint8_t` | Message action/result type |
| `param_type` | `uint8_t` | Parameter group |
| `interval_ms` | `uint32_t` | Optional repeat interval for recurring GET requests |
| `data[72]` | `uint8_t[]` | Payload for the selected parameter group |
| `checksum` | `uint8_t` | Message checksum |

### Message types

| Value | Name | Meaning |
|---:|---|---|
| 0 | `EMPTY` | Empty message |
| 1 | `GET_PARAMETERS` | Read a parameter group |
| 2 | `SET_PARAMETERS` | Change a parameter group |
| 3 | `CURRENT_PARAMETERS` | DigiView response containing current values |
| 4 | `ACKNOWLEDGEMENT` | Change accepted |
| 5 | `CHECKSUM_ERROR` | Invalid checksum |
| 6 | `DATA_ERROR` | Invalid or unusable payload |
| 7 | `FORBIDDEN` | Operation not allowed |
| 8 | `UNKNOWN` | Unknown message or parameter type |
| 255 | `QUIT` | Close the connection |

### Recurring GET requests

In DigiView 0.6, recurring updates use the base field `interval_ms`.

- Send `GET_PARAMETERS` with `interval_ms = 0` for a one-time response.
- Send `GET_PARAMETERS` with a non-zero `interval_ms` to request repeated updates.
- The minimum practical recurring interval is `50 ms`.
- Requests below `50 ms` behave like one-shot requests.

## Parameter types

| Value | Name | GET | SET | Description |
|---:|---|---|---|---|
| 0 | `SYSTEM_STATUS` | Yes | Limited | Application state |
| 1 | `AI` | Yes | Yes | AI enable state and selected models |
| 2 | `MODEL` | Yes | No | Available model names |
| 3 | `VIDEO_OUTPUT` | Yes | Yes | Stream layout and output settings |
| 4 | `CAPTURE` | Yes | Yes | Recording control |
| 5 | `DETECTION` | Yes | Yes | Detection thresholds and score tuning |
| 6 | `TRACKED_DETECTION` | Yes | No | Information about current detections |
| 13 | `CAM_TARGETING` | Yes | Yes | Camera aiming and lock modes |
| 14 | `CAM_OPTICS_AND_CONTROL` | Yes | Yes | Camera FOV and zoom control |
| 15 | `CAM_OFFSET` | Yes | No | Convert a point in the view into angles |
| 16 | `SENSOR` | Yes | Yes | Sensor exposure, gain, brightness |
| 17 | `CAM_DEPTH_ESTIMATION` | No | No | Reserved parameter group, not used by DigiView in this release |
| 18 | `SINGLE_TARGET_TRACKING` | Yes | Yes | Single-target-tracking control and status |
| 19 | `CALIBRATION` | Yes | Yes | Calibration command and progress |
| 20 | `NAVIGATION` | No | No | Parameter group exists, but DigiView does not produce it in this release |

## Common usage notes

### `stream_name`

Many messages include `stream_name` as `char[16]`.

- Use it to select which output stream a message applies to.
- The maximum length is 16 characters.
- For some GET requests, an empty stream name returns data for all streams.

### `cam_id`

Many camera-related messages include `cam_id`.

- It selects a user view within a stream.
- Valid values are typically `0 .. num_user_views - 1`.
- `num_user_views` is returned by `VIDEO_OUTPUT`.

### Enum and flag value names

Some fields use symbolic enum or bit-flag names in code. Those names come from
`digiview_commons/public_enums.hpp`, which is included by `msg_defs.hpp`.
This document lists the customer-facing values you need for integration.

### Angles and FOV in DigiView 0.6

In current DigiView 0.6 behavior, the customer-facing native protocol uses **degrees** for returned and requested angles in the message groups that expose angle values.

This applies to:

- `CAM_TARGETING`
- `CAM_OPTICS_AND_CONTROL`
- `CAM_OFFSET`
- `TRACKED_DETECTION`
- `SINGLE_TARGET_TRACKING`

### Frames of reference

Some messages return both global and relative angles. The relative frame is identified by:

| Value | Meaning |
|---:|---|
| 0 | Global frame |
| 1 | Autopilot heading frame |
| 2 | Camera frame |

## Message reference

## `SYSTEM_STATUS`

Read current application state.

### Fields

| Field | Type | Notes |
|---|---|---|
| `status` | `uint8_t` | Current DigiView state |
| `error` | `uint8_t` | Reserved |
| `jetson_temp` | `float` | Jetson temperature |

### Status values

| Value | Meaning | Notes
|---:|---|---|
| 0 | Startup |  |
| 1 | Loading |  |
| 2 | Running |  |
| 3 | Error |  |
| 4 | Halt |  |



### Behavior

- `GET` returns the current status, reserved error byte, and device temperature.
- `SET` only supports switching between `Running` and `Halt`.
- Other status writes should not be relied on.
- In this release, DigiView uses `Startup`, `Loading`, `Running`, and `Halt`.
- `Error` exists in the message definitions, but is not used by DigiView as an application state in this release.

## `AI`

Enable or disable AI and select the active scan and track models.

### Fields

| Field | Type | Notes |
|---|---|---|
| `run_ai` | `bool` | Enable or disable AI |
| `track_model_name` | `char[16]` | Active tracking model |
| `scan_model_name` | `char[16]` | Active scanning model |

### Behavior

- `GET` returns the current enable state and selected model names.
- `SET` updates the AI enable flag and selected models.
- Model names should match the names returned by `MODEL`.

## `MODEL`

List available models.

### Fields

| Field | Type | Notes |
|---|---|---|
| `model_name` | `char[16]` | Name of one available model |

### Behavior

- `GET` returns one `CURRENT_PARAMETERS` response per available model.
- The first returned names are the active scan model and active track model, followed by the remaining available models.
- `SET` is not supported.

## `VIDEO_OUTPUT`

Control stream resolution and layout, and read back current output geometry.

### Fields

| Field | Type | Notes |
|---|---|---|
| `stream_name` | `char[16]` | Selected stream |
| `width` | `uint16_t` | Output width |
| `height` | `uint16_t` | Output height |
| `fps` | `uint8_t` | Target frame rate |
| `layout_mode` | `uint8_t` | User-view layout |
| `detection_overlay_mode` | `uint8_t` | Detection overlay layout |
| `num_user_views` | `uint8_t` | Number of active user views returned by DigiView |
| `views[4]` | `bounding_box[]` | Bounding boxes for user views |
| `detection_overlay_box` | `bounding_box` | Bounding box for the full detection overlay area |
| `single_detection_size` | `uint16_t` | Side length of each detection tile |

`bounding_box` contains `x`, `y`, `w`, and `h`, all as `uint16_t`.

### Layout values

| Value | Meaning |
|---:|---|
| 0 | 1 view |
| 1 | 2 stacked |
| 2 | 2 side by side |
| 3 | 3 views |
| 4 | 4-view grid |
| 5 | 4 views with 3+1 layout |
| 6 | Debug layout |

### Detection overlay values

| Value | Meaning |
|---:|---|
| 0 | No overlay |
| 1 | Single overlay in top-right corner |
| 2 | Column on right |
| 3 | Column on left |
| 4 | Row on top |
| 5 | Row on bottom |

### Behavior

- `GET` returns the current output size, layout, number of views, and view geometry.
- `SET` can change resolution, frame rate, layout, and detection overlay mode.
- An empty `stream_name` in a GET request can be used to request all enabled streams.

## `CAPTURE`

Control video recording per stream.

### Fields

| Field | Type | Notes |
|---|---|---|
| `stream_name` | `char[16]` | Selected stream |
| `cap_single_image` | `bool` | Not used by DigiView in this release |
| `record_video` | `bool` | Start or stop recording |
| `images_captured` | `uint16_t` | Not maintained by DigiView as a usable counter in this release |
| `videos_captured` | `uint16_t` | Not maintained by DigiView as a usable counter in this release |

### Behavior

- `GET` returns current capture-related state for the selected stream.
- An empty `stream_name` in a GET request can be used to request all enabled streams.
- `SET record_video = true` starts recording for that stream.
- `SET record_video = false` stops recording.
- `cap_single_image` is not used by DigiView in this release.
- `images_captured` and `videos_captured` are not maintained by DigiView as usable counters in this release.
- Use `record_video` for the supported customer-facing capture workflow in this release.

## `DETECTION`

Adjust detection thresholds and score tuning.

### Fields

| Field | Type | Notes |
|---|---|---|
| `mode` | `uint8_t` | Present in the message format, but not a primary user control in 0.6 |
| `sorting_mode` | `uint8_t` | Present in the message format, but not a primary user control in 0.6 |
| `track_confidence_threshold` | `float` | Tracking threshold |
| `scan_confidence_threshold` | `float` | Scan threshold |
| `track_box_overlap` | `float` | Tracking overlap limit |
| `scan_box_overlap` | `float` | Scan overlap limit |
| `creation_score_scale` | `uint8_t` | Initial score for new detections |
| `bonus_detection_scale` | `uint8_t` | Score bonus when a detection is found again |
| `bonus_redetection_scale` | `uint8_t` | Score bonus during re-detection |
| `missed_detection_penalty` | `uint8_t` | Penalty when a detection is missed |
| `missed_redetection_penalty` | `uint8_t` | Penalty when re-detection fails |

### Behavior

- `GET` returns the current thresholds and score settings.
- `SET` updates supported thresholds and score settings.
- The threshold and overlap float values are quantized on the wire.
- A value of `255` in the score-weight fields means “leave unchanged”.

## `TRACKED_DETECTION`

Request one or more current detections.

### Fields

| Field | Type | Notes |
|---|---|---|
| `index` | `uint8_t` | Requested detection index |
| `score` | `uint8_t` | Detection score |
| `total_detections` | `uint8_t` | Number of returned detections |
| `type` | `int16_t` | Detection type/class field |
| `yaw_global` | `float` | Global yaw in degrees |
| `pitch_global` | `float` | Global pitch in degrees |
| `rel_frame_of_reference` | `uint8_t` | Relative angle frame |
| `yaw_rel` | `float` | Relative yaw in degrees |
| `pitch_rel` | `float` | Relative pitch in degrees |
| `latitude` | `float` | Latitude of the detection |
| `longitude` | `float` | Longitude of the detection |
| `altitude` | `float` | Not filled by DigiView in this release |
| `distance` | `float` | Not filled by DigiView in this release |
| `width` | `float` | Detection width |
| `height` | `float` | Detection height |
| `track_id` | `uint16_t` | Tracked object ID |
| `publish_timestamp_us` | `uint64_t` | Detection publish timestamp |
| `view_id` | `uint8_t` | AI-view slot index |

### Behavior

- `GET` with `index = 255` requests all current detections.
- DigiView returns one response per detection.
- `track_id` is the stable ID to keep if you want to reference a detection later.
- If there are no detections, DigiView still responds with `CURRENT_PARAMETERS` and `total_detections = 0`.
- `latitude` and `longitude` may be returned when DigiView has those values available.
- `altitude` and `distance` are not filled by DigiView in this release.
- `SET` is not supported.

## `CAM_TARGETING`

Aim a selected camera by angle, coordinate, detection, or single-target-tracking mode.

### Fields

| Field | Type | Notes |
|---|---|---|
| `stream_name` | `char[16]` | Selected stream |
| `cam_id` | `uint8_t` | Selected camera/view |
| `targeting_mode` | `uint8_t` | Aiming mode |
| `euler_delta` | `bool` | Treat yaw/pitch/roll as relative changes |
| `yaw` | `float` | Degrees |
| `pitch` | `float` | Degrees |
| `roll` | `float` | Degrees |
| `lock_flags` | `uint8_t` | Axis lock bitmask |
| `x_offset` | `float` | Normalized horizontal offset |
| `y_offset` | `float` | Normalized vertical offset |
| `target_latitude` | `float` | Target coordinate latitude |
| `target_longitude` | `float` | Target coordinate longitude |
| `target_altitude` | `float` | Target coordinate altitude |
| `track_id` | `uint16_t` | Tracked object ID |
| `view_id` | `int16_t` | AI-view slot index |
| `lock_target` | `bool` | Request DigiView to lock onto the currently selected target |

### Targeting modes

| Value | Meaning |
|---:|---|
| 0 | Directional targeting |
| 1 | Coordinal targeting |
| 2 | Detection targeting |
| 3 | Single target tracking |

### Lock flags

`lock_flags` uses the lowest 3 bits:

- bit 0: yaw lock
- bit 1: pitch lock
- bit 2: roll lock

### Behavior

- `GET` returns the current targeting state for the selected stream and camera.
- `SET` updates targeting mode and the relevant fields for that mode.
- `roll` is available, but should not be treated as a primary control path.

## `CAM_OPTICS_AND_CONTROL`

Control camera FOV and relative zoom.

### Fields

| Field | Type | Notes |
|---|---|---|
| `stream_name` | `char[16]` | Selected stream |
| `cam_id` | `uint8_t` | Selected camera/view |
| `zoom` | `int8_t` | Relative zoom command |
| `fov` | `float` | Field of view in degrees |
| `crop_mode` | `uint8_t` | Crop mode enum |

### Crop mode values

| Value | Meaning |
|---:|---|
| 0 | None |
| 1 | Cartesian |
| 2 | Panorama |
| 3 | Lustiga huset |
| 4 | Full frame |

### Behavior

- `GET` returns the current optics state for the selected stream and camera.
- `SET zoom` acts as a relative change.
- `SET fov` acts as an absolute FOV request.
- In current DigiView 0.6 usage, crop mode changes should not be relied on as a primary customer workflow.
- Treat FOV and crop mode as the current state indicators rather than expecting a persistent absolute zoom value.

## `CAM_OFFSET`

Convert a point inside a camera view into global and relative angles.

### Fields

| Field | Type | Notes |
|---|---|---|
| `stream_name` | `char[16]` | Selected stream |
| `cam_id` | `uint8_t` | Selected camera/view |
| `x` | `float` | Normalized horizontal offset |
| `y` | `float` | Normalized vertical offset |
| `yaw_global` | `float` | Returned global yaw in degrees |
| `pitch_global` | `float` | Returned global pitch in degrees |
| `yaw_rel` | `float` | Returned relative yaw in degrees |
| `pitch_rel` | `float` | Returned relative pitch in degrees |

### Behavior

- `GET` returns the direction of a point in the selected view.
- `x` and `y` are normalized view offsets in the range `-1.0` to `1.0`.
- `stream_name` and `cam_id` must identify a valid view.
- `SET` is not supported.

## `SENSOR`

Control exposure, gain, and brightness targets.

### Fields

| Field | Type | Notes |
|---|---|---|
| `min_exposure` | `uint32_t` | Minimum exposure |
| `max_exposure` | `uint32_t` | Maximum exposure |
| `min_gain` | `uint32_t` | Minimum gain |
| `max_gain` | `uint32_t` | Maximum gain |
| `target_brightness` | `float` | Brightness target |

### Behavior

- `GET` returns current sensor settings.
- `SET` updates supported sensor settings.

## `CAM_DEPTH_ESTIMATION`

This parameter group exists in the message format.

### Fields

| Field | Type | Notes |
|---|---|---|
| `stream_name` | `char[16]` | Selected stream |
| `cam_id` | `uint8_t` | Selected camera/view |
| `depth_estimation_mode` | `uint8_t` | Reserved |
| `depth` | `float` | Reserved |

### Behavior

- DigiView does not provide a customer-facing depth estimation feature through this message group in this release.

## `SINGLE_TARGET_TRACKING`

Control and monitor DigiView's single target tracking mode.

### Fields

| Field | Type | Notes |
|---|---|---|
| `command` | `uint8_t` | Tracking command |
| `stream_name` | `char[16]` | Selected stream |
| `cam_id` | `uint8_t` | Selected camera/view |
| `x_offset` | `float` | Normalized horizontal target offset |
| `y_offset` | `float` | Normalized vertical target offset |
| `detection_id` | `uint8_t` | Current tracked object ID used by single-target tracking |
| `zoom_level` | `uint16_t` | Tracking zoom level |
| `confidence` | `float` | Current tracker confidence |
| `yaw_global` | `float` | Current target yaw in degrees |
| `pitch_global` | `float` | Current target pitch in degrees |
| `rel_frame_of_reference` | `uint8_t` | Relative angle frame |
| `yaw_rel` | `float` | Relative yaw in degrees |
| `pitch_rel` | `float` | Relative pitch in degrees |
| `publish_timestamp_us` | `uint64_t` | Tracking output publish timestamp |
| `status` | `uint8_t` | Tracking status |
| `lock_target` | `bool` | Request DigiView to lock onto the current target |

### Command values

| Value | Meaning |
|---:|---|
| 0 | Off |
| 1 | Set target vector |
| 2 | No operation |

### Status values

| Value | Meaning |
|---:|---|
| 0 | Off |
| 1 | Init |
| 2 | Running |
| 3 | Dropped |

### Behavior

- `GET` returns current tracking state, target direction, confidence, timestamp, and status.
- The most reliable 0.6 customer workflow is:
  - set a target vector
  - switch a camera to `CAM_TARGETING` mode `Single target tracking`
  - poll tracking status with `GET`
- `lock_target` can be set when you want DigiView to lock onto the current target.

## `CALIBRATION`

Start calibration and read current calibration progress.

This message group is intended for the MOSS One hardware platform.

### Fields

| Field | Type | Notes |
|---|---|---|
| `cam_id` | `uint8_t` | Selected camera |
| `calib_command` | `uint8_t` | Requested calibration action |
| `calib_status` | `uint8_t` | Current calibration state |

### Calibration command values

| Value | Meaning |
|---:|---|
| 0 | No operation |
| 1 | Start all calibration |
| 2 | Start 6DoF calibration |
| 3 | Start magnetometer calibration |

### Calibration status values

| Value | Meaning |
|---:|---|
| 0 | Not started |
| 1 | Hold +X orientation |
| 2 | Hold -X orientation |
| 3 | Hold +Y orientation |
| 4 | Hold -Y orientation |
| 5 | Hold +Z orientation |
| 6 | Hold -Z orientation |
| 7 | Magnetometer calibration in progress |

### Behavior

- `GET` returns current status for the selected camera.
- `SET` starts the requested calibration action.

## `NAVIGATION`

This parameter group exists in the message format, but DigiView does not produce it in this release.

### Fields

| Field | Type | Notes |
|---|---|---|
| `altitude` | `float` | Altitude above home |
| `visual_lat` | `float` | Not filled by DigiView in this release |
| `visual_lon` | `float` | Not filled by DigiView in this release |
| `next_waypoint_target_yaw` | `float` | Not filled by DigiView in this release |
| `next_waypoint_target_pitch` | `float` | Not filled by DigiView in this release |
| `next_waypoint_target_roll` | `float` | Not filled by DigiView in this release |
| `visual_vel_x` | `float` | Not filled by DigiView in this release |
| `visual_vel_y` | `float` | Not filled by DigiView in this release |
| `visual_vel_z` | `float` | Not filled by DigiView in this release |

### Behavior

- DigiView runtime does not produce `NAVIGATION` responses in this release.
- `SET` is not supported.
- In MAVLink integrations, requesting recurring `NAVIGATION_PARAMETERS` output with `SET_MESSAGE_INTERVAL` should be treated as unsupported in this release.

## MAVLink usage

DigiView 0.6 supports both standard MAVLink interaction and the Synclair custom MAVLink dialect.

## Standard MAVLink workflows

The standard MAVLink workflows most relevant to customers in DigiView 0.6 are:

- autopilot telemetry input such as attitude and global position
- gimbal-manager camera control
- `MESSAGE_INTERVAL` for recurring message output

### Practical standard MAVLink messages

| Type | Name | Purpose |
|---|---|---|
| Message | `ATTITUDE` | Vehicle attitude input |
| Message | `HIGHRES_IMU` | IMU input |
| Message | `GLOBAL_POSITION_INT` | Vehicle position input |
| Message | `HOME_POSITION` | Home position input |
| Message | `GIMBAL_MANAGER_SET_ATTITUDE` | Directional camera control |
| Message | `GIMBAL_MANAGER_SET_PITCHYAW` | Directional camera control |
| Message | `MESSAGE_INTERVAL` | Request periodic output |

## Synclair MAVLink dialect

The custom dialect mirrors the DigiView parameter groups with one MAVLink message per parameter family.

Examples include:

- `SYSTEM_STATUS_PARAMETERS`
- `AI_PARAMETERS`
- `VIDEO_OUTPUT_PARAMETERS`
- `TRACKED_DETECTION_PARAMETERS`
- `CAM_TARGETING_PARAMETERS`
- `SINGLE_TARGET_TRACKING_PARAMETERS`

### Conceptual mapping

- Native DigiView `GET_PARAMETERS` maps to requesting output for the corresponding parameter group.
- Native DigiView `SET_PARAMETERS` maps to sending the matching custom MAVLink parameter message.
- Native DigiView `CURRENT_PARAMETERS` maps to the matching outgoing custom MAVLink parameter message.

## MAVLink notes for 0.6

- The custom MAVLink dialect follows the same parameter families as the native protocol.
- Standard gimbal-manager messages are the recommended MAVLink path for camera pointing.
- Some fields available in the native protocol are newer than the current custom dialect and may not be exposed identically in MAVLink-based integrations.
- `NAVIGATION_PARAMETERS` remains part of the dialect, but DigiView runtime does not produce it in this release.
- For external integrations, validate the exact field set you depend on if you need both native DigiView and MAVLink compatibility.

## Recommended integration sequence

For a new client, the recommended sequence is:

1. read `SYSTEM_STATUS`
2. read `MODEL`
3. read `VIDEO_OUTPUT`
4. configure `AI`, `DETECTION`, and `VIDEO_OUTPUT` as needed
5. use `CAM_TARGETING`, `CAM_OPTICS_AND_CONTROL`, `TRACKED_DETECTION`, or `SINGLE_TARGET_TRACKING` for the mission workflow
