# GitHub Status Lamp

Eigenständige Firmware für einen **Seeed Studio XIAO ESP32C3** und einen
**WS2812B-LED-Streifen**. Die Lampe fragt den öffentlichen GitHub-Systemstatus ab
und zeigt ihn farbig an. Kein GitHub-Token, WLED oder eigener Server erforderlich.

## Funktionen

- GitHub-Status über HTTPS mit Zertifikatsprüfung, Abfrage alle 60 Sekunden.
- Grün, Gelb, Orange, Rot oder Blau je nach Status.
- WLAN-Einrichtung über ein passwortgeschütztes Setup-WLAN.
- LED-Anzahl (1–300) und Helligkeit (0–255) über die Webseite einstellbar.
- Einstellungen bleiben nach einem Neustart erhalten.
- Webseite im Heimnetz unter http://github-status-lamp.local.
- Automatische Wiederverbindung; veraltete oder ungültige Statusdaten werden blau angezeigt.

## Hardware

| Anschluss | Verbindung |
| --- | --- |
| XIAO D2 / GPIO4 | DIN des LED-Streifens |
| GND | Gemeinsame Masse von XIAO, Streifen und Netzteil |
| LED +5 V | Passendes externes 5-V-Netzteil |

Den Streifen nicht über den 3V3-Pin versorgen. Bei USB-Versorgung des XIAO und
separater LED-Versorgung die externe 5-V-Leitung nicht zusätzlich an den XIAO
anschließen. Die externe WLAN-Antenne des XIAO anschließen.

Für eine zuverlässige Datenleitung sind ein 3,3-V-auf-5-V-Pegelwandler
(z. B. 74AHCT125) und ein Widerstand von 330–470 Ohm zum DIN empfehlenswert.

Die reguläre Firmware nutzt **Adafruit NeoPixel**. Im Hardware-Vergleichstest
funktionierte diese Ansteuerung; mit FastLED blieb der getestete Streifen weiß.
Beide Diagnosevarianten sind zum Vergleich enthalten.

## Lokal einrichten

Voraussetzung ist PlatformIO Core oder die PlatformIO-Erweiterung für VS Code.
Das Projektverzeichnis in VS Code öffnen oder die folgenden Befehle darin ausführen.

1. `include/secrets.example.h` nach `include/secrets.h` kopieren.
2. Darin `LAMP_SETUP_PASSWORD` durch ein eigenes Passwort mit 8–63 ASCII-Zeichen ersetzen.
3. Firmware bauen und über USB aufspielen:

```sh
pio run
pio device list
pio run -t upload
pio device monitor -b 115200
```

Bei mehreren Geräten den Anschluss ausdrücklich angeben, beispielsweise
`pio run -t upload --upload-port COM4` unter Windows oder
`--upload-port /dev/ttyACM0` unter Linux.

`secrets.h`, lokale IDE-Dateien und Build-Ausgaben sind von Git ausgeschlossen.
Das Setup-Passwort steckt auch in der kompilierten Firmware; lokale Binärdateien
deshalb nicht veröffentlichen. Das öffentliche CA-Zertifikat in
`include/github_status_ca.h` ist dagegen kein Geheimnis.

## Einrichtung der Lampe

1. Mit **GitHub-Status-Lamp-Setup** verbinden. Das Passwort steht in deiner lokalen
   `include/secrets.h`.
2. **http://192.168.4.1** öffnen und unter **Configure WiFi** das eigene
   2,4-GHz-WLAN einrichten.
3. Unter **Setup** die LED-Anzahl und Helligkeit einstellen und **Save** drücken.
   Die Werte werden sofort übernommen und dauerhaft gespeichert.
4. Nach Verbindung und Zeitsynchronisierung zeigt die Lampe den GitHub-Status.

Im Heimnetz ist die Seite unter **http://github-status-lamp.local** oder der
vom Router vergebenen IP erreichbar. Direkt zu den LED-Einstellungen:
**http://github-status-lamp.local/param**. Die Auflösung von `.local` hängt vom
Netzwerk und Client ab; bei Problemen die IP verwenden.

Die LED-Anzahl bezeichnet einzelne LEDs, nicht Meter. Helligkeit 0 bedeutet aus.
Startwerte sind 30 LEDs und Helligkeit 50 von 255. GPIO, Standardwerte,
Zeitintervalle und Stromlimit stehen in `include/config.h`.

Das geschätzte Stromlimit beträgt 500 mA. Die Firmware berücksichtigt 1 mA
Ruhestrom pro eingestellter LED und bis zu 20 mA je Farbkanal und reduziert
gegebenenfalls die Helligkeit. Das ersetzt keine passend dimensionierte Versorgung.

