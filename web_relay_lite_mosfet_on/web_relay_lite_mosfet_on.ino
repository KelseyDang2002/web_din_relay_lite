// DONE (OLD):
// 1. count number of reboots
// 2. keep autoping enabled, if autoping is detected disabled then enable it
// 3. enable autoping once a day at 1 AM
// 4. ssh into VSign Hub, if ssh attempts fail then power cycle
// 5. keep modem outlet (MOSFET) enabled at all times, if disabled then enable it
// 6. (optional) keep Ethernet switch outlet (MOSFET) enabled at all times, if disabled then enable it
// 7. remote code upload over Ethernet instead of WiFi (need a lot more testing)
// 8. number of reboots get logged into .mob file and is stored in /storage folder
// 9. logcopy.sh will send the .mob file to the VSign Hub
// 10. web interface to adjust parameters
// 11. still need to figure out how to do remote code
// 12. File writing to a txt file
// 13. Send txt file to VSign Hub over Ethernet
// 14. If there broken ethernet, or there is an issue communicating with modem, WPS does not power cycle

// DONE 7/24/25:
// 1. Test exponential backoff with shorter wait times DONE
// 2. How to write to a log file (log_it) DONE
// 3. Work on monitor_modem() DONE?
// 4. How to do WPS boot count resets to 0 after code upload DONE
// 5. Test Ethernet with ETH.h library and define pins with D DONE
// 6. Integrate Ethernet init + Ethernet OTA into main WPS program DONE
// 7. Get event timestamps for log_it() function DONE
// 8. How to read log file for debugging DONE
// 9. Running monitor_ssh with its helper functions is causing a boot loop along with other tasks DONE
// 10. Turn on MOSFET one by one with delay in between DONE
// 11. Add monitor_ESwitch() and monitor_USBHub() functions DONE
// 12. Wrap log_it() function around with a mutex so that only one task writes to the same log file at a time DONE
// 13. VSign Hub still keeps rebooting after 2 reboots, it should try again only after the wait time DONE
// 14. Create http web server DONE
// 15. username and password for accessing web interface DONE
// 16. base web interface for editing configuration file DONE

// DONE 8/11/25:
// 17. hide password in web interface DONE
// 18. log boot count after Ethernet is successfully initialized and date can be retrieved successfully, call once (similar to checking if web server started) DONE
// 19. Use a web interface to modify a configuration file to set the parameters for the ESP32S3 WPS, this can be done with a json file stored in SPIFFS DONE
// 20. How to replicate autoping, target IP addresses, parameters DONE
// 21. add /mosfet web interface and parameters to enable/disable certain MOSFETs DONE
// - mosfet on/off defined in config.json file
// - config.json determines whether mosfets are enabled/disabled throughout the entire program
// - on bootup (setup) mosfets will be HIGH if set to 1, and LOW if set to 0
// - if mosfet is off during setup but changes to on during operation, add wrapper around all mosfet HIGH/LOW calls
// - mosfets can be power cycled if enabled, if disabled then a mosfet cannot be power cycled
// - can only be called once
// 22. convert power cycle delays to config.json parameters DONE
// 23. there is still a problem with rebooting when ssh fails DONE
// - works for one day after initial boot up
// - when it continues to run until the next day, the wait time shows 24h but the wait time after 2 reboots is 3 or 4 minutes instead
// - it looks like at a certain wait time, the pdMS_TO_TICKS() function maxes out at a certain time
// - vTaskDelay() maxes out at 1 hour

// DONE 10/8/25:
// 1. implement a /upload page in the web interface to upload a config.json file DONE
// 2. how to do /systemlog and system log for events happening (ping gateway, ssh, autoping, etc. similar to serial monitor) DONE
// 3. add navigation panel on the left side DONE
// 4. /config will eventually become /setup, /datetime, and /autoping DONE

// TODO:
// 5. How to send log file (.mob) to VSign Hub (log_copy), deletes log files after a couple days
// 6. How to do send_passivectl()
// 7. Reboot Web Relay Lite if failed to ping modem (related to autoping) NEED TESTING
// 8. VSign Hub backoff wait timer cancels once it can be SSH into again
// 9. reboot VSign Hub at 2:00 AM each day
// 10. monitor_and_re_enable_autoping()
// - keeps task_autoping running all the time
// 11. re_enable_autoping_once_a_day()
// - set enable_autoping = 1 at 01:00 each day
// 12. scripts / tasks enabling is separate from mosfet on/off WIP
// - some tasks can be enabled/disabled via editing config.json
// - if a tasks involves a mosfet that is disabled, the task itself should enable it (set to 0 in config.json)
// - freertos task enable/disable logic, use xTaskSuspend and xTaskResume (similar to mosfet control HIGH and LOW)
// 12/22/25 BUG:
// - left WDR running continuously for over a week starting from 12/11 to 12/22
// - there were 344 reboots for ESP which was triggered everytime modem went into recovery mode
// - when modem enters recovery mode, modem is rebooted so we get MOD 0 and MOD 9
// - it seems like the modem did not have enough time to get cellular signal after rebooting, so MOSFET keeps power cycling it

// QUEUE
// 13. ssh_ping() and ping_gateway() timeout conditions WIP
// 14. Stack size allocation for running tasks WIP
// 15. Test log_it() with multiple tasks
// 16. look into kill ssh service, see if TCP connection ssh still goes through, can it detect that ssh service is down?
// 17. if not connected to modem/internet for a while, reboot modem?
// 18. figure out how to reset ip when WPS cannot be accessed through web interface
// 19. DHCP mode? auto detect gateway?
// 20. add login page, able to access other pages after successful login WIP
// 21. add /log page to list all log files in web interface and maybe let user view the log files WIP
// 22. /logout ?
// 23. clean up code / rename variables / remove unnecessary delays
// 24. (testing) cannot get datetime without Ethernet initalizing and connecting to internet
// - maybe add function where if there is no internet connection, use default / utc / onboard timezone
// 25. (testing) add DHCP fallback option when ESP32S3 static IP is not known or gateway IP is not known WIP
// 26. (testing) modify initEthernet to show successful connection to gateway if it can be pinged and a DNS server can be pinged? WIP
// 27. have the ability to set the timezone easily WIP
// - for example, selecting PST, MDT, CST, EST, international timezones, etc.

// TESTING (NEEDS TO BE FLASHED):
// - ssh fail exponential backoff timer over a long period (overnight) DONE
// - reduced ping_gateway mutex delay time for better timestamp accuracy DONE
// - system log page for monitoring (no need to use serial monitor) DONE
// - initEthernetWithFallback, uses DHCP if static IP fails, pings gateway to see if on IP is on network

// ********************************************************************************************************
// ESP32S3 + W5500 + MOSFET WPS
// ********************************************************************************************************

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include "SPIFFS.h"
#include "FS.h"
#include <Preferences.h>
#include <Update.h>
#include "time.h"
#include <SPI.h>
#include <ETH.h>
#include <ESPping.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// *********************************
// STRUCT FOR TIMEZONE AND CONFIG FILE
// *********************************

struct TZDef {
  const char *name;
  const char *posix;
};

const TZDef timezone_table[] = {
  { "UTC_minus_12", "GMT+12" },
  { "UTC_minus_11", "GMT+11" },
  { "Hawaii", "HST10" },
  { "Alaska", "AKST9AKDT,M3.2.0,M11.1.0" },
  { "Los_Angeles", "PST8PDT,M3.2.0,M11.1.0" },
  { "Denver", "MST7MDT,M3.2.0,M11.1.0" },
  { "Chicago", "CST6CDT,M3.2.0,M11.1.0" },
  { "New_York", "EST5EDT,M3.2.0,M11.1.0" },
  { "Atlantic", "AST4ADT,M3.2.0,M11.1.0" },
  { "Argentina", "GMT+3" },
  { "South_Georgia", "GMT+2" },
  { "Azores", "AZOT1AZOST,M3.5.0,M10.5.0" },
  { "GMT", "GMT0" },
  { "Central_Europe", "CET-1CEST,M3.5.0,M10.5.0" },
  { "Eastern_Europe", "EET-2EEST,M3.5.0,M10.5.0" },
  { "Moscow", "MSK-3" },
  { "Dubai", "GST-4" },
  { "Pakistan", "PKT-5" },
  { "Bangladesh", "BST-6" },
  { "Thailand", "WIB-7" },
  { "China", "CST-8" },
  { "Japan", "JST-9" },
  { "Sydney", "AEST-10AEDT,M10.1.0,M4.1.0" },
  { "Solomon_Islands", "GMT-11" },
  { "New_Zealand", "NZST-12NZDT,M9.5.0,M4.1.0" },
  { "Tonga", "GMT-13" },
  { "Line_Islands", "GMT-14" }
};

struct Config {
  String siteID_param;

  char *username_param;
  char *password_param;

  IPAddress ethIP_param;
  IPAddress vsignIP_param;
  IPAddress gateway_param;
  IPAddress subnet_param;
  IPAddress primaryDNS_param;
  IPAddress secondaryDNS_param;
  uint16_t webPort_param;

  unsigned long mosfet_startup_delay_param;
  int mosfet_1;
  int mosfet_2;
  int mosfet_3;
  int mosfet_4;

  unsigned long reboot_delay_param;
  unsigned long wps_startup_delay_param;
  unsigned long ssh_ping_interval_param;
  unsigned long time_allowance_param;
  // unsigned long reboot_wait_param;
  unsigned long max_reboot_wait_param;
  unsigned long wait_divisor_param;

  char *ntpServer_param;
  long gmtOffset_sec_param;
  int daylightOffset_sec_param;
  // unsigned long baudrate_param;

  char *timezone_param;

  int enable_monitor_and_re_enable_autoping;
  int enable_re_enable_autoping_once_a_day;
  int enable_monitor_ssh;
  int enable_monitor_modem;
  int enable_monitor_USBHub;
  int enable_monitor_ESwitch;

  IPAddress autoping_target1_param;
  IPAddress autoping_target2_param;
  IPAddress autoping_target3_param;
  int enable_autoping;
  unsigned long autoping_interval_param;
  unsigned long autoping_reboot_timeout_param;
  unsigned long autoping_ping_responses_param;
  unsigned long autoping_reboot_attempts_param;
  unsigned long autoping_total_reboot_attempts_param;  // no limit
  unsigned long autoping_reboot_delay_param;

  // default configurations
  Config() {
    siteID_param = "site000";

    username_param = "admin";
    password_param = "CAV2007$!smart";

    ethIP_param = IPAddress(192, 168, 2, 150);
    vsignIP_param = IPAddress(192, 168, 2, 100);
    gateway_param = IPAddress(192, 168, 2, 50);
    subnet_param = IPAddress(255, 255, 255, 0);
    primaryDNS_param = IPAddress(8, 8, 8, 8);
    secondaryDNS_param = IPAddress(1, 1, 1, 1);
    webPort_param = 2660;  // WIP

    mosfet_startup_delay_param = 3 * 1000;
    mosfet_1 = 1;
    mosfet_2 = 1;
    mosfet_3 = 0;
    mosfet_4 = 0;

    reboot_delay_param = 10 * 1000;
    wps_startup_delay_param = 3 * 60 * 1000;
    ssh_ping_interval_param = 5 * 60 * 1000;
    time_allowance_param = 2 * 60 * 1000;
    // reboot_wait_param = 60 * 60 * 1000; // TBD
    max_reboot_wait_param = 24 * 60 * 60 * 1000;
    wait_divisor_param = 60 * 60 * 1000;

    ntpServer_param = "pool.ntp.org";
    gmtOffset_sec_param = -8 * 60 * 60;
    daylightOffset_sec_param = 60 * 60;
    // baudrate_param = 115200;

    timezone_param = "Los_Angeles";

    enable_monitor_and_re_enable_autoping = 1;  // keeps task_autoping running
    enable_re_enable_autoping_once_a_day = 0;   // set enable_autoping = 1 at 01:00 each day
    enable_monitor_ssh = 1;                     // keeps task_monitor_ssh running
    enable_monitor_modem = 1;                   // keeps MOSFET_1 on
    enable_monitor_USBHub = 1;                  // keeps MOSFET_3 on
    enable_monitor_ESwitch = 1;                 // keeps MOSFET_4 on

    autoping_target1_param = IPAddress(8, 8, 8, 8);
    autoping_target2_param = IPAddress(1, 1, 1, 1);
    autoping_target3_param = IPAddress(8, 8, 4, 4);
    enable_autoping = 1;
    autoping_interval_param = 30 * 1000;
    autoping_reboot_timeout_param = 3 * 60 * 1000;
    autoping_ping_responses_param = 0;
    autoping_reboot_attempts_param = 0;
    autoping_total_reboot_attempts_param = 0;  // 0 = no limit
    autoping_reboot_delay_param = 3 * 60 * 1000;
  }
};

Config config;
File uploadFile;

// *********************************
// PIN DEFINITIONS / ETHERNET CONFIG
// *********************************

#define MOSI_PIN D10
#define MISO_PIN D9
#define SCK_PIN D8
#define CS_PIN D7

// WDR Lite V3:
// D4 = MOSFET_1 (12V) (Modem)
// D5 = MOSFET_2 (12V)
// D1 = MOSFET_3 (12V)
// D3 = MOSFET_4 (5V) (VSign Hub)
#define MOSFET_1 D4  // D6 original
#define MOSFET_2 D3  // D5 original
#define MOSFET_3 D1  // D4 original
#define MOSFET_4 D5  // D3 original

#ifndef ETH_PHY_CS
#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1
#define ETH_PHY_CS CS_PIN
#define ETH_PHY_IRQ -1
#define ETH_PHY_RST -1
#endif

// for log_it_safe function
#define LOG_ENTRY_MAX 512
#define LOG_FILENAME_MAX 64
#define LOG_MIN_FREE_HEAP 4096  // skip writes if heap critical

// for systemLog_safe
#define SYSLOG_BUFFER_SIZE 8192
static char syslog_buf[SYSLOG_BUFFER_SIZE];
static size_t syslog_head = 0;  // index of first byte valid
static size_t syslog_len = 0;   // number of valid bytes

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };
WebServer *server;
// *********************************
// GLOBAL DEFINITIONS
// *********************************

int boot = 0;

unsigned long reboot_count = 0;
unsigned long reboot_wait = 60 * 60 * 1000;  // 1 hour, config file

// for init
bool ethernetConnected = false;
bool otaInitialized = false;
bool webServerStarted = false;

int prev_mosfet_1 = -1;
int prev_mosfet_2 = -1;
int prev_mosfet_3 = -1;
int prev_mosfet_4 = -1;

int prev_enable_monitor_and_re_enable_autoping = -1;
int prev_enable_re_enable_autoping_once_a_day = -1;
int prev_enable_monitor_ssh = -1;
int prev_enable_monitor_modem = -1;
int prev_enable_monitor_USBHub = -1;
int prev_enable_monitor_ESwitch = -1;

