# ESP32 WiFiConfigManager 库

## 简介

ESP32 WiFiConfigManager 是一个便捷的Arduino ESP32 WiFi配置库，它能够帮助您的ESP32项目更容易地进行WiFi连接管理。通过该库，您可以：

- 创建一个配置门户（AP模式），允许用户通过手机或电脑轻松配置WiFi连接
- 自动保存用户配置的WiFi凭据到EEPROM
- 在设备重启后自动连接到保存的WiFi网络
- 配置并保存MQTT服务器连接参数
- 配置并保存UDP广播参数
- 通过友好的域名访问配置页面，无需记忆IP地址
- 自定义连接成功和AP模式激活的回调函数
- 简化ESP32项目中的WiFi管理逻辑

## 特性

- 🚀 简单易用的API设计
- 🔄 自动模式切换（STA/AP）
- 💾 配置持久化存储
- 📱 响应式Web配置界面
- 🔌 自定义DNS服务，可使用域名访问
- ⚙️ 可定制的超时和回调机制
- 🔒 支持AP模式密码保护
- 📊 MQTT连接配置
- 📡 UDP广播配置

__注意，这个库引用了很多乐鑫的官方库，导致占用空间很多，单独编译该库的占用如下(代码修改以后又不一样了但是差别不是很大，参考即可)__


FOR ESP32 4MB:
>Sketch uses 946732 bytes (72%) of program storage space. Maximum is 1310720 bytes.
>Global variables use 45780 bytes (13%) of dynamic memory, leaving 281900 bytes for local variables. Maximum is 327680 bytes.

FOR ESP32S3 8MB:
>Sketch uses 911140 bytes (27%) of program storage space. Maximum is 3342336 bytes.
>Global variables use 44764 bytes (13%) of dynamic memory, leaving 282916 bytes for local variables. Maximum is 327680 bytes.

FOR ESP32C3 4MB:
>Sketch uses 973764 bytes (74%) of program storage space. Maximum is 1310720 bytes.
>Global variables use 35900 bytes (10%) of dynamic memory, leaving 291780 bytes for local variables. Maximum is 327680 bytes.

## 版本

v0.3：
优化了EEPROM读写操作，减少写入次数以延长Flash寿命；增加了配置读取功能，可以通过简单的API获取已保存的配置；改进了错误处理和边界情况优化；配置页面现在能显示已保存的设置；修复了部分安全和稳定性问题。

v0.2：
HTML网页加入MQTT选项和基于JS的折叠表单，包括ID USERNAME PASSWORD SERVER和UDP广播选项，加入打印功能用于测试，计划在下个版本加入MQTT连接和在内网UDP广播自己的IP与设定的设备名功能。

v0.1：
实现最基础的HTML表单和POST请求的回读，模拟EEPROM的读写和DNS等业务逻辑。

## 安装

很遗憾我不会上传到arduino lib也不会把库打包成zip，这就是两个文件.cpp和.h构成的类，可以将WiFiConfigManager.h文件和WiFiConfigManager.cpp文件直接复制粘贴到你的Arduino项目文件夹内（和.ino文件在一个目录下），arduino会自动引用同文件夹下的所有源文件，你可以直接在项目中 `#include <WiFiConfigManager.h>` 来使用这个类库。

## 基本使用

以下是一个简单的示例，演示如何在您的项目中使用WiFiConfigManager:

```cpp
#include <Arduino.h>
#include <WiFiConfigManager.h>

// 创建WiFiConfigManager实例
// 参数: AP名称, AP密码, AP域名
WiFiConfigManager wifiManager("ESP32_Config", "12345678", "wificonfig.com");

// 连接成功回调函数
void onWiFiConnected() {
  Serial.println("WiFi Connected Successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // 打印保存的配置信息
  if (wifiManager.getMQTTEnabled()) {
    Serial.println("MQTT Configuration:");
    Serial.println("- Server: " + wifiManager.getMQTTServer());
    Serial.println("- Port: " + wifiManager.getMQTTPort());
  }
}

// AP模式激活回调函数
void onAPModeActivated() {
  Serial.println("AP Mode Activated!");
  Serial.println("Please connect to ESP32_Config WiFi network with password 12345678");
  Serial.println("Then open http://wificonfig.com in your browser to configure");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  
  // 设置回调函数
  wifiManager.setConnectedCallback(onWiFiConnected);
  wifiManager.setAPModeCallback(onAPModeActivated);
  
  // 设置连接尝试超时时间（秒）
  wifiManager.setConnectionTimeout(15);
  
  // 初始化EEPROM
  wifiManager.eepromBegin();
  
  // 强制进入配置模式（可选，取消注释即可启用）
  // wifiManager.forceEnterAPConfigMode();
  
  // 初始化WiFiConfigManager
  wifiManager.begin();
}

void loop() {
  // 必须定期调用loop函数以处理HTTP请求和WiFi连接
  wifiManager.loop();
  
  // 检查WiFi连接状态并执行相应操作
  if (wifiManager.isConnected()) {
    // WiFi已连接，执行需要网络连接的操作
    // 例如：发送传感器数据到服务器等
  } else {
    // WiFi未连接，执行不需要网络连接的操作
    // 例如：本地数据采集、显示等
  }
  
  // 短暂延时以降低CPU使用率
  delay(10);
}
```
__（图还是0.1版本的，等0.3版本后再更新）__

