#include "WiFiConfigManager.h"

// 初始化静态成员变量
WiFiConfigManager* WiFiConfigManager::_instance = nullptr;

// 构造函数：初始化WiFiConfigManager对象，设置AP模式参数和Web服务器
WiFiConfigManager::WiFiConfigManager(const char* apSSID,
                                     const char* apPassword,
                                     const char* apDomain,
                                     int eepromSize)
  : _apSSID(apSSID),              // AP模式的SSID
    _apPassword(apPassword),      // AP模式的密码
    _apDomain(apDomain),          // AP模式的域名
    _eepromSize(eepromSize),      // EEPROM分配大小
    _shouldConnect(false),        // 是否尝试连接WiFi的标志
    _connectionTimeout(20),       // WiFi连接超时时间（秒）
    _connectedCallback(nullptr),  // WiFi连接成功的回调函数
    _apModeCallback(nullptr),     // 进入AP模式的回调函数
    _commitNeeded(false),         // 是否需要提交EEPROM更改的标志
    _lastCommitTime(0) {          // 上次提交EEPROM的时间
  _server = new WebServer(80);    // 创建Web服务器实例
  _dnsServer = new DNSServer();   // 创建DNS服务器实例
  _instance = this;               // 设置静态实例指针

  // 配置页面的HTML模板，包含CSS和JavaScript
  _htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WiFi Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f2f2f2;
        }
        .container {
            background-color: white;
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
            max-width: 500px;
            margin: 0 auto;
        }
        h1 {
            color: #0066cc;
            text-align: center;
        }
        h2 {
            color: #333;
            margin-top: 20px;
        }
        label {
            display: block;
            margin-top: 10px;
            font-weight: bold;
        }
        input[type=text], input[type=password], input[type=number] {
            width: 100%;
            padding: 10px;
            margin: 8px 0;
            display: inline-block;
            border: 1px solid #ccc;
            border-radius: 4px;
            box-sizing: border-box;
        }
        input[type=checkbox] {
            margin-right: 10px;
        }
        input[type=submit] {
            width: 100%;
            background-color: #4CAF50;
            color: white;
            padding: 14px 20px;
            margin: 8px 0;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        input[type=submit]:hover {
            background-color: #45a049;
        }
        .mqtt-config, .udp-config {
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 5px;
            margin-top: 10px;
            background-color: #f9f9f9;
        }
        .hidden {
            display: none;
        }
    </style>
    <script>
        function toggleMQTT() {
            var checkbox = document.getElementById('enableMQTT');
            var mqttConfig = document.getElementById('mqttConfig');
            mqttConfig.className = checkbox.checked ? 'mqtt-config' : 'mqtt-config hidden';
        }

        function toggleUDP() {
            var checkbox = document.getElementById('enableUDP');
            var udpConfig = document.getElementById('udpConfig');
            udpConfig.className = checkbox.checked ? 'udp-config' : 'udp-config hidden';
        }
        window.onload = function() {
            toggleMQTT();
            toggleUDP();
        }
    </script>
</head>
<body>
    <div class="container">
        <h1>ESP32 WiFi Configuration</h1>
        <form action="/save" method="post">
            <h2>WiFi Settings</h2>
            <label for="ssid">WiFi SSID:</label>
            <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi name" required>

            <label for="password">WiFi Password:</label>
            <input type="password" id="password" name="password" placeholder="Enter WiFi password" required>

            <h2>MQTT Configuration</h2>
            <label>
                <input type="checkbox" id="enableMQTT" name="enableMQTT" onchange="toggleMQTT()"> Enable MQTT Connection
            </label>

            <div id="mqttConfig" class="mqtt-config hidden">
                <label for="mqttServer">MQTT Server Address:</label>
                <input type="text" id="mqttServer" name="mqttServer" placeholder="e.g., mqtt.example.com">

                <label for="mqttPort">MQTT Port:</label>
                <input type="number" id="mqttPort" name="mqttPort" placeholder="e.g., 1883">

                <label for="mqttUsername">MQTT Username:</label>
                <input type="text" id="mqttUsername" name="mqttUsername" placeholder="Enter MQTT username">

                <label for="mqttPassword">MQTT Password:</label>
                <input type="password" id="mqttPassword" name="mqttPassword" placeholder="Enter MQTT password">

                <label for="mqttClientID">MQTT Client ID:</label>
                <input type="text" id="mqttClientID" name="mqttClientID" placeholder="Enter MQTT client ID">
            </div>

            <h2>UDP Broadcast</h2>
            <label>
                <input type="checkbox" id="enableUDP" name="enableUDP" onchange="toggleUDP()"> Enable Periodic UDP Broadcast
            </label>

            <div id="udpConfig" class="udp-config hidden">
                <label for="udpPort">UDP Broadcast Port:</label>
                <input type="number" id="udpPort" name="udpPort" placeholder="e.g., 4210">

                <label for="deviceName">Device Name:</label>
                <input type="text" id="deviceName" name="deviceName" placeholder="Enter device name">
            </div>

            <input type="submit" value="Save Configuration">
        </form>
        <p id="status"></p>
    </div>
</body>
</html>
)rawliteral";

  // 成功页面的HTML模板
  _successPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Configuration Successful</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f2f2f2;
            text-align: center;
        }
        .container {
            background-color: white;
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
            max-width: 500px;
            margin: 0 auto;
        }
        h1 {
            color: #0066cc;
        }
        p {
            font-size: 16px;
            line-height: 1.5;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WiFi Configuration Successful!</h1>
        <p>ESP32 will try to connect to the WiFi network you specified.</p>
        <p>If connection is successful, this AP will shut down.</p>
        <p>If connection fails, the AP will restart and you can try to configure again.</p>
    </div>
</body>
</html>
)rawliteral";
}

