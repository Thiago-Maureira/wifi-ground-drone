# 🚗 Wi-Fi Teleoperated Ground Drone — ESP32 + ESP32-CAM + WebSocket + GPS

> Remotely controlled ground vehicle with live video streaming, real-time GPS tracking,
> and WebSocket-based control — all served from the ESP32 itself, no cloud required.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/language-C%2FC%2B%2B-brightgreen)
![Protocol](https://img.shields.io/badge/protocol-Wi--Fi%20%7C%20WebSocket%20%7C%20HTTP-informational)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## How It Works

The system uses **two ESP32 modules** working together:

| Module | Role |
|---|---|
| **ESP32-WROOM-32** | Motor control, WebSocket server, web UI, GPS parsing |
| **ESP32-CAM** | Live MJPEG video stream on its own IP |

The main ESP32 creates a Wi-Fi access point and serves a **responsive web control panel** on port 80. A **WebSocket server** on port 81 receives real-time directional commands from the browser with minimal latency. The video stream from the ESP32-CAM is embedded as an `<iframe>` in the same control panel — a distributed architecture that avoids memory bottlenecks on a single MCU.

```
Browser ──HTTP(80)──► ESP32 (AP mode)   serves HTML control panel
Browser ──WS(81)───► ESP32              receives movement commands → L298N → Motors
Browser ──HTTP──────► ESP32-CAM         live MJPEG stream (iframed in control panel)
ESP32   ──UART──────► GPS NEO-6M        parses NMEA: lat, lon, speed, altitude, HDOP
```

---

## Hardware

| Component | Qty | Notes |
|---|---|---|
| ESP32-WROOM-32 | 1 | AP + web server + motor control + GPS |
| ESP32-CAM (AI-Thinker) | 1 | Live video stream |
| L298N H-Bridge | 1 | Dual motor driver |
| DC Motors (TT gear motors) | 2 | Left / Right drive |
| GPS NEO-6M | 1 | UART2 — pins GPIO16 (RX), GPIO17 (TX) |
| LED (headlight) | 1 | Controlled via transistor on GPIO13 |
| LiPo / power bank | 1 | Vehicle power supply |

### Pin Mapping — ESP32-WROOM-32

| Function | GPIO |
|---|---|
| Motor 1 — IN1 | 27 |
| Motor 1 — IN2 | 26 |
| Motor 2 — IN3 | 25 |
| Motor 2 — IN4 | 33 |
| LED / Headlight | 13 |
| GPS RX (from NEO-6M TX) | 16 |
| GPS TX (to NEO-6M RX) | 17 |

---

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    Browser (phone / PC)                   │
│   Control Panel ──── WebSocket commands ────────────┐    │
│   GPS Panel     ──── HTTP polling                   │    │
│   Video iframe  ──── MJPEG stream from ESP32-CAM    │    │
└─────────────────────────────────┬───────────────────┘    │
                                  │                         │
              ┌───────────────────▼──────────────────┐      │
              │          ESP32-WROOM-32 (AP)          │      │
              │  WiFi AP: "TM"                        │      │
              │  WebServer (port 80) — serves HTML    │      │
              │  WebSocketsServer (port 81)           │      │
              │  TinyGPS++ — parses NMEA from NEO-6M  │      │
              └────────┬──────────────┬───────────────┘      │
                       │              │                       │
              ┌────────▼───┐   ┌─────▼──────┐               │
              │   L298N    │   │  GPS NEO-6M│               │
              │  H-Bridge  │   │  UART2     │               │
              └──┬──────┬──┘   └────────────┘               │
              Motor1  Motor2                                 │
                                                            │
              ┌─────────────────────────────────────────────▼─┐
              │              ESP32-CAM (AI-Thinker)            │
              │   Runs Arduino CameraWebServer example         │
              │   MJPEG stream on http://[cam-ip]/stream       │
              └────────────────────────────────────────────────┘
```

---

## Software Dependencies

```cpp
// Main ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>   // Links2004/arduinoWebSockets
#include <TinyGPS++.h>          // mikalhart/TinyGPSPlus
```

Install via Arduino Library Manager:
- **arduinoWebSockets** by Markus Sattler (Links2004)
- **TinyGPS++** by Mikal Hart
- **ESP32 Arduino Core** — includes WiFi, WebServer

For the ESP32-CAM:
- Flash the built-in **CameraWebServer** example from Arduino IDE (File → Examples → ESP32 → Camera → CameraWebServer)
- Set your AP SSID/password to match the main ESP32's network

---

## Setup & Flash Instructions

```bash
# ── Main ESP32 ──────────────────────────────────────────────
# 1. Open wifi_drone/wifi_drone.ino in Arduino IDE
# 2. Board: ESP32 Dev Module
# 3. Upload
# 4. Connect to Wi-Fi AP "TM" (password: see .env or ask maintainer)
# 5. Open browser → http://192.168.4.1

# ── ESP32-CAM ───────────────────────────────────────────────
# 1. File → Examples → ESP32 → Camera → CameraWebServer
# 2. Set SSID/password to match the drone's AP
# 3. Board: AI Thinker ESP32-CAM
# 4. Flash via FTDI adapter (GPIO0 to GND during flash)
# 5. Note the CAM's IP from Serial Monitor
# 6. Update CAM_IP in the main firmware if needed
```

---

## Performance

| Metric | Result |
|---|---|
| Video stream latency | < 300 ms |
| Control range (open area) | ~200 m |
| WebSocket command latency | < 20 ms |
| LED power reduction vs always-on | ~40% (transistor switching) |
| GPS fix time (cold start) | ~45 s |
| GPS data: fields | Lat, Lon, Speed (km/h), Altitude (m), HDOP, Satellites, Date, Time |

---

## Web Control Panel Features

- **Directional pad** — Forward / Backward / Left / Right / Stop
- **LED toggle** — Headlight on/off
- **GPS live panel** — Coordinates, speed, altitude, HDOP, satellite count, date/time
- **Video feed** — ESP32-CAM stream embedded in the same page
- Responsive layout: works on mobile and desktop

---

## Security Note

The AP credentials are hardcoded in the firmware for simplicity (educational project). For any deployment, move them to a config header excluded from version control.

---

## Future Improvements

- [ ] Replace fixed AP with STA mode + mDNS for easier discovery
- [ ] Add PWM speed control (currently full speed only)
- [ ] Stream GPS coordinates to a map (Leaflet.js integration)
- [ ] OTA firmware updates
- [ ] Battery voltage monitoring via ADC

---

## Authors

- **Thiago Maureira Garcia** — Full firmware, web UI, electronics, GPS integration
- **Team members** — Assembly, field testing

**Institution:** E.E.S.T. N°2 "Ing. César Cipolletti" — Bahía Blanca, Argentina (2024)

---

## License

MIT — free to use, modify and distribute with attribution.