到手机或电脑连接WiFi：

![WiFi Connect](image/1.jpg)

到浏览器输入配置的域名，默认配置为"wificonfig.com":

![Config page](image/2.jpg)

使用输入SSID和PASSWORD，点击连接，不出意外就会收到连接成功的页面：

![Config page](image/3.jpg)

如果连接WiFi成功，这个SSID和PASSWORD就会被记录到ESP32的Flash模拟的EEPROM中，下次启动时会直接尝试连接，如果连接失败会重新启动配置AP，如此就完成了一次WiFi的配置。

## 详细API文档

### 构造函数

```cpp
WiFiConfigManager(const char* apSSID = "ESP32_Config", 
                 const char* apPassword = "12345678",
                 const char* apDomain = "wificonfig.com",
                 int eepromSize = 1024);
```

- `apSSID`: AP模式的WiFi名称
- `apPassword`: AP模式的WiFi密码
- `apDomain`: AP模式下的域名，用于访问配置页面
- `eepromSize`: EEPROM分配的大小（以字节为单位）

### 主要方法

#### `void eepromBegin()`
初始化EEPROM并加载保存的配置数据。必须在`begin()`之前调用。

#### `void begin()`
初始化WiFiConfigManager。如果有已保存的WiFi配置，会尝试连接；如果连接失败或没有保存的配置，会进入AP模式。

#### `void loop()`
必须在主loop中定期调用，用于处理HTTP请求和WiFi状态管理。

#### `bool isConnected()`
返回WiFi是否连接成功。

#### `IPAddress getIP()`
返回当前IP地址（AP模式时返回AP的IP，STA模式时返回分配的IP）。

#### `bool setWiFiCredentials(const String& ssid, const String& password)`
手动设置WiFi凭据并保存到EEPROM。

#### `void clearWiFiCredentials()`
清除存储的WiFi凭据。

#### `void forceEnterAPConfigMode()`
强制设备进入AP配置模式，无论是否有已保存的配置。

#### `void setConnectionTimeout(int seconds)`
设置连接尝试的超时时间（默认20秒）。

#### `void setConnectedCallback(void (*callback)())`
设置WiFi连接成功的回调函数。

#### `void setAPModeCallback(void (*callback)())`
设置进入AP模式的回调函数。

### 新增配置获取方法

#### `String getMQTTServer() const`
获取配置的MQTT服务器地址。

#### `String getMQTTPort() const`
获取配置的MQTT服务器端口。

#### `String getMQTTUsername() const`
获取配置的MQTT用户名。

#### `String getMQTTPassword() const`
获取配置的MQTT密码。

#### `String getMQTTClientID() const`
获取配置的MQTT客户端ID。

#### `String getUDPPort() const`
获取配置的UDP广播端口。

#### `String getDeviceName() const`
获取配置的设备名称。

#### `bool getMQTTEnabled() const`
获取MQTT功能是否启用。

#### `bool getUDPEnabled() const`
获取UDP广播功能是否启用。

## 配置页面使用指南

1. 当ESP32进入配置模式时，它会创建一个名为`apSSID`的WiFi热点
2. 使用您的手机或电脑连接到此热点（密码为`apPassword`）
3. 打开浏览器并访问`http://wificonfig.com`或`http://192.168.4.1`
4. 在配置页面中输入您要连接的WiFi网络的SSID和密码
5. 如需配置MQTT连接，勾选"Enable MQTT Connection"并填写相关信息
6. 如需配置UDP广播，勾选"Enable Periodic UDP Broadcast"并填写相关信息
7. 点击"Save Configuration"按钮
8. 若连接成功，ESP32将保存这些设置并重启为正常模式
9. 若连接失败，它将重新进入配置模式，您可以再次尝试

