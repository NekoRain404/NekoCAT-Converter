"""
nekoecat.engine — Business logic: classify, merge, check rules, fix, validate.

Public API:
    classify_objects(device)
    merge(esi_device, sdo_device) → DeviceModel
    run_rules(device)
    auto_fix(device)
    validate(device) → bool
    apply_ssc_log_fixes(device, log_result) → int
"""
from .classifier import classify_objects
from .merger import merge
from .rule_engine import run_rules
from .fix_engine import auto_fix
from .validator import validate
from .ssc_fix_engine import apply_ssc_log_fixes

__all__ = [
    "classify_objects",
    "merge",
    "run_rules",
    "auto_fix",
    "validate",
    "apply_ssc_log_fixes",
]