// 初始化并启动EEPROM，然后加载配置数据
void WiFiConfigManager::eepromBegin() {
  // 初始化EEPROM，设置预定义大小
  EEPROM.begin(_eepromSize);
  // 加载配置数据
  loadConfigFromEEPROM();
}

// 从EEPROM读取并加载所有配置数据到内存中
void WiFiConfigManager::loadConfigFromEEPROM() {
  // 读取WiFi凭据
  _targetSSID = readFromEEPROM(SSID, 32);
  _targetPassword = readFromEEPROM(PASS, 32);

  // 读取MQTT配置
  _mqttEnabled = readFromEEPROM(MQTT_ENABLE, 1);
  _mqttClientID = readFromEEPROM(MQTT_DEVICE_ID, 100);
  _mqttServer = readFromEEPROM(MQTT_SERVER, 31);
  _mqttPort = readFromEEPROM(MQTT_PORT, 5);
  _mqttUsername = readFromEEPROM(MQTT_USERNAME, 100);
  _mqttPassword = readFromEEPROM(MQTT_PASSWD, 256);

  // 读取UDP配置
  _udpEnabled = readFromEEPROM(UDP_ENABLE, 1);
  _deviceName = readFromEEPROM(UDP_DEVICE_NAME, 31);
  _udpPort = readFromEEPROM(UDP_PORT, 5);

  Serial.println("Configuration data loaded from EEPROM");
}

// 强制设备进入AP配置模式，通常用于重置设置或首次配置
void WiFiConfigManager::forceEnterAPConfigMode() {
  // 检查当前模式，如果不是AP模式则切换
  if (EEPROM.read(AP_MOD) != 1) {
    Serial.println("Resetting to AP Configuration mode");
    EEPROM.write(AP_MOD, 1);  // 标记为AP模式
    EEPROM.commit();          // 确保数据写入EEPROM
    ESP.restart();            // 重启设备以应用更改
  }
}

// 开始WiFiConfigManager的主要功能，尝试连接WiFi或启动AP模式
void WiFiConfigManager::begin() {
  // 首先检查是否需要强制进入AP模式
  uint8_t apMode = EEPROM.read(AP_MOD);
  if (apMode == 1) {
    Serial.println("Forced into AP Configuration mode");
    setupAPMode();
    // 重置AP模式标记，下次启动将尝试正常连接
    EEPROM.write(AP_MOD, 0);
    EEPROM.commit();
    return;  // 提前返回，不继续执行后续代码
  }

  // 检查是否有保存的WiFi凭据
  if (_targetSSID.length() > 0) {
    connectToWiFi();
  } else {
    Serial.println("No saved WiFi credentials found");
    setupAPMode();
    return;
  }

  // 如果连接失败，启动AP模式
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed, starting AP mode");
    setupAPMode();
  } else {
    Serial.println("WiFi connection successful");
    if (_connectedCallback) {
      _connectedCallback();
    }
  }
}

