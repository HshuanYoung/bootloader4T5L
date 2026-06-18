# T5L51 SHJB BOOT底座

本工程是一个精简的Template4T5L风格BOOT底座，保留原SHJB BOOT的代码拷贝、
CRC校验和跳转行为，并集成UART5 OTA下载流程。启动控制字使用VP `0x00DD`。

## 目录结构

```text
bootloader4T5L/
├── docs/       工程说明
├── include/    平台配置和系统服务代码
├── modules/    BOOT与OTA模块
├── project/    Keil uVision工程和启动文件
├── source/     UART5与Timer0支持
├── tools/      构建转换工具
└── user/       启动加载器入口
```

## 构建

使用Keil uVision打开 `project/T5L51.uvproj`，编译 `Target 1`。编译成功后，
`project/boot.bat` 会将 `project/Objects/T5L51.hex` 转换为工程根目录下的
`T5L51_SHJ_BOOT.bin`。

## 说明

- `0x00DD`字节为 `5A A5 5A A5` 时进入UART5升级模式。
- `0x00DD`字节为 `AA 55 xx xx` 时加载NOR起始块 `0xXXXX`。
- `0x00DD`为其他值时加载默认块 `112`。
- UART5 OTA沿用现有AB CD 04/05/06协议样式，分包大小为4KB。
- 精简底座不包含调试串口和printf。
- 启动文件保留原BOOT内存行为。
- boot交接函数通过Keil链接设置固定到 `?PR?BOOTLOADAPP?BOOT(0xFF70)`。
- BOOT底座不主动发送F3；升级端先写入 `5A A5 5A A5`，再发送UART5升级包。
