# ESP32 Tesla BLE TPMS Reader v0.3

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)

A reverse-engineered tool designed to sniff and decode Bluetooth Low Energy (BLE) payloads from Tesla Model 3 and Model Y TPMS sensors using any ESP32 microcontroller.

## 📖 Overview
This project allows you to monitor tire pressure, temperature, and battery levels of Tesla BLE TPMS sensors. It features a robust command-line interface via Serial Monitor and persistent data logging in the ESP32 Flash memory (NVS).

## ✨ Features
- **Full Compatibility:** Works with both first-gen and "Highland" Model 3/Y sensors.
- **Real-time Decoding:** Provides Pressure (PSI/Bar), Temperature (°F/°C), and Battery voltage (mV).
- **Advanced Filtering:** Toggle between Tesla-only mode or raw BLE sniffing.
- **Data Persistence:** Automatically stores total readings and unique MAC addresses.
- **Interactive Shell:** Comprehensive command system to manage scans and memory.
- **Power Management:** Includes a Deep Sleep command for energy saving.

## 🚗 Vehicle Compatibility
| Model | Compatibility | Protocol |
| :--- | :--- | :--- |
| **Model 3 / Model Y** | ✅ Supported | BLE (2.4GHz) |
| **Model 3 Highland** | ✅ Supported | BLE (2.4GHz) |
| **Model S / Model X** | ❌ Not Supported | 433 MHz (Legacy) |

## 🛠 Hardware & Setup
- **Core:** Built for **ESP32 Arduino Core v3.3.8**.
- **Tested Hardware:** ESP32-D0WD-V3 (v3.1), ESP32U, ESP32-C3, ESP32-S3.
- **Partition Scheme:** If the sketch exceeds space or crashes, use **"Huge APP (3MB No OTA)"**.
- **Serial Baudrate:** 115200

## 🎮 Commands
Open your Serial Monitor and type `help` to see the full list:

| Command | Description |
| :--- | :--- |
| `help` | Show available commands. |
| `scan on/off` | Start or stop the BLE background scan. |
| `scan <MAC>` | Probe a specific MAC for 15 seconds. |
| `filter on/off`| Enable (Tesla only) or Disable (All BLE devices) filtering. |
| `debug on/off` | Show/Hide raw hex payloads. |
| `list` | Print the stored database of unique MAC addresses. |
| `reset <p>` | Reset `total` counts, `list` of MACs, or `all` (NVS wipe). |
| `uptime` | Show time since boot. |
| `about` | Show device status and version. |

> [!TIP]
> **Sleep Mode:** If the tire is not moving or has no pressure, the sensor transmits very little data. To trigger a sensor on a workbench, apply a **125kHz square wave** nearby to wake it up.

## 🔬 Technical Insight
The decoding logic identifies the payload start at `0x2B 0x02`. The pressure is calculated using the following formula:
$$P = \frac{(raw\_value - 100)}{7}$$

## 🤝 Contributing & Feedback
This project is based on reverse engineering and might require fine-tuning for different sensor batches. Feel free to open an issue or submit a pull request!

---
*Created by Conny (c) 2026*
