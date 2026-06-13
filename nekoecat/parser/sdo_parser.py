"""
SDO parser — reads SDO data into DeviceModel.

Supports two formats per dev doc §7:
  1. XML format: <ObjectList><Object ...>...</Object>
  2. Text dump format:
       SDO 0x7009, "Actuator"
       0x7009:00, r-r-r-, uint8, 8 bit, "SubIndex 000"
       0x7009:01, r-r-rw, uint8, 8 bit, "Actuator Control"

Only depends on: model/, utils, constants.
"""
import re
from pathlib import Path
from typing import List
from rich.console import Console

from nekoecat.model import ObjectEntry, SubIndex, Issue, Severity, RuleCode
from nekoecat.model.device import DeviceModel
from nekoecat import utils

console = Console()

# ── Text format regex patterns (dev doc §7) ──
_RE_OBJECT = re.compile(r'SDO\s+0x([0-9a-fA-F]+),\s*"([^"]*)"')
_RE_ENTRY = re.compile(
    r'0x([0-9a-fA-F]+):([0-9a-fA-F]+),\s*([^,]+),\s*([^,]+),\s*(\d+)\s*bit,\s*"([^"]*)"'
)


class SdoParser:
    """Parse an SDO file (XML or text) into a DeviceModel."""

    def parse(self, filepath: str) -> DeviceModel:
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"SDO file not found: {path}")

        text = path.read_text(encoding="utf-8", errors="replace")
        device = DeviceModel()

        # Detect format
        if text.lstrip().startswith("<") or "<ObjectList" in text[:500]:
            objects = self._parse_xml(path, text)
        else:
            objects = self._parse_text(text)

        for obj in objects:
            device.objects[obj.index] = obj

        console.print(f"[green]SDO parsed:[/green] {len(device.objects)} objects")
        return device

    # ──────────────────────────────────────────
    # XML format
    # ──────────────────────────────────────────

    @staticmethod
    def _parse_xml(path: Path, text: str) -> List[ObjectEntry]:
        root = utils.parse_xml_file(str(path))
        objects: List[ObjectEntry] = []

        for obj_el in utils.xml_findall(root, ".//Object"):
            obj = _parse_xml_object(obj_el)
            if obj is not None:
                objects.append(obj)

        # Alternative: <ODObject> format
        if not objects:
            for obj_el in utils.xml_findall(root, ".//ODObject"):
                obj = _parse_xml_od_object(obj_el)
                if obj is not None:
                    objects.append(obj)

        return objects

    # ──────────────────────────────────────────
    # Text dump format (dev doc §7)
    # ──────────────────────────────────────────

    @staticmethod
    def _parse_text(text: str) -> List[ObjectEntry]:
        objects: dict[int, ObjectEntry] = {}
        current: ObjectEntry | None = None

        for line in text.splitlines():
            line = line.strip()
            if not line:
                continue

            # Match object header: SDO 0x7009, "Actuator"
            m_obj = _RE_OBJECT.match(line)
            if m_obj:
                index = int(m_obj.group(1), 16)
                name = m_obj.group(2)
                current = ObjectEntry(index=index, name=name, object_type=7, source="sdo")
                objects[index] = current
                continue

            # Match entry: 0x7009:01, r-r-rw, uint8, 8 bit, "Actuator Control"
            m_ent = _RE_ENTRY.match(line)
            if m_ent:
                index = int(m_ent.group(1), 16)
                subindex = int(m_ent.group(2), 16)
                access_raw = m_ent.group(3).strip()
                dtype_raw = m_ent.group(4).strip()
                bit_len = int(m_ent.group(5))
                name = m_ent.group(6)

                access = _normalize_access(access_raw)
                data_type = _normalize_sdo_type(dtype_raw)

                # Ensure parent object exists
                obj = objects.get(index)
                if obj is None:
                    obj = ObjectEntry(
                        index=index,
                        name=f"Object 0x{index:04X}",
                        object_type=9,
                        source="sdo",
                    )
                    objects[index] = obj

                if subindex == 0:
                    # SubIndex 0 = count or variable
                    obj.data_type = data_type
                    obj.bit_length = bit_len
                    obj.access = access
                    obj.default_value = name  # often "SubIndex 000"
                else:
                    obj.subindices.append(SubIndex(
                        subindex=subindex,
                        name=name,
                        data_type=data_type,
                        bit_length=bit_len,
                        access=access,
                    ))

                # Infer object type
                if len(obj.subindices) > 0:
                    obj.object_type = 9  # RECORD

        return list(objects.values())


