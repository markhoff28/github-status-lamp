# GitHub Status Lamp

Standalone firmware for a **Seeed Studio XIAO ESP32C3** and a **WS2812B LED strip**.
The lamp polls GitHub's public system status and displays it using colored LEDs.
No GitHub token, WLED, or custom server is required.

## Features

- GitHub status over HTTPS with certificate verification, polled every 60 seconds.
- Green, yellow, orange, red, or blue depending on the status.
- Wi-Fi provisioning through a password-protected setup access point.
- LED count (1–300) and brightness (0–255) configurable through the web interface.
- Settings persist across restarts.
- Web interface on your home network at http://github-status-lamp.local.
- Automatic reconnection; stale or invalid status data is displayed in blue.

## Hardware

| Connection | Wiring |
| --- | --- |
| XIAO D2 / GPIO4 | LED strip DIN |
| GND | Common ground for the XIAO, strip, and power supply |
| LED +5 V | Suitable external 5 V power supply |

Do not power the strip from the 3V3 pin. When powering the XIAO through USB and the
LED strip from a separate supply, do not also connect the external 5 V line to
the XIAO. Connect the XIAO's external Wi-Fi antenna.

For a reliable data signal, a 3.3 V to 5 V level shifter (such as a 74AHCT125)
and a 330–470 ohm resistor in the data line to DIN are recommended.

The main firmware uses **Adafruit NeoPixel**. In a hardware comparison test,
this driver worked, while the tested strip remained white with FastLED.
Both diagnostic variants are included for comparison.

## Local setup

Install PlatformIO Core or the PlatformIO extension for VS Code.
Open the project directory in VS Code or run the following commands from it.

1. Copy `include/secrets.example.h` to `include/secrets.h`.
2. Set `LAMP_SETUP_PASSWORD` to your own password containing 8–63 ASCII characters.
3. Build the firmware and upload it over USB:

```sh
pio run
pio device list
pio run -t upload
pio device monitor -b 115200
```

If multiple devices are connected, specify the port explicitly, for example
`pio run -t upload --upload-port COM4` on Windows or
`--upload-port /dev/ttyACM0` on Linux.

`secrets.h`, local IDE files, and build outputs are excluded from Git.
The setup password is also embedded in the compiled firmware, so do not publish
your local firmware binaries. The public CA certificate in
`include/github_status_ca.h` is not a secret.

## Setting up the lamp

1. Connect to **GitHub-Status-Lamp-Setup** using the password from your local
   `include/secrets.h`.
2. Open **http://192.168.4.1** and use **Configure WiFi** to connect the lamp to
   your 2.4 GHz Wi-Fi network.
3. Under **Setup**, adjust the LED count and brightness, then press **Save**.
   Settings take effect immediately and are saved persistently.
4. Once connected and its clock is synchronized, the lamp displays GitHub's status.

On your home network, open **http://github-status-lamp.local** or use the IP address
assigned by your router. Direct link to the LED settings:
**http://github-status-lamp.local/param**. Resolution of `.local` names depends on
your network and client; use the IP address if it does not work.

LED count means individual LEDs, not strip length in meters. Brightness 0 turns
the LEDs off. Defaults are 30 LEDs and brightness 50 out of 255. The GPIO pin,
defaults, timing intervals, and current limit are defined in `include/config.h`.

The estimated current limit is 500 mA. The firmware assumes 1 mA of idle current
per configured LED and up to 20 mA per color channel, reducing brightness when
necessary. This does not replace a suitably rated power supply.

The setup access point starts on every boot and remains active after saving.
**Close** shuts down the portal. Restart the device, hold **BOOT** for three
seconds while it is running, or send the serial command `p` to reopen it.
Do not hold BOOT during power-on, as this enters download mode.

The hotspot password protects Wi-Fi access. The configuration page itself has no
additional login and is also accessible on your home network. Use it only on
trusted networks and do not forward its port to the internet.
Wi-Fi credentials are stored on the ESP, not in project files.

## Status display and controls

| Status | Display | Serial color test |
| --- | --- | --- |
| Operational / `none` | Steady green | `1` |
| `minor` | Slowly pulsing yellow | `2` |
| `major` | Faster pulsing orange | `3` |
| `critical` | Blinking red | `4` |
| Unknown / startup | Blue | `0` |

Color tests end after 30 seconds; `a` immediately restores the live status display.
`p` opens the portal, and `s` prints network and BOOT button diagnostics.

HTTPS requests run in a separate FreeRTOS task. Network, HTTP, and JSON errors,
as well as unrecognized status values, result in Unknown. Results become stale
after 120 seconds. LED animations use `millis()`; saving Wi-Fi settings may
briefly pause the main loop.

Certificate verification requires an NTP-synchronized clock and uses ISRG Root X1.
If the endpoint changes its certificate authority, update
`include/github_status_ca.h`. Response bodies are limited to 4096 bytes.
OTA updates and individual GitHub component statuses are not implemented yet.

## Tests

Run the Unity tests on the connected board:

```sh
pio test -e seeed_xiao_esp32c3
pio run -e seeed_xiao_esp32c3 -t upload
```

The tests cover status mapping, invalid JSON, LED count validation, and status
freshness, including timer rollover. They temporarily replace the application;
upload the main firmware again afterward.

The standalone hardware test runs without Wi-Fi. It cycles through off, red,
green, and blue, spending five seconds on each. The first ten LEDs use brightness
32 out of 255.

```sh
# Driver confirmed working in the hardware comparison:
pio run -e led_diagnostic_neopixel -t upload
# Alternative FastLED test:
pio run -e led_diagnostic -t upload
# Restore the main application:
pio run -e seeed_xiao_esp32c3 -t upload
```

Diagnostic tests do not change stored Wi-Fi or LED settings.
Color changes and persistence of LED count and brightness have been tested on
the XIAO. Comprehensive long-term and network failure testing is still pending.

## Publishing as a new GitHub repository

Create an **empty** repository named `github-status-lamp` on GitHub without adding
a README, license, or gitignore. Then run these commands in the local project directory:

```sh
git remote add origin https://github.com/YOUR-USERNAME/github-status-lamp.git
git push -u origin main
```

The prepared local repository already contains the initial commit.
Before future commits, review your changes with `git status` and `git diff --cached`.

## License

No license has been selected for the original project code.
Dependencies and third-party materials remain subject to their respective licenses;
dependency source copies are not bundled in this repository.

## References

- [Seeed: XIAO ESP32C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [GitHub Status API](https://www.githubstatus.com/api)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [PlatformIO board documentation](https://docs.platformio.org/en/stable/boards/espressif32/seeed_xiao_esp32c3.html)