int prev_enable_autoping = -1;
bool inRecovery = false;
unsigned long lastPingTime = 0;
unsigned long recoveryStart = 0;

// define a mutex globally
SemaphoreHandle_t netMutex;
SemaphoreHandle_t spiffsMutex;
SemaphoreHandle_t logMutex;

String systemLogBuffer = "";
const size_t MAX_LOG_BUFFER_SIZE = 4096;
SemaphoreHandle_t systemLogMutex = NULL;

// *********************************
// Function Definitions
// *********************************
void logMessage(const String &msg);
void ssh_ping();
bool ping_gateway();

void log_boot_count();
void monitor_modem();
void monitor_ESwitch();
void monitor_USBHub();
void monitor_and_re_enable_autoping();
void re_enable_autoping_once_a_day();
void reboot_modem();
void monitor_ssh();
void reboot_linux_box();
void systemLog(const String &message);
void startWebServer();
void autoping();
void log_it(const String &aLogEntry);
void log_copy();
bool pingAnyTarget();

// *********************************
// SETUP
// *********************************
void setup() {
  netMutex = xSemaphoreCreateMutex();
  spiffsMutex = xSemaphoreCreateMutex();
  logMutex = xSemaphoreCreateMutex();
  systemLogMutex = xSemaphoreCreateMutex();  // Add this line

  delay(100);

  systemLog("System starting up...");
  // systemLog("Boot count: " + String(boot));

  // Now test logging
  // systemLog("=== SYSTEM LOG TEST ===");

  pinMode(CS_PIN, OUTPUT);
  pinMode(MOSFET_1, OUTPUT);
  pinMode(MOSFET_2, OUTPUT);
  pinMode(MOSFET_3, OUTPUT);
  pinMode(MOSFET_4, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(CS_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);  // OFF

  Serial.begin(115200);
  Serial.println("Booting");
  systemLog("Booting");

  systemLog("=== SYSTEM BOOT ===");
  systemLog("ESP32S3 WPS System Starting");

  WiFi.disconnect(true);  // disconnect and erase credentials
  WiFi.mode(WIFI_OFF);    // fully disable WiFi hardware
  btStop();               // disable bluetooth

  boot = getBootCount();  // called once per power cycle

  if (spiffsMutex == NULL) {
    Serial.println("failed to create SPIFFS mutex");
    systemLog("failed to create SPIFFS mutex");
    while (true)
      ;
  }

  // Create systemLog mutex FIRST, before any other code
  if (systemLogMutex == NULL) {
    Serial.println("FAILED TO CREATE SYSTEM LOG MUTEX!");
    systemLog("FAILED TO CREATE SYSTEM LOG MUTEX!");
  } else {
    Serial.println("System log mutex created successfully");
    systemLog("System log mutex created successfully");
  }

  // mount SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed!");
    systemLog("SPIFFS Mount Failed!");
    while (true)
      ;
  }
  Serial.println("SPIFFS Mounted");
  systemLog("SPIFFS Mounted");
  // systemLog("Total SPIFFS size: " + String(SPIFFS.totalBytes()) + " bytes");
  // systemLog("Used SPIFFS: " + String(SPIFFS.usedBytes()) + " bytes");

  // uncomment and edit to delete a file
  // deleteFile("/config.json");
  // deleteFile("/site000-2025-08-11.txt");

  // get SPIFFS size
  Serial.print("Total SPIFFS size: ");
  Serial.print(SPIFFS.totalBytes());
  Serial.println(" bytes");
  Serial.print("Used SPIFFS: ");
  Serial.print(SPIFFS.usedBytes());
  Serial.println(" bytes");

  systemLog("Total SPIFFS size: ");
  systemLog(String(SPIFFS.totalBytes()));
  systemLog(" bytes");
  systemLog("Used SPIFFS: ");
  systemLog(String(SPIFFS.usedBytes()));
  systemLog(" bytes");

  loadConfig();  // load config
  server = new WebServer(config.webPort_param);

  if (config.mosfet_1 == 1) {
    digitalWrite(MOSFET_1, LOW);
    Serial.println("MOSFET 1 ON");
    systemLog("MOSFET 1 ON");
    delay(config.mosfet_startup_delay_param);
  } else {
    digitalWrite(MOSFET_1, HIGH);
    Serial.println("MOSFET 1 OFF");
    systemLog("MOSFET 1 OFF");
  }

  if (config.mosfet_2 == 1) {
    digitalWrite(MOSFET_2, LOW);
    Serial.println("MOSFET 2 ON");
    systemLog("MOSFET 2 ON");
    delay(config.mosfet_startup_delay_param);
  } else {
    digitalWrite(MOSFET_2, HIGH);
    Serial.println("MOSFET 2 OFF");
    systemLog("MOSFET 2 OFF");
  }

  if (config.mosfet_3 == 1) {
    digitalWrite(MOSFET_3, LOW);
    Serial.println("MOSFET 3 ON");
    systemLog("MOSFET 3 ON");
    delay(config.mosfet_startup_delay_param);
  } else {
    digitalWrite(MOSFET_3, HIGH);
    Serial.println("MOSFET 3 OFF");
    systemLog("MOSFET 3 OFF");
  }

  if (config.mosfet_4 == 1) {
    digitalWrite(MOSFET_4, LOW);
    Serial.println("MOSFET 4 ON");
    systemLog("MOSFET 4 ON");
    delay(config.mosfet_startup_delay_param);
  } else {
    digitalWrite(MOSFET_4, HIGH);
    Serial.println("MOSFET 4 OFF");
    systemLog("MOSFET 4 OFF");
  }

  Serial.flush();

  showConfig();  // show config

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  delay(1000);
  Serial.println("Initializing W5500 ...");
  systemLog("Initializing W5500 ...");

  // running tasks
  xTaskCreatePinnedToCore(task_handleInits, "Ethernet and OTA init", 8192, NULL, 3, NULL, 0);      // 32 KB
  xTaskCreatePinnedToCore(task_start_web_server, "startWebServer", 8192, NULL, 1, NULL, 0);        // 32 KB
  xTaskCreatePinnedToCore(task_handleWebRequests, "handle web requests", 8192, NULL, 1, NULL, 0);  // 32 KB
  // xTaskCreatePinnedToCore(task_log_boot_count, "log_boot_count", 1024, NULL, 1, NULL, 1); // 4 KB
  xTaskCreatePinnedToCore(task_monitor_and_re_enable_autoping, "monitor_and_re_enable_autoping", 1024, NULL, 1, NULL, 0);
  // xTaskCreatePinnedToCore(task_re_enable_autoping_once_a_day, "re_enable_autoping_once_a_day", 1024, NULL, 1, NULL, 0);
  // xTaskCreatePinnedToCore(task_power_cycle_vsign_once_a_day, "power_cycle_vsign_once_a_day", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(task_autoping, "autoping", 4096, NULL, 2, NULL, 1);                // 16 KB
  xTaskCreatePinnedToCore(task_monitor_ssh, "monitor_ssh", 8192, NULL, 2, NULL, 1);          // 32 KB
  xTaskCreatePinnedToCore(task_monitor_modem, "monitor_modem", 4096, NULL, 1, NULL, 1);      // 16 KB
  xTaskCreatePinnedToCore(task_monitor_ESwitch, "monitor_ESwitch", 1024, NULL, 1, NULL, 1);  // 4 KB
  xTaskCreatePinnedToCore(task_monitor_USBHub, "monitor_USBHub", 1024, NULL, 1, NULL, 1);    // 4 KB

  systemLog("All tasks started successfully");
}

// *********************************
// LOOP
// *********************************

void loop() {

  static unsigned long lastTest = 0;
  if (millis() - lastTest > 10000) {
    // systemLog("System alive - uptime: " + String(millis()/1000) + "s");
    lastTest = millis();
  }

  if (config.mosfet_1 != -1 && config.mosfet_1 != prev_mosfet_1) {
    if (config.mosfet_1 == 1) {
      digitalWrite(MOSFET_1, LOW);
      Serial.println("MOSFET 1 ON");
      systemLog("MOSFET 1 ON");
    } else if (config.mosfet_1 == 0) {
      digitalWrite(MOSFET_1, HIGH);
      Serial.println("MOSFET 1 OFF");
      systemLog("MOSFET 1 OFF");
    }
    prev_mosfet_1 = config.mosfet_1;
  }

  if (config.mosfet_2 != -1 && config.mosfet_2 != prev_mosfet_2) {
    if (config.mosfet_2 == 1) {
      digitalWrite(MOSFET_2, LOW);
      Serial.println("MOSFET 2 ON");
      systemLog("MOSFET 2 ON");
    } else if (config.mosfet_2 == 0) {
      digitalWrite(MOSFET_2, HIGH);
      Serial.println("MOSFET 2 OFF");
      systemLog("MOSFET 2 OFF");
    }
    prev_mosfet_2 = config.mosfet_2;
  }

  if (config.mosfet_3 != -1 && config.mosfet_3 != prev_mosfet_3) {
    if (config.mosfet_3 == 1) {
      digitalWrite(MOSFET_3, LOW);
      Serial.println("MOSFET 3 ON");
      systemLog("MOSFET 3 ON");
    } else if (config.mosfet_3 == 0) {
      digitalWrite(MOSFET_3, HIGH);
      Serial.println("MOSFET 3 OFF");
      systemLog("MOSFET 3 OFF");
    }
    prev_mosfet_3 = config.mosfet_3;
  }

  if (config.mosfet_4 != -1 && config.mosfet_4 != prev_mosfet_4) {
    if (config.mosfet_4 == 1) {
      digitalWrite(MOSFET_4, LOW);
      Serial.println("MOSFET 4 ON");
      systemLog("MOSFET 4 ON");
    } else if (config.mosfet_4 == 0) {
      digitalWrite(MOSFET_4, HIGH);
      Serial.println("MOSFET 4 OFF");
      systemLog("MOSFET 4 OFF");
    }
    prev_mosfet_4 = config.mosfet_4;
  }

  // check which tasks are enabled/disabled
  // config.enable_monitor_and_re_enable_autoping
  // - if enabled, allow task_enable_autoping to run
  // config.enable_re_enable_autoping_once_a_day
  // - if enabled, allow task_enable_re_enable_autoping_once_a_day to run
  // config.enable_monitor_ssh
  // - if enabled, allow task_monitor_ssh to run
  // config.enable_monitor_modem
  // - if enabled, allow config.mosfet_1 = 1 when modem is connected to internet
  // config.enable_monitor_ESwitch
  // - if enabled, allow config.mosfet_3 = 1
  // config.enable_monitor_USBHub
  // - if enabled, allow config.mosfet_4 = 1

  // NEED TESTING, enable/disable FreeRTOS tasks or running thteads
  if (config.enable_monitor_and_re_enable_autoping != -1 && config.enable_monitor_and_re_enable_autoping != prev_enable_monitor_and_re_enable_autoping) {
    if (config.enable_monitor_and_re_enable_autoping == 1) {
      // start task
      // task_monitor_and_re_enable_autoping()
      Serial.println("monitor_and_re_enable_autoping ENABLED");
      systemLog("monitor_and_re_enable_autoping ENABLED");
    } else if (config.enable_monitor_and_re_enable_autoping == 0) {
      // stop task
      Serial.println("monitor_and_re_enable_autoping DISABLED");
      systemLog("monitor_and_re_enable_autoping DISABLED");
    }
    prev_enable_monitor_and_re_enable_autoping = config.enable_monitor_and_re_enable_autoping;
  }

  if (config.enable_re_enable_autoping_once_a_day != -1 && config.enable_re_enable_autoping_once_a_day != prev_enable_re_enable_autoping_once_a_day) {
    if (config.enable_re_enable_autoping_once_a_day == 1) {
      // start task
      // task_re_enable_autoping_once_a_day()
      Serial.println("re_enable_autoping_once_a_day ENABLED");
      systemLog("re_enable_autoping_once_a_day ENABLED");
    } else if (config.enable_re_enable_autoping_once_a_day == 0) {
      // stop task
      Serial.println("re_enable_autoping_once_a_day DISABLED");
      systemLog("re_enable_autoping_once_a_day DISABLED");
    }
    prev_enable_re_enable_autoping_once_a_day = config.enable_re_enable_autoping_once_a_day;
  }

  if (config.enable_monitor_ssh != -1 && config.enable_monitor_ssh != prev_enable_monitor_ssh) {
    if (config.enable_monitor_ssh == 1) {
      // start task
      // task_monitor_ssh()
      Serial.println("monitor_ssh ENABLED");
      systemLog("monitor_ssh ENABLED");
    } else if (config.enable_monitor_ssh == 0) {
      // stop task
      Serial.println("monitor_ssh DISABLED");
      systemLog("monitor_ssh DISABLED");
    }
    prev_enable_monitor_ssh = config.enable_monitor_ssh;
  }

  if (config.enable_monitor_modem != -1 && config.enable_monitor_modem != prev_enable_monitor_modem) {
    if (config.enable_monitor_modem == 1) {
      // start task
      // task_monitor_modem()
      Serial.println("monitor_modem ENABLED");
      systemLog("monitor_modem ENABLED");
    } else if (config.enable_monitor_modem == 0) {
      // stop task
      Serial.println("enable_monitor_modem DISABLED");
      systemLog("monitor_modem DISABLED");
    }
    prev_enable_monitor_modem = config.enable_monitor_modem;
  }

  if (config.enable_monitor_USBHub != -1 && config.enable_monitor_USBHub != prev_enable_monitor_USBHub) {
    if (config.enable_monitor_USBHub == 1) {
      // start task
      // task_monitor_USBHub()
      Serial.println("monitor_USBHub ENABLED");
      systemLog("monitor_USBHub ENABLED");
    } else if (config.enable_monitor_USBHub == 0) {
      // stop task
      Serial.println("monitor_USBHub DISABLED");
      systemLog("monitor_USBHub DISABLED");
    }
    prev_enable_monitor_USBHub = config.enable_monitor_USBHub;
  }

  if (config.enable_monitor_ESwitch != -1 && config.enable_monitor_ESwitch != prev_enable_monitor_ESwitch) {
    if (config.enable_monitor_ESwitch == 1) {
      // start task
      Serial.println("monitor_ESwitch ENABLED");
      systemLog("monitor_ESwitch ENABLED");
    } else if (config.enable_monitor_ESwitch == 0) {
      // stop task
      Serial.println("monitor_ESwitch DISABLED");
      systemLog("monitor_ESwitch DISABLED");
    }
    prev_enable_monitor_ESwitch = config.enable_monitor_ESwitch;
  }

  if (config.enable_autoping != -1 && config.enable_autoping != prev_enable_autoping) {
    if (config.enable_autoping == 1) {
      // start task
      // task_autoping()
      Serial.println("autoping ENABLED");
      systemLog("autoping ENABLED");
    } else if (config.enable_autoping == 0) {
      // stop task
      Serial.println("autoping DISABLED");
      systemLog("autoping DIABLED");
    }
    prev_enable_autoping = config.enable_autoping;
  }

  // user SPIFFS file monitoring commands
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'r') {
      if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(5000))) {
        String filename = "/" + config.siteID_param + "-" + getDate() + ".txt";
        // readLogFile(filename);
        readLogFile("/site000-2025-12-29.txt");
        readLogFile("/site000-2025-12-30.txt");
        readLogFile("/site000-2025-12-31.txt");
        xSemaphoreGive(spiffsMutex);
      }
    } else if (cmd == 'l') {
      if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(5000))) {
        listLogFiles();
        xSemaphoreGive(spiffsMutex);
      }
    } else if (cmd == 'd') {
      // add 'd' to delete config file and then load a new default config
      if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(5000))) {
        deleteFile("/config.json");
        loadConfig();
        xSemaphoreGive(spiffsMutex);
      }
    }
  }
}

