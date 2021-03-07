#include "boot.h"
#include "reset.h"

// Local logging tag
static const char TAG[] = __FILE__;

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

  RTC_runmode = RUNMODE_NORMAL; // if watchdog fires, boot to production

  hw_timer_t *timer = NULL;
  timer = timerBegin(2, 80, true);                 // timer 2, div 80, countup
  timerAttachInterrupt(timer, &esp_restart, true); // apply watchdog reset
  timerAlarmWrite(timer, BOOTDELAY * 1000000, false); // watchdog time in us
  timerAlarmEnable(timer);                            // enable watchdog

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
      "<td><input type='submit' onclick='start(this.form)' "
      "value='Start'></td>"
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

  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE,
              INADDR_NONE); // call is only a workaround for bug in WiFi class
  // see https://github.com/espressif/arduino-esp32/issues/806
  WiFi.setHostname(host);
  WiFi.mode(WIFI_STA);

  // Connect to WiFi network
  // workaround applied here to bypass WIFI_AUTH failure
  // see https://github.com/espressif/arduino-esp32/issues/2501

  // 1st try
  WiFi.begin(ssid, password);
  while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
  }
  // 2nd try
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  MDNS.begin(host);

  // send start button page
  server.on("/", HTTP_GET, [&server, &loginMenu]() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginMenu);
  });

  // send upload button page
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
        delay(5000);
        WiFi.disconnect(true);
        if (!Update.hasError())
          RTC_runmode = RUNMODE_POWERCYCLE;
        esp_restart();
      },

      [&server, &timer]() {
        bool success = false;
        HTTPUpload &upload = server.upload();

        switch (upload.status) {

        case UPLOAD_FILE_START:
          // start file transfer
          ESP_LOGI(TAG, "Uploading %s", upload.filename.c_str());
          success = Update.begin();
          break;

        case UPLOAD_FILE_WRITE:
          // flashing firmware to ESP
          success = (Update.write(upload.buf, upload.currentSize) ==
                     upload.currentSize);
          break;

        case UPLOAD_FILE_END:
          success = Update.end(true); // true to set the size to the current
          if (success)
            ESP_LOGI(TAG, "Upload finished, %u bytes written",
                     upload.totalSize);
          else
            ESP_LOGE(TAG, "Upload failed, status=%d", upload.status);
          break;

        case UPLOAD_FILE_ABORTED:
        default:
          break;

        } // switch

        if (!success) {
          ESP_LOGE(TAG, "Error: %s", Update.errorString());
          WiFi.disconnect(true);
          esp_restart();
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