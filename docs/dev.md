可以，你这个方案很合适：**Python 负责 EtherCAT 文件解析/转换规则，Qt5 负责界面和流程引导**。不要一开始做得太复杂，先做成一个能稳定把 `ESI.XML + SDO.XML + digital_io.xlsx` 转成 **SSC Tool 可导入 `.xlsx`** 的工具。

我建议软件名字可以叫：

```text
NekoECAT Converter
```

定位：

```text
ESI/SDO → SSC Tool Application Description xlsx
ESI/SDO → TwinCAT ESI
ESI/SDO → PDO/SM/Callback 检查报告
```

---

# 1. 总体开发路线

你的软件分成两层：

```text
┌──────────────────────────────┐
│ Qt5 GUI                       │
│ 文件选择 / 参数配置 / 报告显示 │
└──────────────┬───────────────┘
               ↓
┌──────────────────────────────┐
│ Python Converter Core         │
│ ESI解析 / SDO解析 / 规则修复   │
│ xlsx生成 / ESI生成 / 报告生成  │
└──────────────────────────────┘
```

核心原则：

```text
GUI 不写转换逻辑
转换逻辑全部放 core
GUI 只调用 core
```

这样以后你可以同时支持：

```text
命令行模式
Qt5 图形界面模式
批量转换模式
```

---

# 2. 技术栈

## Python 转换核心

推荐：

```text
Python 3.10 或 3.11
lxml              解析 XML
openpyxl          读写 xlsx
pydantic          数据模型和校验
jinja2            生成报告/代码模板
typer             命令行 CLI
rich              命令行彩色日志
```

## Qt5 界面

两种选择：

```text
PyQt5
PySide2
```

我建议用 **PyQt5**，资料更多，先开发快。

```bash
pip install PyQt5 lxml openpyxl pydantic typer rich jinja2
```

后期打包：

```bash
pip install pyinstaller
```

---

# 3. 项目目录设计

建议项目结构这样：

```text
nekoecat_converter/
├── main.py                         # GUI 启动入口
├── cli.py                          # 命令行入口
├── requirements.txt
├── README.md
│
├── nekoecat/
│   ├── __init__.py
│   │
│   ├── model/
│   │   ├── device.py               # EtherCAT 统一设备模型
│   │   ├── object_dict.py          # Object / SubIndex 数据模型
│   │   ├── pdo.py                  # PDO / SM 数据模型
│   │   └── rules.py                # Warning/Error 类型
│   │
│   ├── parser/
│   │   ├── esi_parser.py           # 解析 ESI XML
│   │   ├── sdo_parser.py           # 解析 SDO XML
│   │   └── template_parser.py      # 读取 digital_io.xlsx 模板
│   │
│   ├── engine/
│   │   ├── merger.py               # 合并 ESI + SDO
│   │   ├── classifier.py           # 对象分类
│   │   ├── rule_engine.py          # 规则检查
│   │   ├── fix_engine.py           # 自动修复
│   │   └── validator.py            # 最终校验
│   │
│   ├── generator/
│   │   ├── ssc_xlsx_generator.py   # 生成 SSC Tool xlsx
│   │   ├── esi_generator.py        # 生成 TwinCAT ESI
│   │   ├── report_generator.py     # 生成 Markdown/HTML 报告
│   │   ├── pdo_csv_generator.py    # 生成 PDO 布局表
│   │   └── code_generator.py       # 可选，生成 C 代码骨架
│   │
│   ├── log/
│   │   └── ssc_log_parser.py       # 解析 SSC Tool 导入报错
│   │
│   └── core.py                     # 对外统一转换接口
│
├── gui/
│   ├── main_window.py
│   ├── widgets/
│   │   ├── file_select_page.py
│   │   ├── identity_page.py
│   │   ├── rule_page.py
│   │   ├── object_table_page.py
│   │   ├── pdo_page.py
│   │   ├── report_page.py
│   │   └── generate_page.py
│   └── resources/
│
├── templates/
│   ├── report.md.j2
│   ├── pdo_structs.h.j2
│   └── coe_callbacks.c.j2
│
├── examples/
│   ├── D507/
│   │   ├── ESI.XML
│   │   ├── SDO.XML
│   │   └── digital_io.xlsx
│
└── tests/
    ├── test_esi_parser.py
    ├── test_sdo_parser.py
    ├── test_rule_engine.py
    ├── test_xlsx_generator.py
    └── test_d507.py
```

