# DAQ63050 — 过程信号源

> Process Signal Source

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## 简介 | Introduction

DAQ63050 是一款基于 **AT32L021** 的过程信号源，配合 12-bit DAC TPC112S1 和高精度电压基准 TPR3540 实现了一个可靠的信号源，支持直流（DC）、1 kHz 方波、1 kHz 正弦三种输出模式，可通过 [Linux CLI 风味命令](https://github.com/YaoheWangEC/linux-cli-style-command-driver-framework) 进行控制，也可通过板载按键本地操作。

DAQ63050 is a process signal source based on the **AT32L021** MCU, paired with a 12-bit DAC (TPC112S1) and a high-precision voltage reference (TPR3540). It supports three output modes — DC, 1 kHz square wave, and 1 kHz sine wave — and can be controlled via [Linux CLI-style commands](https://github.com/YaoheWangEC/linux-cli-style-command-driver-framework) or locally through on-board keys.

![DAQ63050](resource/overview.jpg)

> 视频演示：*待补充* ｜ 立创开源：*待补充*

## 主要参数 | Specifications

| 参数 | 值 | 备注 |
| ------ | ------------- | ----------------------- |
| 供电 | 5 V 100 mA | 隔离供电 |
| 输出范围 | 0.1 ~ 4.0 V | 输出阻抗 100 Ω |
| 输出分辨率 | 典型 1 mV | VREF = 4.096 V @ 12 bit |
| 输出模式 | DC / 1 kHz 方波 / 1 kHz 正弦 | 采样率 50 kHz |
| 通信接口 | UART（CH340） | 115200 / 8N1 |

---

## 快速上手 | Getting Started

### 1. 接线 | Wiring

| 接口名 | 接口形态 | 功能 | 备注 |
| --- | --- | --- | --- |
| USB 接口 | USB Type-B | 供电 + 通信 | CH340, 115200 8N1 |
| 信号输出接口 | 弹簧接线端子 | 波形输出 | 0.1 ~ 4.0 V |

### 2. 上电 | Power On

接通电源后，OLED 显示 `Hello world!` / `DAQ63050`，随后进入输出界面。默认输出直流 2.495 V。

### 3. 输出连接 | Output Connection

将信号输出端接入负载或被测设备。输出为 0.1 ~ 4.0 V 单极性信号，请勿超出量程。

DC 模式下，固件通过 ADC 回采实际输出并与设定值比较；当偏差超过 ±30 mV 时（输出电流超过 3 mA），OLED 显示 `OVERLOAD`，提示输出因负载过重发生跌落。

### 4. 本地操作 | Local Control

| 按键 | 短按 | 长按 |
| --- | --- | --- |
| FN | 切换"选择项 / 修改值"状态 | 切换输出模式（DC → 方波 → 正弦 → DC） |
| INC | 非修改态：切换选项；修改态：+1 | 修改态：+10 |
| DEC | 非修改态：切换选项；修改态：-1 | 修改态：-10 |

可调选项为 **直流偏置（DC）** 与 **峰峰值（VPP）**，修改值同时受峰谷约束（见下）。

### 5. UART 控制 | UART Control

使用 USB Type-B 数据线连接电脑：

```
PC (USB) -> USB Type-B 线 -> DAQ63050 (CH340 -> UART)
```

串口参数：**115200 bps, 8 data bits, 1 stop bit, no parity**

上电后发送 `lscmd` 可列出所有命令。DAQ63050 采用 [Linux CLI 风格](https://github.com/YaoheWangEC/linux-cli-style-command-driver-framework) 的 ASCII 文本协议，命令以 `\r\n` 结束，响应同样以 `\r\n` 结束，一问一答。

> `offset` 与 `vpp` 命令的数值单位为 mV，与 DAC 码值一一对应（1 LSB = 1 mV），范围 `0 ~ 4095`。

#### 命令参考 | Command Reference

**`lscmd`** — 列出所有已注册命令

| 参数 | 说明 |
| --- | --- |
| `-h`, `--help` | 显示帮助 |

```
> lscmd
Supported commands:
  lscmd
  echo
  device
  mode
  offset
  vpp
  status
```

**`echo`** — 回显参数，用于测试通信链路

| 参数 | 说明 |
| --- | --- |
| `<text>` | 任意文本，多参数以空格拼接 |

```
> echo hello world
hello world
```

**`device`** — 读取设备名称与 MCU 96-bit 唯一 ID

无参数。

```
> device
Name  : DAQ63050 process-signal-source
UID   : 12345678ABCDEF0012345678
```

**`mode`** — 查询或设置输出模式

| 参数 | 说明 |
| --- | --- |
| *(无)* | 查询当前模式，返回 `DC` / `SQUARE` / `SINE` |
| `dc` | 直流输出模式 |
| `square` | 1 kHz 方波模式 |
| `sine` | 1 kHz 正弦模式 |

```
> mode
DC
> mode square
OK
> mode
SQUARE
```

**`offset`** — 查询或设置直流偏置（单位 mV）

| 参数 | 说明 |
| --- | --- |
| *(无)* | 查询当前偏置，返回原始数值（mV） |
| `<value>` | 设定偏置，范围 `0 ~ 4095` |

```
> offset
2495
> offset 1000
OK
> offset 4095
ERR: peak(4595) or valley(3595) exceeds range [0, 4095]
```

**`vpp`** — 查询或设置峰峰值（单位 mV）

| 参数 | 说明 |
| --- | --- |
| *(无)* | 查询当前峰峰值，返回原始数值（mV） |
| `<value>` | 设定峰峰值，范围 `0 ~ 4095` |

```
> vpp
1000
> vpp 500
OK
```

**`status`** — 读取当前设备状态

无参数。返回运行时间、模式、偏置与峰峰值。

```
> status
Tick  : 12345 ms
Mode  : SQUARE
Offset: 1000 mV
Vpp   : 1000 mV
```

> 切换模式会重置输出量为模式默认值：`dc` → offset=2495 / vpp=0；`square` / `sine` → offset=1000 / vpp=1000。
> `offset ± vpp/2` 必须落在 `0 ~ 4095`，越界时设备返回 `ERR:`。
> **DC 模式下 vpp 恒为 0**（固件在 50 kHz 更新中断中强制清零），故 DC 模式下设置 `vpp` 无效。

---

## Python 驱动 | Python Driver

`software/driver/` 下提供纯驱动库 `daq63050.py`，封装上述 UART 命令协议。

### 依赖 | Requirements

- Python 3.10+
- `pyserial`

```
pip install pyserial
```

### 快速开始 | Quick Start

```python
from daq63050 import DAQ63050

with DAQ63050() as dev:                  # 自动嗅探串口（也可 DAQ63050('COM3')）
    print(dev.get_device_name(), dev.get_uid())
    print(dev.get_status())

    dev.set_mode('square')               # 'dc' | 'square' | 'sine'
    dev.set_offset(1.0)                  # 直流偏置 V (0~4.095)
    dev.set_vpp(1.0)                     # 峰峰值 V (0~4.095)
```

### API 速查 | API Overview

| 区域 | 方法 |
| --- | --- |
| 生命周期 | `__init__(port=None)` / `close()` / `with` |
| 设备信息 | `get_device_name()` / `get_uid()` |
| 状态查询 | `get_status()` |
| 输出模式 | `get_mode()` / `set_mode(mode)` |
| 直流偏置 | `get_offset()` / `set_offset(volts)` |
| 峰峰值 | `get_vpp()` / `set_vpp(volts)` |

> 驱动对外以 **V** 为单位收发，内部换算为 DAC 码值（1 码 = 1 mV）。详细说明见 [`software/driver/README.md`](software/driver/README.md)。

---

## 许可证 | License

本项目由不同许可的组件构成：

- **应用代码**（`software/process-signal-source-l021k8u7-4/project/MDK_V5/process-signal-source/` 下用户编写的业务逻辑、命令框架、OLED / 按键驱动等，以及 `software/driver/` 的 Python 驱动）以 [MIT License](LICENSE) 发布，版权归 [YaoheWangEC](https://github.com/YaoheWangEC)。
- **Artery BSP**（`libraries/drivers/`、`project/src/` 中 `wk_*.c`、`main.c` 模板等）版权归 Artery Technology，允许在与 Artery MCU 配合使用的场合复制和分发。
- **CMSIS-Core** 版权归 Arm Limited，遵循 Apache License 2.0。

This project consists of components under different licenses:

- **Application code** (user-written logic, command framework, OLED / key drivers under `software/process-signal-source-l021k8u7-4/project/MDK_V5/process-signal-source/`, and the Python driver under `software/driver/`) is released under the [MIT License](LICENSE), copyright [YaoheWangEC](https://github.com/YaoheWangEC).
- **Artery BSP** (`libraries/drivers/`, `project/src/` `wk_*.c`, `main.c` template, etc.) is copyright Artery Technology, with permission to copy and distribute for use with Artery MCUs.
- **CMSIS-Core** is copyright Arm Limited under Apache License 2.0.

## 资助致谢 | Sponsorship Acknowledgment

感谢梁文锋叔叔的百亿 Token 补贴政策，使本项目的 AI 辅助开发成本趋近于零。

## 致谢 | Acknowledgements

本项目由 [YaoheWangEC](https://github.com/YaoheWangEC) 设计开发。在项目开发与开源文档整理过程中，DeepSeek V4提供了大量的辅助，包括代码编写、设计建议和文档组织。

得益于DeepSeek V4 Pro与Opencode，本项目的MCU软件开发仅消耗15工时，消耗Token合计99,369,930。

## 免责声明 | Disclaimer

本项目由 DeepSeek 辅助编写，可能存在纰漏。本项目仅供学习研究，未经工业场景充分验证。使用者应自行评估安全风险，作者与 DeepSeek 不对因使用本项目及其文档所造成的任何损失承担责任。

This documentation was drafted with the assistance of DeepSeek and may contain errors. This project is provided for educational and research purposes only and has not been fully validated for industrial environments. Users should evaluate safety risks on their own. The author and DeepSeek assume no liability for any damages arising from the use of this project or its documentation.

---
