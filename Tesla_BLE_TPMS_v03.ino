/* Tesla BLE TPMS v0.3 by Conny (c)2026
This tool is made by reverse engineering Tesla's M3 and MY TPMS sensors BLE payloads.
It can read both first-gen and highland m3 and my tpms. NO MS and MX (they're 433mhz).
Accuracy is still to be perfectioned, the device works pretty fine tho.
If the tire isn't inflated or is not moving with enough speed, the sensor will report very few to no data (try turning debug on).
If you wish to trigger them without having them pressured up and running, just make something that makes a 125kHz square wave near them.
Feedback from the device will be provided by serial monitor (baudrate 115200),
and several commands are available to play with it. Just type "help" in the serial monitor.
It also stores in the flash memory how many tpms have been read and the unique MACs to be able to recognise new devices from already known.
Dedicated command also to delete one, the other or both.
ESP32 library version 3.3.8, if the device crashes on startup try compiling with Huge APP partition.
Works pretty fine in ESP32-D0WD-V3 (revision v3.1), ESP32U, should also work in ESP32-C3 (single core) and ESP32-S3 (tested partially).
On the device i tested it successfully, Sketch uses 1118861 bytes (85%) of program storage space. Maximum is 1310720 bytes.
Enjoy! */
#include <BLEDevice.h>
#include <Preferences.h>
//Config
const char* VERSION = "0.3";
bool isScanningActive = true; //Default ON
bool isDebugActive = false; //Default OFF
bool isFilterActive = true;  // Default ON
const float PRESS_OFFSET = 100.0; //Calibration variable to calculate pressure
const float PRESS_DIVISOR = 7.0; //Calibration variable to calculate pressure
//Other variables
Preferences preferences;
unsigned int totalReadings = 0;
unsigned int uniqueSensors = 0;
String inputString = "";
BLEScan* pBLEScan;
String targetMAC = "";
unsigned long targetSearchStart = 0;
bool targetFound = false;
const float PSI_TO_BAR = 0.0689476; //Psi to bar conversion


String formatMAC(String raw) {
  if (raw.length() != 12) return "";
  String formatted = "";
  for (int i = 0; i < 12; i += 2) {
    formatted += raw.substring(i, i + 2);
    if (i < 10) formatted += ":";
  }
  formatted.toUpperCase();
  return formatted;
}

struct HelpDetail {
  const char* command;
  const char* content;
};

// Advanced help table
const HelpDetail helpTable[] = {
  { "HELP", "\n--- <help> shows the list of available commands." },
  { "LIST", "\n--- <list> shows the list of MAC adresses in memory." },
  { "SCAN", "\n--- HELP: SCAN <on/off/MAC_ADDRESS> ---\non/off -> Starts and stop bluetooth scanning for devices.\nMAC_ADDR -> Waits to listen only to the specified MAC device for 15 seconds." },
  { "DEBUG", "\n--- HELP: DEBUG <on/off> ---\non  -> Show raw hex payload of the devices listened.\noff -> (Default) DO NOT show raw hex payload of the devices listened." },
  { "FILTER", "\n--- HELP: FILTER <on/off> ---\non  -> (Default) Show only Tesla TPMS sensors.\noff -> Show ALL nearby BLE devices (MAC + Raw Payload)." },
  { "RESET", "\n--- HELP: RESET <parameter> ---\nw/o param -> Restart esp32 cpu.\ntotal-> Resets total reads counter.\nlist -> Resets MAC list.\nall -> Resets ALL memory (both the above)." },
  { "UPTIME", "\n--- HELP: UPTIME ---\nuptime -> Shows the time since the device is on." },
  { "ABOUT", "\n--- HELP: ABOUT ---\nabout -> Shows status and version of the device." }
};

void printHelp(String subCmd = "") {
  subCmd.toUpperCase();

  // If no sub-param, show command list.
  if (subCmd == "") {
    Serial.println("Commands: help, list, scan <p>, debug <p>, filter <p>, reset <p>, uptime, about");
    Serial.println("Advanced: help <command>");
    return;
  }

  // If param, search param in the table
  for (const auto& entry : helpTable) {
    if (subCmd == entry.command) {
      Serial.println(entry.content);
      return;
    }
  }

  // If param is not a command, return error
  Serial.printf("No detailed help for '%s'.\n", subCmd.c_str());
}


class MyCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (!isScanningActive && targetMAC == "") return;

    String name = advertisedDevice.getName().c_str();
    String currentMAC = advertisedDevice.getAddress().toString().c_str();
    currentMAC.toUpperCase();

    bool isTesla = (name.indexOf("tsTPMS") != -1);

    // If filter is on and the device is not a tesla tpms, stop here.
    if (isFilterActive && !isTesla) return;

    // If filter is off and device is not tesla, print mac rssi and raw payload.
    if (!isFilterActive && !isTesla) {
      Serial.printf("[BLE] MAC: %s | RSSI: %d | Data: ", currentMAC.c_str(), advertisedDevice.getRSSI());
      uint8_t* p = advertisedDevice.getPayload();
      for (int i = 0; i < advertisedDevice.getPayloadLength(); i++) {
        Serial.printf("%02X ", p[i]);
      }
      Serial.println("");
      return;
    }

    // If we are scanning for a specific mac and we get a message from some device that's not our target, return.
    if (targetMAC != "" && currentMAC != targetMAC) return;

    uint8_t* payload = advertisedDevice.getPayload();
    size_t payloadLen = advertisedDevice.getPayloadLength();
    int startIdx = -1;
    for (int i = 0; i < (int)payloadLen - 1; i++) {
      if (payload[i] == 0x2B && payload[i + 1] == 0x02) {
        startIdx = i;
        break;
      }
    }

    if (startIdx != -1) {
      if (targetMAC != "") targetFound = true;
      totalReadings++;
      preferences.putUInt("count", totalReadings);

      bool known = false;
      for (int i = 1; i <= uniqueSensors; i++) {
        if (preferences.getString(("m" + String(i)).c_str(), "") == currentMAC) {
          known = true;
          break;
        }
      }
      if (!known && targetMAC == "") {
        uniqueSensors++;
        preferences.putString(("m" + String(uniqueSensors)).c_str(), currentMAC);
        preferences.putUInt("uCount", uniqueSensors);
      }

      uint8_t type = payload[startIdx + 4];
      Serial.printf("### %s #%u ###\n", (targetMAC != "" ? "PROBE DATA" : "TPMS READ"), totalReadings);
      Serial.printf("%s\nMAC:  %s\nRSSI: %d dBm\n", known ? "[ KNOWN DEVICE ]" : "[ NEW DEVICE ] !!!", currentMAC.c_str(), advertisedDevice.getRSSI());

      if (type < 0x05) {
        Serial.println("@ SLEEP MODE [NO DATA]");
      } else {
        float psi = (((payload[startIdx + 6] << 8) | payload[startIdx + 5]) - PRESS_OFFSET) / PRESS_DIVISOR;
        int tF = payload[startIdx + 7] - 1;
        Serial.printf("PRES: %.1fpsi [%.2fb]\nTEMP: %dF [%.1fC]\nBATT: %dmV\n",
                      psi, psi * PSI_TO_BAR, tF, (tF - 32) * 5.0 / 9.0, (payload[startIdx + 9] << 8) | payload[startIdx + 8]);
      }
      if (isDebugActive) {
        Serial.print("RAW DATA: ");
        for (int i = 0; i < payloadLen; i++) Serial.printf("%02X ", payload[i]);
        Serial.println("");
      }
      Serial.println("####################\n");
      if (targetMAC != "") {
        targetMAC = "";
        Serial.println("Target found. Resuming scan.");
      }
    }
  }
};

