#include "boot.h"
#include "reset.h"

// Local logging tag
static const char TAG[] = __FILE__;

void IRAM_ATTR exit_boot_menu() {
  RTC_runmode = RUNMODE_NORMAL;
  esp_restart();
}

// start local web server with user interface for maintenance mode
// used for manually uploading a firmware file via wifi

void start_boot_menu(void) {

  uint8_t mac[6];
  char clientId[20];

  // hash 6 byte MAC to 4 byte hash
  esp_eth_get_mac(mac);
  const uint32_t hashedmac = myhash((const char *)mac, 6);
  snprintf(clientId, 20, "paxcounter_%08x", hashedmac);

  const char *host = clientId;
  const char *ssid = WIFI_SSID;
  const char *password = WIFI_PASS;

  hw_timer_t *timer = NULL;
  timer = timerBegin(2, 80, true); // timer 2, div 80, countup
  timerAttachInterrupt(timer, &exit_boot_menu, true); // attach callback
  timerAlarmWrite(timer, BOOTDELAY * 1000000, false); // set time in us
  timerAlarmEnable(timer);                            // enable interrupt

  WebServer server(80);

  const char *loginMenu =
      "<form name='loginForm'>"
      "<table width='20%' bgcolor='A09F9F' align='center'>"
      "<tr>"
      "<td colspan=2>"
      "<center><font size=4><b>Maintenance Menu</b></font></center>"
      "<br>"
      "</td>"
      "<br>"
      "<br>"
      "</tr>"
      "<tr>"
      "<td><input type='submit' onclick='start(this.form)' value='Start'></td>"
      "</tr>"
      "</table>"
      "</form>"
      "<script>"
      "function start(form) {window.open('/serverIndex')}"
      "</script>";

  const char *serverIndex =
      "<script "
      "src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/"
      "jquery.min.js'></script>"
      "<form method='POST' action='#' enctype='multipart/form-data' "
      "id='upload_form'>"
      "<input type='file' name='update'>"
      "<input type='submit' value='Update'>"
      "</form>"
      "<div id='prg'>progress: 0%</div>"
      "<script>"
      "$('form').submit(function(e){"
      "e.preventDefault();"
      "var form = $('#upload_form')[0];"
      "var data = new FormData(form);"
      " $.ajax({"
      "url: '/update',"
      "type: 'POST',"
      "data: data,"
      "contentType: false,"
      "processData:false,"
      "xhr: function() {"
      "var xhr = new window.XMLHttpRequest();"
      "xhr.upload.addEventListener('progress', function(evt) {"
      "if (evt.lengthComputable) {"
      "var per = evt.loaded / evt.total;"
      "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
      "}"
      "}, false);"
      "return xhr;"
      "},"
      "success:function(d, s) {"
      "console.log('success!')"
      "},"
      "error: function (a, b, c) {"
      "}"
      "});"
      "});"
      "</script>";

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  MDNS.begin(host);

  server.on("/", HTTP_GET, [&server, &loginMenu]() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginMenu);
  });

  server.on("/serverIndex", HTTP_GET, [&server, &serverIndex, &timer]() {
    timerAlarmWrite(timer, BOOTTIMEOUT * 1000000, false);
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  // handling uploading firmware file
  server.on(
      "/update", HTTP_POST,
      [&server]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      },
      [&server, &timer]() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          ESP_LOGI(TAG, "Update: %s\n", upload.filename.c_str());
#if (HAS_LED != NOT_A_PIN)
#ifndef LED_ACTIVE_LOW
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH, HAS_LED, HIGH)) {
#else
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH, HAS_LED, LOW)) {
#endif
#else
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#endif

            ESP_LOGE(TAG, "Error: %s", Update.errorString());
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          // flashing firmware to ESP
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            ESP_LOGE(TAG, "Error: %s", Update.errorString());
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(
                  true)) { // true to set the size to the current progress
            ESP_LOGI(TAG, "Update finished, %u bytes written",
                     upload.totalSize);
            WiFi.disconnect(true, true);
            do_reset(false); // coldstart
          } else {
            ESP_LOGE(TAG, "Update failed");
          }
        }
      });

  server.begin();
  MDNS.addService("http", "tcp", 80);
  ESP_LOGI(TAG,
           "WiFi connected to '%s', open http://%s.local or http://%s in your "
           "browser",
           WiFi.SSID().c_str(), clientId, WiFi.localIP().toString().c_str());

  while (1) {
    server.handleClient();
    delay(1);
  }
}