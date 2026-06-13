"""
Issue and severity definitions for the rule engine layer.

Pure data types — no logic, no imports from other nekoecat modules.
"""
from enum import Enum
from typing import Optional
from pydantic import BaseModel


class Severity(str, Enum):
    """Issue severity levels."""
    INFO = "info"
    WARNING = "warning"
    ERROR = "error"
    FATAL = "fatal"


class RuleCode(str, Enum):
    """Unique codes for every rule the engine can flag."""
    # ── Boolean ──
    BOOL_DEFAULT_INVALID = "BOOL_DEFAULT_INVALID"
    # ── OCTET_STRING ──
    OCTET_STRING_UNSUPPORTED = "OCTET_STRING_UNSUPPORTED"
    # ── Word alignment ──
    WORD_ALIGNMENT_ERROR = "WORD_ALIGNMENT_ERROR"
    # ── Callback ──
    CALLBACK_ENTRY_LEVEL = "CALLBACK_ENTRY_LEVEL"
    CALLBACK_MISSING = "CALLBACK_MISSING"
    # ── PDO / SM ──
    PDO_LENGTH_MISMATCH = "PDO_LENGTH_MISMATCH"
    SM_LENGTH_MISMATCH = "SM_LENGTH_MISMATCH"
    # ── Object issues ──
    UNSUPPORTED_DATA_TYPE = "UNSUPPORTED_DATA_TYPE"
    OBJECT_SKIPPED = "OBJECT_SKIPPED"
    COMM_OBJECT_FILTERED = "COMM_OBJECT_FILTERED"
    MISSING_DATA_TYPE = "MISSING_DATA_TYPE"
    INDEX_OUT_OF_RANGE = "INDEX_OUT_OF_RANGE"
    # ── General ──
    PARSE_ERROR = "PARSE_ERROR"
    MERGE_CONFLICT = "MERGE_CONFLICT"


class Issue(BaseModel):
    """A single diagnostic issue attached to a DeviceModel."""
    code: RuleCode
    severity: Severity
    message: str
    object_index: Optional[int] = None
    subindex: Optional[int] = None
    auto_fixable: bool = False
    fixed: bool = False