void processCommand(String fullCmd) {
  fullCmd.trim();
  int spaceIdx = fullCmd.indexOf(' ');
  String cmd = (spaceIdx == -1) ? fullCmd : fullCmd.substring(0, spaceIdx);
  String param = (spaceIdx == -1) ? "" : fullCmd.substring(spaceIdx + 1);
  cmd.toUpperCase();
  param.toUpperCase();

  if (cmd == "HELP") printHelp(param);
  else if (cmd == "FILTER") {
    if (param == "ON") {
      if (isFilterActive) Serial.println("Filter is ALREADY ON.");
      else {
        isFilterActive = true;
        Serial.println("Filter ENABLED (Tesla only).");
      }
    } else if (param == "OFF") {
      if (!isFilterActive) Serial.println("Filter is ALREADY OFF.");
      else {
        isFilterActive = false;
        Serial.println("Filter DISABLED (All BLE devices).");
      }
    } else Serial.println("Error: Use 'filter on' or 'filter off'");
  } else if (cmd == "DEBUG") {
    if (param == "ON") {
      if (isDebugActive) Serial.println("Debug is ALREADY ON.");
      else {
        isDebugActive = true;
        Serial.println("Debug ON.");
      }
    } else if (param == "OFF") {
      if (!isDebugActive) Serial.println("Debug is ALREADY OFF.");
      else {
        isDebugActive = false;
        Serial.println("Debug OFF.");
      }
    } else Serial.println("Error: Use 'debug on' or 'debug off'");
  } else if (cmd == "ABOUT") {
    Serial.println("\n--- DEVICE INFO ---");
    Serial.printf("# Tesla BLE TPMS Reader v%s by Conny (c)2026\n", VERSION);
    Serial.printf("# Tot readings: %u | Unique MACs: %u\n", totalReadings, uniqueSensors);
    Serial.printf("# STATUS: Scan: %s | Debug: %s | Filter: %s\n", isScanningActive ? "ON" : "OFF", isDebugActive ? "ON" : "OFF", isFilterActive ? "ON" : "OFF");
    unsigned long sec = millis() / 1000;
    Serial.printf("# UPTIME: %02lu:%02lu:%02lu\n", sec / 3600, (sec % 3600) / 60, sec % 60);  //UPTIME
  } else if (cmd == "SCAN") {
    if (param == "ON") {
      if (isScanningActive) {
        Serial.println("Scan is ALREADY ON.");
      } else {
        isScanningActive = true;
        Serial.println("Scan RESUMED.");
      }
    } else if (param == "OFF") {
      if (!isScanningActive) {
        Serial.println("Scan is ALREADY OFF.");
      } else {
        isScanningActive = false;
        Serial.println("Scan STOPPED.");
      }
    } else if (param.length() == 12) { //If we search a specific MAC
      targetMAC = formatMAC(param);
      targetFound = false;
      targetSearchStart = millis();
      Serial.printf("Probing for MAC: %s...\n", targetMAC.c_str());
    } else {
      Serial.println("Error: Use 'scan on', 'scan off' or 'scan <MAC_ADDRESS>' (12 chars without separators).");
    }
  } else if (cmd == "LIST") {
    Serial.println("\n--- MAC DATABASE ---");
    for (int i = 1; i <= uniqueSensors; i++) Serial.printf("#%d: %s\n", i, preferences.getString(("m" + String(i)).c_str(), "ERR").c_str());
  } else if (cmd == "UPTIME") {
    unsigned long sec = millis() / 1000;
    Serial.printf("Uptime: %02lu:%02lu:%02lu\n", sec / 3600, (sec % 3600) / 60, sec % 60);
  } else if (cmd == "RESET") {
    if (param == "LIST") {
      preferences.putUInt("uCount", 0);
      Serial.println("Cleared MAC LIST memory!");
    } else if (param == "TOTAL") {
      preferences.putUInt("count", 0);
      Serial.println("Cleared TOTAL COUNTS memory!");
    } else if (param == "ALL") {
      preferences.clear();
      Serial.println("Cleared ALL memory!");
    }
    Serial.println("Restarting in 500ms from now...");
    delay(500);
    esp_restart();
  } else if (cmd == "DSLEEP") {
    Serial.println("Going in DEEP SLEEP mode! Hard reset the device to restart it!");
    delay(500);
    esp_deep_sleep_start();
  } else Serial.printf("Unknown: '%s'! Type 'help'!\n", cmd.c_str());
}

void setup() {
  Serial.begin(115200);                              //Serial setup
  preferences.begin("tpms-data", false);             //Flash memory setup
  totalReadings = preferences.getUInt("count", 0);   //Retrieve readings from flash
  uniqueSensors = preferences.getUInt("uCount", 0);  //Retrieve unique mac count from flash
  //Welcome message
  delay(1000);  //Wait for serial connection to estabilish properly
  Serial.println("\n###########################");
  Serial.printf("@ Tesla BLE TPMS Reader v%s\n", VERSION);
  Serial.printf("@ Tot readings: %u\n", totalReadings);
  Serial.printf("@ Unique MACs:  %u\n", uniqueSensors);
  Serial.println("###########################");
  //Bluetooth setup
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyCallbacks(), false);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(150);
  pBLEScan->setWindow(120);
  pBLEScan->start(0, NULL, false);
}

void loop() {
  //If 15 seconds passed and target mac is not found, just reset to default and listen to everybody
  if (targetMAC != "" && (millis() - targetSearchStart > 15000)) {
    if (!targetFound) Serial.printf("\n[ERROR] Target %s not found.\n", targetMAC.c_str());
    targetMAC = "";
  }
  //Process serial commands
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputString.length() > 0) {
        processCommand(inputString);
        inputString = "";
      }
    } else inputString += inChar;
  }
  yield();
}
