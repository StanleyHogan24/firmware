# Sthealth3 Firmware

This example contains the sources and project files for the **sthealth3** firmware built on top of the Nordic nRF5 SDK.

## Prerequisites

- **nRF5 SDK 17.1.0** – download and extract the SDK so that this project resides inside the `examples` folder as shown in the repository layout.
- **SEGGER Embedded Studio (SES)** – version 5 or later is recommended. This is used to build and debug the application.
- **nRF Command Line Tools** – provides `nrfjprog` which is used for flashing the resulting HEX file to a development kit.

## Building

1. Launch SEGGER Embedded Studio and open the project file
   `pca10040/s132/ses/ble_app_alert_notification_pca10040_s132.emProject`.
2. Select the desired build configuration (e.g. `Release` or `Debug`).
3. Build the project using **Build → Build Solution**.

Alternatively, the GCC Makefile located in `pca10040/s132/armgcc` can be
used by running `make` from the command line if the GNU Arm Embedded
Toolchain is installed.

The resulting firmware binary (`.hex`) can be found in the `hex` directory
or in the SES output directory after a successful build.

## Flashing

Connect your nRF52 DK and flash the firmware using `nrfjprog`:

```bash
nrfjprog --eraseall
nrfjprog --program hex/ble_app_alert_notification_pca10040_s132.hex --verify
nrfjprog --reset
```

Make sure the correct HEX file is chosen for your target board.

## Enabling BLE

BLE support requires that the SoftDevice is enabled and that the RAM
settings in the project match the SoftDevice's requirements. If BLE does
not start after flashing, open the project options in SEGGER Embedded
Studio and adjust the `RAM_START` and `RAM_SIZE` values so they align with
the SoftDevice memory configuration reported by the build log. After
updating the values, rebuild and flash the firmware again.