---

# 4. 软件工作流程

用户使用流程应该是：

```text
1. 选择 ESI.XML
2. 选择 SDO.XML
3. 选择 SSC Tool 模板 digital_io.xlsx
4. 设置新设备身份
5. 选择转换策略
6. 软件解析并显示对象字典
7. 软件自动检查问题
8. 软件自动修复
9. 生成 SSC Tool xlsx
10. 用户拿 xlsx 导入 SSC Tool
11. 如果 SSC Tool 报错，把日志贴回软件自动修复
```

界面流程：

```text
导入文件
  ↓
设备身份配置
  ↓
规则策略配置
  ↓
对象字典预览
  ↓
PDO/SM 校验
  ↓
问题报告
  ↓
导出文件
```

---

# 5. 核心数据模型

第一步一定要建统一模型，不要直接 XML 转 xlsx。

## DeviceModel

```python
from dataclasses import dataclass, field
from typing import Dict, List, Optional

@dataclass
class DeviceIdentity:
    vendor_id: int
    product_code: int
    revision: int
    serial: int = 0
    name: str = "HPM_ECAT_Device"
    physics: str = "YY"


@dataclass
class EthercatDevice:
    identity: DeviceIdentity
    objects: Dict[int, "ObjectDef"] = field(default_factory=dict)
    sync_managers: List["SyncManager"] = field(default_factory=list)
    rx_pdos: List["PdoDef"] = field(default_factory=list)
    tx_pdos: List["PdoDef"] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)
```

## ObjectDef

```python
@dataclass
class ObjectDef:
    index: int
    name: str
    object_code: str = "RECORD"
    entries: List["SubEntry"] = field(default_factory=list)

    is_communication_object: bool = False
    is_pdo_mapping_object: bool = False
    is_sm_object: bool = False

    need_coe_read_callback: bool = False
    need_coe_write_callback: bool = False

    strategy: str = "AUTO_STORAGE"
    risk_flags: List[str] = field(default_factory=list)
```

## SubEntry

```python
@dataclass
class SubEntry:
    subindex: int
    name: str
    data_type: str
    bit_len: int
    access: str
    default_value: str = ""
    min_value: str = ""
    max_value: str = ""
    pdo_direction: str = ""  # "", "rx", "tx"
    semantic_type: str = ""  # "", "REAL32", "OCTET_STRING"
```

## PDO

```python
@dataclass
class PdoEntry:
    index: int
    subindex: int
    bit_len: int
    name: str
    data_type: str
    bit_offset: int = 0


@dataclass
class PdoDef:
    index: int
    name: str
    direction: str  # "rx" or "tx"
    sm: int
    entries: List[PdoEntry] = field(default_factory=list)
```

---

# 6. ESI 解析模块

`esi_parser.py` 负责解析：

```text
Vendor ID
Product Code
Revision
Device Name
SM0/SM1/SM2/SM3
RxPDO
TxPDO
PDO Entry
DataType
BitLen
```

伪代码：

```python
from lxml import etree

def parse_esi(path: str) -> EthercatDevice:
    tree = etree.parse(path)
    root = tree.getroot()

    vendor_id = parse_int(root.findtext(".//Vendor/Id", default="0"))

    device_node = root.find(".//Descriptions/Devices/Device")
    type_node = device_node.find("Type")

    product_code = parse_hex_or_int(type_node.get("ProductCode", "0"))
    revision = parse_hex_or_int(type_node.get("RevisionNo", "0"))
    name = device_node.findtext("Name", default=type_node.text or "Unknown")

    device = EthercatDevice(
        identity=DeviceIdentity(
            vendor_id=vendor_id,
            product_code=product_code,
            revision=revision,
            name=name,
        )
    )

    parse_sms(device_node, device)
    parse_pdos(device_node, device)

    return device
```

数字解析要支持：

```text
162
#x00000197
0x00000197
00000197
```

```python
def parse_hex_or_int(value: str) -> int:
    value = str(value).strip()
    if value.startswith("#x"):
        return int(value[2:], 16)
    if value.startswith("0x"):
        return int(value[2:], 16)
    return int(value)
```