## EEPROM存储机制

本库使用EEPROM来保存配置信息，存储布局如下：

- AP_MOD: 地址0，长度1字节（控制是否进入AP模式）
- SSID: 地址1-32，长度32字节
- PASS: 地址33-64，长度32字节
- MQTT_ENABLE: 地址65，长度1字节
- MQTT_DEVICE_ID: 地址66-166，长度101字节
- MQTT_SERVER: 地址167-198，长度32字节
- MQTT_PORT: 地址199-204，长度6字节
- MQTT_USERNAME: 地址205-305，长度101字节
- MQTT_PASSWD: 地址306-562，长度257字节
- UDP_ENABLE: 地址563，长度1字节
- UDP_DEVICE_NAME: 地址564-595，长度32字节
- UDP_PORT: 地址596-601，长度6字节

总共需要602字节的EEPROM空间。为了延长Flash寿命，库采用了以下优化：

- 仅在数据变化时才写入EEPROM
- 使用延迟提交机制，减少频繁写入
- 在写入前检查数据是否已存在

## 注意事项

- AP模式的DNS功能仅在设备连接到ESP32热点时有效
- 确保选择唯一的AP名称，避免与现有网络冲突
- 建议设置AP密码以增强安全性
- 配置页面使用的是纯HTML/CSS/JavaScript，无需外部资源，适合离线环境
- 由于EEPROM存储空间有限，请注意各字段的最大长度限制

## 高级使用

### 在特定条件下强制进入配置模式

```cpp
// 例如：使用一个按钮触发配置模式
const int configPin = 0; // GPIO0通常连接到ESP32开发板上的BOOT按钮

void setup() {
  // ...
  pinMode(configPin, INPUT_PULLUP);
  
  // 如果按钮被按下，强制进入AP模式
  if (digitalRead(configPin) == LOW) {
    Serial.println("Button pressed, entering config mode");
    wifiManager.forceEnterAPConfigMode();
  }
  
  wifiManager.eepromBegin();
  wifiManager.begin();
}
```

### 手动设置和清除WiFi凭据

```cpp
// 手动设置WiFi凭据
wifiManager.setWiFiCredentials("MyWiFi", "MyPassword");

// 清除保存的WiFi凭据
wifiManager.clearWiFiCredentials();
```

### 定期检查WiFi连接状态

```cpp
unsigned long lastCheckTime = 0;
const long checkInterval = 30000; // 检查间隔为30秒

void loop() {
  wifiManager.loop();
  
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTime >= checkInterval) {
    lastCheckTime = currentTime;
    
    if (wifiManager.isConnected()) {
      Serial.print("Connected to WiFi - SSID: ");
      Serial.print(WiFi.SSID());
      Serial.print(", IP: ");
      Serial.println(wifiManager.getIP());
    } else {
      Serial.println("WiFi not connected");
    }
  }
}
```

## 故障排除

如果您在使用过程中遇到问题，请尝试以下步骤：

1. **无法连接到AP热点**
   - 确保ESP32有足够的电源
   - 尝试重启设备
   - 检查AP密码是否正确

2. **配置页面不显示**
   - 尝试使用IP地址 `192.168.4.1` 代替域名
   - 确认您的设备已连接到ESP32的AP热点
   - 检查浏览器是否能正常加载其他网页

3. **WiFi凭据已保存但无法连接**
   - 确认输入的SSID和密码正确
   - 检查目标WiFi网络信号强度
   - 目标WiFi是否是隐藏网络或需要Web认证

4. **提交表单后页面出现错误**
   - 检查ESP32的串口输出查看详细错误信息
   - 可能是连接尝试过程中断开了AP连接，尝试增加超时时间

5. **EEPROM读写问题**
   - 如果遇到EEPROM读写错误，尝试增加eepromSize参数
   - 确保在调用其他方法前已调用eepromBegin()

## 项目贡献

欢迎为这个项目做出贡献！您可以通过以下方式参与：

- 报告bug和提交功能需求
- 提交pull request改进代码或文档
- 分享您使用此库的项目案例

## 许可证

该项目采用MIT许可证

## 声明

本项目的注释和描述文档大部分使用AI生成，也有部分自己撰写。
希望这个库能简化您的ESP32项目WiFi配置过程！如有任何问题或建议，欢迎在GitHub仓库中提出issue。