# ──────────────────────────────────────────────
# XML parsing helpers (unchanged from before)
# ──────────────────────────────────────────────

def _parse_xml_object(el) -> ObjectEntry | None:
    index = utils.to_int(utils.xml_attr(el, "Index"), -1)
    if index < 0:
        index = utils.to_int(utils.xml_text(utils.xml_find(el, "Index")), -1)
    if index < 0:
        return None

    obj_type = utils.to_int(
        utils.xml_attr(el, "ObjectType",
                       utils.xml_text(utils.xml_find(el, "ObjectType"), "7")),
        7,
    )
    name = utils.xml_attr(el, "Name") or utils.xml_text(utils.xml_find(el, "Name"))
    data_type = utils.normalize_data_type(
        utils.xml_text(utils.xml_find(el, "DataType"))
        or utils.xml_attr(el, "DataType")
    )
    bit_length = utils.to_int(
        utils.xml_text(utils.xml_find(el, "BitSize"))
        or utils.xml_text(utils.xml_find(el, "BitLen"))
    )
    access = utils.xml_text(utils.xml_find(el, "Access"))
    default_value = utils.xml_text(
        utils.xml_find(el, "DefaultValue")
        or utils.xml_find(el, "Default")
    )
    pdo_mapping = utils.xml_text(utils.xml_find(el, "PdoMapping"))

    obj = ObjectEntry(
        index=index, name=name, object_type=obj_type,
        data_type=data_type, bit_length=bit_length,
        access=access, default_value=default_value,
        pdo_mapping=pdo_mapping, source="sdo",
    )

    for sub_el in utils.xml_findall(el, "SubItem"):
        sub = _parse_xml_subitem(sub_el)
        if sub is not None:
            obj.subindices.append(sub)

    for sub_obj_el in utils.xml_findall(el, "Object"):
        sub_idx = utils.to_int(utils.xml_attr(sub_obj_el, "SubIndex"), -1)
        if sub_idx < 0:
            continue
        obj.subindices.append(SubIndex(
            subindex=sub_idx,
            name=utils.xml_attr(sub_obj_el, "Name") or utils.xml_text(utils.xml_find(sub_obj_el, "Name")),
            data_type=utils.normalize_data_type(utils.xml_text(utils.xml_find(sub_obj_el, "DataType"))),
            bit_length=utils.to_int(utils.xml_text(utils.xml_find(sub_obj_el, "BitSize"))),
            access=utils.xml_text(utils.xml_find(sub_obj_el, "Access")),
            default_value=utils.xml_text(utils.xml_find(sub_obj_el, "DefaultValue")),
        ))

    return obj


def _parse_xml_od_object(el) -> ObjectEntry | None:
    index = utils.to_int(utils.xml_attr(el, "Index"), -1)
    if index < 0:
        return None

    name = utils.xml_attr(el, "Name") or utils.xml_text(utils.xml_find(el, "Name"))
    obj_type = utils.to_int(utils.xml_attr(el, "ObjectType"), 7)
    data_type = utils.normalize_data_type(utils.xml_text(utils.xml_find(el, "DataType")))
    bit_length = utils.to_int(utils.xml_text(utils.xml_find(el, "BitLength")))
    access = utils.xml_text(utils.xml_find(el, "AccessType"))
    default_value = utils.xml_text(utils.xml_find(el, "DefaultValue"))

    obj = ObjectEntry(
        index=index, name=name, object_type=obj_type,
        data_type=data_type, bit_length=bit_length,
        access=access, default_value=default_value, source="sdo",
    )

    for sub_el in utils.xml_findall(el, "SubItem"):
        sub = _parse_xml_subitem(sub_el)
        if sub is not None:
            obj.subindices.append(sub)

    return obj


