#include "WiFiConfigManager.h"

// 静态成员初始化
WiFiConfigManager* WiFiConfigManager::_instance = nullptr;

// 构造函数
WiFiConfigManager::WiFiConfigManager(const char* apSSID, const char* apPassword, int eepromSize) 
    : _apSSID(apSSID), 
      _apPassword(apPassword),
      _eepromSize(eepromSize),
      _shouldConnect(false),
      _connectionTimeout(20),
      _connectedCallback(nullptr),
      _apModeCallback(nullptr)
{
    _server = new WebServer(80);
    _instance = this;

    // HTML页面
    _htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WiFi 配置</title>
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
        label {
            display: block;
            margin-top: 10px;
            font-weight: bold;
        }
        input[type=text], input[type=password] {
            width: 100%;
            padding: 10px;
            margin: 8px 0;
            display: inline-block;
            border: 1px solid #ccc;
            border-radius: 4px;
            box-sizing: border-box;
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
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 WiFi 配置</h1>
        <form action="/save" method="post">
            <label for="ssid">WiFi 名称:</label>
            <input type="text" id="ssid" name="ssid" placeholder="输入WiFi名称" required>

            <label for="password">WiFi 密码:</label>
            <input type="password" id="password" name="password" placeholder="输入WiFi密码" required>

            <input type="submit" value="连接">
        </form>
        <p id="status"></p>
    </div>
</body>
</html>
)rawliteral";

    // 成功页面
    _successPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>配置成功</title>
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
        <h1>WiFi 配置成功！</h1>
        <p>ESP32 将尝试连接到您指定的WiFi网络。</p>
        <p>如果连接成功，此AP将关闭。</p>
        <p>如果连接失败，AP将重新启动，您可以重新尝试配置。</p>
    </div>
</body>
</html>
)rawliteral";
}

void WiFiConfigManager::begin() {
    // 初始化EEPROM
    EEPROM.begin(_eepromSize);

    // 尝试从EEPROM读取保存的WiFi凭据
    _targetSSID = readFromEEPROM(SSID_ADDR);
    _targetPassword = readFromEEPROM(PASS_ADDR);

    Serial.println("Stored WiFi credentials:");
    Serial.println("SSID: " + _targetSSID);
    Serial.println("Password: " + _targetPassword);

    // 如果有保存的凭据，尝试连接
    if (_targetSSID.length() > 0) {
        connectToWiFi();
    }

    // 如果连接失败或没有保存的凭据，启动AP模式
    if (WiFi.status() != WL_CONNECTED) {
        setupAPMode();
    }
}

void WiFiConfigManager::loop() {
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
}

bool WiFiConfigManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiConfigManager::getIP() {
    if (WiFi.getMode() == WIFI_STA) {
        return WiFi.localIP();
    } else {
        return WiFi.softAPIP();
    }
}

bool WiFiConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    if (ssid.length() == 0) {
        return false;
    }

    _targetSSID = ssid;
    _targetPassword = password;

    // 将WiFi凭据保存到EEPROM
    writeToEEPROM(SSID_ADDR, _targetSSID);
    writeToEEPROM(PASS_ADDR, _targetPassword);
    EEPROM.commit();

    _shouldConnect = true;
    return true;
}

void WiFiConfigManager::clearWiFiCredentials() {
    writeToEEPROM(SSID_ADDR, "");
    writeToEEPROM(PASS_ADDR, "");
    EEPROM.commit();

    _targetSSID = "";
    _targetPassword = "";
}

void WiFiConfigManager::setConnectionTimeout(int seconds) {
    _connectionTimeout = seconds;
}

void WiFiConfigManager::setConnectedCallback(void (*callback)()) {
    _connectedCallback = callback;
}

void WiFiConfigManager::setAPModeCallback(void (*callback)()) {
    _apModeCallback = callback;
}

void WiFiConfigManager::setupAPMode() {
    Serial.println("Setting up AP Mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSSID.c_str(), _apPassword.c_str());

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

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
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // 触发连接成功回调
        if (_connectedCallback) {
            _connectedCallback();
        }
    } else {
        Serial.println("\nFailed to connect to WiFi!");
    }
}

void WiFiConfigManager::handleRoot() {
    _server->send(200, "text/html", _htmlPage);
}

void WiFiConfigManager::handleSave() {
    if (_server->hasArg("ssid") && _server->hasArg("password")) {
        _targetSSID = _server->arg("ssid");
        _targetPassword = _server->arg("password");

        // 将WiFi凭据保存到EEPROM
        writeToEEPROM(SSID_ADDR, _targetSSID);
        writeToEEPROM(PASS_ADDR, _targetPassword);
        EEPROM.commit();

        Serial.println("New WiFi credentials saved:");
        Serial.println("SSID: " + _targetSSID);
        Serial.println("Password: " + _targetPassword);

        _server->send(200, "text/html", _successPage);

        // 设置标志，在主循环中尝试连接
        _shouldConnect = true;
    } else {
        _server->send(400, "text/plain", "Missing required parameters");
    }
}

void WiFiConfigManager::handleNotFound() {
    _server->send(404, "text/plain", "Not found");
}

// 从EEPROM读取字符串
String WiFiConfigManager::readFromEEPROM(int startAddr) {
    char data[32];
    for (int i = 0; i < 32; i++) {
        data[i] = char(EEPROM.read(startAddr + i));
    }
    data[31] = '\0'; // 确保字符串结束
    return String(data);
}

// 将字符串写入EEPROM
void WiFiConfigManager::writeToEEPROM(int startAddr, String data) {
    for (int i = 0; i < data.length(); i++) {
        EEPROM.write(startAddr + i, data[i]);
    }
    // 写入字符串结束符
    EEPROM.write(startAddr + data.length(), '\0');
}

// 静态回调包装器
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