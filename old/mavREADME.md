# sv_mavlink_dialect
SynclairVisions custom MAVLink dialect with generated C headers and a Python module.

## Layout
- `v.0.x/xml/sv_msg_defs.xml`: source XML for each dialect version.
- `v.0.x/generated/c`: header-only C implementation.
- `v.0.x/generated/python.py`: Python implementation.
- `docs.md`: extra notes/background.

Use the highest available version (currently `v.0.6`) unless you must match an older system.

## C headers
1. Copy `v.0.x/generated/c` into your project, or add it to your compiler include path.
2. Include the main header:
   ```c
   #include "sv_msg_defs/mavlink.h"
   ```
3. Build with the include path pointing to `generated/c`:
   ```sh
   gcc -I path/to/generated/c demo.c -o demo
   ```
   The code is header-only; no extra libraries are required.

### QGroundControl example
1. Ensure `qgroundcontrol/custom-example` is renamed to `qgroundcontrol/custom`.
2. In `qgroundcontrol/custom/cmake/CustomOverrides.cmake`, point CPM to the generated C root:
   ```cmake
   set(CPM_mavlink_SOURCE "/path/to/sv_mavlink_dialect/v.0.x/generated/c")
   ```
3. Rebuild QGC with your usual toolchain, e.g.:
   ```sh
   ~/Qt/<qt_version>/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug
   ./build/Debug/Custom-QGroundControl
   ```

## Python module
1. Copy `v.0.x/generated/python.py` into your project (you can rename it to `sv_mavlink.py`).
2. Make sure its directory is on `PYTHONPATH` (placing it next to your script works).
3. Import and use:
   ```python
   import sv_mavlink as mav  # if you renamed the file
   print("Dialect:", mav.DIALECT, "Wire:", mav.WIRE_PROTOCOL_VERSION)
   ```

# Generating Manually

If you need to modify the message definitions or regenerate the code, you must use the mavgen tool. The simplest way is to clone the mavlink repo and generate from there.


Clone the mavlink repo
```sh
# Clone mavlink into the directory of your choice
git clone https://github.com/mavlink/mavlink.git --recursive
cd mavlink

python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r pymavlink/requirements.txt
```
Avoiding Message ID Clashes

Crucial: When generating this dialect, it must be processed together with the standard mavlink/all.xml definitions.

We generate sv_mavlink_dialect alongside all.xml to ensure that:

 All standard MAVLink enums and messages are available to the dialect.
 There are no clashes between Message IDs. The generator validates that our custom SynclairVisions messages do not overlap or conflict with standard MAVLink message IDs.

# Add the sv_msg_defs.xml
Once you are in the mavlink repo. Enter the message_definitions/v1.0 directory. 
Here you will see all the official messages that are maintained by the mavlink team.

1. Add your sv_msg_defs.xml into the message_definitions/v1.0 directory.

2. Edit the message_definitions/v1.0/all.xml by adding the sv_msg_defs.xml as an include:
<include>sv_msg_defs.xml</include>

Now the dialect is added to all.xml and will be generated together with all official messages. 
This ensures that we dont have any message conflicts.

# Generation Commands

To generate the dialect, run the following commands from the mavlink root:

For C Headers:
```sh
python3 -m pymavlink.tools.mavgen \
    --lang=C \
    --wire-protocol=2.0 \
    --output=path/to/where/you/want/the/output/to/be/generated \
    message_definitions/v1.0/all.xml
```
For Python:
```sh

python3 -m pymavlink.tools.mavgen \
    --lang=Python \
    --wire-protocol=2.0 \
    --output=path/to/where/you/want/the/output/to/be/generated/python.py \
    message_definitions/v1.0/all.xml
```

Now you have all messages including the sv_msg_defs mavlink dialect in your:
path/to/where/you/want/the/output/to/be/generated

Nice.
