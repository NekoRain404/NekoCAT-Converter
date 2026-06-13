"""
Object classifier — assigns category and object_type to objects.

Pure function layer: takes a DeviceModel, mutates category fields in-place.
Only depends on model/ and constants.
"""
from nekoecat.model import DeviceModel, ObjectEntry
from nekoecat.constants import COMM_OBJECT_RANGE, MFG_OBJECT_RANGE, DEVICE_PROFILE_RANGE


def classify_objects(device: DeviceModel) -> None:
    """Walk every object and set its ``category`` field."""
    for obj in device.objects.values():
        obj.category = _classify_one(obj)


def _classify_one(obj: ObjectEntry) -> str:
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
