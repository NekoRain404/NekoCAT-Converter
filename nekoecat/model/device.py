"""
Unified EtherCAT device model — the single source of truth after parsing.

Contains convenience accessors, but no business logic (no validation,
no fixing, no classification). That belongs in the engine layer.
"""
from typing import Dict, List, Optional
from pydantic import BaseModel, Field
from .object_dict import ObjectEntry, SubIndex
from .pdo import (
    PdoDefinition, SmDefinition, FmmuDefinition,
    DcDefinition, DeviceIdentity,
)
from .rules import Issue


class DeviceModel(BaseModel):
    """Complete representation of one EtherCAT slave device."""

    identity: DeviceIdentity = Field(default_factory=DeviceIdentity)

    # Object dictionary keyed by index
    objects: Dict[int, ObjectEntry] = Field(default_factory=dict)

    # Sync Managers (SM0-SM3 typically)
    sm_configs: List[SmDefinition] = Field(default_factory=list)

    # FMMU configurations
    fmmu_configs: List[FmmuDefinition] = Field(default_factory=list)

    # Distributed Clock
    dc_config: Optional[DcDefinition] = None

    # RxPDO / TxPDO assignment lists (PDO indices)
    rxpdo_assign: List[int] = Field(default_factory=list)
    txpdo_assign: List[int] = Field(default_factory=list)

    # Issues found during any processing stage
    issues: List[Issue] = Field(default_factory=list)

    # ── convenience accessors (read-only, no cross-layer coupling) ──

    def sorted_objects(self) -> List[ObjectEntry]:
        """Return objects sorted by index ascending."""
        return [self.objects[i] for i in sorted(self.objects.keys())]

    def add_object(self, obj: ObjectEntry) -> None:
        """Insert or replace an object by its index."""
        self.objects[obj.index] = obj

    def get_object(self, index: int) -> Optional[ObjectEntry]:
        """Look up an object by index, or None."""
        return self.objects.get(index)

    def add_issue(self, issue: Issue) -> None:
        self.issues.append(issue)

    def issue_count(self, severity: Optional[str] = None) -> int:
        if severity:
            return sum(1 for i in self.issues if i.severity == severity)
        return len(self.issues)

    def fixed_count(self) -> int:
        return sum(1 for i in self.issues if i.fixed)