// 循环处理DNS请求、HTTP请求和WiFi连接状态
void WiFiConfigManager::loop() {
  // 处理DNS请求
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    _dnsServer->processNextRequest();
  }

  // 处理HTTP请求
  _server->handleClient();

  // 如果收到新的WiFi配置，尝试连接
  if (_shouldConnect) {
    _shouldConnect = false;
    connectToWiFi();

    // 如果连接成功，关闭AP模式
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.softAPdisconnect(true);
      Serial.println("AP mode disabled after successful connection");

      // 触发连接成功回调
      if (_connectedCallback) {
        _connectedCallback();
      }
    } else {
      // 如果连接失败，重新启动AP模式
      setupAPMode();
    }
  }

  // 检查是否需要提交EEPROM更改
  if (_commitNeeded && (millis() - _lastCommitTime > COMMIT_INTERVAL)) {
    commitEEPROM();
  }
}

// 提交EEPROM更改，减少频繁写入对EEPROM的损耗
void WiFiConfigManager::commitEEPROM() {
  if (_commitNeeded) {
    Serial.println("Committing EEPROM changes");
    EEPROM.commit();
    _commitNeeded = false;
    _lastCommitTime = millis();
  }
}

// 检查WiFi连接状态
bool WiFiConfigManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

// 获取设备当前IP地址，根据工作模式返回不同的IP
IPAddress WiFiConfigManager::getIP() {
  if (WiFi.getMode() == WIFI_STA) {
    return WiFi.localIP();
  } else {
    return WiFi.softAPIP();
  }
}

// 设置WiFi凭据并准备连接
bool WiFiConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
  if (ssid.length() == 0) {
    return false;
  }

  _targetSSID = ssid;
  _targetPassword = password;

  // 将WiFi凭据保存到EEPROM
  bool success = true;
  success &= writeToEEPROM(SSID, _targetSSID, 32);
  success &= writeToEEPROM(PASS, _targetPassword, 32);

  commitEEPROM();

  _shouldConnect = true;
  return success;
}

// 清除保存的WiFi凭据
void WiFiConfigManager::clearWiFiCredentials() {
  writeToEEPROM(SSID, "", 32);
  writeToEEPROM(PASS, "", 32);
  commitEEPROM();

  _targetSSID = "";
  _targetPassword = "";
}

// 设置WiFi连接超时时间
void WiFiConfigManager::setConnectionTimeout(int seconds) {
  _connectionTimeout = seconds;
}

// 设置WiFi连接成功后的回调函数
void WiFiConfigManager::setConnectedCallback(void (*callback)()) {
  _connectedCallback = callback;
}

// 设置进入AP模式后的回调函数
void WiFiConfigManager::setAPModeCallback(void (*callback)()) {
  _apModeCallback = callback;
}

