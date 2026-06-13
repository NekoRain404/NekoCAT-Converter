"""
Rule engine — checks a DeviceModel against SSC Tool import rules.

Rule implementations follow dev doc §10.  Does NOT mutate model data;
only appends Issue objects.  Fixes live in fix_engine.

Only depends on: model/, constants.
"""
from rich.console import Console

from nekoecat.model import (
    DeviceModel, ObjectEntry, SubIndex,
    Issue, Severity, RuleCode,
)
from nekoecat.constants import SSC_CALLBACK_TYPES, COMM_OBJECT_RANGE

console = Console()


def run_rules(device: DeviceModel) -> None:
    """Run every rule check.  New issues are appended to device.issues."""
    _check_boolean_defaults(device)
    _check_octet_strings(device)
    _check_word_alignment(device)
    _check_callback_placement(device)
    _check_comm_objects(device)
    _check_missing_data_types(device)
    _check_unsupported_types(device)
    _check_pdo_sm_lengths(device)
    console.print(
        f"[cyan]Rules checked:[/cyan] {len(device.issues)} issues found"
    )


# ──────────────────────────────────────────────
# 1. BOOLEAN default must be TRUE / FALSE (§10.1)
# ──────────────────────────────────────────────

def _check_boolean_defaults(device: DeviceModel):
    for obj in device.objects.values():
        for dt, val, sub_idx in _iter_datatype_value(obj):
            if dt == "BOOLEAN" and val.strip() in ("0", "1"):
                device.add_issue(Issue(
                    code=RuleCode.BOOL_DEFAULT_INVALID,
                    severity=Severity.WARNING,
                    message=(
                        f"0x{obj.index:04X}:{sub_idx} BOOLEAN default "
                        f"'{val}' → should be TRUE/FALSE"
                    ),
                    object_index=obj.index,
                    subindex=sub_idx,
                    auto_fixable=True,
                ))


# ──────────────────────────────────────────────
# 2. OCTET_STRING / UNICODE_STRING unsupported (§10.2)
# ──────────────────────────────────────────────

def _check_octet_strings(device: DeviceModel):
    for obj in device.objects.values():
        for dt, _val, sub_idx in _iter_datatype_value(obj):
            base = dt.split("(")[0].strip()
            if base in SSC_CALLBACK_TYPES:
                device.add_issue(Issue(
                    code=RuleCode.OCTET_STRING_UNSUPPORTED,
                    severity=Severity.WARNING,
                    message=(
                        f"0x{obj.index:04X}:{sub_idx} type '{dt}' "
                        f"→ rename to STRING + set CoE callback"
                    ),
                    object_index=obj.index,
                    subindex=sub_idx,
                    auto_fixable=True,
                ))


# ──────────────────────────────────────────────
# 3. Word offset alignment (§10.3)
# ──────────────────────────────────────────────

def _check_word_alignment(device: DeviceModel):
    """If a ≥16-bit entry doesn't start at a 16-bit boundary, flag it."""
    for obj in device.objects.values():
        if not obj.subindices:
            continue
        bit_offset = 0
        for sub in obj.subindices:
            if sub.subindex == 0:
                # SubIndex 0 is the count, doesn't occupy PDO space
                continue
            if sub.bit_length >= 16 and bit_offset % 16 != 0:
                device.add_issue(Issue(
                    code=RuleCode.WORD_ALIGNMENT_ERROR,
                    severity=Severity.WARNING,
                    message=(
                        f"0x{obj.index:04X}:{sub.subindex} "
                        f"{sub.bit_length}bit entry at bit offset {bit_offset} "
                        f"(not word-aligned) → needs object-level CoE callback"
                    ),
                    object_index=obj.index,
                    subindex=sub.subindex,
                    auto_fixable=True,
                ))
            bit_offset += sub.bit_length


# ──────────────────────────────────────────────
# 4. CoE callback at entry level (wrong place) (§10.3 / §17.4)
# ──────────────────────────────────────────────

def _check_callback_placement(device: DeviceModel):
    for obj in device.objects.values():
        for sub in obj.subindices:
            if sub.coe_read is True or sub.coe_write is True:
                device.add_issue(Issue(
                    code=RuleCode.CALLBACK_ENTRY_LEVEL,
                    severity=Severity.WARNING,
                    message=(
                        f"0x{obj.index:04X}:{sub.subindex} CoE flags at "
                        f"subindex level → move to object level"
                    ),
                    object_index=obj.index,
                    subindex=sub.subindex,
                    auto_fixable=True,
                ))


# ──────────────────────────────────────────────
# 5. Communication objects (filtered by SSC Tool)
# ──────────────────────────────────────────────

def _check_comm_objects(device: DeviceModel):
    for obj in device.objects.values():
        if obj.index in COMM_OBJECT_RANGE:
            device.add_issue(Issue(
                code=RuleCode.COMM_OBJECT_FILTERED,
                severity=Severity.INFO,
                message=(
                    f"0x{obj.index:04X} comm object — "
                    f"excluded from application xlsx"
                ),
                object_index=obj.index,
            ))


