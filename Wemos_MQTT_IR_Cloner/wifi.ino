boolean load_wifi() {
  File config_file = SPIFFS.open("/wifi.json", "r");
  if (!config_file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  String  config_string = config_file.readString();
  deserializeJson(WIFI, config_string);
  config_file.close();

  ssid = (const char*)WIFI["SSID"];
  password = (const char*)WIFI["Pass"];
  mqtt_server = (const char*)WIFI["MQTT_Server"];
  topic_READ = (const char*)WIFI["Topic_READ"];
  topic_CMD = (const char*)WIFI["Topic_CMD"];
  HostName = (const char*)WIFI["HostName"];
  Serial.println("WiFi JSON loaded");
  return true;
}

void save_wifi() {
  String config_string;
  //Open the config.json file (Write Mode)
  File config_file = SPIFFS.open("/wifi.json", "w");
  if (!config_file) {
    Serial.println("Failed to open file (Writing mode)");
    return;
  }
  serializeJson(WIFI, config_string);
  //Save and close the JSON file
  if (config_file.println(config_string)) {
    Serial.println("New Config");
    Serial.println(config_string);
  } else {
    Serial.println("File write failed");
  }
  config_file.close();
}

boolean setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname(HostName);
  WiFi.begin(ssid, password);
  int count = 0;
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(53, "*", apIP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

void startWebServer() {
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.softAPIP());
  webServer.onNotFound([]() {
    String s = "<h1>WiFI§MQTT Settings</h1>";
    s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
    s += ssidList;
    s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\">";
    s += "</select><br>MQTT Server: <input name=\"mqttserv\" length=64 type=\"text\">";
    s += "</select><br>Topic Read: <input name=\"tread\" length=64 type=\"text\">";
    s += "</select><br>Topic CMD: <input name=\"tcmd\" length=64 type=\"text\">";
    s += "</select><input type=\"submit\"></form>";
    webServer.send(200, "text/html", makePage("WiFi/MQTT Settings", s));
  });

  webServer.on("/setap", []() {
    WIFI["SSID"] = urlDecode(webServer.arg("ssid"));
    WIFI["Pass"] = urlDecode(webServer.arg("pass"));
    if (urlDecode(webServer.arg("tread")) != "") {
      WIFI["Topic_READ"] = urlDecode(webServer.arg("tread"));
    }
    if (urlDecode(webServer.arg("tcmd")) != "") {
      WIFI["Topic_CMD"] = urlDecode(webServer.arg("tcmd"));
    }
    if (urlDecode(webServer.arg("mqttserv")) != "") {
      WIFI["MQTT_Server"] = urlDecode(webServer.arg("mqttserv"));
    }
    save_wifi();
    ESP.restart();
  });

  webServer.begin();
}

bool reconnect() {
  int count = 0;
  while ( count < 5 ) {
    if (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(HostName)) {
        Serial.println("connected");
        client.subscribe(topic_CMD);
        digitalWrite(LED_BUILTIN, HIGH);
        return(true);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
        digitalWrite(LED_BUILTIN, LOW);
        count++;
      }
    }
  }
  settingMode = true;
  Serial.println("Cannot connect to MQTT");
  setuponce = true;;
  return(false);
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
