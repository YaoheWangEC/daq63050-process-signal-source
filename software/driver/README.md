# DAQ63050 Python 驱动

DAQ63050 过程信号源（AT32L021K8U7 + TPC112S1 12bit DAC，UART 命令面）的 Python 上位机驱动。

## 文件

| 文件 | 说明 |
|---|---|
| `daq63050.py` | 驱动主体（`DAQ63050` 类） + `__main__` 冒烟自检 |

## 依赖

- Python 3.10+（使用 `X | None` 类型标注）
- `pyserial`

```
pip install pyserial
```

## 快速开始

```python
from daq63050 import DAQ63050

with DAQ63050() as dev:                  # 自动嗅探串口（也可 DAQ63050('COM3')）
    print(dev.get_device_name(), dev.get_uid())
    print(dev.get_status())

    dev.set_mode('square')               # 'dc' | 'square' | 'sine'
    dev.set_offset(1.0)                  # 直流偏置 V (0~4.095)
    dev.set_vpp(1.0)                     # 峰峰值 V (0~4.095)

# with 退出时 close(): 仅关串口（固件无 reboot, 不复位设备）
```

## API 速查

| 区域 | 方法 |
|---|---|
| 生命周期 | `__init__(port=None)` / `close()` / `with` |
| 设备信息 | `get_device_name()` / `get_uid()` |
| 状态查询 | `get_status()` → dict(tick / mode / offset / vpp) |
| 输出模式 | `get_mode()` / `set_mode(mode)` |
| 直流偏置 | `get_offset()` / `set_offset(volts)` |
| 峰峰值 | `get_vpp()` / `set_vpp(volts)` |

约定：`offset` / `vpp` 单位 **V**，范围 `VOUT_MIN`~`VOUT_MAX`（0~4.095V；1 码 = 1mV = `LSB_VOLT`）。驱动对外收 / 发 V，内部换算为 DAC 码值下发。

## 输出模式与固件行为

- 模式：`dc`（直流）/ `square`（1kHz 方波）/ `sine`（1kHz 正弦）。
- 切换模式会重置输出量为模式默认值：`dc` → offset=2495 / vpp=0；`square` / `sine` → offset=1000 / vpp=1000。
- `set_offset` / `set_vpp` 受峰谷约束：`offset ± vpp/2` 必须落在 0~4.095V；越界时设备回 `ERR: ...`，驱动抛 `RuntimeError`。
- **DC 模式下 vpp 恒为 0**：固件在 50kHz 更新中断中强制清零，故 `get_vpp()` 恒返回 0，`set_vpp()` 无效。
- 固件无 `reboot` 命令，`close()` 仅关闭串口、不复位设备。
- OLED 每 100ms 刷新一次；快速连发命令时屏上变化可能一闪而过或不可见（不影响实际输出）。

## 命令协议速查

命令以 `\r\n` 结尾，一问一答，ASCII；单条命令 ≤63 字节。错误响应以 `ERR:` / `Unknown command:` / `Invalid ` / `Usage:` 开头（驱动统一转 `RuntimeError`）。

| 命令 | 说明 | 响应 |
|---|---|---|
| `device` | 设备名 + UID | 多行 `Name:` / `UID:` |
| `status` | 配置/状态全量 | 多行 `Tick` / `Mode` / `Offset` / `Vpp` |
| `mode` / `mode <dc\|square\|sine>` | 查询 / 设置输出模式 | `DC\|SQUARE\|SINE` / `OK` |
| `offset` / `offset <n>` | 查询 / 设置直流偏置（原始单位 mV，即码值） | 数值 / `OK` |
| `vpp` / `vpp <n>` | 查询 / 设置峰峰值（原始单位 mV，即码值） | 数值 / `OK` |
| `lscmd` | 列出已注册命令 | 多行 |
| `echo <text>` | 回显 | 文本 |

> 驱动收帧：单行响应见到尾 `\r\n` 即收；多行（`device` / `status`）在尾 `\r\n` 后再静默 50ms 确认整段结束。

## 自动嗅探

`DAQ63050()` 无参构造时遍历所有串口，向每个候选口**连发两次 `device`**（第一次对齐链路、清掉上电毛刺，取第二次响应判定），命中设备名特征串 `DAQ63050` 即返回该口。

## 自检

```
python daq63050.py
```

覆盖设备信息、状态、模式 / 偏置 / 峰峰值读写、越界与设备拒绝的错误路径、`_send` 收帧与错误判定；结束时尽力恢复进入前的设置。需连接硬件。
