"""
Fix engine — automatically repairs flagged issues in a DeviceModel.

Each fix_* function:
  1. Finds matching issues (by RuleCode)
  2. Applies the data mutation
  3. Marks the issue as fixed

Dev doc §17 rules: BOOL, OCTET_STRING, word alignment, callback placement.

Only depends on: model/.  No parser/generator imports.
"""
from rich.console import Console

from nekoecat.model import DeviceModel, Issue, Severity, RuleCode

console = Console()


def auto_fix(device: DeviceModel) -> None:
    """Run all auto-fixers on the device."""
    _fix_boolean_defaults(device)
    _fix_octet_strings(device)
    _fix_word_alignment(device)
    _fix_callback_placement(device)
    fixed = device.fixed_count()
    console.print(f"[yellow]Auto-fix applied:[/yellow] {fixed} issues fixed")


# ──────────────────────────────────────────────
# 1. BOOLEAN: 0 → FALSE, 1 → TRUE (§17.1)
# ──────────────────────────────────────────────

def _fix_boolean_defaults(device: DeviceModel):
    BOOL_MAP = {"0": "FALSE", "1": "TRUE", "false": "FALSE", "true": "TRUE"}

    for issue in device.issues:
        if issue.code != RuleCode.BOOL_DEFAULT_INVALID or issue.fixed:
            continue
        obj = device.get_object(issue.object_index)
        if obj is None:
            continue

        if issue.subindex and issue.subindex > 0:
            for sub in obj.subindices:
                if sub.subindex == issue.subindex:
                    raw = sub.default_value.strip()
                    if raw in BOOL_MAP:
                        sub.default_value = BOOL_MAP[raw]
                        sub.auto_fixed = True
                        issue.fixed = True
        else:
            raw = obj.default_value.strip()
            if raw in BOOL_MAP:
                obj.default_value = BOOL_MAP[raw]
                issue.fixed = True


# ──────────────────────────────────────────────
# 2. OCTET_STRING → STRING + CoE callback (§17.2)
# ──────────────────────────────────────────────

def _fix_octet_strings(device: DeviceModel):
    for issue in device.issues:
        if issue.code != RuleCode.OCTET_STRING_UNSUPPORTED or issue.fixed:
            continue
        obj = device.get_object(issue.object_index)
        if obj is None:
            continue

        if issue.subindex and issue.subindex > 0:
            for sub in obj.subindices:
                if sub.subindex == issue.subindex:
                    sub.original_data_type = sub.data_type
                    sub.data_type = _rename_to_string(sub.data_type)
                    # Dev doc §10.2: callback on OBJECT, not entry
                    obj.coe_read = True
                    if sub.access in ("rw", "wo"):
                        obj.coe_write = True
                    sub.auto_fixed = True
                    issue.fixed = True
        else:
            obj.original_data_type = obj.data_type
            obj.data_type = _rename_to_string(obj.data_type)
            obj.coe_read = True
            obj.coe_write = True
            issue.fixed = True


def _rename_to_string(data_type: str) -> str:
    """OCTET_STRING(n) → STRING(n), UNICODE_STRING(n) → STRING(n)."""
    if "(" in data_type:
        size_part = data_type[data_type.index("("):]
        return f"STRING{size_part}"
    return "STRING"


# ──────────────────────────────────────────────
# 3. Word alignment → object-level CoE callback (§17.3)
# ──────────────────────────────────────────────

def _fix_word_alignment(device: DeviceModel):
    """If a ≥16bit entry is not word-aligned, set object-level CoE callback."""
    for issue in device.issues:
        if issue.code != RuleCode.WORD_ALIGNMENT_ERROR or issue.fixed:
            continue
        obj = device.get_object(issue.object_index)
        if obj is None:
            continue

        # Dev doc §17.3: set callback on object, not entry
        obj.coe_read = True
        if obj.access in ("rw", "wo"):
            obj.coe_write = True
        issue.fixed = True


# ──────────────────────────────────────────────
# 4. Move CoE flags from subindex to object level (§17.4)
# ──────────────────────────────────────────────

def _fix_callback_placement(device: DeviceModel):
    for issue in device.issues:
        if issue.code != RuleCode.CALLBACK_ENTRY_LEVEL or issue.fixed:
            continue
        obj = device.get_object(issue.object_index)
        if obj is None:
            continue

        for sub in obj.subindices:
            if sub.subindex == issue.subindex:
                if sub.coe_read is True:
                    obj.coe_read = True
                    sub.coe_read = None
                if sub.coe_write is True:
                    obj.coe_write = True
                    sub.coe_write = None
                sub.auto_fixed = True

        issue.fixed = True