// *********************************
// FREERTOS TASKS
// *********************************

void task_handleInits(void *parameter) {
  while (true) {
    handleInits();
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

void task_handleWebRequests(void *parameter) {
  // Wait until web server has started
  while (!webServerStarted) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  while (true) {
    server->handleClient();         // Non-blocking, serves incoming HTTP requests
    vTaskDelay(pdMS_TO_TICKS(10));  // Short delay to yield to other tasks
  }
}

void task_start_web_server(void *parameter) {
  while (!webServerStarted) {
    if (ETH.linkUp() && ETH.localIP()) {
      if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500))) {
        startWebServer();
        xSemaphoreGive(spiffsMutex);
        webServerStarted = true;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
  }
  vTaskDelete(NULL);  // Kill this task after server starts
}

// maybe not needed
void task_log_boot_count(void *parameter) {
  while (true) {
    log_boot_count();
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

// TODO
void task_monitor_and_re_enable_autoping(void *parameter) {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  while (true) {
    monitor_and_re_enable_autoping();
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

// TODO
void task_re_enable_autoping_once_a_day(void *parameter) {
  // vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  while (true) {
    // WIP
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

// TODO
void task_power_cycle_vsign_once_a_day(void *parameter) {
  // vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  while (true) {
    // WIP
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

void task_autoping(void *parameter) {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  while (true) {
    autoping();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// modified 7/24/25
void task_monitor_ssh(void *parameter) {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));

  while (true) {
    ssh_ping();

    // if 2 reboots have occurred, apply exponential backoff
    if (reboot_count >= 2) {

      // prevent divide by zero error
      if (config.wait_divisor_param == 0) {
        config.wait_divisor_param = 60 * 60 * 1000;
        saveConfig();
      }

      unsigned long reboot_wait_hours = reboot_wait / config.wait_divisor_param;
      String rlog = "Reboot wait: " + String(reboot_wait_hours) + " hour(s)";
      log_it(rlog);
      Serial.println(rlog);
      systemLog(rlog);

      // vTaskDelay(pdMS_TO_TICKS(reboot_wait));  // backoff delay
      workaround_delay_ms(reboot_wait);
      reboot_count = 0;  // reset after backoff
      // reboot_wait = reboot_wait * 2; // double the wait for next backoff
      reboot_wait = min(reboot_wait * 2, config.max_reboot_wait_param);

      // if (reboot_wait > max_reboot_wait) {
      // reboot_wait = max_reboot_wait; // max wait time is 24 hours
      // }
    } else {
      vTaskDelay(pdMS_TO_TICKS(config.ssh_ping_interval_param));  // 5 minutes, normal wait
    }
  }
}

void task_monitor_modem(void *parameter) {
  while (true) {
    monitor_modem();
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

void task_monitor_ESwitch(void *parameter) {
  while (true) {
    monitor_ESwitch();
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

void task_monitor_USBHub(void *parameter) {
  while (true) {
    monitor_USBHub();
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}

// *********************************
// UTILITY FUNCTIONS
// *********************************

// debug functions for W5500 etherent init
void resetW5500() {
  digitalWrite(CS_PIN, LOW);
  vTaskDelay(pdMS_TO_TICKS(100));
  digitalWrite(CS_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
}

void resetSPI() {
  SPI.end();
  vTaskDelay(pdMS_TO_TICKS(100));
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);  // Reinitialize clean SPI
  vTaskDelay(pdMS_TO_TICKS(100));
}

String applyTimezone(const String &tzName) {
  for (auto &tz : timezone_table) {
    if (tzName.equalsIgnoreCase(tz.name)) {
      setenv("TZ", tz.posix, 1);
      tzset();
      return String(tz.posix);
    }
  }
  // fallback
  setenv("TZ", "GMT0", 1);
  tzset();
  return "GMT0";
}

// function to get time
String getTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "TimeUnavailable";
  }

  char timeStr[20];  // YYYY-MM-DD HH:MM:SS
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  return String(timeStr);
}

// function to get date
String getDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "TimeUnavailable";
  }

  char dateStr[20];  // YYYY-MM-DD HH:MM:SS
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
  return String(dateStr);
}

// function to get number of WPS reboots
int getBootCount() {
  Preferences prefs;
  prefs.begin("bootdata", false);  // open NVS namespace in read/write mode

  String currentMD5 = ESP.getSketchMD5();  // hash of current firmware
  String storedMD5 = prefs.getString("firmware_md5", "");

  if (storedMD5 != currentMD5) {
    prefs.putUInt("boot_count", 0);
    prefs.putString("firmware_md5", currentMD5);
    Serial.println("Firmware changed. Boot count reset.");
    systemLog("Firmware changed. Boot count reset.");
  }

  uint32_t boot_count = prefs.getUInt("boot_count", 0);  // get value or 0 if not found
  boot_count = boot_count + 1;
  prefs.putUInt("boot_count", boot_count);

  prefs.end();  // close NVS

  Serial.print("Boot count: ");
  Serial.println(boot_count);
  Serial.println();

  systemLog("Boot count: ");
  systemLog(String(boot_count));
  return boot_count;
}

// function to list all files on SPIFFS
void listLogFiles() {
  Serial.println("Files in SPIFFS:");
  systemLog("Files in SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print(" ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    systemLog(" ");
    systemLog(String(file.name()));
    systemLog(" (");
    systemLog(String(file.size()));
    systemLog(" bytes)");
    file = root.openNextFile();
  }
  Serial.println();
}

// function to read current log file
void readLogFile(const String &filename) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open log file: " + filename);
    systemLog("Failed to open log file: " + filename);
    return;
  }

  Serial.println("Reading log file: " + filename);
  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.print(" ");
    Serial.println(line);
    systemLog(" ");
    systemLog(line);
  }

  file.close();
  Serial.println("Finished reading log.");
  Serial.println();
  systemLog("Finished reading log.");
}

// function to delete a file
void deleteFile(const String &filename) {
  if (SPIFFS.exists(filename)) {
    if (SPIFFS.remove(filename)) {
      Serial.println("File deleted: " + filename);
      systemLog("File deleted: " + filename);
    } else {
      Serial.println("Failed to delete file: " + filename);
      systemLog("Failed to delete file: " + filename);
    }
  } else {
    Serial.println("File does not exist: " + filename);
    systemLog("File does not exist: " + filename);
  }
}

// MIGHT REMOVE, replaces strdup() in loadConfig()
void replace_cstr(char *&dest, const char *src) {
  if (dest != NULL) {
    // only free heap pointers; check if dest points to a string literal?
    // We assume any prior strdup allocation should be freed.
    free(dest);
    dest = NULL;
  }
  if (src != NULL) {
    dest = strdup(src);
  } else {
    dest = NULL;
  }
}

// WIP, function to load config file and use parameters
bool loadConfig() {
  // create default config file if does not exist
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("Config file not found, creating default one...");
    systemLog("Config file not found, creating default one...");

    DynamicJsonDocument doc(2048);

    doc["siteID"] = config.siteID_param;

    doc["username"] = config.username_param;
    doc["password"] = config.password_param;

    doc["ethIP"] = config.ethIP_param.toString();
    doc["vsignIP"] = config.vsignIP_param.toString();
    doc["gateway"] = config.gateway_param.toString();
    doc["subnet"] = config.subnet_param.toString();
    doc["primaryDNS"] = config.primaryDNS_param.toString();
    doc["secondaryDNS"] = config.secondaryDNS_param.toString();
    doc["webPort"] = config.webPort_param;

    doc["enable_monitor_and_re_enable_autoping"] = config.enable_monitor_and_re_enable_autoping;
    doc["enable_re_enable_autoping_once_a_day"] = config.enable_re_enable_autoping_once_a_day;
    doc["enable_monitor_ssh"] = config.enable_monitor_ssh;
    doc["enable_monitor_modem"] = config.enable_monitor_modem;
    doc["enable_monitor_USBHub"] = config.enable_monitor_USBHub;
    doc["enable_monitor_ESwitch"] = config.enable_monitor_ESwitch;

    doc["mosfet_startup_delay"] = config.mosfet_startup_delay_param;
    doc["mosfet_1"] = config.mosfet_1;
    doc["mosfet_2"] = config.mosfet_2;
    doc["mosfet_3"] = config.mosfet_3;
    doc["mosfet_4"] = config.mosfet_4;

    doc["reboot_delay"] = config.reboot_delay_param;
    doc["wps_startup_delay"] = config.wps_startup_delay_param;
    doc["ssh_ping_interval"] = config.ssh_ping_interval_param;
    doc["time_allowance"] = config.time_allowance_param;
    // doc["reboot_wait"] = config.reboot_wait_param;
    doc["max_reboot_wait"] = config.max_reboot_wait_param;
    doc["wait_divisor"] = config.wait_divisor_param;

    doc["ntpServer"] = config.ntpServer_param;
    doc["gmtOffset_sec"] = config.gmtOffset_sec_param;
    doc["daylightOffset_sec"] = config.daylightOffset_sec_param;
    // doc["baudrate"] = config.baudrate_param;

    doc["timezone"] = config.timezone_param;

    doc["autoping_target1"] = config.autoping_target1_param.toString();
    doc["autoping_target2"] = config.autoping_target2_param.toString();
    doc["autoping_target3"] = config.autoping_target3_param.toString();
    doc["enable_autoping"] = config.enable_autoping;
    doc["autoping_interval"] = config.autoping_interval_param;
    doc["autoping_reboot_timeout"] = config.autoping_reboot_timeout_param;
    doc["autoping_ping_responses"] = config.autoping_ping_responses_param;
    doc["autoping_reboot_attempts"] = config.autoping_reboot_attempts_param;
    doc["autoping_total_reboot_attempts"] = config.autoping_total_reboot_attempts_param;
    doc["autoping_reboot_delay"] = config.autoping_reboot_delay_param;

    File newFile = SPIFFS.open("/config.json", FILE_WRITE);
    if (!newFile) {
      Serial.println("Failed to create default config file.");
      systemLog("Failed to create default config file.");
      return false;
    }

    serializeJsonPretty(doc, newFile);
    newFile.close();
    Serial.println("Default config file created.");
    systemLog("Default config file created.");
  }

  // load if config file if exists
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    systemLog("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 2048) {
    Serial.println("Config file too large");
    systemLog("Config file too large");
    return false;
  }

  // Use a DynamicJsonDocument with enough capacity
  DynamicJsonDocument doc(2048);
  // int oldWebPort = config.webPort_param;
  // bool webPortChanged = false;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("Failed to parse config file: ");
    Serial.println(error.c_str());
    systemLog("Failed to parse config file: ");
    systemLog(String(error.c_str()));
    return false;
  }

  // Set fields if they exist
  if (doc.containsKey("siteID")) config.siteID_param = doc["siteID"].as<String>();

  if (doc.containsKey("username")) config.username_param = strdup(doc["username"]);
  if (doc.containsKey("password")) config.password_param = strdup(doc["password"]);
  // if (doc.containsKey("username")) replace_cstr(config.username_param, doc["username"]);
  // if (doc.containsKey("password")) replace_cstr(config.password_param, doc["password"]);

  if (doc.containsKey("ethIP")) config.ethIP_param.fromString(doc["ethIP"].as<const char *>());
  if (doc.containsKey("vsignIP")) config.vsignIP_param.fromString(doc["vsignIP"].as<const char *>());
  if (doc.containsKey("gateway")) config.gateway_param.fromString(doc["gateway"].as<const char *>());
  if (doc.containsKey("subnet")) config.subnet_param.fromString(doc["subnet"].as<const char *>());
  if (doc.containsKey("primaryDNS")) config.primaryDNS_param.fromString(doc["primaryDNS"].as<const char *>());
  if (doc.containsKey("secondaryDNS")) config.secondaryDNS_param.fromString(doc["secondaryDNS"].as<const char *>());
  if (doc.containsKey("webPort")) config.webPort_param = doc["webPort"].as<uint16_t>();

  if (doc.containsKey("enable_monitor_and_re_enable_autoping")) config.enable_monitor_and_re_enable_autoping = doc["enable_monitor_and_re_enable_autoping"];
  if (doc.containsKey("enable_re_enable_autoping_once_a_day")) config.enable_re_enable_autoping_once_a_day = doc["enable_re_enable_autoping_once_a_day"];
  if (doc.containsKey("enable_monitor_ssh")) config.enable_monitor_ssh = doc["enable_monitor_ssh"];
  if (doc.containsKey("enable_monitor_modem")) config.enable_monitor_modem = doc["enable_monitor_modem"];
  if (doc.containsKey("enable_monitor_USBHub")) config.enable_monitor_USBHub = doc["enable_monitor_USBHub"];
  if (doc.containsKey("enable_monitor_ESwitch")) config.enable_monitor_ESwitch = doc["enable_monitor_ESwitch"];

  if (doc.containsKey("mosfet_startup_delay")) config.mosfet_startup_delay_param = doc["mosfet_startup_delay"];
  if (doc.containsKey("mosfet_1")) config.mosfet_1 = doc["mosfet_1"];
  if (doc.containsKey("mosfet_2")) config.mosfet_2 = doc["mosfet_2"];
  if (doc.containsKey("mosfet_3")) config.mosfet_3 = doc["mosfet_3"];
  if (doc.containsKey("mosfet_4")) config.mosfet_4 = doc["mosfet_4"];

  if (doc.containsKey("reboot_delay")) config.reboot_delay_param = doc["reboot_delay"];
  if (doc.containsKey("wps_startup_delay")) config.wps_startup_delay_param = doc["wps_startup_delay"];
  if (doc.containsKey("ssh_ping_interval")) config.ssh_ping_interval_param = doc["ssh_ping_interval"];
  if (doc.containsKey("time_allowance")) config.time_allowance_param = doc["time_allowance"];
  // if (doc.containsKey("reboot_wait")) config.reboot_wait_param = doc["reboot_wait"];
  if (doc.containsKey("max_reboot_wait")) config.max_reboot_wait_param = doc["max_reboot_wait"];
  if (doc.containsKey("wait_divisor")) config.wait_divisor_param = doc["wait_divisor"];

  if (doc.containsKey("ntpServer")) config.ntpServer_param = strdup(doc["ntpServer"]);
  // if (doc.containsKey("ntpServer")) replace_cstr(config.ntpServer_param, doc["ntpServer"]);
  if (doc.containsKey("gmtOffset_sec")) config.gmtOffset_sec_param = doc["gmtOffset_sec"];
  if (doc.containsKey("daylightOffset_sec")) config.daylightOffset_sec_param = doc["daylightOffset_sec"];
  // if (doc.containsKey("baudrate")) config.baudrate_param = doc["baudrate"];

  if (doc.containsKey("timezone")) config.timezone_param = strdup(doc["timezone"]);
  // if (doc.containsKey("timezone")) replace_cstr(config.timezone_param, doc["timezone"]);

  if (doc.containsKey("autoping_target1")) config.autoping_target1_param.fromString(doc["autoping_target1"].as<const char *>());
  if (doc.containsKey("autoping_target2")) config.autoping_target2_param.fromString(doc["autoping_target2"].as<const char *>());
  if (doc.containsKey("autoping_target3")) config.autoping_target3_param.fromString(doc["autoping_target3"].as<const char *>());
  if (doc.containsKey("enable_autoping")) config.enable_autoping = doc["enable_autoping"];
  if (doc.containsKey("autoping_interval")) config.autoping_interval_param = doc["autoping_interval"];
  if (doc.containsKey("autoping_ping_responses")) config.autoping_ping_responses_param = doc["autoping_ping_responses"];
  if (doc.containsKey("autoping_reboot_timeout")) config.autoping_reboot_timeout_param = doc["autoping_reboot_timeout"];
  if (doc.containsKey("autoping_reboot_attempts")) config.autoping_reboot_attempts_param = doc["autoping_reboot_attempts"];
  if (doc.containsKey("autoping_total_reboot_attempts")) config.autoping_total_reboot_attempts_param = doc["autoping_total_reboot_attempts"];
  if (doc.containsKey("autoping_reboot_delay")) config.autoping_reboot_delay_param = doc["autoping_reboot_delay"];

  Serial.println("Config loaded successfully.");
  return true;
}

// WIP, function to show configurations used
void showConfig() {
  char *filename = "/config.json";
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    systemLog("Failed to mount SPIFFS");
    return;
  }

  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open config file: " + String(filename));
    systemLog("Failed to open config file: " + String(filename));
    return;
  }

  Serial.println("====== Raw Config File ======");
  systemLog("====== Raw Config File ======");
  while (file.available()) {
    Serial.write(file.read());  // Reads raw bytes and prints them
  }
  Serial.println("\n=============================");
  systemLog("\n=============================");
  file.close();
}

// NEEDS TESTING
void saveConfig() {
  DynamicJsonDocument doc(2048);

  doc["siteID"] = config.siteID_param;

  doc["username"] = config.username_param;
  doc["password"] = config.password_param;

  doc["ethIP"] = config.ethIP_param.toString();
  doc["vsignIP"] = config.vsignIP_param.toString();
  doc["gateway"] = config.gateway_param.toString();
  doc["subnet"] = config.subnet_param.toString();
  doc["primaryDNS"] = config.primaryDNS_param.toString();
  doc["secondaryDNS"] = config.secondaryDNS_param.toString();
  doc["webPort"] = config.webPort_param;

  doc["mosfet_startup_delay"] = config.mosfet_startup_delay_param;
  doc["mosfet_1"] = config.mosfet_1;
  doc["mosfet_2"] = config.mosfet_2;
  doc["mosfet_3"] = config.mosfet_3;
  doc["mosfet_4"] = config.mosfet_4;

  doc["reboot_delay"] = config.reboot_delay_param;
  doc["wps_startup_delay"] = config.wps_startup_delay_param;
  doc["ssh_ping_interval"] = config.ssh_ping_interval_param;
  doc["time_allowance"] = config.time_allowance_param;
  doc["max_reboot_wait"] = config.max_reboot_wait_param;
  doc["wait_divisor"] = config.wait_divisor_param;

  doc["ntpServer"] = config.ntpServer_param;
  doc["gmtOffset_sec"] = config.gmtOffset_sec_param;
  doc["daylightOffset_sec"] = config.daylightOffset_sec_param;

  doc["timezone"] = config.timezone_param;

  doc["enable_monitor_and_re_enable_autoping"] = config.enable_monitor_and_re_enable_autoping;
  doc["enable_re_enable_autoping_once_a_day"] = config.enable_re_enable_autoping_once_a_day;
  doc["enable_monitor_ssh"] = config.enable_monitor_ssh;
  doc["enable_monitor_modem"] = config.enable_monitor_modem;
  doc["enable_monitor_USBHub"] = config.enable_monitor_USBHub;
  doc["enable_monitor_ESwitch"] = config.enable_monitor_ESwitch;

  doc["autoping_target1"] = config.autoping_target1_param.toString();
  doc["autoping_target2"] = config.autoping_target2_param.toString();
  doc["autoping_target3"] = config.autoping_target3_param.toString();

  doc["enable_autoping"] = config.enable_autoping;
  doc["autoping_interval"] = config.autoping_interval_param;
  doc["autoping_reboot_timeout"] = config.autoping_reboot_timeout_param;
  doc["autoping_ping_responses"] = config.autoping_ping_responses_param;
  doc["autoping_reboot_attempts"] = config.autoping_reboot_attempts_param;
  doc["autoping_total_reboot_attempts"] = config.autoping_total_reboot_attempts_param;
  doc["autoping_reboot_delay"] = config.autoping_reboot_delay_param;

  File configFile = SPIFFS.open("/config.json", FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  serializeJsonPretty(doc, configFile);
  configFile.close();
}

// safe delay function that works beyond FreeRTOS 1 hour limit
void workaround_delay_ms(uint64_t total_ms) {
  const uint32_t chunk = 60 * 60 * 1000;  // FreeRTOS max ms
  while (total_ms > chunk) {
    vTaskDelay(pdMS_TO_TICKS(chunk));
    total_ms = total_ms - chunk;
  }
  if (total_ms > 0) {
    vTaskDelay(pdMS_TO_TICKS(total_ms));
  }
}

// *********************************
// INITIALIZATION FUNCTIONS
// *********************************

// function to handle Ethernet initialization and OTA initialization
void handleInits() {
  static bool bootLogged = false;
  // static bool timezoneApplied = false;
  // static bool ntpSynced = false;
  // static String tzStr = "";

  if (!ETH.linkUp()) {
    digitalWrite(LED_BUILTIN, HIGH);  // OFF
    ethernetConnected = false;
    initEthernet();
    // ntpSynced = false;
  } else if (!ethernetConnected) {
    // first time cable is plugged back in
    digitalWrite(LED_BUILTIN, HIGH);  // OFF
    initEthernet();
    // ntpSynced = false;
  }

  if (ethernetConnected) {
    digitalWrite(LED_BUILTIN, LOW);  // ON

    // if (!timezoneApplied) {
    //   // returns posix timezone string (e.g., "PST8PDT,M3.2.0,M11.1.0");
    //   String tzStr = applyTimezone(config.timezone_param);
    //   timezoneApplied = true;
    // }

    // if (!ntpSynced) {
    //   // configTime(config.gmtOffset_sec_param, config.daylightOffset_sec_param, config.ntpServer_param);  // call this after Ethernet connected
    //   configTzTime(tzStr.c_str(), config.ntpServer_param); // sync NTP with timezone
    //   ntpSynced = true;
    // }

    if (!otaInitialized) {
      initOTA();
    }

    ArduinoOTA.handle();
    String tzStr = applyTimezone(config.timezone_param);
    configTzTime(tzStr.c_str(), config.ntpServer_param);  // sync NTP with timezone

    if (!bootLogged) {
      // log WPS boot count
      String blog = "Boot " + String(boot);
      Serial.println(blog);
      systemLog(blog);
      log_it(blog);
      bootLogged = true;
    }
  }
}

// function to inititialize W5500 Ethernet module
void initEthernet() {
  resetW5500();
  resetSPI();

  Serial.println("Attempting Ethernet init ...");
  systemLog("Attempting Ethernet init ...");


  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
  vTaskDelay(pdMS_TO_TICKS(1000));
  // ETH.config(mac, ethIP, gateway, subnet, primaryDNS, secondaryDNS);
  ETH.config(mac, config.ethIP_param, config.gateway_param, config.subnet_param, config.primaryDNS_param, config.secondaryDNS_param);
  vTaskDelay(pdMS_TO_TICKS(1000));

  if (!ETH.linkUp()) {
    systemLog("Ethernet cable not connected or W5500 not responding!");
    Serial.println("Ethernet cable not connected or W5500 not responding!");
    ethernetConnected = false;
    return;
  }

  // if (!ETH.config(ethIP, gateway, subnet, primaryDNS, secondaryDNS)) {
  //   Serial.println("Failed to configure static IP");
  // }
  if (!ETH.config(config.ethIP_param, config.gateway_param, config.subnet_param, config.primaryDNS_param, config.secondaryDNS_param)) {
    Serial.println("Failed to configure static IP!");
    systemLog("Failed to configure static IP!");
  }

  if (!ETH.localIP()) {
    Serial.println("Failed to obtain IP!");
    systemLog("Failed to obtain IP!");
    ethernetConnected = false;
    return;
  }

  Serial.println("Ethernet initialized");
  Serial.print("IP Address: ");
  Serial.println(ETH.localIP());
  Serial.println();
  Serial.flush();


  systemLog("Ethernet initialized");
  systemLog("IP Address: ");
  systemLog(String(ETH.localIP()));
  // systemLog("Ethernet initialized");
  // systemLog("IP Address: " +  String(ETH.localIP().toString()));

  ethernetConnected = true;
  otaInitialized = false;  // reset OTA flag to re-init OTA
}

// NOT CALLED
void initEthernetWithFallback() {
  resetW5500();
  resetSPI();

  Serial.println("Attempting Ethernet init ...");
  systemLog("Attempting Ethernet init ...");

  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
  vTaskDelay(pdMS_TO_TICKS(1000));

  // attempt static IP first
  Serial.println("Configuring static IP ...");
  systemLog("Configuring static IP ...");
  if (!ETH.config(config.ethIP_param, config.gateway_param, config.subnet_param, config.primaryDNS_param, config.secondaryDNS_param)) {
    Serial.println("Failed to configure static IP!");
    systemLog("Failed to configure static IP!");
  }

  vTaskDelay(pdMS_TO_TICKS(1000));

  if (!ETH.linkUp()) {
    Serial.println("Ethernet cable not connected or W5500 not responding.");
    systemLog("Ethernet cable not connected or W5500 not responding.");
    ethernetConnected = false;
    return;
  }

  // verify same subnet
  bool sameSubnet = (((uint32_t)ETH.localIP() & (uint32_t)config.subnet_param)) == (((uint32_t)config.gateway_param & (uint32_t)config.subnet_param));
  bool gatewayReachable = false;

  if (sameSubnet) {
    Serial.print("Waiting for gateway ");
    Serial.println(config.gateway_param);
    systemLog("Waiting for gateway ");
    systemLog(String(config.gateway_param));
    const unsigned long maxAttempts = 12;  // number of tries
    // const unsigned long delay = 30 * 1000; // delay between tries

    for (int i = 0; i < maxAttempts; i++) {
      if (ping_gateway()) {
        Serial.println("Gateway reachable with static IP");
        Serial.println("Ethernet initialized");
        Serial.print("IP Address: ");
        Serial.println(ETH.localIP());
        Serial.println();
        Serial.flush();

        systemLog("Gateway reachable with static IP");
        systemLog("Ethernet initialized");
        systemLog("IP Address: ");
        systemLog(String(ETH.localIP()));
        gatewayReachable = true;
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(5000));  // wait 5 seconds between tries
    }
  }

  // fall back to DHCP if needed
  if (!sameSubnet || !gatewayReachable) {
    Serial.println("Static IP failed. Falling back to DHCP ...");
    systemLog("Static IP failed. Falling back to DHCP ...");

    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (!ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE)) {
      Serial.println("Failed to start DHCP!");
      systemLog("Failed to start DHCP!");
      ethernetConnected = false;
      return;
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
    if (ping_gateway()) {
      Serial.println("Gateway reachable with DHCP");
      Serial.println("Ethernet initialized");
      Serial.print("IP Address: ");
      Serial.println(ETH.localIP());
      Serial.println();
      Serial.flush();
      systemLog("Gateway reachable with DHCP");
      systemLog("Ethernet initialized");
      systemLog("IP Address: ");
      systemLog(String(ETH.localIP()));
      ethernetConnected = true;
    } else {
      Serial.println("Gateway still unreachable after DHCP!");
      systemLog("Gateway still unreachable after DHCP!");
      ethernetConnected = false;
    }
  }
}

// function to setup Ethernet OTA
void initOTA() {
  ArduinoOTA.setHostname("esp32s3-ethernet");
  ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
    systemLog("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
    systemLog("\End OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    systemLog("Progress: %u%%\r" + String((progress / (total / 100))));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");


    systemLog("Error[%u]: " + String(error));
    if (error == OTA_AUTH_ERROR) systemLog("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) systemLog("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) systemLog("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) systemLog("Receive Failed");
    else if (error == OTA_END_ERROR) systemLog("End Failed");
  });

  ArduinoOTA.begin();
  otaInitialized = true;
  Serial.println("OTA service started.");
  Serial.println();
  Serial.flush();
  systemLog("OTA service started.");
}

// *********************************
// WEB FUNCTIONS
// *********************************

bool isAuthenticated() {
  if (server->hasHeader("Cookie")) {
    String cookie = server->header("Cookie");
    Serial.println("Cookie: " + cookie);
    systemLog("Cookie: " + String(cookie));
    return cookie.indexOf("session=valid") != -1;
  }
  Serial.println("No Cookie header");
  systemLog("No Cookie header");
  return false;
}

void handleRoot() {
  if (isAuthenticated()) {
    server->sendHeader("Location", "/log");  // default landing page
    server->send(302, "text/plain", "Redirecting ...");
    return;
  }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Login</title>";
  html += "<style>body{font-family:sans-serif; padding:20px;} label{display:block; margin-top:10px;} input{width:300px; padding:5px;} input[type=submit]{width:auto;}</style>";
  html += "</head><body>";
  html += "<h2>Login</h2><form method='POST' action='/login'>";
  html += "Username: <input type='text' name='username'><br>";
  html += "Password: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Login'></form></body></html>";
  server->send(200, "text/html", html);
}

void handleLogin() {
  if (!server->hasArg("username") || !server->hasArg("password")) {
    server->send(400, "text/plain", "Bad Request");
    return;
  }

  String username = server->arg("username");
  String password = server->arg("password");

  if (username == config.username_param && password == config.password_param) {
    // Correct credentials — set cookie and show redirect page
    server->sendHeader("Set-Cookie", "session=valid; Path=/; HttpOnly");
    server->send(200, "text/html", "<!DOCTYPE html><html><body>Login successful. Redirecting...<script>window.location.href='/log';</script></body></html>");
  } else {
    server->send(403, "text/plain", "Forbidden: Invalid credentials");
  }
}


void handleLog() {
  if (!server->authenticate(config.username_param, config.password_param)) {
    return server->requestAuthentication();
  }

  String filename = "/" + config.siteID_param + "-" + getDate() + ".txt";
  if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
    server->send(500, "text/plain", "SPIFFS busy.");
    return;
  }

  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    xSemaphoreGive(spiffsMutex);
    server->send(404, "text/plain", "File not found: " + filename);
    return;
  }

  // --- Build styled HTML page ---
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Log</title>";

  // reuse same CSS as systemlog/config
  html += R"rawliteral(
    <style>
      .header {
        background-color: #A9a9a9;
        color: #fff;
        display: flex;
        align-items: center;
        height: 80px;
        justify-content: space-between;
        padding: 0 20px;
      }
      .header-left {
        background-color: #A9a9a9;
        display: flex;
        align-items: center;
        gap: 15px;
        width: 20%;
        height: 100%;
        justify-content: center;
      }
      .header-left img {
        width: 100%;
        height: 100%;
        object-fit: contain;
      }
      .header-info {
        flex: 1;
        padding: 0 20px;
        text-align: right;
        font-size: 18px;
        color: black;
      }
      .header-nav {
        display: flex;
        gap: 15px;
      }
      .header-nav a {
        color: black;
        text-decoration: none;
        font-weight: bold;
        padding: 6px 12px;
        border-radius: 5px;
      }
      .header-nav a:hover {
        background: #145a9c;
        color: #fff;
      }
      body {
        font-family: Arial, sans-serif;
        background-color: #e8e8e8;
        margin: 0;
        height: 100vh;
        display: flex;
        flex-direction: column;
      }
      .content {
        flex: 1;
        background: #A9a9a9;
        padding: 20px;
        overflow-y: auto;
        height: calc(100vh - 80px);
        box-sizing: border-box;
      }
      .log-box {
        background: #000;
        color: #0f0;
        padding: 10px;
        border: 1px solid #ccc;
        font-family: monospace;
        white-space: pre-wrap;
        overflow-y: auto;
        height: 100%;
      }
    </style>
  )rawliteral";

  html += "</head><body>";

  // --- Header ---
  html += R"rawliteral(
    <div class="header">
      <div class="header-left">
        <img src = data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAVIAAADCCAYAAAAbxEoYAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA3ZpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuNi1jMTQ1IDc5LjE2MzQ5OSwgMjAxOC8wOC8xMy0xNjo0MDoyMiAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDplNmQwYjFjMS0xMTIzLWI0NDYtODdiMi00MGY2MTBlODljNDQiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6MjhEQjlDOEQxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MjhEQjlDOEMxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENDIDIwMTkgKFdpbmRvd3MpIj4gPHhtcE1NOkRlcml2ZWRGcm9tIHN0UmVmOmluc3RhbmNlSUQ9InhtcC5paWQ6ZTAyNGIzNmMtYWEwMi04ODQxLTk5OTEtOGEzNWIwNjNkODhjIiBzdFJlZjpkb2N1bWVudElEPSJ4bXAuZGlkOmU2ZDBiMWMxLTExMjMtYjQ0Ni04N2IyLTQwZjYxMGU4OWM0NCIv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 alt="VSign Logo">
      </div>
      <div class="header-nav">
        <a href="/config">Config</a>
        <a href="/log">Log</a>
        <a href="/systemlog">System Log</a>
        <a href="/upload">Upload</a>
      </div>
        <div class="header-info">
          <strong>XIAO_ESP32S3</strong><br>
          Site: <span>192 OFFICE</span>
        </div>
    </div>
  )rawliteral";

  // --- Main Content ---
  html += "<div class='content'>";
  html += "<h2>Log File: " + filename + "</h2>";
  html += "<div class='log-box'>";

  while (file.available()) {
    html += file.readStringUntil('\n') + "<br>";
  }

  html += "</div></div></body></html>";

  file.close();
  xSemaphoreGive(spiffsMutex);

  server->send(200, "text/html", html);
}


