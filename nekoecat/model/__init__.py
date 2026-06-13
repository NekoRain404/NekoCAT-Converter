"""
nekoecat.model — Pure data models.

Only re-exports the public model types. No business logic here.
"""
from .device import DeviceModel
from .object_dict import ObjectEntry, SubIndex
from .pdo import (
    PdoDefinition, PdoEntry, SmDefinition,
    FmmuDefinition, DcDefinition, DeviceIdentity,
)
from .rules import Issue, Severity, RuleCode

__all__ = [
    # Core device
    "DeviceModel",
    # Object dictionary
    "ObjectEntry", "SubIndex",
    # PDO / transport
    "PdoDefinition", "PdoEntry", "SmDefinition",
    "FmmuDefinition", "DcDefinition", "DeviceIdentity",
    # Diagnostics
    "Issue", "Severity", "RuleCode",
]