def _parse_xml_subitem(el) -> SubIndex | None:
    sub_idx = utils.to_int(utils.xml_attr(el, "SubIndex"), -1)
    if sub_idx < 0:
        sub_idx = utils.to_int(utils.xml_text(utils.xml_find(el, "SubIndex")), -1)
    if sub_idx < 0:
        return None

    return SubIndex(
        subindex=sub_idx,
        name=utils.xml_attr(el, "Name") or utils.xml_text(utils.xml_find(el, "Name")),
        data_type=utils.normalize_data_type(utils.xml_text(utils.xml_find(el, "DataType"))),
        bit_length=utils.to_int(
            utils.xml_text(utils.xml_find(el, "BitSize"))
            or utils.xml_text(utils.xml_find(el, "BitLen"))
        ),
        access=utils.xml_text(utils.xml_find(el, "Access")),
        default_value=utils.xml_text(
            utils.xml_find(el, "DefaultValue")
            or utils.xml_find(el, "Default")
        ),
        pdo_mapping=utils.xml_text(utils.xml_find(el, "PdoMapping")),
    )


# ──────────────────────────────────────────────
# Text-format normalizers (dev doc §7)
# ──────────────────────────────────────────────

def _normalize_sdo_type(raw: str) -> str:
    """Normalize SDO text type names to CoE standard names."""
    t = raw.strip().lower()
    mapping = {
        "bool": "BOOLEAN",
        "boolean": "BOOLEAN",
        "uint8": "UNSIGNED8",
        "uint16": "UNSIGNED16",
        "uint32": "UNSIGNED32",
        "uint64": "UNSIGNED64",
        "usint": "UNSIGNED8",
        "uint": "UNSIGNED16",
        "udint": "UNSIGNED32",
        "ulint": "UNSIGNED64",
        "int8": "INTEGER8",
        "int16": "INTEGER16",
        "int32": "INTEGER32",
        "int64": "INTEGER64",
        "sint": "INTEGER8",
        "int": "INTEGER16",
        "dint": "INTEGER32",
        "lint": "INTEGER64",
        "float": "REAL32",
        "real": "REAL32",
        "lreal": "REAL64",
        "double": "REAL64",
        "string": "VISIBLE_STRING",
        "visible_string": "VISIBLE_STRING",
        "octet_string": "OCTET_STRING",
        "unicode_string": "UNICODE_STRING",
        "domain": "DOMAIN",
    }
    # Handle OCTET_STRING(n)
    if t.startswith("octet_string"):
        return "OCTET_STRING" + t[len("octet_string"):]
    return mapping.get(t, raw.strip().upper())


def _normalize_access(raw: str) -> str:
    """Normalize TwinCAT-style access (r-r-rw) to SSC style (ro/rw/wo)."""
    s = raw.strip().lower().replace(" ", "")
    # TwinCAT format: "r-r-rw" → look at last segment
    if "-" in s and len(s) >= 3:
        last = s.split("-")[-1]
        if "w" in last and "r" in last:
            return "rw"
        if "r" in last:
            return "ro"
        if "w" in last:
            return "wo"
    # Standard strings
    if s in ("ro", "rw", "wo", "const", "r", "w", "r/w"):
        if s == "r":
            return "ro"
        if s == "w":
            return "wo"
        if s == "r/w":
            return "rw"
        return s
    return raw.strip()


# ── Public convenience ────────────────────────

def parse_sdo(filepath: str) -> DeviceModel:
    """Parse an SDO file (auto-detects XML vs text) and return a DeviceModel."""
    return SdoParser().parse(filepath)