// 设置AP模式的配置，启动DNS和HTTP服务器
void WiFiConfigManager::setupAPMode() {
  Serial.println("Setting up AP mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(_apSSID.c_str(), _apPassword.c_str());

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // 设置DNS服务器，将所有域名请求重定向到AP的IP
  _dnsServer->start(DNS_PORT, "*", IP);
  Serial.println("DNS server started");
  Serial.println("Domain: " + _apDomain);

  // 配置Web服务器路由
  _server->on("/", HTTP_GET, handleRootWrapper);
  _server->on("/save", HTTP_POST, handleSaveWrapper);
  _server->onNotFound(handleNotFoundWrapper);

  _server->begin();
  Serial.println("HTTP server started");

  // 触发AP模式回调
  if (_apModeCallback) {
    _apModeCallback();
  }
}

// 尝试连接到配置的WiFi网络
void WiFiConfigManager::connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.println("SSID: " + _targetSSID);
  Serial.println("Password: " + _targetPassword);

  WiFi.mode(WIFI_STA);
  WiFi.begin(_targetSSID.c_str(), _targetPassword.c_str());

  // 尝试连接，根据设置的超时时间
  int timeout = _connectionTimeout;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connection successful!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // 触发连接成功回调
    if (_connectedCallback) {
      _connectedCallback();
    }
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

// 处理配置Web界面的根路径请求，显示配置页面
void WiFiConfigManager::handleRoot() {
  // 准备HTML模板
  String html = _htmlPage;

  // 替换模板中的占位符，填充当前配置值
  // html.replace("%SSID%", _targetSSID);
  // html.replace("%PASSWORD%", _targetPassword);
  // html.replace("%MQTT_CHECKED%", _mqttEnabled == "1" ? "checked" : "");
  // html.replace("%MQTT_SERVER%", _mqttServer);
  // html.replace("%MQTT_PORT%", _mqttPort);
  // html.replace("%MQTT_USERNAME%", _mqttUsername);
  // html.replace("%MQTT_PASSWORD%", _mqttPassword);
  // html.replace("%MQTT_CLIENT_ID%", _mqttClientID);
  // html.replace("%UDP_CHECKED%", _udpEnabled == "1" ? "checked" : "");
  // html.replace("%UDP_PORT%", _udpPort);
  // html.replace("%DEVICE_NAME%", _deviceName);

  _server->send(200, "text/html", html);
}

// 处理保存配置的请求，将配置写入EEPROM并尝试连接WiFi
void WiFiConfigManager::handleSave() {
  bool configChanged = false;

  if (_server->hasArg("ssid") && _server->hasArg("password")) {
    // 获取WiFi配置
    String newSSID = _server->arg("ssid");
    String newPassword = _server->arg("password");

    // 检查是否有变化
    if (newSSID != _targetSSID || newPassword != _targetPassword) {
      _targetSSID = newSSID;
      _targetPassword = newPassword;

      // 保存到EEPROM
      writeToEEPROM(SSID, _targetSSID, 32);
      writeToEEPROM(PASS, _targetPassword, 32);
      configChanged = true;
    }

    // 获取并保存MQTT配置
    bool enableMQTT = _server->hasArg("enableMQTT");
    String mqttEnabled = enableMQTT ? "1" : "0";

    if (mqttEnabled != _mqttEnabled) {
      _mqttEnabled = mqttEnabled;
      writeToEEPROM(MQTT_ENABLE, _mqttEnabled, 1);
      configChanged = true;
    }

    // 如果启用了MQTT，获取相关参数
    if (enableMQTT) {
      String mqttServer = _server->arg("mqttServer");
      String mqttPort = _server->arg("mqttPort");
      String mqttUsername = _server->arg("mqttUsername");
      String mqttPassword = _server->arg("mqttPassword");
      String mqttClientID = _server->arg("mqttClientID");

      // 检查是否有变化并保存
      if (mqttServer != _mqttServer) {
        _mqttServer = mqttServer;
        writeToEEPROM(MQTT_SERVER, _mqttServer, 31);
        configChanged = true;
      }

      if (mqttPort != _mqttPort) {
        _mqttPort = mqttPort;
        writeToEEPROM(MQTT_PORT, _mqttPort, 5);
        configChanged = true;
      }

      if (mqttUsername != _mqttUsername) {
        _mqttUsername = mqttUsername;
        writeToEEPROM(MQTT_USERNAME, _mqttUsername, 100);
        configChanged = true;
      }

      if (mqttPassword != _mqttPassword) {
        _mqttPassword = mqttPassword;
        writeToEEPROM(MQTT_PASSWD, _mqttPassword, 256);
        configChanged = true;
      }

      if (mqttClientID != _mqttClientID) {
        _mqttClientID = mqttClientID;
        writeToEEPROM(MQTT_DEVICE_ID, _mqttClientID, 100);
        configChanged = true;
      }
    }

    // 获取UDP广播配置
    bool enableUDP = _server->hasArg("enableUDP");
    String udpEnabled = enableUDP ? "1" : "0";

    if (udpEnabled != _udpEnabled) {
      _udpEnabled = udpEnabled;
      writeToEEPROM(UDP_ENABLE, _udpEnabled, 1);
      configChanged = true;
    }

    if (enableUDP) {
      String udpPort = _server->arg("udpPort");
      String deviceName = _server->arg("deviceName");

      // 检查是否有变化并保存
      if (udpPort != _udpPort) {
        _udpPort = udpPort;
        writeToEEPROM(UDP_PORT, _udpPort, 5);
        configChanged = true;
      }

      if (deviceName != _deviceName) {
        _deviceName = deviceName;
        writeToEEPROM(UDP_DEVICE_NAME, _deviceName, 31);
        configChanged = true;
      }
    }

    // 如果配置有变化，确保提交EEPROM
    if (configChanged) {
      commitEEPROM();
    }

    Serial.println("Configuration saved:");
    Serial.println("SSID: " + _targetSSID);
    Serial.println("Password: " + _targetPassword);
    Serial.println("MQTT Enabled: " + _mqttEnabled);
    Serial.println("MQTT Server: " + _mqttServer);
    Serial.println("MQTT Port: " + _mqttPort);
    Serial.println("MQTT Username: " + _mqttUsername);
    Serial.println("MQTT Client ID: " + _mqttClientID);
    Serial.println("UDP Broadcast Enabled: " + _udpEnabled);
    Serial.println("UDP Port: " + _udpPort);
    Serial.println("Device Name: " + _deviceName);

    // 先发送响应
    _server->send(200, "text/html", _successPage);

    // 添加短暂延迟确保响应被发送
    delay(1000);

    // 设置连接标志
    _shouldConnect = true;
  } else {
    _server->send(400, "text/plain", "Missing required parameters");
  }
}

// 处理未找到的页面请求，将所有未找到的请求重定向到配置页面
void WiFiConfigManager::handleNotFound() {
  _server->sendHeader("Location", "/", true);
  _server->send(302, "text/plain", "");
}

// 从EEPROM读取字符串，指定起始地址和最大长度
String WiFiConfigManager::readFromEEPROM(int startAddr, int maxLength) {
  char data[maxLength + 1];  // +1 用于字符串结束符

  // 读取数据直到结束符或达到最大长度
  int i;
  for (i = 0; i < maxLength; i++) {
    char c = char(EEPROM.read(startAddr + i));
    if (c == '\0') {
      break;  // 找到字符串结束符
    }
    data[i] = c;
  }

  // 确保字符串以结束符结束
  data[i] = '\0';

  // 特殊处理：如果是空字符或只有结束符，返回空字符串
  if (i == 0 || (i == 1 && data[0] == '\0')) {
    return String("");
  }

  return String(data);
}

// 将字符串写入EEPROM，指定起始地址和最大长度，减少不必要的写入操作
bool WiFiConfigManager::writeToEEPROM(int startAddr, const String& data, int maxLength) {
  // 检查起始地址是否有效
  if (startAddr < 0 || startAddr + maxLength >= _eepromSize) {
    return false;
  }

  // 检查数据是否超出限制
  if (data.length() > maxLength) {
    Serial.println("Warning: Data length exceeds limit, will be truncated");
    // 数据将被截断不报错，但会警告
  }

  // 写入数据
  int dataLength = min((int)data.length(), maxLength);
  for (int i = 0; i < dataLength; i++) {
    // 只有当数据不同时才写入，减少写入次数
    if (EEPROM.read(startAddr + i) != (uint8_t)data.charAt(i)) {
      EEPROM.write(startAddr + i, data.charAt(i));
      _commitNeeded = true;
    }
  }

  // 写入字符串结束符
  if (EEPROM.read(startAddr + dataLength) != '\0') {
    EEPROM.write(startAddr + dataLength, '\0');
    _commitNeeded = true;
  }

  // 清除剩余字节
  for (int i = dataLength + 1; i < maxLength; i++) {
    // 只有当数据不是0时才清除，减少写入次数
    if (EEPROM.read(startAddr + i) != 0) {
      EEPROM.write(startAddr + i, 0);
      _commitNeeded = true;
    }
  }

  // 延迟提交，但确保在需要时进行提交
  _lastCommitTime = millis();

  return true;
}

// 静态回调函数包装器，将Web服务器请求转发给单例实例处理
void WiFiConfigManager::handleRootWrapper() {
  if (_instance) {
    _instance->handleRoot();
  }
}

void WiFiConfigManager::handleSaveWrapper() {
  if (_instance) {
    _instance->handleSave();
  }
}

void WiFiConfigManager::handleNotFoundWrapper() {
  if (_instance) {
    _instance->handleNotFound();
  }
}