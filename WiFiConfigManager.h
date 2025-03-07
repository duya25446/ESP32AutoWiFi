#ifndef WIFI_CONFIG_MANAGER_H
#define WIFI_CONFIG_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>

class WiFiConfigManager {
public:
  // 构造函数
  WiFiConfigManager(const char* apSSID = "ESP32_Config",
                    const char* apPassword = "12345678",
                    const char* apDomain = "wificonfig.com",
                    int eepromSize = 1024);

  // 初始化函数
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

  // 强制重置到AP模式
  void forceEnterAPConfigMode();

  // 设置连接尝试超时时间（秒）
  void setConnectionTimeout(int seconds);

  // 设置回调函数
  void setConnectedCallback(void (*callback)());
  void setAPModeCallback(void (*callback)());

  // EEPROM初始化
  void eepromBegin();

  // 获取配置参数
  String getMQTTServer() const {
    return _mqttServer;
  }
  String getMQTTPort() const {
    return _mqttPort;
  }
  String getMQTTUsername() const {
    return _mqttUsername;
  }
  String getMQTTPassword() const {
    return _mqttPassword;
  }
  String getMQTTClientID() const {
    return _mqttClientID;
  }
  String getUDPPort() const {
    return _udpPort;
  }
  String getDeviceName() const {
    return _deviceName;
  }
  bool getMQTTEnabled() const {
    return _mqttEnabled == "1";
  }
  bool getUDPEnabled() const {
    return _udpEnabled == "1";
  }

private:
  // AP模式参数
  String _apSSID;
  String _apPassword;

  // 目标WiFi参数
  String _targetSSID;
  String _targetPassword;

  // EEPROM地址
  /*
  AP_MOD: 地址从0开始，长度为1字节
  SSID: 地址从1开始，长度为32字节(从1到32)
  PASS: 地址从33开始，长度为32字节(从33到64)
  MQTT_ENABLE: 地址从65开始，长度为1字节
  MQTT_DEVICE_ID: 地址从66开始，长度为101字节(从66到166)
  MQTT_SERVER: 地址从167开始，长度为32字节(从167到198)
  MQTT_PORT: 地址从199开始，长度为6字节(从199到204)
  MQTT_USERNAME: 地址从205开始，长度为101字节(从205到305)
  MQTT_PASSWD: 地址从306开始，长度为257字节(从306到562)
  UDP_ENABLE: 地址从563开始，长度为1字节
  UDP_DEVICE_NAME: 地址从564开始，长度为32字节(从564到595)
  UDP_PORT: 地址从596开始，长度为6字节(从596到601)
  总共需要602字节的EEPROM空间(从地址0到601)。
  */
  int _eepromSize;
  // 基础设置
  static const int AP_MOD = 0;  // 1字节
  static const int SSID = 1;    // 32字节
  static const int PASS = 33;   // 32字节

  // MQTT配置
  static const int MQTT_ENABLE = 65;     // 1字节
  static const int MQTT_DEVICE_ID = 66;  // 101字节
  static const int MQTT_SERVER = 167;    // 32字节
  static const int MQTT_PORT = 199;      // 6字节
  static const int MQTT_USERNAME = 205;  // 101字节
  static const int MQTT_PASSWD = 306;    // 257字节

  // UDP配置
  static const int UDP_ENABLE = 563;       // 1字节
  static const int UDP_DEVICE_NAME = 564;  // 32字节
  static const int UDP_PORT = 596;         // 6字节

  // 总长度: 602字节

  // MQTT和UDP广播的字段
  String _mqttServer;
  String _mqttPort;
  String _mqttUsername;
  String _mqttPassword;
  String _mqttClientID;
  String _udpPort;
  String _udpEnabled;
  String _mqttEnabled;
  String _deviceName;

  // DNS服务器相关
  DNSServer* _dnsServer;
  String _apDomain;
  static const byte DNS_PORT = 53;

  // Web服务器
  WebServer* _server;

  // 连接状态标志
  bool _shouldConnect;
  int _connectionTimeout;

  // 防止过多的EEPROM写入
  bool _commitNeeded;
  unsigned long _lastCommitTime;
  static const unsigned long COMMIT_INTERVAL = 5000;  // 5秒间隔

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
  String readFromEEPROM(int startAddr, int maxLength);
  bool writeToEEPROM(int startAddr, const String& data, int maxLength);
  void commitEEPROM();
  void loadConfigFromEEPROM();

  // 静态回调处理器
  static WiFiConfigManager* _instance;
  static void handleRootWrapper();
  static void handleSaveWrapper();
  static void handleNotFoundWrapper();

  // HTML页面
  const char* _htmlPage;
  const char* _successPage;
};

#endif  // WIFI_CONFIG_MANAGER_H