---

# 7. SDO 解析模块

你的 SDO 文件格式像这样：

```text
SDO 0x7009, "Actuator"
0x7009:00, r-r-r-, uint8, 8 bit, "SubIndex 000"
0x7009:01, r-r-rw, uint8, 8 bit, "Actuator Control"
0x7009:02, r-r-rw, float, 32 bit, "Actuator Position SP [REAL]"
```

所以 SDO parser 不一定是标准 XML parser，而是要兼容 TwinCAT 导出的文本/XML混合格式。

建议写成两层：

```text
如果是标准 XML → XML Parser
如果是文本式 SDO dump → Regex Parser
```

核心 regex：

```python
import re

RE_OBJECT = re.compile(r'SDO\s+0x([0-9a-fA-F]+),\s+"([^"]*)"')
RE_ENTRY = re.compile(
    r'0x([0-9a-fA-F]+):([0-9a-fA-F]+),\s*([^,]+),\s*([^,]+),\s*(\d+)\s*bit,\s*"([^"]*)"'
)
```

解析函数：

```python
def parse_sdo_text(path: str) -> dict[int, ObjectDef]:
    objects = {}
    current = None

    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()

            m_obj = RE_OBJECT.match(line)
            if m_obj:
                index = int(m_obj.group(1), 16)
                name = m_obj.group(2)
                current = ObjectDef(index=index, name=name)
                objects[index] = current
                continue

            m_ent = RE_ENTRY.match(line)
            if m_ent:
                index = int(m_ent.group(1), 16)
                subindex = int(m_ent.group(2), 16)
                access = m_ent.group(3).strip()
                dtype = normalize_sdo_type(m_ent.group(4).strip())
                bit_len = int(m_ent.group(5))
                name = m_ent.group(6)

                obj = objects.get(index)
                if obj is None:
                    obj = ObjectDef(index=index, name=f"Object 0x{index:04X}")
                    objects[index] = obj

                obj.entries.append(SubEntry(
                    subindex=subindex,
                    name=name,
                    data_type=dtype,
                    bit_len=bit_len,
                    access=normalize_access(access),
                ))

    return objects
```

类型归一化：

```python
def normalize_sdo_type(t: str) -> str:
    t = t.lower().strip()

    mapping = {
        "bool": "BOOL",
        "uint8": "USINT",
        "uint16": "UINT",
        "uint32": "UDINT",
        "uint64": "ULINT",
        "int8": "SINT",
        "int16": "INT",
        "int32": "DINT",
        "float": "REAL",
        "string": "STRING",
    }

    if t == "octet_string":
        return "OCTET_STRING"

    return mapping.get(t, t.upper())
```

---

# 8. 合并 ESI + SDO

合并逻辑：

```text
ESI 提供 PDO/SM 权威信息
SDO 提供对象字典完整信息
```

流程：

```python
def merge_esi_sdo(esi_device: EthercatDevice, sdo_objects: dict[int, ObjectDef]) -> EthercatDevice:
    device = esi_device

    # 先加入 SDO 对象
    for index, obj in sdo_objects.items():
        device.objects[index] = obj

    # 再用 ESI 的 PDO 信息给对象 entry 标记 rx/tx
    for pdo in device.rx_pdos:
        for e in pdo.entries:
            mark_pdo_entry(device, e.index, e.subindex, "rx")

    for pdo in device.tx_pdos:
        for e in pdo.entries:
            mark_pdo_entry(device, e.index, e.subindex, "tx")

    return device
```

```python
def mark_pdo_entry(device, index, subindex, direction):
    obj = device.objects.get(index)
    if not obj:
        obj = ObjectDef(index=index, name=f"PDO Object 0x{index:04X}")
        device.objects[index] = obj

    for entry in obj.entries:
        if entry.subindex == subindex:
            entry.pdo_direction = direction
            return
```

---

# 9. 对象分类规则

`classifier.py`：

```python
def classify_object(obj: ObjectDef):
    idx = obj.index

    if 0x1000 <= idx <= 0x1FFF:
        obj.is_communication_object = True

    if 0x1600 <= idx <= 0x17FF or 0x1A00 <= idx <= 0x1BFF:
        obj.is_pdo_mapping_object = True

    if idx in (0x1C00, 0x1C12, 0x1C13, 0x1C32, 0x1C33):
        obj.is_sm_object = True

    if 0xF000 <= idx <= 0xFFFF:
        obj.risk_flags.append("HIGH_PROFILE_OR_VENDOR_OBJECT")
```

