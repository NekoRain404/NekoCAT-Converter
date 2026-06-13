"""
Object / SubIndex data model for EtherCAT object dictionary.

Pure data containers. Business logic (classification, validation)
lives in the engine layer.
"""
from typing import Optional, List
from pydantic import BaseModel, Field


class SubIndex(BaseModel):
    """A single subindex entry in the object dictionary."""
    subindex: int = 0
    name: str = ""
    data_type: str = ""
    bit_length: int = 0
    access: str = ""              # "ro", "rw", "wo", "const"
    default_value: str = ""
    value: str = ""
    pdo_mapping: str = ""         # "optional", "default", "not mappable"
    unit: str = ""
    description: str = ""

    # SSC xlsx flags
    coe_read: Optional[bool] = None
    coe_write: Optional[bool] = None
    backup: bool = False
    setting: bool = False

    # PDO direction (filled by merger from ESI PDO info)
    pdo_direction: str = ""       # "", "rx", "tx"

    # Tracking (set by fix_engine)
    skipped: bool = False
    auto_fixed: bool = False
    original_data_type: Optional[str] = None


class ObjectEntry(BaseModel):
    """A single object in the EtherCAT object dictionary."""
    index: int
    name: str = ""
    object_type: int = 7          # 7=VAR, 8=ARRAY, 9=RECORD
    data_type: str = ""
    bit_length: int = 0
    access: str = ""
    category: str = ""            # filled by engine/classifier
    subindices: List[SubIndex] = Field(default_factory=list)

    # SSC xlsx flags
    coe_read: Optional[bool] = None
    coe_write: Optional[bool] = None
    backup: bool = False
    setting: bool = False
    pdo_mapping: str = ""
    default_value: str = ""
    value: str = ""

    # Source tracking
    source: str = ""              # "esi", "sdo", "merged"

    # Tracking (set by fix_engine)
    skipped: bool = False
    auto_fixed: bool = False
    original_data_type: Optional[str] = None

    # ── classifier flags (set by engine/classifier, dev doc §9) ──
    is_pdo_mapping_object: bool = False   # 0x1600-0x17FF, 0x1A00-0x1BFF
    is_sm_object: bool = False            # 0x1C00, 0x1C12, 0x1C13, 0x1C32, 0x1C33
    risk_flags: List[str] = Field(default_factory=list)

    # ── convenience properties (read-only, no side effects) ──

    @property
    def is_array(self) -> bool:
        return self.object_type == 8

    @property
    def is_record(self) -> bool:
        return self.object_type == 9

    @property
    def is_variable(self) -> bool:
        return self.object_type == 7

    @property
    def total_bit_length(self) -> int:
        if self.subindices:
            return sum(si.bit_length for si in self.subindices)
        return self.bit_length
