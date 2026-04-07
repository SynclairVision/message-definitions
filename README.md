# DigiView Message Definitions

This repository contains the official DigiView message definitions used by Synclair Vision systems and third-party integrations. It brings together protocol documentation, native definitions, and MAVLink dialect data so integrators can work from a consistent source.

## Repository contents and where to start

Start with **`message-definitions.md`**.
It is the main reference for message behavior, fields, and expected usage.

You will also find:

- **`msg_defs.hpp`**  
  Native DigiView message and parameter definitions for platform-native integrations.

- **`sv_mavlink_dialect.xml`**  
  DigiView MAVLink dialect definition used to generate MAVLink bindings.

Generated MAVLink code is intentionally not stored in this repository; it is typically generated and versioned in the consuming project.

## CMake integration

CMake-based projects can include this repository with `add_subdirectory(...)`.
When included, the build generates DigiView MAVLink C headers in your build tree using the vendored MAVLink submodule.

Before configuring CMake, initialize submodules (for example):

`git submodule update --init --recursive`

## MAVLink bindings generation guidance

Integrators who need language-specific MAVLink bindings can generate them from **`sv_mavlink_dialect.xml`** using `mavgen`, either with the manual flow below or with the helper script **`generate_sv_mavlink_bindings.sh`**.

Use the dialect together with MAVLink's standard **`message_definitions/v1.0/all.xml`**.
Here, `message_definitions/v1.0/` means the directory in your local clone of the official MAVLink repository, not this DigiView `message-definitions` repository.
Generating from `all.xml` is recommended because it:

- includes standard MAVLink messages and enums during generation
- checks DigiView message IDs against the standard MAVLink set to catch ID conflicts early

Typical manual integration flow:

1. Use a local clone of the official MAVLink repository.
2. Place `sv_mavlink_dialect.xml` in `message_definitions/v1.0/`.
3. Add `<include>sv_mavlink_dialect.xml</include>` to `message_definitions/v1.0/all.xml`.
4. Run `mavgen` for your target language(s) using `message_definitions/v1.0/all.xml` as input.

If you prefer an automated flow, run `generate_sv_mavlink_bindings.sh` to handle cloning (or reusing) the MAVLink repository, copying `sv_mavlink_dialect.xml`, updating `all.xml`, and running `mavgen` for you.