Application Description 里不要盲目导入：

```text
0x1000
0x1008
0x1009
0x100A
0x1018
0x1C00
0x1C12
0x1C13
0x1C32
0x1C33
```

这些应该由 SSC Tool 配置或自动生成。

---

# 10. 规则引擎

这是你的核心竞争力。

## 10.1 BOOL 默认值修复

```python
def fix_bool_default(entry: SubEntry):
    if entry.data_type != "BOOL":
        return

    v = str(entry.default_value).strip().lower()
    if v in ("", "0", "false", "none"):
        entry.default_value = "FALSE"
    elif v in ("1", "true"):
        entry.default_value = "TRUE"
```

## 10.2 OCTET_STRING 修复

SSC Tool 不太认 `OCTET_STRING(n)`，建议策略：

```text
OCTET_STRING → STRING
同时对象级 CoE callback = TRUE
报告中标记原始语义是 OCTET_STRING
```

```python
def fix_octet_string(obj: ObjectDef):
    for e in obj.entries:
        if "OCTET_STRING" in e.data_type:
            e.semantic_type = e.data_type
            e.data_type = guess_string_type(e.bit_len)
            obj.need_coe_read_callback = True
            if is_writable(e.access):
                obj.need_coe_write_callback = True
            obj.risk_flags.append("OCTET_STRING_CONVERTED_TO_STRING")
```

```python
def guess_string_type(bit_len: int) -> str:
    length = max(1, bit_len // 8)
    return f"STRING({length})"
```

## 10.3 word offset 对齐检查

这次你遇到最多的就是这个。

```python
def check_word_alignment(obj: ObjectDef):
    bit_offset = 0

    for e in obj.entries:
        if e.subindex == 0:
            continue

        if e.bit_len >= 16 and bit_offset % 16 != 0:
            obj.need_coe_read_callback = True
            if is_writable(e.access):
                obj.need_coe_write_callback = True
            obj.risk_flags.append(
                f"UNALIGNED_ENTRY_SUB{e.subindex}_OFFSET_{bit_offset}"
            )

        bit_offset += e.bit_len
```

注意：callback 只写对象行，不写 entry 行。

## 10.4 PDO/SM 长度校验

```python
def validate_pdo_sm_size(device: EthercatDevice):
    rx_bytes = sum(e.bit_len for pdo in device.rx_pdos for e in pdo.entries) // 8
    tx_bytes = sum(e.bit_len for pdo in device.tx_pdos for e in pdo.entries) // 8

    sm2 = find_sm(device, 2)
    sm3 = find_sm(device, 3)

    if sm2 and sm2.default_size != rx_bytes:
        device.errors.append(f"SM2 size mismatch: SM2={sm2.default_size}, RxPDO={rx_bytes}")

    if sm3 and sm3.default_size != tx_bytes:
        device.errors.append(f"SM3 size mismatch: SM3={sm3.default_size}, TxPDO={tx_bytes}")
```

---

# 11. SSC xlsx 生成器

最关键。

你应该以 `digital_io.xlsx` 为模板，复制它的 sheet、列宽、表头，然后填数据。

## 表头

按你的示例，保持：

```text
Index
ObjectCode
SI
DataType
Name
Default Value
Min Value
Max Value
M/O/C
B/S
Access
rx/tx
CoeRead
CoeWrite
Description
```

如果模板有 17 列，你就保留 17 列。

## Object row 生成规则

```python
def append_object_row(ws, row_idx, obj: ObjectDef, columns):
    row = {
        "Index": f"0x{obj.index:04X}",
        "ObjectCode": obj.object_code,
        "SI": "",
        "DataType": "",
        "Name": obj.name,
        "Default Value": "",
        "Min Value": "",
        "Max Value": "",
        "M/O/C": "O",
        "B/S": "S",
        "Access": "",
        "rx/tx": "",
        "CoeRead": "TRUE" if obj.need_coe_read_callback else "",
        "CoeWrite": "TRUE" if obj.need_coe_write_callback else "",
        "Description": "; ".join(obj.risk_flags),
    }
    write_row(ws, row_idx, row, columns)
```