void handleMosfet() {
  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>MOSFET Control</title>";
  html += "<style>body{font-family:sans-serif; padding:20px;} label{display:block; margin-top:10px;} input{width:300px; padding:5px;} input[type=submit]{width:auto;}</style>";
  html += "</head><body>";
  html += "<h2>MOSFET Control</h2>";
  html += "<form method='POST' action='/saveConfig'>";

  html += "<label>MOSFET Startup Delay (s): <input type='number' name='mosfet_startup_delay' value='" + String(config.mosfet_startup_delay_param / 1000) + "'></label>";

  html += "<label>MOSFET 1: <input name='mosfet_1' value='" + String(config.mosfet_1) + "'></label>";
  html += "<label>MOSFET 2: <input name='mosfet_2' value='" + String(config.mosfet_2) + "'></label>";
  html += "<label>MOSFET 3: <input name='mosfet_3' value='" + String(config.mosfet_3) + "'></label>";
  html += "<label>MOSFET 4: <input name='mosfet_4' value='" + String(config.mosfet_4) + "'></label>";

  html += "<br><input type='submit' value='Save & Reload Config'>";
  html += "</form>";

  html += "<form method='POST' action='/reboot' onsubmit='return confirm(\"Are you sure you want to reboot the ESP32S3?\");'>";
  html += "<input type='submit' value='Reboot ESP32S3' style='margin-top:20px; background:red; color:white;'>";
  html += "</form>";

  html += "</body></html>";

  server->send(200, "text/html", html);
}

