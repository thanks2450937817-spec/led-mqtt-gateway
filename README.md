# 基于ESP32-S3的物联网网关（MQTT + OTA）

## 项目简介
一个具备远程状态监控与固件自更新能力的工业物联网网关原型。
- 硬件：ESP32-S3
- 系统：FreeRTOS 多任务架构
- 通信：MQTT 双向数据交互
- 升级：A/B 分区安全 OTA（防变砖）

## 快速开始
1. 安装 ESP-IDF v5.3
2. 克隆本项目
3. 进入 menuconfig 配置 WiFi SSID 和密码
4. 编译烧录：`idf.py build flash monitor`
5. 使用 MQTTX 连接 broker.emqx.io，订阅 `/led/status` 查看设备状态

## OTA 升级测试
1. 启动本地 HTTP 服务器：`python -m http.server 8070`
2. 通过 MQTTX 向 `/led/ota` 发送固件 URL
3. 设备自动下载、校验、重启

## 核心设计亮点
- 三任务协同架构（主控、MQTT后台、OTA事务），通过队列和事件解耦
- 事务性OTA写入（begin→write→end），任意步骤断电自动回滚
- 指数退避重连机制，弱网环境自动恢复
- 防御性编程，goto cleanup 统一资源管理

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