## Entry row 生成规则

```python
def append_entry_row(ws, row_idx, obj: ObjectDef, e: SubEntry, columns):
    row = {
        "Index": f"0x{obj.index:04X}",
        "ObjectCode": "",
        "SI": e.subindex,
        "DataType": e.data_type,
        "Name": e.name,
        "Default Value": e.default_value,
        "Min Value": e.min_value,
        "Max Value": e.max_value,
        "M/O/C": "O",
        "B/S": "S",
        "Access": to_ssc_access(e.access),
        "rx/tx": e.pdo_direction,
        "CoeRead": "",
        "CoeWrite": "",
        "Description": e.semantic_type,
    }
    write_row(ws, row_idx, row, columns)
```

重点：

```text
CoeRead/CoeWrite 只写 Object row
Entry row 必须为空
```

这正是你最后一次 SSC Tool 报错的原因。

---

# 12. GUI 设计

用 Qt5 做 6 个页面就够。

## 12.1 页面一：文件导入

控件：

```text
ESI 文件路径
SDO 文件路径
digital_io.xlsx 模板路径
输出目录
```

按钮：

```text
浏览
解析
```

解析后显示：

```text
Vendor ID
Product Code
Revision
Device Name
对象数量
RxPDO 数量
TxPDO 数量
SM2/SM3 长度
```

## 12.2 页面二：身份配置

显示原始身份：

```text
原 Vendor ID
原 Product Code
原 Revision
原 Device Name
```

用户填写生成身份：

```text
新 Vendor ID
新 Product Code
新 Revision
新 Device Name
Physics: YY / YYY
```

模式：

```text
Learning Mode
Lab Clone Mode
Product Mode
```

默认选：

```text
Learning Mode
```

## 12.3 页面三：规则策略

选项：

```text
OCTET_STRING 处理：
[转 STRING + callback]
[跳过]
[保留并手动 callback]

未对齐对象：
[对象级 CoE callback]
[插入 padding]
[跳过对象]

通信对象：
[由 SSC 自动生成]
[强制导入]
[跳过]

Fxxx 对象：
[包含]
[跳过]
[只生成 callback stub]
```

## 12.4 页面四：对象字典表

表格列：

```text
Index
Name
SubCount
Range
PDO
Risk
Strategy
Callback
```

颜色：

```text
绿色：自动对象
黄色：callback
红色：错误
灰色：跳过/SSC自动生成
```

支持过滤：

```text
只看错误
只看 callback
只看 PDO
只看 Fxxx
只看跳过对象
```

## 12.5 页面五：PDO/SM 校验

显示：

```text
SM2 DefaultSize: 13
RxPDO calculated: 13
Status: OK

SM3 DefaultSize: 53
TxPDO calculated: 53
Status: OK
```

显示 PDO 布局：

```text
RxPDO 0x1600
0x7003:01 32bit offset 0
0x7009:01 8bit  offset 32
0x7009:02 32bit offset 40
0x7008:01 32bit offset 72
```

## 12.6 页面六：生成输出

按钮：

```text
生成 SSC xlsx
生成 TwinCAT ESI
生成报告
全部生成
```

输出文件列表：

```text
D507_SSC_DESCRIPTION.xlsx
D507_TwinCAT_ESI.xml
D507_report.md
D507_pdo_layout.csv
D507_callback_list.csv
```

---

# 13. Qt5 线程设计

转换过程不要卡 UI。用 `QThread`。

```python
from PyQt5.QtCore import QThread, pyqtSignal

class ConvertWorker(QThread):
    log = pyqtSignal(str)
    progress = pyqtSignal(int)
    finished_ok = pyqtSignal(dict)
    failed = pyqtSignal(str)

    def __init__(self, config):
        super().__init__()
        self.config = config

    def run(self):
        try:
            self.log.emit("开始解析 ESI...")
            self.progress.emit(10)

            result = run_conversion(self.config, logger=self.log.emit)

            self.progress.emit(100)
            self.finished_ok.emit(result)

        except Exception as e:
            self.failed.emit(str(e))
```

GUI 里连接：