// *********************************
// handleConfig - Restructured with Tabs
// *********************************


void handleConfig() {
  if (!server->authenticate(config.username_param, config.password_param)) {
    return server->requestAuthentication();
  }

  String html = R"rawliteral(
    <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Configuration</title>
        <style>
          .header {
            background-color: #A9a9a9;
            color: #fff;
            display: flex;
            align-items: center;
            height: 80px; /* fixed header height */
            justify-content: space-between;
            padding: 0 20px;
          }
          .header-left {
            background-color: #A9a9a9;
            display: flex;
            align-items: center;
            gap: 15px;
            width: 20%; /* same as sidebar */
            height: 100%; /* same height as header */
            justify-content: center;
          }

          .header-left img {
            width: 100%;
            height: 100%;
            object-fit: contain; /* keeps aspect ratio */
          }
          .header-info {
            flex: 1;
            padding: 0 20px;
            text-align: right;
            font-size: 18px;
            color: black;
          }

          .header-nav {
            display: flex;
            gap: 15px;
          }

          .header-nav a {
            color: black; /* or #fff if you want white links */
            text-decoration: none;
            font-weight: bold;
            padding: 6px 12px;
            border-radius: 5px;
          }

          .header-nav a:hover {
            background: #145a9c;
            color: #fff;
          }
          .main {
            flex: 1;
            display: flex;
            height: calc(100vh - 80px);
          }
          .sidebar {
            width: 20%;
            background-color: #A9a9a9;
            color: #fff;
            display: flex;
            flex-direction: column;
            /* align-items: stretch; */
            padding-top: 20px;
            height: 100%;
          }
          .sidebar button {
            margin: 5px 10px;
            padding: 12px;
            background: #1e6cc1; 
            border: none;
            color: #fff;
            font-size: 16px;
            cursor: pointer;
            border-radius: 5px;
            text-align: left;
            padding-left: 20px;
          }
          .sidebar button:hover {
            background: #145a9c;
          }
          .content {
            flex: 1;
            background: #A9a9a9;
            padding: 20px;
            overflow-y: auto;
            height: 100%;
            box-sizing: border-box;
          }
          body {
            font-family: Arial, sans-serif;
            background-color: #e8e8e8;
            margin: 0;
            height: 100vh;
            display: flex;
            flex-direction: column;
          }
          .tab { overflow: hidden; border-bottom: 1px solid #ccc; background: #fff; }
          .tabcontent {
            display: none;
            padding: 20px;
            border: 2px solid #ccc;
            /* border-top: solid; */
            background: #fff;
          }
          .form-section label { display: block; margin: 6px 0 2px; }
          .form-section input { width: 100%; padding: 6px; margin-bottom: 10px;}
        </style>
      </head>
      <body>
        <div class="header">
          <div class = "header-left">
            <img src = data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAVIAAADCCAYAAAAbxEoYAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA3ZpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuNi1jMTQ1IDc5LjE2MzQ5OSwgMjAxOC8wOC8xMy0xNjo0MDoyMiAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDplNmQwYjFjMS0xMTIzLWI0NDYtODdiMi00MGY2MTBlODljNDQiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6MjhEQjlDOEQxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MjhEQjlDOEMxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENDIDIwMTkgKFdpbmRvd3MpIj4gPHhtcE1NOkRlcml2ZWRGcm9tIHN0UmVmOmluc3RhbmNlSUQ9InhtcC5paWQ6ZTAyNGIzNmMtYWEwMi04ODQxLTk5OTEtOGEzNWIwNjNkODhjIiBzdFJlZjpkb2N1bWVudElEPSJ4bXAuZGlkOmU2ZDBiMWMxLTExMjMtYjQ0Ni04N2IyLTQwZjYxMGU4OWM0NCIv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 alt="VSign Logo">
          </div>
          <div class = "header-nav">
            <a href="/config">Config</a>
            <a href="/log">Log</a>
            <a href="/systemlog">SystemLog</a>
            <a href="/upload">Upload</a>
          </div>
        <div class="header-info">
          <strong>XIAO_ESP32S3</strong><br>
          Site: <span>192 OFFICE</span>
        </div>
        </div>
        <form method="POST" action="/saveConfig">
          <!-- Tabs header -->
          <div class = "main">
            <div class = "sidebar">
              <button type="button" class="tablinks" onclick="openTab(event,'network')" id="defaultOpen">Network</button>
              <button type="button" class="tablinks" onclick="openTab(event,'system')">System</button>
              <button type="button" class="tablinks" onclick="openTab(event,'mosfet')">DC Control</button>
              <button type="button" class="tablinks" onclick="openTab(event,'monitor')">Monitoring</button>
              <button type="button" class="tablinks" onclick="openTab(event,'autoping')">AutoPing</button>
              <button type="button" class="tablinks" onclick="openTab(event,'authentication')">Authentication</button>
            </div>
            <!-- Network Tab -->
            <div class ="content">
              <div id="network" class="tabcontent">
                <h3>Network Settings</h3>
                <div class="form-section">
                )rawliteral";

  html += "<label>Site ID</label><input type='text' name='siteID' id='siteID' value='" + config.siteID_param + "'><br>";
  html += "<label>Device IP</label><input type='text' name='ethIP' id='ethIP' value='" + String(config.ethIP_param.toString()) + "'><br>";
  html += "<label>VSign IP</label><input type='text' name='vsignIP' id='vsignIP' value='" + String(config.vsignIP_param.toString()) + "'><br>";
  html += "<label>Gateway</label><input type='text' name='gateway' id='gateway' value='" + String(config.gateway_param.toString()) + "'><br>";
  html += "<label>Subnet</label><input type='text' name='subnet' id='subnet' value='" + String(config.subnet_param.toString()) + "'><br>";
  html += "<label>Primary DNS</label><input type='text' name='primaryDNS' id='primaryDNS' value='" + String(config.primaryDNS_param.toString()) + "'><br>";
  html += "<label>Secondary DNS</label><input type='text' name='secondaryDNS' id='secondaryDNS' value='" + String(config.secondaryDNS_param.toString()) + "'><br>";
  html += "<label>Web Port</label><input type='number' id='webPort' name='webPort' value='" + String(config.webPort_param) + "'><br>";

  html += R"rawliteral(
                </div>
              </div>

              <!-- System Tab -->
              <div id="system" class="tabcontent">
                <h3>System Settings</h3>
                <div class="form-section">
              )rawliteral";

  html += "<label>Reboot Delay (s)</label><input type='number' name='reboot_delay' value='" + String(config.reboot_delay_param / 1000) + "'>";
  html += "<label>WPS Startup Delay (s)</label><input type='number' name='wps_startup_delay' value='" + String(config.wps_startup_delay_param / 1000) + "'>";
  html += "<label>MOSFET Startup Delay (s)</label><input type='number' name='mosfet_startup_delay' value='" + String(config.mosfet_startup_delay_param / 1000) + "'>";
  html += "<label>NTP Server</label><input type='text' name='ntpServer' value='" + String(config.ntpServer_param) + "'>";
  html += "<label>GMT Offset (sec)</label><input type='number' name='gmtOffset' value='" + String(config.gmtOffset_sec_param) + "'>";
  html += "<label>Daylight Offset (sec)</label><input type='number' name='daylightOffset' value='" + String(config.daylightOffset_sec_param) + "'>";
  html += "<label>Timezone</label><input type='text' name='timezone' value='" + String(config.timezone_param) + "'>";
  html += R"rawliteral(
                </div>
              </div>

              <!-- MOSFETs Tab -->
              <div id="mosfet" class="tabcontent">
                <h3>MOSFET Control</h3>
                <div class="form-section">
              )rawliteral";

  html += "<label>MOSFET 1</label><input type='number' name='mosfet_1' value='" + String(config.mosfet_1) + "'>";
  html += "<label>MOSFET 2</label><input type='number' name='mosfet_2' value='" + String(config.mosfet_2) + "'>";
  html += "<label>MOSFET 3</label><input type='number' name='mosfet_3' value='" + String(config.mosfet_3) + "'>";
  html += "<label>MOSFET 4</label><input type='number' name='mosfet_4' value='" + String(config.mosfet_4) + "'>";
  html += R"rawliteral(
                </div>
              </div>

              <!-- Monitoring Tab -->
              <div id="monitor" class="tabcontent">
                <h3>Monitoring</h3>
                <div class="form-section">
              )rawliteral";

  html += "<label>SSH Ping Interval (s)</label><input type='number' name='ssh_ping_interval' value='" + String(config.ssh_ping_interval_param / 1000) + "'>";
  html += "<label>Time Allowance (s)</label><input type='number' name='time_allowance' value='" + String(config.time_allowance_param / 1000) + "'>";
  html += "<label>Max Reboot Wait (s)</label><input type='number' name='max_reboot_wait' value='" + String(config.max_reboot_wait_param / 1000) + "'>";
  html += "<label>Wait Divisor (s)</label><input type='number' name='wait_divisor' value='" + String(config.wait_divisor_param / 1000) + "'>";

  html += "<label>Enable Monitor And Re-Enable Autoping</label><input type='number' name='enable_monitor_and_re_enable_autoping' value='" + String(config.enable_monitor_and_re_enable_autoping) + "'>";

  html += "<label>Enable Re-Enable Autoping Once A Day</label><input type='number' name='enable_re_enable_autoping_once_a_day' value='" + String(config.enable_re_enable_autoping_once_a_day) + "'>";

  // html += "<label>Enable Monitor SSH</label><input type='checkbox' name='enable_monitor_ssh' ";
  // html += (config.enable_monitor_ssh ? "checked" : "");
  // html += ">";
  html += "<label>Enable Monitor SSH</label><input type='number' name='enable_monitor_ssh' value='" + String(config.enable_monitor_ssh) + "'>";

  // html += "<label>Enable Monitor Modem</label><input type='checkbox' name='enable_monitor_modem' ";
  // html += (config.enable_monitor_modem ? "checked" : "");
  // html += ">";
  html += "<label>Enable Monitor Modem</label><input type='number' name='enable_monitor_modem' value='" + String(config.enable_monitor_modem) + "'>";

  // html += "<label>Enable Monitor USB Hub</label><input type='checkbox' name='enable_monitor_USBHub' ";
  // html += (config.enable_monitor_USBHub ? "checked" : "");
  // html += ">";
  html += "<label>Enable Monitor USB Hub</label><input type='number' name='enable_monitor_USBHub' value='" + String(config.enable_monitor_USBHub) + "'>";

  // html += "<label>Enable Monitor ESwitch</label><input type='checkbox' name='enable_monitor_ESwitch' ";
  // html += (config.enable_monitor_ESwitch ? "checked" : "");
  // html += ">";
  html += "<label>Enable Monitor ESwitch</label><input type='number' name='enable_monitor_ESwitch' value='" + String(config.enable_monitor_ESwitch) + "'>";
  html += R"rawliteral(
                </div>
              </div>

              <!-- AutoPing Tab -->
              <div id="autoping" class="tabcontent">
                <h3>AutoPing</h3>
                <div class="form-section">
              )rawliteral";
  html += "<label>Target 1</label><input type='text' name='autoping_target1' value='" + String(config.autoping_target1_param.toString()) + "'>";
  html += "<label>Target 2</label><input type='text' name='autoping_target2' value='" + String(config.autoping_target2_param.toString()) + "'>";
  html += "<label>Target 3</label><input type='text' name='autoping_target3' value='" + String(config.autoping_target3_param.toString()) + "'>";

  // html += "<label>Enable AutoPing</label><input type='checkbox' name='enable_autoping' ";
  // html += (config.enable_autoping ? "checked" : "");
  // html += ">";
  html += "<label>Enable Autoping</label><input type='number' name='enable_autoping' value='" + String(config.enable_autoping) + "'>";

  html += "<label>Interval (s)</label><input type='number' name='autoping_interval' value='" + String(config.autoping_interval_param / 1000) + "'>";
  html += "<label>Reboot Timeout (s)</label><input type='number' name='autoping_reboot_timeout' value='" + String(config.autoping_reboot_timeout_param / 1000) + "'>";
  html += "<label>Ping Responses Required</label><input type='number' name='autoping_ping_responses' value='" + String(config.autoping_ping_responses_param) + "'>";
  html += "<label>Reboot Attempts</label><input type='number' name='autoping_reboot_attempts' value='" + String(config.autoping_reboot_attempts_param) + "'>";
  html += "<label>Total Reboot Attempts</label><input type='number' name='autoping_total_reboot_attempts' value='" + String(config.autoping_total_reboot_attempts_param) + "'>";
  html += "<label>Reboot Delay (s)</label><input type='number' name='autoping_reboot_delay' value='" + String(config.autoping_reboot_delay_param / 1000) + "'>";
  html += R"rawliteral(
                </div>
              </div
              ><!-- Authentication Tab -->
              <div id="authentication" class="tabcontent">
                <h3>Authentication</h3>
                <div class="form-section">
              )rawliteral";
  html += "<label>Username</label><input type='text' name='username' value='" + String(config.username_param) + "'>";
  html += "<label>Password</label><input type='text' name='password' value='" + String(config.password_param) + "'>";
  html += R"rawliteral(
                </div>
              </div
            <div class = "form-section">
              <input type="submit" value="Save Configuration">
            </div>
          </div>  
        </form>

        <!-- JS for Tabs -->
        <script>
          function openTab(evt, tabName) {
            var i, tabcontent, tablinks;
            tabcontent = document.getElementsByClassName("tabcontent");
            for (i = 0; i < tabcontent.length; i++) {
              tabcontent[i].style.display = "none";
            }
            tablinks = document.getElementsByClassName("tablinks");
            for (i = 0; i < tablinks.length; i++) {
              tablinks[i].className = tablinks[i].className.replace(" active", "");
            }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
          }
          document.getElementById("defaultOpen").click();
        </script>
      </body>
    </html>
  )rawliteral";
  server->send(200, "text/html", html);
}

