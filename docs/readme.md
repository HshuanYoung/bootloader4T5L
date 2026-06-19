# T5L51 SHJB BOOT底座

本工程是一个精简的Template4T5L风格BOOT底座，保留原SHJB BOOT的代码拷贝、
CRC校验和跳转行为，并集成UART5 OTA下载流程。启动控制字使用VP `0x0020`。

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

- 每次启动都会先开放500ms UART5 recovery窗口，再扫描 `0x0020`。
- recovery窗口结束后，BOOT会先从片内NOR Flash恢复 `0x0020` 到 `0x0025`，再进行升级或加载判断。
- recovery升级帧为 `5A A5 07 82 00 20 5A A5 5A A5`。
- recovery指定加载帧为 `5A A5 07 82 00 20 AA 55 xx xx`，`0xXXXX`为NOR起始块。
- `0x0020`字节为 `5A A5 5A A5` 时进入UART5升级模式。
- `0x0020`字节为 `AA 55 xx xx` 时加载NOR起始块 `0xXXXX`。
- `0x0020`为其他值时加载默认块 `112`。
- 进入升级模式后，BOOT会先发送握手 `AA BB 00 01 F3`，升级端收到后发送AB CD 04命令开始升级。
- `0x0022`为 `5A A5` 时启用升级切页，升级前切到 `0x0024`配置页，完成后切到 `0x0025`配置页。
- `0x0023`为升级进度显示地址，范围为0到100。
- UART5 OTA沿用现有AB CD 04/05/06协议样式，分包大小为4KB。
- 兼容的OTA头标识为 `0x43335AA5`。
- 精简底座不包含调试串口和printf。
- 启动文件保留原BOOT内存行为。
- boot交接函数通过Keil链接设置固定到 `?PR?BOOTLOADAPP?BOOT(0xFF70)`。
- 升级完成后，升级端应写入 `AA 55 xx xx` 指定程序起始块；若超时未指定，BOOT写入默认 `AA 55 00 70`。
- 升级完成后BOOT总会写 `0x0004 = 55 AA 5A A5` 触发软复位，复位后仍先进入recovery窗口。