```python
self.worker.log.connect(self.append_log)
self.worker.progress.connect(self.progressBar.setValue)
self.worker.finished_ok.connect(self.on_finished)
self.worker.failed.connect(self.on_failed)
self.worker.start()
```

---

# 14. Core 统一接口

`core.py` 对 GUI 和 CLI 提供一个统一函数：

```python
@dataclass
class ConvertConfig:
    esi_path: str
    sdo_path: str
    template_xlsx_path: str
    output_dir: str

    vendor_id: int
    product_code: int
    revision: int
    device_name: str
    physics: str = "YY"

    mode: str = "learning"

    octet_string_strategy: str = "string_with_callback"
    unaligned_strategy: str = "coe_callback"
    communication_object_strategy: str = "ssc_auto"
    include_fxxx: bool = True


def run_conversion(config: ConvertConfig, logger=print):
    logger("解析 ESI")
    esi_device = parse_esi(config.esi_path)

    logger("解析 SDO")
    sdo_objects = parse_sdo(config.sdo_path)

    logger("合并模型")
    device = merge_esi_sdo(esi_device, sdo_objects)

    logger("应用新设备身份")
    apply_identity(device, config)

    logger("对象分类")
    classify_device(device)

    logger("运行规则检查")
    run_rules(device, config)

    logger("自动修复")
    apply_fixes(device, config)

    logger("最终校验")
    validate_device(device)

    logger("生成 SSC xlsx")
    ssc_xlsx = generate_ssc_xlsx(device, config)

    logger("生成 TwinCAT ESI")
    esi_xml = generate_twincat_esi(device, config)

    logger("生成报告")
    report = generate_report(device, config)

    return {
        "ssc_xlsx": ssc_xlsx,
        "esi_xml": esi_xml,
        "report": report,
        "warnings": device.warnings,
        "errors": device.errors,
    }
```

---

# 15. CLI 先行开发

建议先写 CLI，再接 Qt5。

`cli.py`：

```python
import typer
from nekoecat.core import ConvertConfig, run_conversion

app = typer.Typer()

@app.command()
def generate(
    esi: str,
    sdo: str,
    template: str,
    out: str,
    vendor_id: int = 999999,
    product_code: str = "0xD5070001",
    revision: str = "0x00000001",
    name: str = "HPM_D507_Play",
):
    config = ConvertConfig(
        esi_path=esi,
        sdo_path=sdo,
        template_xlsx_path=template,
        output_dir=out,
        vendor_id=vendor_id,
        product_code=int(product_code, 16),
        revision=int(revision, 16),
        device_name=name,
    )

    result = run_conversion(config)
    typer.echo("生成完成")
    typer.echo(result)

if __name__ == "__main__":
    app()
```

使用：

```bash
python cli.py generate \
  --esi ESI.XML \
  --sdo SDO.XML \
  --template digital_io.xlsx \
  --out out/d507
```

CLI 成功后再做 GUI。

---

# 16. 开发阶段安排

## 第 1 阶段：核心转换器，3~5 天

目标：

```text
能解析 ESI
能解析 SDO
能合并对象
能生成基础 xlsx
```

完成标准：

```text
生成的 xlsx 能被 SSC Tool 读取
即使有 warning，也不能格式不认
```

---

## 第 2 阶段：规则引擎，3~5 天

实现这几个规则：

```text
BOOL 默认值 TRUE/FALSE
OCTET_STRING → STRING + callback
未对齐对象 → object callback
通信对象过滤
PDO/SM 长度校验
callback 只写对象行
```

完成标准：

```text
你这次 D507 的导入日志不再出现 parser error
尽量不出现 Skip object
```

---

## 第 3 阶段：报告系统，2~3 天

生成：

```text
report.md
pdo_layout.csv
callback_list.csv
skipped_objects.csv
```

报告里列：

```text
对象总数
子对象总数
RxPDO/TxPDO 长度
转换策略
风险对象
callback 对象
跳过对象
```

---

## 第 4 阶段：Qt5 GUI，1 周

先做简单界面：

```text
文件选择
身份配置
规则选择
转换按钮
日志窗口
结果文件列表
```

不要一开始做复杂对象字典编辑器。

---

## 第 5 阶段：对象字典编辑器，1~2 周

增加：

```text
对象表格
风险过滤
手动修改策略
禁用某对象
强制 callback
强制跳过
```