Das Setup-WLAN startet bei jedem Einschalten und bleibt nach dem Speichern aktiv.
**Close** beendet das Portal; ein Neustart, drei Sekunden **BOOT** am laufenden
Gerät oder der serielle Befehl `p` öffnen es erneut. BOOT nicht während des
Einschaltens gedrückt halten, sonst startet der Downloadmodus.

Das Hotspot-Passwort schützt den WLAN-Zugang. Die Konfigurationsseite selbst hat
keine zusätzliche Anmeldung und ist auch im Heimnetz erreichbar. Nur in
vertrauenswürdigen Netzen verwenden, keine Portweiterleitung ins Internet einrichten.
WLAN-Zugangsdaten werden auf dem ESP gespeichert, nicht in Projektdateien.

## Status und Bedienung

| Status | Anzeige | Serieller Farbtest |
| --- | --- | --- |
| Operational / `none` | Grün, konstant | `1` |
| `minor` | Gelb, langsam pulsierend | `2` |
| `major` | Orange, schneller pulsierend | `3` |
| `critical` | Rot blinkend | `4` |
| Unknown / Start | Blau | `0` |

Farbtests enden nach 30 Sekunden; `a` kehrt sofort zur Live-Anzeige zurück.
`p` öffnet das Portal, `s` zeigt Netzwerk- und BOOT-Tasten-Diagnosedaten.

HTTPS läuft in einem separaten FreeRTOS-Task. Netzwerk-, HTTP- und JSON-Fehler
sowie unbekannte Statuswerte führen zu Unknown. Ein Ergebnis veraltet nach
120 Sekunden. Die LED-Animation läuft mit `millis()`; eine WLAN-Einrichtung
kann beim Speichern kurz warten.

Die Zertifikatsprüfung benötigt eine per NTP synchronisierte Uhr und nutzt
ISRG Root X1. Bei einem Wechsel der Zertifizierungsstelle muss
`include/github_status_ca.h` aktualisiert werden. Antwortkörper sind auf
4096 Bytes begrenzt. OTA und einzelne GitHub-Komponenten sind noch nicht implementiert.

## Tests

Unity-Tests auf dem angeschlossenen Board:

```sh
pio test -e seeed_xiao_esp32c3
pio run -e seeed_xiao_esp32c3 -t upload
```

Die Tests prüfen Status-Mapping, fehlerhafte JSON-Daten, LED-Anzahl und Statusalter
einschließlich Timerüberlauf. Sie ersetzen vorübergehend die Anwendung; anschließend
die reguläre Firmware wieder aufspielen.

Für einen Hardwaretest ohne WLAN: fünf Sekunden Aus, Rot, Grün und Blau im Wechsel,
erste zehn LEDs mit Helligkeit 32 von 255.

```sh
# Funktionierender Vergleichstreiber:
pio run -e led_diagnostic_neopixel -t upload
# Alternativer FastLED-Test:
pio run -e led_diagnostic -t upload
# Reguläre Anwendung wiederherstellen:
pio run -e seeed_xiao_esp32c3 -t upload
```

Die Diagnosetests verändern gespeicherte WLAN- und LED-Einstellungen nicht.
Hardware-Farbwechsel und Speichern von LED-Anzahl/Helligkeit wurden am
XIAO geprüft. Ein umfassender Langzeit- und Netzwerkausfalltest steht noch aus.

## Als neues GitHub-Repository veröffentlichen

Auf GitHub ein **leeres** Repository `github-status-lamp` anlegen, ohne zusätzliche
README, Lizenz oder Gitignore. Danach im lokalen Projektordner:

```sh
git remote add origin https://github.com/DEIN-USERNAME/github-status-lamp.git
git push -u origin main
```

Das lokal vorbereitete Repository enthält bereits den ersten Commit.
Vor späteren Commits mit `git status` und `git diff --cached` die Änderungen prüfen.

## Lizenz

Für den eigenen Projektcode ist derzeit keine Lizenz festgelegt.
Die verwendeten Bibliotheken und Drittmaterialien unterliegen ihren jeweiligen
Lizenzen; sie werden nicht als Quellkopien in diesem Repository mitgeliefert.

## Referenzen

- [Seeed: XIAO ESP32C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [GitHub Status API](https://www.githubstatus.com/api)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [PlatformIO-Board](https://docs.platformio.org/en/stable/boards/espressif32/seeed_xiao_esp32c3.html)
