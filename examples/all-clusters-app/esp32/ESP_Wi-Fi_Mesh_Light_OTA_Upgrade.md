# ESP Wi-Fi Mesh Light OTA Upgrade

This document deals with upgrading the ESP32 Wi-Fi Mesh Light firmware with CHIP all-clusters-app firmware.

## Building the Firmware

1. Make sure that you are using [this](https://github.com/dhrishi/connectedhomeip/tree/esp32/meshkit_light_all_clusters) as the connectedhomeip repository. It has the required changes that support OTA upgrade of the default lightbulb firmware and further facilitate development on it.
2. Follow the instructions [here](https://github.com/dhrishi/connectedhomeip/tree/esp32/meshkit_light_all_clusters/examples/all-clusters-app/esp32#building-the-example-application) to compile the application. The binary (chip-all-clusters-app.bin) will be generated in `examples/all-clusters-app/esp32/build/` folder.

## Setting up ESP Wi-Fi Mesh Light

1. Download the ESP Mesh Android app v1.1.0 from [here](https://github.com/EspressifApp/EspMeshForAndroid/releases/tag/v1.1.0) and install it.
2. Turn on the ESP Wi-Fi mesh Light bulb, It should keep blinking yellow, meaning that it is in the setup mode.
	- If it isn’t in the setup mode, turn it off and on 3 times in quick succession (ON time about 1 sec, OFF time about 500 msec)
3. Open the ESP Mesh app. Tap on "Add Device"
4. You should see the bulb with a name like `MESH_1_8b8862`. Select it and tap Next
5. Enter your Wi-Fi network credentials (note that the this should be a normal AP and not a WPA2 Enterprise AP)
6. Tap on next. The bulb should then get configured to your Wi-Fi network and will show up in the device list. It should also stop blinking.


## Upgrading the ESP Wi-Fi Mesh Light

1. Copy the chip-all-clusters-app.bin built above to "File Management/Phone Storage/Espressif/Esp32/upgrade" on your phone.
2. Go back to the ESP-Mesh Android app.
3. Tap and hold on the device name. A list of options will popup from the bottom. Select "Firmware update"
5. It will then show you the chip-all-clusters-app.bin file that we had copied. Select it (it would mostly be selected by default)  and tap on the check mark on top right.
6. The Firmware update will then proceed and once it is completed, you will see the message "Devices upgraded". Tap on "Confirm"
7. Wait for some time as the light reboots.

If the above does not work, follow these steps

1. Host the chip-all-cluster-app.bin on a webserver. Simplest way is to start a python web server using `python -m SimpleHTTPServer 8070` (for python 2.x) OR `python -m http.server 8070` (for python 3.x)
2. In the ESP-Mesh app, Tap and hold on the device name. A list of options will popup from the bottom. Select "Firmware update".
3. Select `Enter bin url` at the bottom.
4. Give the address as `http://<webserver-ip-addr>/chip-all-clusters-app.bin`. (For python webserver recommended above, it would be `http://<host-ip-addr>:8070/chip-all-clusters-app.bin`.
5. Tap on `Send`.
6. The Firmware update will then proceed and once it is completed, you will see the message "Devices upgraded". Tap on "Confirm"
7. Wait for some time as the light reboots.

## Indicators

Once the lightbulb is upgraded with the CHIP firmware, it will reboot and start the breathe routine with the following colors for 3 seconds for different conditions indicated below. After 3 seconds, it will go back to the earlier state.

|Condition | Color |
|-----------|------|
| Bootup | Blue |
| Wi-Fi connect failed/disconnect | Red |
| OTA started | Cyan |
| OTA Failed | Orange |
| OTA succeeded | Green |

## Development

To execute the below commands during development, make sure that the host machine running these curl commands is connected to the SoftAP (while using the SoftAP interface - IP Address: 192.168.4.1) or to the Access Point to which the ESP32 is connected (while using the STA interface - IP Address can be found out using mDNS)

### Commands

- OTA Firmware Update (useful across multiple development cycles)
```
curl --data-binary '<ota-url>' 192.168.4.1/ota
```
For e.g.

i. You can start a webserver using Apache and place the new binary in its root directory (/var/www/html/)
```
curl --data-binary http://<webserver-ip-addr>/chip-all-clusters-app.bin 192.168.4.1/ota
```
ii. Alternatively, you can start a Python HTTP server mentioned above and place the binary in the same directory
```
curl --data-binary http://<webserver-ip-addr>:8070/chip-all-clusters-app.bin 192.168.4.1/ota
```

The status of OTA will be returned to the curl command and the appropriate indicator will be flashed on the light.
Please wait till the OTA operation completes. On success, the device will auto reboot in a few seconds with the new firmware and the bootup indicator will be flashed. If this does not happen, then manually turn it off and on again. On failure, do not retry to upgrade the device in the same boot cycle (to be on the safer side). Instead manually turn it off and on again before carrying out the operation.

- Reboot
```
curl -d '{"cmd":"reboot","val":"dummy"}' 192.168.4.1/command
```

- Get flash partitions' info
```
curl -d '{"cmd":"partition","val":"dummy"}' 192.168.4.1/command
``` 

Notes:
1. As mentioned above, even when light is connected to a network, you can use the curl commands. To find the IP address of the light, search for `_chip._tcp` mDNS service
2. The lightbulb will be off by default. You can change the state of the lightbulb (on/off, color, brightness) by using the chip-tool cluster commands. Currently, color change is supported through hue and saturation (ColorControl cluster) and brightness can be adjusted through level cluster.
