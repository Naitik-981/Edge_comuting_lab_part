# Three-Layer Wireless Communication Using ESP32

This project demonstrates a scalable, robust, and real-time edge computing system using a three-layer wireless architecture. Multiple ESP32 sensor nodes send data to aggregator ESP32s, which forward aggregated information to an edge server (such as Arduino UNO R4 WiFi). The edge server hosts a real-time dashboard for monitoring and analysis.

## Features

- Hierarchical 3-layer wireless communication: Sensors → Aggregator → Edge Server
- Real-time temperature and humidity monitoring
- Modern live dashboard hosted on the edge device
- Efficient, scalable, and suitable for IoT and smart environments

## Repository Structure

```
.
├── architecture/
│   └── esp32-3layer-diagram.png
├── dashboard/
│   └── arduino_r4_dashboard_modern_fancy.ino
├── report/
│   └── Three_Layer_Wireless_Communication_ESP32_Report.md
├── examples/
│   ├── aggregator_esp32_example.ino
│   └── sensor_esp32_example.ino
└── README.md
```

## Quick Start

1. **Deploy Sensor (Layer 1) and Aggregator (Layer 2) code** from `examples/` to your ESP32 boards.
2. **Deploy dashboard code** from `dashboard/` to your Arduino UNO R4 WiFi (Layer 3).
3. View the dashboard in your browser using the Arduino’s IP address.
4. See `report/` for documentation and coursework report.

## System Architecture

![3-Layer Block Diagram](architecture/esp32-3layer-diagram.png)

---

## Documentation

- [Coursework Report](report/Three_Layer_Wireless_Communication_ESP32_Report.md)

---

## License

MIT License
