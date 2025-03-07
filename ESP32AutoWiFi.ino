#include <Arduino.h>
#include <WiFi.h>
#include <WiFiConfigManager.h>

// 创建WiFiConfigManager实例
// 参数说明: AP模式的SSID, AP模式的密码, 域名(captive portal重定向域名)
WiFiConfigManager wifiManager("ESP32_Config", "12345678", "wificonfig.com");

/**
 * WiFi连接成功回调函数
 * 当ESP32成功连接到WiFi网络时，会调用此函数
 * 用于打印连接信息和已保存的配置
 */
void onWiFiConnected() {
  Serial.println("WiFi Connected Successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // 打印MQTT配置信息（如果已启用）
  // getMQTTEnabled()返回布尔值，表示是否启用了MQTT功能
  if (wifiManager.getMQTTEnabled()) {
    Serial.println("MQTT Configuration:");
    Serial.println("- Server: " + wifiManager.getMQTTServer());
    Serial.println("- Port: " + wifiManager.getMQTTPort());
    Serial.println("- Username: " + wifiManager.getMQTTUsername());
    Serial.println("- Client ID: " + wifiManager.getMQTTClientID());
  } else {
    Serial.println("MQTT Function Disabled");
  }

  // 打印UDP广播配置信息（如果已启用）
  // getUDPEnabled()返回布尔值，表示是否启用了UDP广播功能
  if (wifiManager.getUDPEnabled()) {
    Serial.println("UDP Broadcast Configuration:");
    Serial.println("- Device Name: " + wifiManager.getDeviceName());
    Serial.println("- UDP Port: " + wifiManager.getUDPPort());
  } else {
    Serial.println("UDP Broadcast Function Disabled");
  }
}

/**
 * AP模式激活回调函数
 * 当设备进入AP配置模式时，会调用此函数
 * 用于提示用户如何连接到AP并进行配置
 */
void onAPModeActivated() {
  Serial.println("AP Mode Activated!");
  Serial.println("SSID: ESP32_Config");
  Serial.println("Password: 12345678");
  Serial.println("Please connect to this WiFi network and visit http://192.168.4.1 or http://wificonfig.com to configure");
}

void setup() {
  // 初始化串口通信，波特率设为115200
  Serial.begin(115200);
  Serial.println("\nStarting WiFi Configuration Manager...");

  // 设置回调函数
  // 这些回调函数会在特定事件发生时被调用
  wifiManager.setConnectedCallback(onWiFiConnected);  // 设置WiFi连接成功的回调
  wifiManager.setAPModeCallback(onAPModeActivated);   // 设置进入AP模式的回调

  // 设置WiFi连接尝试的超时时间（单位：秒）
  // 如果在设定时间内无法连接到WiFi，会进入AP配置模式
  wifiManager.setConnectionTimeout(15);

  // 初始化EEPROM - 必须先调用此方法以准备存储配置数据
  // 这一步会从EEPROM中读取之前保存的配置
  wifiManager.eepromBegin();

  // 如果需要强制进入AP配置模式，可以取消下面代码的注释
  // 这在需要重新配置设备时非常有用
  wifiManager.forceEnterAPConfigMode();

  // 开始WiFiConfigManager，会自动尝试连接之前的WiFi或启动AP模式
  // 如果EEPROM中保存了有效的WiFi凭据，会尝试连接
  // 如果连接失败或没有保存的凭据，会启动AP配置模式
  wifiManager.begin();
}

// 上次状态检查的时间戳（毫秒）
unsigned long lastStatusCheckTime = 0;
// 状态检查的时间间隔（毫秒），10秒检查一次
const unsigned long STATUS_CHECK_INTERVAL = 10000;

void loop() {
  // 必须定期调用loop函数以处理HTTP请求和WiFi连接状态变化
  // 在AP模式下，这会处理用户的配置请求
  // 在STA模式下，这会监控WiFi连接状态
  wifiManager.loop();

  // 定期检查状态
  // 使用millis()而不是delay()可以避免阻塞其他操作
  unsigned long currentTime = millis();
  if (currentTime - lastStatusCheckTime > STATUS_CHECK_INTERVAL) {
    lastStatusCheckTime = currentTime;

    // 检查WiFi连接状态
    // isConnected()返回布尔值，表示是否已连接到WiFi
    if (wifiManager.isConnected()) {
      Serial.print("WiFi Connected - SSID: ");
      Serial.print(WiFi.SSID());  // 打印已连接的WiFi名称
      Serial.print(", IP: ");
      Serial.println(wifiManager.getIP());  // 打印分配的IP地址
    } else {
      Serial.println("WiFi Not Connected");
    }
  }

  // 这里可以添加其他需要定期执行的代码
  // 例如：读取传感器数据、处理业务逻辑等

  // 短暂延时以降低CPU使用率，但不要使用较长时间的delay
  // 10毫秒的延时足够降低功耗，又不会影响响应速度
  delay(10);
}
