"""
SSC log auto-fix engine — takes parsed SSC Tool import errors and
applies fixes to the DeviceModel.  Implements dev doc §16.6.

Only depends on: model/, log/.
"""
from rich.console import Console

from nekoecat.model import DeviceModel, Issue, Severity, RuleCode
from nekoecat.log import SscLogParseResult

console = Console()


def apply_ssc_log_fixes(device: DeviceModel, log_result: SscLogParseResult) -> int:
    """Apply fixes based on SSC Tool import error log.

    Returns number of fixes applied.
    """
    fixes = 0
    for entry in log_result.entries:
        if entry.level != "Error":
            continue

        msg = entry.message.lower()

        # ── "failed to convert '0' to datatype '1'" ──
        if "failed to convert" in msg and "datatype" in msg:
            fixes += _fix_failed_convert(device, entry)

        # ── 'data type "octet_string(x)" not supported' ──
        elif "data type" in msg and "not supported" in msg:
            fixes += _fix_unsupported_type(device, entry)

        # ── "entry doesn't start at word offset" ──
        elif "word offset" in msg:
            fixes += _fix_word_offset(device, entry)

        # ── "a coe callback shall be defined for an object" ──
        elif "coe callback" in msg and "shall be defined" in msg:
            fixes += _fix_missing_callback(device, entry)

    console.print(f"[yellow]SSC log auto-fix:[/yellow] {fixes} fixes applied")
    return fixes


def _fix_failed_convert(device: DeviceModel, entry) -> int:
    """Fix 'failed to convert' errors — usually BOOL 0/1 issue."""
    idx = _parse_index(entry.index)
    if idx is None:
        return 0

    obj = device.get_object(idx)
    if obj is None:
        return 0

    # Check for BOOL default values
    if obj.data_type == "BOOLEAN":
        if obj.default_value.strip() in ("0", "1"):
            obj.default_value = "FALSE" if obj.default_value.strip() == "0" else "TRUE"
            return 1

    for sub in obj.subindices:
        if sub.data_type == "BOOLEAN" and sub.default_value.strip() in ("0", "1"):
            sub.default_value = "FALSE" if sub.default_value.strip() == "0" else "TRUE"
            return 1

    return 0


def _fix_unsupported_type(device: DeviceModel, entry) -> int:
    """Fix 'data type not supported' — usually OCTET_STRING."""
    idx = _parse_index(entry.index)
    if idx is None:
        # Try to fix globally
        return _fix_octet_string_global(device)

    obj = device.get_object(idx)
    if obj is None:
        return 0

    fixed = 0
    if "octet_string" in obj.data_type.lower():
        obj.original_data_type = obj.data_type
        obj.data_type = _rename_to_string(obj.data_type)
        obj.coe_read = True
        obj.coe_write = True
        fixed += 1

    for sub in obj.subindices:
        if "octet_string" in (sub.data_type or "").lower():
            sub.original_data_type = sub.data_type
            sub.data_type = _rename_to_string(sub.data_type)
            obj.coe_read = True
            obj.coe_write = True
            sub.auto_fixed = True
            fixed += 1

    return fixed


def _fix_word_offset(device: DeviceModel, entry) -> int:
    """Fix 'entry doesn't start at word offset' — add object-level callback."""
    idx = _parse_index(entry.index)
    if idx is None:
        return 0

    obj = device.get_object(idx)
    if obj is None:
        return 0

    obj.coe_read = True
    obj.coe_write = True
    return 1


def _fix_missing_callback(device: DeviceModel, entry) -> int:
    """Fix 'CoE callback shall be defined' — set callback on object."""
    idx = _parse_index(entry.index)
    if idx is None:
        return 0

    obj = device.get_object(idx)
    if obj is None:
        return 0

    obj.coe_read = True
    if obj.access in ("rw", "wo"):
        obj.coe_write = True
    return 1


def _fix_octet_string_global(device: DeviceModel) -> int:
    """Fix all OCTET_STRING types in the device."""
    fixed = 0
    for obj in device.objects.values():
        if "octet_string" in obj.data_type.lower():
            obj.original_data_type = obj.data_type
            obj.data_type = _rename_to_string(obj.data_type)
            obj.coe_read = True
            obj.coe_write = True
            fixed += 1

        for sub in obj.subindices:
            if "octet_string" in (sub.data_type or "").lower():
                sub.original_data_type = sub.data_type
                sub.data_type = _rename_to_string(sub.data_type)
                obj.coe_read = True
                obj.coe_write = True
                sub.auto_fixed = True
                fixed += 1

    return fixed


# ── Helpers ───────────────────────────────────

def _parse_index(index_str: str) -> int | None:
    if not index_str:
        return None
    try:
        if index_str.startswith("0x"):
            return int(index_str, 16)
        return int(index_str)
    except ValueError:
        return None


def _rename_to_string(data_type: str) -> str:
    """OCTET_STRING(n) → STRING(n)."""
    if "(" in data_type:
        size_part = data_type[data_type.index("("):]
        return f"STRING{size_part}"
    return "STRING"
