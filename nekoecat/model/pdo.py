"""
PDO / SM / FMMU / DC / Identity data models.

Pure data containers — no domain logic.
"""
from typing import List, Optional
from pydantic import BaseModel, Field


class PdoEntry(BaseModel):
    """A single entry within a PDO mapping."""
    index: int
    subindex: int = 0
    name: str = ""
    bit_length: int = 0
    data_type: str = ""


class PdoDefinition(BaseModel):
    """A PDO definition (RxPDO or TxPDO)."""
    index: int
    name: str = ""
    sm_index: Optional[int] = None
    fixed: bool = False
    mandatory: bool = False
    entries: List[PdoEntry] = Field(default_factory=list)

    @property
    def total_bit_length(self) -> int:
        return sum(e.bit_length for e in self.entries)

    @property
    def total_byte_length(self) -> int:
        return (self.total_bit_length + 7) // 8


class SmDefinition(BaseModel):
    """Sync Manager definition."""
    index: int
    name: str = ""
    start_address: int = 0
    control_byte: int = 0
    default_size: int = 0
    enable: bool = True
    pdo_type: str = ""            # "mboxout", "mboxin", "outputs", "inputs"
    pdos: List[PdoDefinition] = Field(default_factory=list)

    @property
    def total_byte_length(self) -> int:
        return sum(p.total_byte_length for p in self.pdos)


class FmmuDefinition(BaseModel):
    """FMMU definition."""
    index: int
    usage: str = ""               # "outputs", "inputs", "mboxstate"


class DcDefinition(BaseModel):
    """Distributed Clock configuration."""
    assign_activate: int = 0
    sync0_cycle: int = 0
    sync0_shift: int = 0
    sync1_cycle: int = 0
    sync1_shift: int = 0


class DeviceIdentity(BaseModel):
    """Vendor / Product identification."""
    vendor_id: int = 0
    vendor_name: str = ""
    product_code: int = 0
    revision_number: int = 0
    serial_number: int = 0
    device_name: str = ""
    device_type: int = 0
    hardware_version: str = ""
    software_version: str = ""
