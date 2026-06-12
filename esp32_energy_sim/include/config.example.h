#pragma once

// Copy this file to config.h and set values for your local network.

#define WIFI_SSID     "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// MQTT broker IP of the PC running Docker.
// On Windows, run: ipconfig -> IPv4 Address
#define MQTT_BROKER   "192.168.1.100"
#define MQTT_PORT     1883

#define DEVICE_ID     "factory_01"

// Simulation parameters
#define BASE_KW       150.0f

// Timing
#define PUBLISH_INTERVAL_MS  5000UL
#define DEMAND_WINDOW_MS     (15UL * 60 * 1000)

// Keep this aligned with the threshold configured in your Node-RED alert flow.
#define ALERT_KW      130.0f
