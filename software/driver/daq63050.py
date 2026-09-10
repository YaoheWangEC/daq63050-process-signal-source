import time

import serial
import serial.tools.list_ports

DEVICE_NAME = 'DAQ63050'         # device 响应中的设备名特征串
MODES = ('dc', 'square', 'sine')  # 合法输出模式 (mode 命令参数)
VREF_VOLT = 4.096                # 片外 DAC 基准电压 (V)
DAC_MIN, DAC_MAX = 0, 4095       # DAC 码值范围 (12bit)
LSB_VOLT = VREF_VOLT / 4096      # 1 码对应电压 (V), 即 1mV
VOUT_MIN, VOUT_MAX = DAC_MIN * LSB_VOLT, DAC_MAX * LSB_VOLT  # 输出量范围 (V): 0~4.095


class DAQ63050:
    """DAQ63050 过程信号源 Python 驱动。

    输出模式: dc (直流) / square (1kHz 方波) / sine (1kHz 正弦), 由 mode 命令切换。
    输出量: offset (直流偏置) 与 vpp (峰峰值), 单位 V, 范围 0~4.095 (1 码 = 1mV)。

    用法示例::

        from daq63050 import DAQ63050

        with DAQ63050('COM5') as dev:      # 也可 DAQ63050() 自动嗅探串口
            print(dev.get_device_name(), dev.get_uid())
            dev.set_mode('square')
            dev.set_offset(1.0)            # 1.0 V
            dev.set_vpp(1.0)               # 1.0 V

    说明: 串口对象打开后须显式 close() 释放 (或使用 with)。固件无 reboot 命令,
    故 close() 仅关闭串口, 不会复位设备。
    """

    # ---- 串口实例 (由 __init__ 打开; pyserial Serial 对象; 关闭不置空, 用 _closed 标记) ----
    ser: serial.Serial
    _closed = False

    # ---- 设备信息 (由 device 命令响应刷新) ----
    device_name = None   # 设备名, 如 'DAQ63050 process-signal-source'
    uid = None           # 24 位十六进制大写 UID; 同型号多台设备靠它区分

    # ---- 配置/状态参数 (status 命令回读缓存, 非权威; 由 get_*/set_*/get_status 刷新) ----
    mode = None          # 当前输出模式: 'dc' | 'square' | 'sine'
    offset = None        # 直流偏置 V (VOUT_MIN~VOUT_MAX)
    vpp = None           # 峰峰值 V (VOUT_MIN~VOUT_MAX)

    # ==============================
    # 生命周期
    # ==============================

    def __init__(self, port: str | None = None) -> None:
        """初始化设备对象并打开串口。

        Args:
            port (str | None): 串口名 (如 'COM5'); None 则自动嗅探。

        Raises:
            RuntimeError: 自动嗅探未找到 DAQ63050, 或指定串口上不是本设备。
            serial.SerialException: 显式指定的串口无法打开。
        """
        explicit = port is not None
        if port is None:
            port = self._probe_port()
            if port is None:
                raise RuntimeError('cannot find any daq63050 port')
        self.ser = serial.Serial(port, 115200, timeout=1)
        if explicit and not self._validate_device():
            self.ser.close()
            self._closed = True
            raise RuntimeError(f'not a DAQ63050 device on {port}')

    def close(self) -> None:
        """释放串口资源, 可重复调用 (第二次起为空操作)。"""
        if self._closed:
            return
        ser = getattr(self, 'ser', None)
        if ser is not None:
            try:
                ser.close()
            except Exception:
                pass
        self._closed = True

    def __enter__(self) -> 'DAQ63050':
        """支持 with 语句使用: 进入 with 块时返回实例自身。"""
        return self

    def __exit__(self, *exc) -> None:
        """支持 with 语句使用: 块结束 (含异常) 自动释放串口资源。"""
        self.close()

    def __del__(self) -> None:
        """析构兜底: 对象被回收时若串口仍打开则释放。

        仅作保险, 不保证执行时机 (交互环境/循环引用下可能很晚才跑);
        确定性释放请用 close() 或 with。异常一律吞掉。
        """
        try:
            self.close()
        except Exception:
            pass

    # ==============================
    # 设备信息
    # ==============================

    def get_device_name(self) -> str:
        """读取设备名, 并刷新设备名/UID 缓存。

        现发 device 命令 (多行响应), 解析出 Name/UID 字段后存入
        self.device_name / self.uid 并返回设备名。

        Returns:
            str: 设备名, 如 'DAQ63050 process-signal-source'。

        Raises:
            RuntimeError: 响应中不含 DEVICE_NAME 特征串 (非本设备)。
        """
        resp = self._send('device', resp_single_line=False)
        name_field, uid = self._parse_device_response(resp)
        if DEVICE_NAME not in name_field:
            raise RuntimeError(f'unexpected device response: {resp!r}')
        self.device_name = name_field
        self.uid = uid
        return self.device_name

    def get_uid(self) -> str:
        """读取设备 UID, 并刷新设备名/UID 缓存。

        现发 device 命令 (多行响应), 解析出 Name/UID 字段后存入
        self.device_name / self.uid 并返回 UID。

        Returns:
            str: 24 位十六进制大写 UID; 同型号多台设备靠它区分。

        Raises:
            RuntimeError: 响应中不含 DEVICE_NAME 特征串 (非本设备)。
        """
        resp = self._send('device', resp_single_line=False)
        name_field, uid = self._parse_device_response(resp)
        if DEVICE_NAME not in name_field:
            raise RuntimeError(f'unexpected device response: {resp!r}')
        self.device_name = name_field
        self.uid = uid
        return self.uid

    # ==============================
    # 状态查询
    # ==============================

    def get_status(self) -> dict:
        """读取配置与状态全量快照 (status 命令, 多行响应)。

        解析后刷新配置/状态缓存 (mode / offset / vpp)。

        Returns:
            dict: 键 tick (ms) / mode ('dc'|'square'|'sine') / offset (V) / vpp (V)。

        Raises:
            RuntimeError: 响应字段缺失或无法解析。
        """
        text = self._send('status', resp_single_line=False)
        kv = self._parse_kv_lines(text)
        try:
            tick = int(kv['Tick'].split()[0])
            mode = kv['Mode'].strip().lower()
            offset = self._code_to_volts(int(kv['Offset'].split()[0]))
            vpp = self._code_to_volts(int(kv['Vpp'].split()[0]))
        except (KeyError, ValueError, IndexError):
            raise RuntimeError(f'unexpected status response: {text!r}')
        if mode not in MODES:
            raise RuntimeError(f'unexpected status response: {text!r}')
        self.mode = mode
        self.offset = offset
        self.vpp = vpp
        return {'tick': tick, 'mode': mode, 'offset': offset, 'vpp': vpp}

    # ==============================
    # 配置
    # ==============================

    def get_mode(self) -> str:
        """查询当前输出模式 (mode 命令)。

        Returns:
            str: 'dc' | 'square' | 'sine'。

        Raises:
            RuntimeError: 响应不是已知模式。
        """
        resp = self._send('mode')
        mode = resp.strip().lower()
        if mode not in MODES:
            raise RuntimeError(f'unexpected mode response: {resp!r}')
        self.mode = mode
        return mode

    def set_mode(self, mode: str) -> str:
        """设置输出模式 (mode 命令)。

        Args:
            mode (str): 目标模式, 须属于 MODES。

        Returns:
            str: 设置后的输出模式 (设备回读)。

        Raises:
            ValueError: mode 不在 MODES。
            RuntimeError: 设备拒绝 (ERR: ...) 或响应无法解析。
        """
        if mode not in MODES:
            raise ValueError(f'invalid mode {mode!r}, legal: {MODES}')
        self._send(f'mode {mode}')
        return self.get_mode()

    def get_offset(self) -> float:
        """查询当前直流偏置 (offset 命令)。

        Returns:
            float: 直流偏置 V (VOUT_MIN~VOUT_MAX)。

        Raises:
            RuntimeError: 响应无法解析。
        """
        resp = self._send('offset')
        try:
            code = self._parse_int_value(resp)
        except ValueError:
            raise RuntimeError(f'unexpected offset response: {resp!r}')
        self.offset = self._code_to_volts(code)
        return self.offset

    def set_offset(self, volts: float) -> float:
        """设置直流偏置 (offset 命令)。

        Args:
            volts (float): 目标偏置 V, 须在 VOUT_MIN~VOUT_MAX。

        Returns:
            float: 设置后的直流偏置 V (设备回读)。

        Raises:
            ValueError: volts 超出 0~4.095。
            RuntimeError: 设备拒绝 (峰/谷越界等) 或响应无法解析。
        """
        if not VOUT_MIN <= volts <= VOUT_MAX:
            raise ValueError(f'offset {volts} V out of range [{VOUT_MIN}, {VOUT_MAX}]')
        code = self._volts_to_code(volts)
        self._send(f'offset {code}')
        return self.get_offset()

    def get_vpp(self) -> float:
        """查询当前峰峰值 (vpp 命令)。

        Returns:
            float: 峰峰值 V (VOUT_MIN~VOUT_MAX)。

        Raises:
            RuntimeError: 响应无法解析。
        """
        resp = self._send('vpp')
        try:
            code = self._parse_int_value(resp)
        except ValueError:
            raise RuntimeError(f'unexpected vpp response: {resp!r}')
        self.vpp = self._code_to_volts(code)
        return self.vpp

    def set_vpp(self, volts: float) -> float:
        """设置峰峰值 (vpp 命令)。

        Args:
            volts (float): 目标峰峰值 V, 须在 VOUT_MIN~VOUT_MAX。

        Returns:
            float: 设置后的峰峰值 V (设备回读)。

        Raises:
            ValueError: volts 超出 0~4.095。
            RuntimeError: 设备拒绝 (峰/谷越界等) 或响应无法解析。
        """
        if not VOUT_MIN <= volts <= VOUT_MAX:
            raise ValueError(f'vpp {volts} V out of range [{VOUT_MIN}, {VOUT_MAX}]')
        code = self._volts_to_code(volts)
        self._send(f'vpp {code}')
        return self.get_vpp()

    # ==============================
    # 私有实现
    # ==============================

    def _send(self, cmd: str, quiet: float = 0.05, timeout: float = 1.0,
              resp_single_line: bool = True, raise_on_error: bool = True) -> str:
        """发送一条命令并返回完整响应文本 (不含末尾 CRLF)。

        收帧: 写入后短轮询 in_waiting 收集数据。单行响应 (resp_single_line=True)
        见到尾 CRLF 立即收帧; 多行响应须等尾 CRLF 后再静默 quiet 秒, 以区分
        "第一行结束"与"整段结束"。超过 timeout 秒仍无完整响应则抛 TimeoutError。

        Args:
            cmd (str): 命令字符串 (不含 CR/LF, ASCII, <=63B)。
            quiet (float): 多行响应判定"整段结束"所需的静默窗 (秒)。
            timeout (float): 总超时 (秒)。
            resp_single_line (bool): 响应是否单行 (默认 True)。
            raise_on_error (bool): True 时命中设备错误回显抛 RuntimeError。

        Returns:
            str: 完整响应文本 (去末尾 CRLF, 保留内部换行)。

        Raises:
            TypeError: cmd 非 str。
            ValueError: cmd 含 CR/LF、非 ASCII 或超过 63B。
            RuntimeError: 设备已 close(), 或命中设备错误回显。
            TimeoutError: 超时无完整响应。
        """
        if self._closed:
            raise RuntimeError('device is closed')
        if not isinstance(cmd, str):
            raise TypeError('cmd must be str')
        if '\r' in cmd or '\n' in cmd:
            raise ValueError(f'cmd must not contain CR/LF: {cmd!r}')
        try:
            payload = cmd.encode('ascii')
        except UnicodeEncodeError:
            raise ValueError(f'cmd must be ascii: {cmd!r}')
        if len(payload) > 63:
            raise ValueError(f'cmd too long ({len(payload)}B > 63): {cmd!r}')

        self.ser.reset_input_buffer()   # 丢弃可能存在的残留字节
        self.ser.write(payload + b'\r\n')

        buf = bytearray()
        last_arrive = time.monotonic()
        t0 = last_arrive
        while True:
            n = self.ser.in_waiting
            if n:
                buf += self.ser.read(n)
                last_arrive = time.monotonic()
                if resp_single_line and buf.endswith(b'\r\n'):
                    break                   # 单行: 读完见尾 CRLF 立即收
            elif buf.endswith(b'\r\n') and (time.monotonic() - last_arrive) >= quiet:
                break                       # 多行: 尾 CRLF + 静默窗确认无后续
            if time.monotonic() - t0 >= timeout:
                raise TimeoutError(f'no response for cmd {cmd!r} (got {bytes(buf)!r})')
            time.sleep(0.001)

        text = bytes(buf).decode('ascii', errors='replace')
        if text.endswith('\r\n'):
            text = text[:-2]
        if raise_on_error and self._looks_like_device_error(text):
            raise RuntimeError(f'device rejected command {cmd!r}: {text}')
        return text

    def _validate_device(self) -> bool:
        """向已打开串口发送 device, 校验是否为本设备。

        命中 DEVICE_NAME 特征则记录设备名/UID 并返回 True, 否则 False。

        Returns:
            bool: True 表示本设备。
        """
        resp = self._probe_device(self.ser)
        name_field, uid = self._parse_device_response(resp)
        if DEVICE_NAME in name_field:
            self.device_name = name_field
            self.uid = uid
            return True
        return False

    def _probe_port(self) -> str | None:
        """扫描所有串口, 用 device 命令嗅探 DAQ63050 并返回其口名。

        逐个打开候选口, 命中 DEVICE_NAME 特征即返回该口名并记录设备名/UID。
        候选口无法打开或不应答则跳过。

        Returns:
            str | None: 命中的口名 (如 'COM5'); 未找到则 None。
        """
        for port_info in serial.tools.list_ports.comports():
            name = port_info.device
            try:
                tmp_ser = serial.Serial(name, 115200, timeout=0.2)
            except serial.SerialException:
                continue

            try:
                resp = self._probe_device(tmp_ser)
            finally:
                tmp_ser.close()

            name_field, uid = self._parse_device_response(resp)
            if DEVICE_NAME in name_field:
                self.device_name = name_field
                self.uid = uid
                return name

        return None

    @staticmethod
    def _probe_device(ser: serial.Serial) -> str:
        """向候选口连发两次 device, 返回第二次的响应文本。

        设备上电瞬间或链路建立时可能产生毛刺字节, 被主机误当作数据。
        第一次 device 用于对齐链路并清掉插入瞬间的杂波, 之后清空接收缓冲
        再发第二次, 取第二次响应作为可靠结果。临时把读超时压到 0.2s,
        避免在长超时串口上阻塞。

        Args:
            ser (serial.Serial): 已打开的候选串口。

        Returns:
            str: 第二次 device 的响应文本 (ASCII, 非法字节替换)。
        """
        prev_timeout = ser.timeout
        ser.timeout = 0.2
        try:
            ser.reset_input_buffer()
            ser.write(b'device\r\n')
            time.sleep(0.1)
            ser.reset_input_buffer()   # 丢弃第一次响应与上电毛刺
            ser.write(b'device\r\n')
            time.sleep(0.1)
            return ser.read(512).decode('ascii', errors='replace')
        finally:
            ser.timeout = prev_timeout

    @staticmethod
    def _volts_to_code(volts: float) -> int:
        """输出电压 V → DAC 码值 (四舍五入)。

        Args:
            volts (float): 输出电压 V。

        Returns:
            int: DAC 码值 (0~4095)。
        """
        return int(volts / LSB_VOLT + 0.5)

    @staticmethod
    def _code_to_volts(code: int) -> float:
        """DAC 码值 → 输出电压 V。

        Args:
            code (int): DAC 码值 (0~4095)。

        Returns:
            float: 输出电压 V。
        """
        return code * LSB_VOLT

    @staticmethod
    def _looks_like_device_error(text: str) -> bool:
        """判断响应是否为固件错误回显。

        命中条件 (均为固件自己输出的错误文本前缀):
          - 'ERR:'              越界/非法/忙/失败等
          - 'Unknown command:'  未注册命令
          - 'Invalid '          参数/选项错误
          - 'Usage:'            用法错误
        正常回显 (裸值/多行 key:value) 不会命中。

        Args:
            text (str): 一条命令的完整响应文本。

        Returns:
            bool: True 表示命中设备错误回显。
        """
        low = text.strip().lower()
        return (low.startswith('err:')
                or low.startswith('unknown command:')
                or low.startswith('invalid ')
                or low.startswith('usage:'))

    @staticmethod
    def _parse_int_value(text: str) -> int:
        """解析单值响应为整数 (兼容 'key : value' 与裸 'value')。

        Args:
            text (str): 单行响应文本。

        Returns:
            int: 解析出的整数。

        Raises:
            ValueError: 无法解析为整数。
        """
        if ':' in text:
            text = text.split(':', 1)[1]
        return int(text.strip())

    @staticmethod
    def _parse_kv_lines(text: str) -> dict[str, str]:
        """把多行 'key : value' 响应解析为 dict (键/值去空白)。

        Args:
            text (str): 多行响应文本 (如 status 全量)。

        Returns:
            dict[str, str]: 键到值的映射; 无冒号的行忽略。
        """
        kv: dict[str, str] = {}
        for line in text.split('\r\n'):
            if ':' in line:
                key, val = line.split(':', 1)
                kv[key.strip()] = val.strip()
        return kv

    @staticmethod
    def _parse_device_response(resp_text: str) -> tuple[str, str]:
        """解析 device 响应文本, 返回 (设备名, UID)。

        Args:
            resp_text (str): device 命令响应文本。

        Returns:
            tuple[str, str]: (设备名, UID); 未找到对应行时为空串。
        """
        name_field = ''
        uid = ''
        for line in resp_text.split('\r\n'):
            line = line.strip()
            if line.lower().startswith('name'):
                name_field = line.split(':', 1)[1].strip()
            elif line.upper().startswith('UID'):
                uid = line.split(':', 1)[1].strip()
        return name_field, uid


