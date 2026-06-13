"""
Code generator — generates C code skeletons using Jinja2 templates.

Generates:
  - coe_callbacks.c  — CoE read/write callback stubs
  - pdo_structs.h    — PDO structure definitions

Only depends on: model/. Templates live in templates/.
"""
from pathlib import Path
from datetime import datetime
from typing import Optional
from rich.console import Console
from jinja2 import Environment, FileSystemLoader

from nekoecat.model import DeviceModel

console = Console()

# Resolve template directory relative to this file
_TEMPLATE_DIR = Path(__file__).resolve().parent.parent.parent / "templates"

# CoE type → C type mapping
_COE_TO_C = {
    "BOOLEAN": "UINT8",
    "INTEGER8": "INT8",
    "INTEGER16": "INT16",
    "INTEGER32": "INT32",
    "INTEGER64": "INT64",
    "UNSIGNED8": "UINT8",
    "UNSIGNED16": "UINT16",
    "UNSIGNED32": "UINT32",
    "UNSIGNED64": "UINT64",
    "REAL32": "float",
    "REAL64": "double",
    "BYTE": "UINT8",
    "WORD": "UINT16",
    "DWORD": "UINT32",
    "VISIBLE_STRING": "char",
    "STRING": "char",
    "OCTET_STRING": "UINT8",
}


def generate_code(
    device: DeviceModel,
    output_dir: str,
) -> dict[str, str]:
    """Generate C code files from the device model.

    Returns dict of {filename: path}.
    """
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)

    env = Environment(
        loader=FileSystemLoader(str(_TEMPLATE_DIR)),
        keep_trailing_newline=True,
    )
    timestamp = datetime.now().isoformat()
    from nekoecat import __version__
    version = __version__

    files: dict[str, str] = {}

    # ── coe_callbacks.c ───────────────────────
    callback_objects = [
        obj for obj in device.sorted_objects()
        if obj.coe_read is True or obj.coe_write is True
    ]
    if callback_objects:
        tpl = env.get_template("coe_callbacks.c.j2")
        content = tpl.render(
            device=device,
            callback_objects=callback_objects,
            timestamp=timestamp,
        )
        path = out / "coe_callbacks.c"
        path.write_text(content, encoding="utf-8")
        files["coe_callbacks.c"] = str(path)

    # ── pdo_structs.h ─────────────────────────
    rx_pdos = _build_pdo_list(device, "rx")
    tx_pdos = _build_pdo_list(device, "tx")
    if rx_pdos or tx_pdos:
        tpl = env.get_template("pdo_structs.h.j2")
        content = tpl.render(
            device=device,
            rx_pdos=rx_pdos,
            tx_pdos=tx_pdos,
            timestamp=timestamp,
        )
        path = out / "pdo_structs.h"
        path.write_text(content, encoding="utf-8")
        files["pdo_structs.h"] = str(path)

    if files:
        console.print(f"[green]Code generated:[/green] {list(files.keys())}")
    return files


# ── Helpers ───────────────────────────────────

def _build_pdo_list(device: DeviceModel, direction: str) -> list[dict]:
    """Build PDO struct data for template rendering."""
    pdo_indices = device.rxpdo_assign if direction == "rx" else device.txpdo_assign
    result = []

    for pdo_idx in pdo_indices:
        obj = device.get_object(pdo_idx)
        if obj is None:
            continue

        entries = []
        for sub in obj.subindices:
            dt = sub.data_type or obj.data_type
            c_type = _coe_to_c_type(dt, sub.bit_length)
            field_name = _to_field_name(sub.name, sub.subindex)

            entries.append({
                "index": obj.index,
                "subindex": sub.subindex,
                "name": sub.name,
                "bit_length": sub.bit_length,
                "data_type": dt,
                "c_type": c_type,
                "field_name": field_name,
            })

        struct_name = _to_struct_name(obj.name, obj.index)
        result.append({
            "index": obj.index,
            "name": obj.name,
            "struct_name": struct_name,
            "entries": entries,
        })

    return result


def _coe_to_c_type(data_type: str, bit_length: int) -> str:
    """Map CoE type to C type."""
    base = data_type.split("(")[0].strip()
    c = _COE_TO_C.get(base)
    if c:
        if c == "char" and bit_length > 8:
            return f"char[{(bit_length + 7) // 8}]"
        return c
    return "UINT8"


def _to_field_name(name: str, subindex: int) -> str:
    """Convert a name to a valid C field name."""
    s = name.replace(" ", "_").replace("-", "_").replace(".", "_")
    s = "".join(c for c in s if c.isalnum() or c == "_")
    if not s or s[0].isdigit():
        s = f"sub{subindex}_{s}"
    return s.lower()


def _to_struct_name(name: str, index: int) -> str:
    """Convert a PDO name to a C struct typedef name."""
    s = name.replace(" ", "_").replace("-", "_")
    s = "".join(c for c in s if c.isalnum() or c == "_")
    if not s:
        s = f"PDO_{index:04X}"
    return s
