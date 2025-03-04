#ifndef WIFI_CONFIG_MANAGER_H
#define WIFI_CONFIG_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

class WiFiConfigManager {
public:
    // 构造函数
    WiFiConfigManager(const char* apSSID = "ESP32_Config", 
                      const char* apPassword = "12345678",
                      int eepromSize = 128);

    // 初始化函数
    void E2PROMbegin();
    void begin();

    // 处理循环
    void loop();

    // 检查WiFi连接状态
    bool isConnected();

    // 获取当前IP地址
    IPAddress getIP();

    // 手动设置WiFi凭据
    bool setWiFiCredentials(const String& ssid, const String& password);

    // 手动清除存储的WiFi凭据
    void clearWiFiCredentials();

    // 设置连接尝试超时时间（秒）
    void setConnectionTimeout(int seconds);

    // 设置回调函数
    void setConnectedCallback(void (*callback)());
    void setAPModeCallback(void (*callback)());

private:
    // AP模式参数
    String _apSSID;
    String _apPassword;

    // 目标WiFi参数
    String _targetSSID;
    String _targetPassword;

    // EEPROM地址
    int _eepromSize;
    static const int SSID_ADDR = 0;
    static const int PASS_ADDR = 32;

    // Web服务器
    WebServer* _server;

    // 连接状态标志
    bool _shouldConnect;
    int _connectionTimeout;

    // 回调函数指针
    void (*_connectedCallback)();
    void (*_apModeCallback)();

    // 内部函数
    void setupAPMode();
    void connectToWiFi();
    void handleRoot();
    void handleSave();
    void handleNotFound();

    // EEPROM操作函数

    String readFromEEPROM(int startAddr);
    void writeToEEPROM(int startAddr, String data);

    // 静态回调处理器
    static WiFiConfigManager* _instance;
    static void handleRootWrapper();
    static void handleSaveWrapper();
    static void handleNotFoundWrapper();

    // HTML页面
    const char* _htmlPage;
    const char* _successPage;
};

#endif // WIFI_CONFIG_MANAGER_H