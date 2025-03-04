#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MQTT.h>
#include <WiFiConfigManager.h>

// 创建WiFiConfigManager实例
WiFiConfigManager wifiManager("ESP32_Config", "12345678", "wificonfig.com");

// 连接成功回调函数
void onWiFiConnected() {
  Serial.println("WiFi connected callback!");
  // 在这里可以添加WiFi连接成功后需要执行的操作
  // 例如：启动其他需要网络连接的服务
}

// AP模式激活回调函数
void onAPModeActivated() {
  Serial.println("AP mode activated callback!");
  // 在这里可以添加进入AP模式后需要执行的操作
  // 例如：点亮指示灯表示设备处于配置模式
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting WiFi Config Manager...");

  // 设置回调函数
  wifiManager.setConnectedCallback(onWiFiConnected);
  wifiManager.setAPModeCallback(onAPModeActivated);

  // 设置连接尝试超时时间（秒）
  wifiManager.setConnectionTimeout(15);

  // 初始化WiFiConfigManager
  wifiManager.E2PROMbegin();
  wifiManager.clearWiFiCredentials();
  wifiManager.begin();

}

void loop() {
  // 必须定期调用loop函数以处理HTTP请求和WiFi连接
  wifiManager.loop();

  // 添加其他需要定期执行的代码

  // 检查WiFi连接状态并执行相应操作
  if (wifiManager.isConnected()) {
    // WiFi已连接，执行需要网络连接的操作

  } else {
    // WiFi未连接，执行不需要网络连接的操作
  }
}
