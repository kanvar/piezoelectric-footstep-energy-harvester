# Piezoelectric Footstep Energy Harvester

A working prototype of an IoT-enabled sidewalk energy harvesting system that converts mechanical energy from footsteps into electrical power. Built for ME 100 using Python and OnShape.

The system integrates an ESP32 microcontroller, INA219 power sensor, and environmental sensors to monitor generated power and atmospheric conditions in real time. Data is displayed externally in a data hub, enabling active evaluation of energy generation and system performance.

## The Power Tiles Ecosystem

The system is modular — each tile has a specific role, and configurations can be custom-ordered to prioritize advertising, power generation, or data collection.

**Motherboard** — The core tile that integrates everything else into the ecosystem. Includes solar panels, piezo buzzers, OLED displays, an IR flame sensor, and humidity/temperature sensors. Collects power while surfacing live info like temperature, humidity, power generation, and safety markings or advertisements on the display.

**Daughterboard** — Configurable to whatever the use case calls for: edge-to-edge LCD/OLED for a giant floor display, or edge-to-edge solar for max power generation. Piezo buzzers are included in every configuration. Motherboard and daughterboards connect through POGO pins (satisfying click included). The number of panels you can chain is effectively infinite — each one generates enough to support itself, so you could line a mall or Times Square with them.

**Receiver** — Sends and receives output data from the tiles. Displays voltage from the piezo and solar panels, has speakers that can hook into a larger system for emergency alerts, and an LCD a technician can use to read live data.

## Repo Contents

- `/code` — Python firmware for the ESP32, sensor integration, and data hub
- `/cad` — OnShape CAD files for the motherboard, daughterboard, and receiver

## Tech

ESP32 · INA219 · Python · OnShape

---

Built by Kanvar Soin, Eduardo Reyes Arciga, and Rajdeep Summan