---

## 第 6 阶段：SSC Tool 日志自动修复，1 周

用户把 SSC Tool 日志粘进去，软件自动识别：

```text
Failed to convert '0' to DataType '1'
Data type "OCTET_STRING(x)" not supported
Entry doesn't start at word offset
A CoE callback shall be defined for an object
```

然后自动修复 xlsx。

---

# 17. 你要重点实现的自动修复规则

根据这次经验，优先做这几个。

## 17.1 Boolean 修复

```text
0 → FALSE
1 → TRUE
```

## 17.2 OCTET_STRING 修复

```text
OCTET_STRING(n) → STRING(n)
object.CoeRead = TRUE
object.CoeWrite = TRUE
```

## 17.3 未对齐修复

如果 entry 16bit/32bit 没有从 word offset 开始：

```text
object.CoeRead = TRUE
object.CoeWrite = TRUE
```

不要自动加 padding，除非用户明确选择。因为 padding 会改变对象布局。

## 17.4 callback 位置修复

错误：

```text
entry.CoeRead = TRUE
entry.CoeWrite = TRUE
```

正确：

```text
object.CoeRead = TRUE
object.CoeWrite = TRUE
entry.CoeRead = 空
entry.CoeWrite = 空
```

---

# 18. 生成文件建议

每次生成一个目录：

```text
out/D507_2026_06_13_1530/
├── D507_SSC_DESCRIPTION.xlsx
├── D507_TwinCAT_ESI.xml
├── D507_report.md
├── D507_pdo_layout.csv
├── D507_callback_list.csv
├── D507_skipped_objects.csv
├── device.normalized.json
└── logs/
    └── convert.log
```

`device.normalized.json` 很重要，用来复现问题。

---

# 19. 打包发布

开发时：

```bash
python main.py
```

打包：

```bash
pyinstaller -F -w main.py ^
  --name NekoECATConverter ^
  --add-data "templates;templates"
```

Windows 输出：

```text
dist/NekoECATConverter.exe
```

注意：如果 PyQt5 打包后缺 DLL，可能要改成目录模式：

```bash
pyinstaller -D -w main.py --name NekoECATConverter
```

---

# 20. MVP 成功标准

你的第一个版本只要做到下面这些，就已经很有价值：

```text
1. 导入 ESI.XML
2. 导入 SDO.XML
3. 导入 digital_io.xlsx 模板
4. 自动生成 SSC Tool xlsx
5. 自动修复 BOOL
6. 自动修复 OCTET_STRING
7. 自动处理 word offset callback
8. callback 只写对象行
9. 自动校验 PDO/SM 长度
10. 生成转换报告
```

用你的 D507 测试，成功标准是：

```text
SSC Tool 能成功 Import
没有 Failed to convert
没有 Data type unsupported
没有 callback 行级错误
没有明显 Skip object
```

---

# 21. 最终产品功能路线

最终版可以做成这样：

```text
NekoECAT Converter
├── ESI/SDO 导入
├── SSC xlsx 模板识别
├── 对象字典自动合并
├── PDO/SM/FMMU/DC 可视化
├── SSC Tool xlsx 生成
├── TwinCAT ESI 生成
├── HPM SDK 代码骨架生成
├── CoE callback 自动生成
├── SSC Tool 报错自动修复
├── 对象字典差异对比
├── 学习模式/实验模式/产品模式
└── 一键打包工程
```

---

# 22. 推荐你马上开始的顺序

不要先写 GUI。按这个顺序：

```text
第 1 步：
写 parser/esi_parser.py

第 2 步：
写 parser/sdo_parser.py

第 3 步：
写 model/device.py

第 4 步：
写 merger.py，把 ESI + SDO 合成统一模型

第 5 步：
写 rule_engine.py，复现这次所有坑

第 6 步：
写 ssc_xlsx_generator.py，生成 xlsx

第 7 步：
用命令行跑通 D507

第 8 步：
再做 Qt5 GUI
```

一句话总结：**先把 Python core 做稳，Qt5 只做壳。你的核心竞争力是把 ESI/SDO 自动归一化，并生成 SSC Tool 真正吃得下的 xlsx，同时自动处理 BOOL、OCTET_STRING、word alignment、CoE callback、PDO/SM 长度这些坑。**