void handleSaveConfig() {
  // if (!isAuthenticated()) {
  //   server->sendHeader("Location", "/");
  //   server->send(302, "text/plain", "Redirecting to login...");
  //   return;
  // }

  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }

  DynamicJsonDocument doc(2048);
  int oldWebPort = config.webPort_param;
  bool webPortChanged = false;

  doc["siteID"] = server->arg("siteID");

  doc["username"] = server->arg("username");
  doc["password"] = server->arg("password");

  doc["ethIP"] = server->arg("ethIP");
  doc["vsignIP"] = server->arg("vsignIP");
  doc["gateway"] = server->arg("gateway");
  doc["subnet"] = server->arg("subnet");
  doc["primaryDNS"] = server->arg("primaryDNS");
  doc["secondaryDNS"] = server->arg("secondaryDNS");
  doc["webPort"] = server->arg("webPort").toInt();
  webPortChanged = (server->arg("webPort").toInt() != oldWebPort);

  doc["enable_monitor_and_re_enable_autoping"] = server->arg("enable_monitor_and_re_enable_autoping").toInt();
  doc["enable_re_enable_autoping_once_a_day"] = server->arg("enable_re_enable_autoping_once_a_day").toInt();
  doc["enable_monitor_ssh"] = server->arg("enable_monitor_ssh").toInt();
  doc["enable_monitor_modem"] = server->arg("enable_monitor_modem").toInt();
  doc["enable_monitor_USBHub"] = server->arg("enable_monitor_USBHub").toInt();
  doc["enable_monitor_ESwitch"] = server->arg("enable_monitor_ESwitch").toInt();
  doc["enable_monitor_and_re_enable_autoping"] = server->arg("enable_monitor_and_re_enable_autoping").toInt();
  doc["enable_re_enable_autoping_once_a_day"] = server->arg("enable_re_enable_autoping_once_a_day").toInt();
  doc["enable_monitor_ssh"] = server->arg("enable_monitor_ssh").toInt();
  doc["enable_monitor_modem"] = server->arg("enable_monitor_modem").toInt();
  doc["enable_monitor_USBHub"] = server->arg("enable_monitor_USBHub").toInt();
  doc["enable_monitor_ESwitch"] = server->arg("enable_monitor_ESwitch").toInt();

  doc["mosfet_startup_delay"] = ((unsigned long)server->arg("mosfet_startup_delay").toInt()) * 1000UL;
  doc["mosfet_1"] = server->arg("mosfet_1").toInt();
  doc["mosfet_2"] = server->arg("mosfet_2").toInt();
  doc["mosfet_3"] = server->arg("mosfet_3").toInt();
  doc["mosfet_4"] = server->arg("mosfet_4").toInt();

  doc["reboot_delay"] = ((unsigned long)server->arg("reboot_delay").toInt()) * 1000UL;
  doc["wps_startup_delay"] = ((unsigned long)server->arg("wps_startup_delay").toInt()) * 1000UL;
  doc["ssh_ping_interval"] = ((unsigned long)server->arg("ssh_ping_interval").toInt()) * 1000UL;
  doc["time_allowance"] = ((unsigned long)server->arg("time_allowance").toInt()) * 1000UL;
  // doc["reboot_wait"] = server->arg("reboot_wait").toInt();
  doc["max_reboot_wait"] = server->arg("max_reboot_wait").toInt();
  doc["wait_divisor"] = server->arg("wait_divisor").toInt();

  doc["ntpServer"] = server->arg("ntpServer");
  doc["gmtOffset_sec"] = server->arg("gmtOffset_sec").toInt();
  doc["daylightOffset_sec"] = server->arg("daylightOffset_sec").toInt();
  // doc["baudrate"] = server->arg("baudrate").toInt();

  doc["timezone"] = server->arg("timezone");

  doc["autoping_target1"] = server->arg("autoping_target1");
  doc["autoping_target2"] = server->arg("autoping_target2");
  doc["autoping_target3"] = server->arg("autoping_target3");
  doc["enable_autoping"] = server->arg("enable_autoping").toInt();
  doc["autoping_interval"] = ((unsigned long)server->arg("autoping_interval").toInt()) * 1000UL;
  doc["autoping_reboot_timeout"] = ((unsigned long)server->arg("autoping_reboot_timeout").toInt()) * 1000UL;
  doc["autoping_ping_responses"] = server->arg("autoping_ping_responses").toInt();
  doc["autoping_reboot_attempts"] = server->arg("autoping_reboot_attempts").toInt();
  doc["autoping_total_reboot_attempts"] = server->arg("autoping_total_reboot_attempts").toInt();
  doc["autoping_reboot_delay"] = ((unsigned long)server->arg("autoping_reboot_delay").toInt()) * 1000UL;

  if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
    server->send(500, "text/plain", "SPIFFS busy.");
    return;
  }

  File configFile = SPIFFS.open("/config.json", FILE_WRITE);
  if (!configFile) {
    xSemaphoreGive(spiffsMutex);
    server->send(500, "text/plain", "Failed to open config file for writing.");
    return;
  }

  serializeJsonPretty(doc, configFile);
  configFile.close();
  xSemaphoreGive(spiffsMutex);
  // String html = "<p>Configuration saved successfully.</p>";
  // // html += "<a href='/mosfet'>Back to mosfet</a>";
  // // html += "<a href='/task'>Back to task</a>";
  // html += "<a href='/config'>Back to config</a>";
  // // html += "<a href='/datetime'>Back to datetime</a>";
  // // html += "<a href='/autoping'>Back to autoping</a>";

  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <title>Configuration Saved</title>
      <style>
        .header {
          background-color: #A9a9a9;
          color: #fff;
          display: flex;
          align-items: center;
          height: 80px;
          justify-content: space-between;
          padding: 0 20px;
        }
        .header-left {
          background-color: #A9a9a9;
          display: flex;
          align-items: center;
          gap: 15px;
          width: 20%;
          height: 100%;
          justify-content: center;
        }
        .header-left img {
          width: 100%;
          height: 100%;
          object-fit: contain;
        }
        .header-info {
          flex: 1;
          padding: 0 20px;
          text-align: right;
          font-size: 18px;
          color: black;
        }
        .header-nav {
          display: flex;
          gap: 15px;
        }
        .header-nav a {
          color: black;
          text-decoration: none;
          font-weight: bold;
          padding: 6px 12px;
          border-radius: 5px;
        }
        .header-nav a:hover {
          background: #145a9c;
          color: #fff;
        }
        body {
          font-family: Arial, sans-serif;
          background-color: #e8e8e8;
          margin: 0;
          height: 100vh;
          display: flex;
          flex-direction: column;
        }
        .content {
          flex: 1;
          background: #A9a9a9;
          padding: 20px;
          overflow-y: auto;
          height: calc(100vh - 80px);
          box-sizing: border-box;
        }
        .message-box {
          background: #fff;
          border: 2px solid #ccc;
          border-radius: 10px;
          padding: 20px;
          max-width: 500px;
          margin: 40px auto;
          text-align: center;
        }
        .message-box a {
          display: inline-block;
          margin-top: 15px;
          padding: 8px 16px;
          background: #1e6cc1;
          color: #fff;
          text-decoration: none;
          border-radius: 5px;
          font-weight: bold;
        }
        .message-box a:hover {
          background: #145a9c;
        }
      </style>
    </head>
    <body>
      <div class="header">
        <div class="header-left">
          <img src = data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAVIAAADCCAYAAAAbxEoYAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA3ZpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuNi1jMTQ1IDc5LjE2MzQ5OSwgMjAxOC8wOC8xMy0xNjo0MDoyMiAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDplNmQwYjFjMS0xMTIzLWI0NDYtODdiMi00MGY2MTBlODljNDQiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6MjhEQjlDOEQxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MjhEQjlDOEMxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENDIDIwMTkgKFdpbmRvd3MpIj4gPHhtcE1NOkRlcml2ZWRGcm9tIHN0UmVmOmluc3RhbmNlSUQ9InhtcC5paWQ6ZTAyNGIzNmMtYWEwMi04ODQxLTk5OTEtOGEzNWIwNjNkODhjIiBzdFJlZjpkb2N1bWVudElEPSJ4bXAuZGlkOmU2ZDBiMWMxLTExMjMtYjQ0Ni04N2IyLTQwZjYxMGU4OWM0NCIv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 alt="VSign Logo">
        </div>
        <div class="header-nav">
          <a href="/config">Config</a>
          <a href="/log">Log</a>
          <a href="/systemlog">System Log</a>
          <a href="/upload">Upload</a>
        </div>
        <div class="header-info">
          <strong>XIAO_ESP32S3</strong><br>
          Site: <span>192 OFFICE</span>
        </div>
      </div>
      <div class="content">
        <div class="message-box">
          <h2>Configuration Saved Successfully</h2>
          <a href="/config">Back to Config</a>
        </div>
      </div>
    </body>
    </html>
    )rawliteral";

  server->send(200, "text/html", html);

  loadConfig();  // reload configuration
  String tzStr = applyTimezone(config.timezone_param);
  configTzTime(tzStr.c_str(), config.ntpServer_param);
  if (webPortChanged) {
    // New web port detected — restart to apply the change.
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP.restart();
  }
}

void handleTask() {
  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Tasks</title>";
  html += "<style>body{font-family:sans-serif; padding:20px;} label{display:block; margin-top:10px;} input{width:300px; padding:5px;} input[type=submit]{width:auto;}</style>";
  html += "</head><body>";
  html += "<h2>Tasks</h2><form method='POST' action='/saveConfig'>";
  html += "</form></body></html>";
  server->send(200, "text/html", html);
}

void handleDateTime() {
  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Date & Time</title>";
  html += "<style>body{font-family:sans-serif; padding:20px;} label{display:block; margin-top:10px;} input{width:300px; padding:5px;} input[type=submit]{width:auto;}</style>";
  html += "</head><body>";
  html += "<h2>Date & Time</h2><form method='POST' action='/saveConfig'>";
  html += "</form></body></html>";
  server->send(200, "text/html", html);
}