if __name__ == '__main__':
    """冒烟自检: 打开设备, 覆盖设备信息/状态/配置读写与 _send 收帧/错误判定。

    直接运行: python daq63050.py
    自检结束会尽力恢复进入前的模式/偏置/峰峰值。
    """
    def approx(a: float, b: float) -> bool:
        """浮点近似相等 (电压经 V↔码值往返换算, 留 1e-9 容差)。"""
        return abs(a - b) < 1e-9

    with DAQ63050() as dev:
        # 设备信息 (device 命令, 多行响应)
        name = dev.get_device_name()
        uid = dev.get_uid()
        print(f'device : {name}')
        print(f'uid    : {uid}')
        assert DEVICE_NAME in name, name
        assert len(uid) == 24, uid

        # 状态全量 (status 命令, 多行响应)
        st = dev.get_status()
        print(f'status : {st}')
        assert st['mode'] in MODES, st
        assert VOUT_MIN <= st['offset'] <= VOUT_MAX, st
        assert VOUT_MIN <= st['vpp'] <= VOUT_MAX, st
        assert dev.mode == st['mode']
        assert approx(dev.offset, st['offset'])
        assert approx(dev.vpp, st['vpp'])

        orig = (st['mode'], st['offset'], st['vpp'])
        try:
            # 配置读写
            assert dev.set_mode('square') == 'square'
            assert dev.get_mode() == 'square'
            assert approx(dev.set_offset(1.0), 1.0)
            assert approx(dev.get_offset(), 1.0)
            assert approx(dev.set_vpp(1.0), 1.0)
            assert approx(dev.get_vpp(), 1.0)
            st2 = dev.get_status()
            print(f'square : {st2}')
            assert st2['mode'] == 'square', st2
            assert approx(st2['offset'], 1.0), st2
            assert approx(st2['vpp'], 1.0), st2

            # 本地校验: 越界/非法模式应抛 ValueError
            for bad in (-0.001, 4.096):
                try:
                    dev.set_offset(bad)
                except ValueError:
                    pass
                else:
                    raise AssertionError(f'offset {bad} V should raise ValueError')
            try:
                dev.set_mode('tri')
            except ValueError:
                pass
            else:
                raise AssertionError('mode tri should raise ValueError')

            # 设备拒绝: offset + vpp/2 超 4.095V 应抛 RuntimeError
            try:
                dev.set_offset(4.0)
            except RuntimeError:
                pass
            else:
                raise AssertionError('offset 4.0 V with vpp 1.0 V should raise RuntimeError')
        finally:
            dev.set_mode(orig[0])
            try:
                if orig[0] == 'dc':
                    dev.set_offset(orig[1])
                else:
                    dev.set_offset(2.048)     # 居中, 允许任意 vpp
                    dev.set_vpp(orig[2])
                    dev.set_offset(orig[1])
            except RuntimeError:
                pass

        # _send 收帧: echo 单行响应
        assert dev._send('echo hello') == 'hello'

        # _send 错误判定: 未知命令应抛 RuntimeError
        try:
            dev._send('nosuchcmd')
        except RuntimeError:
            pass
        else:
            raise AssertionError('unknown command should raise RuntimeError')

        # _send 参数校验: 含 CR/LF 应抛 ValueError
        try:
            dev._send('echo a\rb')
        except ValueError:
            pass
        else:
            raise AssertionError('CRLF command should raise ValueError')

    print('SELF-TEST OK')