# ──────────────────────────────────────────────
# 6. Missing data type
# ──────────────────────────────────────────────

def _check_missing_data_types(device: DeviceModel):
    for obj in device.objects.values():
        if not obj.data_type and not obj.subindices:
            device.add_issue(Issue(
                code=RuleCode.MISSING_DATA_TYPE,
                severity=Severity.ERROR,
                message=f"0x{obj.index:04X} has no data type",
                object_index=obj.index,
            ))


# ──────────────────────────────────────────────
# 7. Unknown / unsupported data types
# ──────────────────────────────────────────────

_KNOWN_TYPES = {
    "BOOLEAN", "INTEGER8", "INTEGER16", "INTEGER32", "INTEGER64",
    "UNSIGNED8", "UNSIGNED16", "UNSIGNED32", "UNSIGNED64",
    "REAL32", "REAL64",
    "VISIBLE_STRING", "OCTET_STRING", "UNICODE_STRING",
    "DOMAIN", "TIME_OF_DAY", "TIME_DIFFERENCE",
    "BYTE", "WORD", "DWORD", "GUID",
    "INTEGER24", "INTEGER40", "INTEGER48", "INTEGER56",
    "UNSIGNED24", "UNSIGNED40", "UNSIGNED48", "UNSIGNED56",
    "STRING",  # after OCTET_STRING rename
    # TwinCAT aliases
    "BOOL", "USINT", "UINT", "UDINT", "ULINT",
    "SINT", "INT", "DINT", "LINT", "REAL", "LREAL",
}


def _check_unsupported_types(device: DeviceModel):
    for obj in device.objects.values():
        for dt, _val, sub_idx in _iter_datatype_value(obj):
            base = dt.split("(")[0].strip()
            if base and base not in _KNOWN_TYPES:
                device.add_issue(Issue(
                    code=RuleCode.UNSUPPORTED_DATA_TYPE,
                    severity=Severity.WARNING,
                    message=f"0x{obj.index:04X}:{sub_idx} unknown type '{dt}'",
                    object_index=obj.index,
                    subindex=sub_idx,
                ))


# ──────────────────────────────────────────────
# 8. PDO / SM length validation (§10.4)
# ──────────────────────────────────────────────

def _check_pdo_sm_lengths(device: DeviceModel):
    """Compare PDO byte sizes against SM default sizes."""
    # Build SM lookup by type
    sm_by_type = {sm.pdo_type: sm for sm in device.sm_configs}

    # Calculate total bytes for RxPDO (outputs, SM2)
    rx_bits = 0
    for pdo_idx in device.rxpdo_assign:
        for obj in device.objects.values():
            # RxPDO mapping objects: 0x1600-0x17FF
            if 0x1600 <= obj.index <= 0x17FF:
                rx_bits += obj.total_bit_length
    if not rx_bits:
        # Fallback: sum all objects with PDO mapping
        for obj in device.objects.values():
            if obj.pdo_mapping and "rx" in obj.pdo_mapping.lower():
                rx_bits += obj.total_bit_length

    # Calculate total bytes for TxPDO (inputs, SM3)
    tx_bits = 0
    for pdo_idx in device.txpdo_assign:
        for obj in device.objects.values():
            if 0x1A00 <= obj.index <= 0x1BFF:
                tx_bits += obj.total_bit_length
    if not tx_bits:
        for obj in device.objects.values():
            if obj.pdo_mapping and "tx" in obj.pdo_mapping.lower():
                tx_bits += obj.total_bit_length

    rx_bytes = (rx_bits + 7) // 8
    tx_bytes = (tx_bits + 7) // 8

    sm2 = sm_by_type.get("outputs")
    sm3 = sm_by_type.get("inputs")

    if sm2 and rx_bytes > 0 and sm2.default_size > 0 and sm2.default_size != rx_bytes:
        device.add_issue(Issue(
            code=RuleCode.SM_LENGTH_MISMATCH,
            severity=Severity.WARNING,
            message=(
                f"SM2 size mismatch: SM2={sm2.default_size} bytes, "
                f"RxPDO calculated={rx_bytes} bytes"
            ),
        ))

    if sm3 and tx_bytes > 0 and sm3.default_size > 0 and sm3.default_size != tx_bytes:
        device.add_issue(Issue(
            code=RuleCode.SM_LENGTH_MISMATCH,
            severity=Severity.WARNING,
            message=(
                f"SM3 size mismatch: SM3={sm3.default_size} bytes, "
                f"TxPDO calculated={tx_bytes} bytes"
            ),
        ))


# ──────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────

def _iter_datatype_value(obj: ObjectEntry):
    """Yield (data_type, value, subindex) for object + its subindices."""
    if obj.subindices:
        for sub in obj.subindices:
            yield (
                sub.data_type or obj.data_type,
                sub.default_value or obj.default_value,
                sub.subindex,
            )
    else:
        yield obj.data_type, obj.default_value, 0