void handleAutoping() {
  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Autoping</title>";
  html += "<style>body{font-family:sans-serif; padding:20px;} label{display:block; margin-top:10px;} input{width:300px; padding:5px;} input[type=submit]{width:auto;}</style>";
  html += "</head><body>";
  html += "<h2>Autoping</h2><form method='POST' action='/saveConfig'>";
  html += "</form></body></html>";
  server->send(200, "text/html", html);
}


void handleSystemLog() {
  if (!server->authenticate(config.username_param, config.password_param)) {
    return server->requestAuthentication();
  }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>System Log</title>";
  html += html += R"rawliteral(
    <style>
      .header {
        background-color: #A9a9a9;
        color: #fff;
        display: flex;
        align-items: center;
        height: 80px; /* fixed header height */
        justify-content: space-between;
        padding: 0 20px;
      }
      .header-left {
        background-color: #A9a9a9;
        display: flex;
        align-items: center;
        gap: 15px;
        width: 20%; /* same as sidebar */
        height: 100%; /* same height as header */
        justify-content: center;
      }
      .header-left img {
        width: 100%;
        height: 100%;
        object-fit: contain; /* keeps aspect ratio */
      }
      .header-info {
        flex: 1;
        padding: 0 20px;
        text-align: right;
        font-size: 18px;
        color: black;
      }
      .header-nav {
        display: flex;
        gap: 15px;
      }
      .header-nav a {
        color: black; /* or #fff if you want white links */
        text-decoration: none;
        font-weight: bold;
        padding: 6px 12px;
        border-radius: 5px;
      }
      .header-nav a:hover {
        background: #145a9c;
        color: #fff;
      }
      .main {
        flex: 1;
        display: flex;
        height: calc(100vh - 80px);
      }
      .sidebar {
        width: 20%;
        background-color: #A9a9a9;
        color: #fff;
        display: flex;
        flex-direction: column;
        padding-top: 20px;
        height: 100%;
      }
      .sidebar button {
        margin: 5px 10px;
        padding: 12px;
        background: #1e6cc1; 
        border: none;
        color: #fff;
        font-size: 16px;
        cursor: pointer;
        border-radius: 5px;
        text-align: left;
        padding-left: 20px;
      }
      .sidebar button:hover {
        background: #145a9c;
      }
      .content {
        flex: 1;
        background: #A9a9a9;
        padding: 20px;
        overflow-y: auto;
        height: 100%;
        box-sizing: border-box;
      }
      body {
        font-family: Arial, sans-serif;
        background-color: #e8e8e8;
        margin: 0;
        height: 100vh;
        display: flex;
        flex-direction: column;
      }
      .log-area {
        width: 100%;
        height: 400px;
        border: 1px solid #ccc;
        padding: 5px;
        background: #000;
        color: #0f0;
        overflow: auto;
        white-space: pre-wrap;
        font-family: monospace;
      }
    </style>
  )rawliteral";

  html += "</head><body>";
  html += R"rawliteral(
    <div class="header">
      <div class="header-left">
        <img src = data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAVIAAADCCAYAAAAbxEoYAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA3ZpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuNi1jMTQ1IDc5LjE2MzQ5OSwgMjAxOC8wOC8xMy0xNjo0MDoyMiAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDplNmQwYjFjMS0xMTIzLWI0NDYtODdiMi00MGY2MTBlODljNDQiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6MjhEQjlDOEQxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MjhEQjlDOEMxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENDIDIwMTkgKFdpbmRvd3MpIj4gPHhtcE1NOkRlcml2ZWRGcm9tIHN0UmVmOmluc3RhbmNlSUQ9InhtcC5paWQ6ZTAyNGIzNmMtYWEwMi04ODQxLTk5OTEtOGEzNWIwNjNkODhjIiBzdFJlZjpkb2N1bWVudElEPSJ4bXAuZGlkOmU2ZDBiMWMxLTExMjMtYjQ0Ni04N2IyLTQwZjYxMGU4OWM0NCIv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 alt="VSign Logo">
      </div>
      <div class="header-nav">
        <a href="/config">Config</a>
        <a href="/log">Log</a>
        <a href="/systemlog">System Log</a>
        <a href="/upload">Upload</a>
      </div>
        <div class="header-info">
          <strong>XIAO_ESP32S3</strong><br>
          Site: <span>192 OFFICE</span>
        </div>
    </div>
  )rawliteral";
  html += "<div class='content'>";
  html += "<h2>System Log - Live Monitor</h2>";
  html += "<button onclick='location.reload()'>Refresh Now</button> ";
  html += "<button onclick='toggleAutoRefresh()' id='autoBtn'>Auto-Refresh: ON</button>";
  html += "<div id='logArea' class='log-area'>Loading...</div>";
  html += "</div>";


  html += R"rawliteral(
    <script>
      let autoRefresh = true; let refreshTimer;
      function updateLog() {
        fetch('/systemlog/data')
          .then(r => r.text())
          .then(d => {
            document.getElementById('logArea').textContent = d;
            const area = document.getElementById('logArea');
            area.scrollTop = area.scrollHeight;
          })
          .catch(e => {
            document.getElementById('logArea').textContent = 'Error loading log: ' + e;
          });
      }
      function toggleAutoRefresh() {
        autoRefresh = !autoRefresh;
        const btn = document.getElementById('autoBtn');
        if(autoRefresh){
          btn.textContent = 'Auto-Refresh: ON';
          refreshTimer = setInterval(updateLog, 2000);
        } else {
          btn.textContent = 'Auto-Refresh: OFF';
          clearInterval(refreshTimer);
        }
      }
      refreshTimer = setInterval(updateLog, 2000);
      updateLog();
    </script>
  )rawliteral";

  html += "</body></html>";

  server->send(200, "text/html", html);
}

void handleSystemLogData() {
  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }
  String response = "";

  if (systemLogMutex != NULL) {
    if (xSemaphoreTake(systemLogMutex, pdMS_TO_TICKS(100))) {
      if (systemLogBuffer.length() > 0) {
        response = systemLogBuffer;
      } else {
        response = "Log buffer is empty (but mutex is working)\n";
      }
      xSemaphoreGive(systemLogMutex);
    } else {
      response = "Could not acquire mutex lock\n";
    }
  } else {
    response = "System log mutex is NULL\n";
  }

  // Add debug information
  response += "\n=== DEBUG INFO ===\n";
  response += "Current time: " + getTime() + "\n";
  response += "Millis: " + String(millis()) + "\n";
  response += "Free heap: " + String(ESP.getFreeHeap()) + "\n";
  response += "Buffer size: " + String(systemLogBuffer.length()) + "\n";
  response += "Mutex status: " + String(systemLogMutex != NULL ? "Created" : "NULL") + "\n";

  server->send(200, "text/plain", response);
}

void handleReboot() {
  // if (!isAuthenticated()) {
  //   server->sendHeader("Location", "/");
  //   server->send(302, "text/plain", "Redirecting to login...");
  //   return;
  // }

  // if (!server->authenticate(config.username_param, config.password_param)) {
  //   return server->requestAuthentication();
  // }

  server->send(200, "text/html", "<p>Rebooting ESP32S3 ...</p>");
  vTaskDelay(pdMS_TO_TICKS(1000));
  ESP.restart();
}


void handleUpload() {
  if (!server->authenticate(config.username_param, config.password_param)) {
    return server->requestAuthentication();
  }

  String html = R"rawliteral(
    <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Upload Configuration</title>
        <style>
          .header {
            background-color: #A9a9a9;
            color: #fff;
            display: flex;
            align-items: center;
            height: 80px; /* fixed header height */
            justify-content: space-between;
            padding: 0 20px;
          }
          .header-left {
            background-color: #A9a9a9;
            display: flex;
            align-items: center;
            gap: 15px;
            width: 20%; /* same as sidebar */
            height: 100%; /* same height as header */
            justify-content: center;
          }

          .header-left img {
            width: 100%;
            height: 100%;
            object-fit: contain; /* keeps aspect ratio */
          }
          .header-info {
            flex: 1;
            padding: 0 20px;
            text-align: right;
            font-size: 18px;
            color: black;
          }

          .header-nav {
            display: flex;
            gap: 15px;
          }

          .header-nav a {
            color: black; /* or #fff if you want white links */
            text-decoration: none;
            font-weight: bold;
            padding: 6px 12px;
            border-radius: 5px;
          }

          .header-nav a:hover {
            background: #145a9c;
            color: #fff;
          }
          .main {
            flex: 1;
            display: flex;
            height: calc(100vh - 80px);
          }
          .sidebar {
            width: 20%;
            background-color: #A9a9a9;
            color: #fff;
            display: flex;
            flex-direction: column;
            /* align-items: stretch; */
            padding-top: 20px;
            height: 100%;
          }
          .sidebar button {
            margin: 5px 10px;
            padding: 12px;
            background: #1e6cc1; 
            border: none;
            color: #fff;
            font-size: 16px;
            cursor: pointer;
            border-radius: 5px;
            text-align: left;
            padding-left: 20px;
          }
          .sidebar button:hover {
            background: #145a9c;
          }
          .content {
            flex: 1;
            background: #A9a9a9;
            padding: 20px;
            overflow-y: auto;
            height: 100%;
            box-sizing: border-box;
          }
          body {
            font-family: Arial, sans-serif;
            background-color: #A9a9a9;
            margin: 0;
            height: 100vh;
            display: flex;
            flex-direction: column;
          }
          .tab { overflow: hidden; border-bottom: 1px solid #ccc; background: #fff; }
          .tabcontent {
            display: none;
            padding: 20px;
            border: 2px solid #ccc;
            /* border-top: solid; */
            background: #fff;
          }
          .form-section label { display: block; margin: 6px 0 2px; }
          .form-section input { width: 100%; padding: 6px; margin-bottom: 10px;}
          .panel { background:#fff; padding:20px; border-radius:8px; box-shadow:0 0 8px rgba(0,0,0,0.08); max-width:800px; margin:0 auto;}
          .btn-primary { background:#1e6cc1; color:#fff; padding:10px 18px; border:none; border-radius:5px; cursor:pointer; }
          .btn-danger { background:red; color:#fff; padding:10px 18px; border:none; border-radius:5px; cursor:pointer; }
        </style>
      </head>
      <body>
        <div class="header">
          <div class = "header-left">
            <img src = data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAVIAAADCCAYAAAAbxEoYAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA3ZpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuNi1jMTQ1IDc5LjE2MzQ5OSwgMjAxOC8wOC8xMy0xNjo0MDoyMiAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDplNmQwYjFjMS0xMTIzLWI0NDYtODdiMi00MGY2MTBlODljNDQiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6MjhEQjlDOEQxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MjhEQjlDOEMxQ0FEMTFFQUFENEFEMzI5NkZBNzlDODMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENDIDIwMTkgKFdpbmRvd3MpIj4gPHhtcE1NOkRlcml2ZWRGcm9tIHN0UmVmOmluc3RhbmNlSUQ9InhtcC5paWQ6ZTAyNGIzNmMtYWEwMi04ODQxLTk5OTEtOGEzNWIwNjNkODhjIiBzdFJlZjpkb2N1bWVudElEPSJ4bXAuZGlkOmU2ZDBiMWMxLTExMjMtYjQ0Ni04N2IyLTQwZjYxMGU4OWM0NCIv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 alt="VSign Logo">
        </div>
        <div class="header-nav">
          <a href="/config">Config</a>
          <a href="/log">Log</a>
          <a href="/systemlog">System Log</a>
          <a href="/upload">Upload</a>
        </div>
        <div class="header-info">
          <strong>XIAO_ESP32S3</strong><br>
          Site: <span>192 OFFICE</span>
        </div>
      </div>
      <div class="main">
        <div class="content">
          <div class="panel">
            <h2>Upload Configuration File</h2>
            <p>Select a JSON file (.json). The configuration will be written to <code>/config.json</code> and applied immediately. If the uploaded file changes the Web Port, you will need to reboot the device for the port change to take effect.</p>
            <form method='POST' action='/uploadConfig' enctype='multipart/form-data'>
              <input type='file' id="configFile" name='configFile' accept='.json'><br><br>
              <input type='submit' id="uploadBtn" class='btn-primary' value='Upload' disabled>
            </form>
            <hr>
            <form method='POST' action='/reboot' onsubmit='return confirm(\"Are you sure you want to reboot?\");'>
              <input type='submit' class='btn-danger' value='Reboot ESP32S3' style='margin-top:10px;'>
            </form>
          </div>
        </div>
      </div>
      <script>
        const fileInput = document.getElementById("configFile");
        const uploadBtn = document.getElementById("uploadBtn");
        uploadBtn.disabled = true; // force disabled on load

        fileInput.addEventListener("change", () => {
          if (fileInput.files.length > 0) {
            uploadBtn.disabled = false;
          } else {
            uploadBtn.disabled = true;
          }
        });
      </script>
    </body>
    </html>
    )rawliteral";
  server->send(200, "text/html", html);
}


void handleUploadConfig() {
  HTTPUpload &upload = server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload start: %s\n", upload.filename.c_str());
    systemLog("Upload start: " + String(upload.filename.c_str()));

    if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
      Serial.println("SPIFFS busy during upload start.");
      systemLog("SPIFFS busy during upload start.");
      return;
    }

    SPIFFS.remove("/config.json");  // delete old config
    uploadFile = SPIFFS.open("/config.json", FILE_WRITE);
    if (!uploadFile) {
      Serial.println("Failed to open file for writing!");
      systemLog("Failed to open file for writing!");
      xSemaphoreGive(spiffsMutex);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    } else {
      Serial.println("Upload file write error: file not open.");
      systemLog("Upload file write error: file not open.");
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      loadConfig();
      Serial.printf("Upload end: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
      systemLog("Upload end: " + String(upload.filename.c_str()) + " (" + String(upload.totalSize) + " bytes)");
    } else {
      Serial.println("Upload failed: file wasn't open at end.");
      systemLog("Upload failed: file wasn't open at end.");
    }
    xSemaphoreGive(spiffsMutex);
  }
}

void startWebServer() {
  server->on("/", handleRoot);
  //server->on("/login", HTTP_POST, handleLogin);
  server->on("/log", handleLog);
  // server->on("/mosfet", handleMosfet);
  server->on("/config", handleConfig);
  server->on("/saveConfig", HTTP_POST, handleSaveConfig);
  // server->on("/task", handleTask);
  // server->on("/datetime", handleDateTime);
  // server->on("/autoping", handleAutoping);

  server->on("/systemlog", handleSystemLog);           // Add this line
  server->on("/systemlog/data", handleSystemLogData);  // Add this line
  server->on("/reboot", HTTP_POST, handleReboot);
  // server->on("/upload", HTTP_GET, handleUpload);
  // server->on("/uploadConfig", HTTP_POST, handleUploadConfig);
  server->on("/upload", HTTP_GET, handleUpload);
  server->on(
    "/uploadConfig", HTTP_POST,
    []() {
      // final response AFTER upload is done
      server->send(200, "text/html",
                   "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Upload</title></head><body>"
                   "<h2>Upload complete</h2>"
                   "<p>Configuration updated. Please reboot the device.</p>"
                   "<a href=\"/config\">Back to Config</a>"
                   "</body></html>");
    },
    handleUploadConfig  // <-- this handles the chunked upload
  );
  server->begin();
  Serial.println("Web server active.");
  systemLog("Web server active.");
  systemLog("Web server started on port " + String(config.webPort_param));
}

// *********************************
// WPS FUNCTIONS
// *********************************

bool pingAnyTarget() {
  if (xSemaphoreTake(netMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
    Serial.println("Ping mutex timeout");
    systemLog("Ping mutex timeout");
    return false;
  }

  IPAddress targets[] = {
    config.autoping_target1_param,
    config.autoping_target2_param,
    config.autoping_target3_param
  };

  const int numTargets = sizeof(targets) / sizeof(targets[0]);
  bool results[numTargets];  // store success/fail for each target
  bool anySuccess = false;

  // print pinging
  for (int i = 0; i < numTargets; i++) {
    Serial.printf("pinging %s ...\n", targets[i].toString().c_str());
  }

  // ping all targets
  for (int i = 0; i < numTargets; i++) {
    results[i] = Ping.ping(targets[i], 1);
    if (results[i]) {
      anySuccess = true;
    }
  }

  // print status for all target pings
  for (int i = 0; i < numTargets; i++) {
    if (results[i]) {
      Serial.printf("ping %s succeeded\n", targets[i].toString().c_str());
    } else {
      Serial.printf("ping %s failed\n", targets[i].toString().c_str());
    }
  }

  xSemaphoreGive(netMutex);  // Always release before returning
  return anySuccess;
}

void enterRecoveryMode() {
  inRecovery = true;
  recoveryStart = millis();
  Serial.println("Entering recovery mode ...");
  systemLog("Entering recovery mode ...");
}

void exitRecoveryMode() {
  inRecovery = false;
  Serial.println("Recovered. Returning to normal operation ...");
  systemLog("Recovered. Returning to normal operation ...");
}

void handleRecovery() {
  unsigned long now = millis();

  // keep trying during recovery
  if (now - lastPingTime >= config.autoping_interval_param) {
    lastPingTime = now;
    if (pingAnyTarget()) {
      exitRecoveryMode();
      return;
    }
  }

  // time window expired -> reboot modem -> reboot Web DIN Relay Lite
  if (now - recoveryStart >= config.autoping_reboot_timeout_param) {
    Serial.println("Recovery failed!");
    systemLog("Recovery failed!");
    reboot_modem();
    vTaskDelay(pdMS_TO_TICKS(config.autoping_reboot_delay_param));
    // inRecovery = false;
    // ESP.restart(); // reboot Web DIN Relay Lite
  }
}

void autoping() {
  unsigned long now = millis();

  if (!inRecovery) {
    // normal mode, ping every interval
    if (now - lastPingTime >= config.autoping_interval_param) {
      lastPingTime = now;
      if (!pingAnyTarget()) {
        enterRecoveryMode();
      }
    }
  } else {
    handleRecovery();  // in recovery mode
  }
}

// function to log events into a txt file
void log_it(const String &aLogEntry) {
  if (getDate() == "TimeUnavailable") {
    Serial.println("Failed to get date!");
    systemLog("Failed to get date!");
    return;
  }

  if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    String filename = "/" + config.siteID_param + "-" + getDate() + ".txt";
    File file = SPIFFS.open(filename, FILE_APPEND);
    if (!file) {
      Serial.println("Failed to open log file: " + filename);
      xSemaphoreGive(spiffsMutex);
      systemLog("Failed to open log file: " + filename);
      return;
    }

    String entry = getDate() + " " + getTime() + " 9 " + aLogEntry;
    file.println(entry);
    file.close();
    xSemaphoreGive(spiffsMutex);
    Serial.println(entry + " -> " + filename);
    systemLog(entry + " -> " + filename);
  } else {
    Serial.println("SPIFFS mutex unavailable for log.");
    systemLog("SPIFFS mutex unavailable for log.");
  }
}

// NEEDS TESTING, log_it function with minimal String usage to avoid heap fragmentation
void log_it_safe(const char *aLogEntry) {
  // heap guard
  if (ESP.getFreeHeap() < LOG_MIN_FREE_HEAP) {
    Serial.println("log_it: low heap, skipping write");
    return;
  }

  char dateBuf[16];
  char timeBuf[16];
  bool haveTime = false;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &timeinfo);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
    haveTime = true;
  } else {
    // fallback to seconds since boot
    snprintf(dateBuf, sizeof(dateBuf), "%lus", millis() / 1000);
    strncpy(timeBuf, "00:00:00", sizeof(timeBuf));
  }

  // build filename safely
  const char *site = config.siteID_param.c_str();
  char filename[LOG_FILENAME_MAX];
  snprintf(filename, sizeof(filename), "/%.20s-%s.txt", site, dateBuf);

  // build log entry
  char entry[LOG_ENTRY_MAX];
  snprintf(entry, sizeof(entry), "%s %s 9 %s", dateBuf, timeBuf, aLogEntry);

  if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
    Serial.println("log_it_safe: spiffsMutex unavailable");
    return;
  }

  File file = SPIFFS.open(filename, FILE_APPEND);
  if (!file) {
    Serial.printf("log_it_safe: Failed to open %s\n", filename);
    xSemaphoreGive(spiffsMutex);
    return;
  }

  file.println(entry);
  file.close();
  xSemaphoreGive(spiffsMutex);

  Serial.print(entry);
  Serial.print(" -> ");
  Serial.println(filename);
}

