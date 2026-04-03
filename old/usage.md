# Sample usage of messages
This document provides examples of how the messages in msg_defs can be used in practice. Each example assumes a connected system consisting of a drone with autopilot, a ground station and a Synclair Vision camera.

## Example: Follow target
In this case, we want to signal the drone to follow a target that has been detected by the camera. The ground station asks the camera for all current detections and their positions. Once the ground station has this information, it can send a command to the drone to follow the detected target.

```cpp
// Ground station requests current detections from the camera using DETECTED_ROI

// Create and pack message to send
message request;
pack_get_detected_roi_all(request);

// Send the request to the camera (this is a pseudo function)
send_message_to_camera(request);

...

// Receiver function on ground station
on_message_received(response) {
    if (response.message_type == DETECTED_ROI) {
        // Unpack the response message
        detected_roi_params params;
        unpack_detected_roi_parameters(response, params);

        // Use some logic to determine which target to follow
        if (params.score < 0.5) {
            // Ignore low-confidence detections
            return;
        }

        // Now send a command to the drone to follow this target
        mavlink_message_t follow_command;
        pack_set_target(follow_command, target.latitude, target.longitude, target.altitude);

        // Or, use direction to the target
        pack_move_towards(follow_command, target.yaw_abs, 0); // Set pitch to 0 for horizontal movement
        //pack_move_towards(follow_command, target.yaw_abs, target.pitch_abs); // Fly directly towards the target >:)
    }
}
```

## Example: Point and move
In this case the ground station could be configured to allow the user to click on a point in the video feed which would then be used to move the drone to that point. The message CAM_OFFSET can be used to retrieve the direction of a point in the video feed relative to the center of the image. This direction can then be used to move the drone.

```cpp
char stream_name[] = "stream";
uint8_t cam_id = 0; // Assuming single camera with ID 0

// Check if user has clicked on a point in the video feed
int x, y;
get_user_click_coordinates(x, y); // Pseudo function to get user click coordinates

// Create and pack message to request offset
message request;
pack_get_cam_offset(request, stream_name, cam_id, x, y);

// Send the request to the camera
send_message_to_camera(request);
...

// Receiver function on ground station
on_message_received(response) {
    if (response.message_type == CAM_OFFSET) {
        // Unpack the response message
        cam_offset_params params;
        unpack_cam_offset_parameters(response, params);

        // Use the offset to move the drone
        mavlink_message_t move_command;
        pack_move_towards(move_command, params.yaw_abs, params.pitch_abs);

        // Or, to avoid crashing into things, only move in the horizontal plane
        //pack_move_towards(move_command, params.yaw_abs, 0);

        // Send the move command to the drone
        send_message_to_drone(move_command);
    }
}
```