# Air Bubble Detection in Blood Transfusion

## Overview

Air Bubble Detection in Blood Transfusion is an ESP32-based embedded system designed to detect air bubbles in blood transfusion and intravenous (IV) fluid lines. The system utilizes infrared optical sensing to monitor fluid flow and identify bubble events in real time.

The project aims to provide a low-cost and reliable solution for improving patient safety by detecting potentially harmful air bubbles during transfusion procedures.

## Features

- Real-time air bubble detection
- Multi-channel monitoring (4 sensing channels)
- ESP32-based embedded platform
- Infrared optical sensing technology
- Low-cost and scalable design
- Serial monitoring and data acquisition
- Expandable for alarms, displays, and IoT integration

## Working Principle

Each sensing channel consists of:

- IR LED (Transmitter)
- Photodiode (Receiver)
- Transparent fluid tube

The IR LED emits infrared light through the fluid path. Under normal conditions, the blood or fluid absorbs and scatters part of the light. When an air bubble passes through the tube, the optical properties change, causing a variation in the amount of light reaching the photodiode.

The ESP32 continuously reads the sensor outputs and detects bubble events based on changes in the received signal.

## Hardware Components

- ESP32 Development Board
- IR LEDs
- Photodiodes
- 4.7kΩ / 10kΩ Resistors
- Transparent Medical Tubing
- Jumper Wires
- Breadboard / PCB
- Power Supply

## GPIO Configuration

| Sensor Channel | ESP32 GPIO |
|---------------|------------|
| Channel 1 | GPIO32 |
| Channel 2 | GPIO33 |
| Channel 3 | GPIO34 |
| Channel 4 | GPIO35 |


## Project Status

🚧 **Ongoing Project**

This project is currently under active development and testing. New features and improvements will be added in future updates.