// TODO, function to send log file to VSign Hub
void log_copy() {
  Serial.println();
}

// WIP, function track number of WPS reboots
void log_boot_count() {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  String blog = "Boot " + String(boot);
  Serial.println(blog);
  systemLog(blog);
  log_it(blog);
}

// WIP, function to keep modem outlet on
void monitor_modem() {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  if (config.enable_monitor_modem == 1 && config.mosfet_1 == 0) {
    // vTaskDelay(pdMS_TO_TICKS(1*10*1000)); // 10 seconds
    // pinMode(MOSFET_1, OUTPUT);
    // digitalWrite(MOSFET_1, HIGH);  // ON
    config.mosfet_1 = 1;
    saveConfig();
    Serial.println("Monitor - Modem was turned off");
    systemLog("Monitor - Modem was turned off");
    vTaskDelay(pdMS_TO_TICKS(config.time_allowance_param));
    log_it("MOD 9");
  }
}

// WIP, function to keep Ethernet switch outlet on
void monitor_ESwitch() {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  if (config.mosfet_3 == 1) {
    // pinMode(MOSFET_3, OUTPUT);
    digitalWrite(MOSFET_3, LOW);  // ON
    // log_it("ESW 9");
  } else {
    digitalWrite(MOSFET_3, HIGH);
  }
}

// WIP, function to keep USB Hub outlet on
void monitor_USBHub() {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  if (config.mosfet_4 == 1) {
    // pinMode(MOSFET_4, OUTPUT);
    digitalWrite(MOSFET_4, LOW);  // ON
    // log_it("USB 9");
  } else {
    digitalWrite(MOSFET_4, HIGH);
  }
}

// WIP, function to check autoping every 2 hours and if autoping is disabled, then enable it
void monitor_and_re_enable_autoping() {
  vTaskDelay(pdMS_TO_TICKS(config.wps_startup_delay_param));
  if (config.enable_autoping != 1) {
    config.enable_autoping = 1;
    saveConfig();
    Serial.println("Monitor - Autoping was turned off");
    systemLog("Monitor - Autoping was turned off");
  }
}

// WIP, function to enable autoping everyday at 1:05
void re_enable_autoping_once_a_day() {
  // if time = 1:00 AM
  // config.enable_autoping = 1
}

// WIP NEW, function to power cycle VSign Hub everyday at 2:00 AM
void power_cycle_vsign_once_a_day() {
  // if time = 2:00 AM
  // reboot_linux_box()
}

// WIP, function to reboot modem
void reboot_modem() {
  Serial.println("Rebooting modem ...");
  systemLog("Rebooting modem ...");
  log_it("MOD 0");
  digitalWrite(MOSFET_1, HIGH);                          // OFF
  vTaskDelay(pdMS_TO_TICKS(config.reboot_delay_param));  // 10 seconds
  log_it("MOD 9");
  digitalWrite(MOSFET_1, LOW);  // ON
}

// THIS FUNCTION IS NOT BEING USED
void monitor_ssh() {
  // vTaskDelay(pdMS_TO_TICKS(wps_startup_delay));

  while (true) {
    ssh_ping();  // ssh VSign Hub and ping gateway
    // vTaskDelay(pdMS_TO_TICKS(5*60*1000)); // 5 minutes
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second

    Serial.print("Reboot count: ");   // debug
    Serial.println(reboot_count);     // debug
    systemLog("Reboot count: ");      // debug
    systemLog(String(reboot_count));  // debug

    if (reboot_count >= 2) {
      Serial.print("Reboot wait: ");                      // debug
      Serial.print(reboot_wait / (60 * 60 * 1000));       //debug
      Serial.println(" hour(s)");                         // debug
      systemLog("Reboot wait: ");                         // debug
      systemLog(String(reboot_wait / (60 * 60 * 1000)));  //debug
      systemLog(" hour(s)");                              // debug
      reboot_count = 0;
      vTaskDelay(pdMS_TO_TICKS(reboot_wait - 300000));  // wait - 5 minutes
      reboot_wait = reboot_wait * 2;

      if (reboot_wait > (24 * 60 * 60 * 1000)) {
        reboot_wait = 24 * 60 * 60 * 1000;  // 24 hours
      }
    }
  }
}

// WIP, function to reboot VSign Hub
void reboot_linux_box() {
  Serial.printf("Before reboot_linux_box: FreeHeap=%u\n", ESP.getFreeHeap());  // NEEDS TESTING

  Serial.println("Rebooting VSign Hub ...");
  systemLog("Rebooting VSign Hub ...");
  // Serial.println("Hub 0");
  Serial.printf("Before log_it: FreeHeap=%u\n", ESP.getFreeHeap());  // NEEDS TESTING
  log_it("Hub 0");
  Serial.printf("After log_it: FreeHeap=%u\n", ESP.getFreeHeap());  // NEEDS TESTING
  digitalWrite(MOSFET_2, HIGH);                                     // OFF
  vTaskDelay(pdMS_TO_TICKS(config.reboot_delay_param));             // 10 seconds
  // Serial.println("Hub 9");
  log_it("Hub 9");
  digitalWrite(MOSFET_2, LOW);  // ON

  reboot_count = reboot_count + 1;

  String clog = "Reboot count: " + String(reboot_count);
  log_it(clog);
  Serial.println(clog);  // debug
  systemLog(clog);

  Serial.printf("Exiting reboot_linux_box: FreeHeap=%u\n", ESP.getFreeHeap());  // NEEDS TESTING
  // send_passivectl();
}

// WIP, function to see if we can ssh into VSign Hub
// Similarly update other functions like ssh_ping():
void ssh_ping() {
  if (!ping_gateway()) return;

  Serial.printf("ssh vsign@%s ...\n", config.vsignIP_param.toString().c_str());
  String sshMsg = "ssh vsign@" + config.vsignIP_param.toString() + " ...";
  systemLog(sshMsg);

  WiFiClient client;
  static unsigned long failed_count = 0;

  bool ssh = client.connect(config.vsignIP_param, 22);

  String resultMsg = "ssh " + config.vsignIP_param.toString();
  if (ssh) {
    Serial.println("ssh " + config.vsignIP_param.toString() + " successful");
    resultMsg += " successful";
    systemLog(resultMsg);
    failed_count = 0;
    client.stop();
  } else {
    resultMsg += " failed!";
    Serial.println(resultMsg);
    systemLog("ssh " + config.vsignIP_param.toString() + " failed!");
    failed_count++;

    String flog = "Hub " + String(failed_count);
    systemLog(flog);
    log_it(flog);

    if (failed_count >= 3) {
      failed_count = 0;
      reboot_linux_box();
    }
  }
}

bool ping_gateway() {
  if (netMutex == NULL) return false;

  const unsigned long timeout = 20 * 1000;
  unsigned long startAttempt = millis();
  bool ping_success = false;

  Serial.println("Pinging " + config.gateway_param.toString() + " ...");
  String pingMsg = "Pinging " + config.gateway_param.toString() + " ...";
  systemLog(pingMsg);

  if (xSemaphoreTake(netMutex, pdMS_TO_TICKS(500))) {
    while (millis() - startAttempt < timeout) {
      if (Ping.ping(config.gateway_param)) {
        ping_success = true;
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    xSemaphoreGive(netMutex);
  } else {
    systemLog("Failed to take netMutex for ping!");
    return false;
  }

  String resultMsg = "Ping " + config.gateway_param.toString();
  if (ping_success) {
    Serial.println("Ping " + config.gateway_param.toString() + " successful");
    resultMsg += " successful";
    systemLog(resultMsg);
    return true;
  } else {
    Serial.println("Ping " + config.gateway_param.toString() + " failed!");
    resultMsg += " failed!";
    systemLog(resultMsg);
    return false;
  }
}

String logs;

void logMessage(const String &msg) {
  Serial.println(msg);  // print to Serial Monitor
  systemLog(msg);
  logs += msg + "<br>";        // append log with line break
  if (logs.length() > 5000) {  // safety: prevent buffer overflow
    logs.remove(0, logs.indexOf("<br>") + 4);
  }
}

// NEEDS REWRITE
void systemLog(const String &message) {
  // Add to system log buffer
  if (systemLogMutex != NULL) {
    if (xSemaphoreTake(systemLogMutex, pdMS_TO_TICKS(50))) {
      String timestamp = getTime();
      if (timestamp == "TimeUnavailable") {
        timestamp = String(millis() / 1000) + "s";  // Use seconds since boot as fallback
      }

      String logEntry = timestamp + " " + message + "\n";
      systemLogBuffer += logEntry;

      // Trim buffer if too long
      while (systemLogBuffer.length() > MAX_LOG_BUFFER_SIZE) {
        int firstNewline = systemLogBuffer.indexOf('\n');
        if (firstNewline >= 0) {
          systemLogBuffer = systemLogBuffer.substring(firstNewline + 1);
        } else {
          systemLogBuffer = systemLogBuffer.substring(systemLogBuffer.length() - (MAX_LOG_BUFFER_SIZE / 2));  // Emergency fallback
        }
      }

      xSemaphoreGive(systemLogMutex);
    } else {
      Serial.println("Warning: could not acquire systemLogMutex");
    }
  } else {
    Serial.println("Warning: could not acquire systemLogMutex");
  }
}