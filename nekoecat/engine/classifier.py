"""
Object classifier — assigns category, flags, and risk markers to objects.

Implements dev doc §9:
  - Communication objects: 0x1000-0x1FFF
  - PDO mapping objects:   0x1600-0x17FF, 0x1A00-0x1BFF
  - SM objects:            0x1C00, 0x1C12, 0x1C13, 0x1C32, 0x1C33
  - High-profile/vendor:   0xF000-0xFFFF

Pure function layer: mutates fields in-place.
Only depends on model/ and constants.
"""
from nekoecat.model import DeviceModel, ObjectEntry
from nekoecat.constants import COMM_OBJECT_RANGE, MFG_OBJECT_RANGE, DEVICE_PROFILE_RANGE

# Specific SM object indices
_SM_OBJECTS = {0x1C00, 0x1C12, 0x1C13, 0x1C32, 0x1C33}

# PDO mapping ranges
_RXPDO_MAP_RANGE = range(0x1600, 0x1800)   # 0x1600-0x17FF
_TXPDO_MAP_RANGE = range(0x1A00, 0x1C00)   # 0x1A00-0x1BFF


def classify_objects(device: DeviceModel) -> None:
    """Walk every object and set category + classifier flags."""
    for obj in device.objects.values():
        obj.category = _classify_category(obj)
        obj.is_pdo_mapping_object = _is_pdo_mapping(obj)
        obj.is_sm_object = _is_sm_object(obj)
        _set_risk_flags(obj)


def _classify_category(obj: ObjectEntry) -> str:
    idx = obj.index
    if idx in COMM_OBJECT_RANGE:
        return "comm"
    if idx in MFG_OBJECT_RANGE:
        return "mfg"
    if idx in DEVICE_PROFILE_RANGE:
        return "standard"
    if idx < 0x1000:
        return "reserved"
    return "other"


def _is_pdo_mapping(obj: ObjectEntry) -> bool:
    """PDO mapping objects: 0x1600-0x17FF (Rx), 0x1A00-0x1BFF (Tx)."""
    return obj.index in _RXPDO_MAP_RANGE or obj.index in _TXPDO_MAP_RANGE


def _is_sm_object(obj: ObjectEntry) -> bool:
    """SM configuration objects: 0x1C00, 0x1C12, 0x1C13, 0x1C32, 0x1C33."""
    return obj.index in _SM_OBJECTS


def _set_risk_flags(obj: ObjectEntry) -> None:
    """Set risk flags per dev doc §9."""
    obj.risk_flags.clear()
    if obj.index >= 0xF000:
        obj.risk_flags.append("HIGH_PROFILE_OR_VENDOR_OBJECT")
