"""
Merger — combines ESI and SDO DeviceModels into a unified model.

Strategy: SDO objects take precedence for data_type/access/default_value
(because SDO is the detailed source), while ESI provides PDO/SM/DC/FMMU config.

Only depends on model/.
"""
from rich.console import Console

from nekoecat.model import DeviceModel, ObjectEntry, Issue, Severity, RuleCode

console = Console()


def merge(esi_device: DeviceModel, sdo_device: DeviceModel) -> DeviceModel:
    """Merge ESI and SDO into a single DeviceModel.

    - Identity: prefer ESI (it has vendor info).
    - Objects: merge by index; SDO data wins for detailed fields.
    - PDO/SM/DC/FMMU: from ESI (SDO usually doesn't carry these).
    - Issues: combined from both.
    """
    merged = DeviceModel()

    # ── identity (ESI preferred) ───────────────
    merged.identity = esi_device.identity

    # ── objects ────────────────────────────────
    all_indices = set(esi_device.objects.keys()) | set(sdo_device.objects.keys())

    for idx in sorted(all_indices):
        esi_obj = esi_device.objects.get(idx)
        sdo_obj = sdo_device.objects.get(idx)

        if esi_obj and sdo_obj:
            obj = _merge_object(esi_obj, sdo_obj)
        elif esi_obj:
            obj = esi_obj.model_copy(update={"source": "merged"})
        else:
            obj = sdo_obj.model_copy(update={"source": "merged"})

        merged.objects[idx] = obj

    # ── transport config (ESI only) ────────────
    merged.sm_configs = esi_device.sm_configs
    merged.fmmu_configs = esi_device.fmmu_configs
    merged.dc_config = esi_device.dc_config
    merged.rxpdo_assign = esi_device.rxpdo_assign
    merged.txpdo_assign = esi_device.txpdo_assign

    # ── issues from both ───────────────────────
    merged.issues = esi_device.issues + sdo_device.issues

    console.print(
        f"[green]Merged:[/green] {len(merged.objects)} objects, "
        f"{len(merged.issues)} inherited issues"
    )
    return merged


def _merge_object(esi: ObjectEntry, sdo: ObjectEntry) -> ObjectEntry:
    """Merge a single object where both ESI and SDO have data.

    SDO wins for: data_type, access, default_value, subindices.
    ESI wins for: name (if SDO name is empty), bit_length.
    """
    data_type = sdo.data_type or esi.data_type
    access = sdo.access or esi.access
    default_value = sdo.default_value or esi.default_value
    name = esi.name or sdo.name
    bit_length = sdo.bit_length or esi.bit_length

    # Prefer SDO subindices (more detailed)
    subindices = sdo.subindices if sdo.subindices else esi.subindices

    return ObjectEntry(
        index=esi.index,
        name=name,
        object_type=max(esi.object_type, sdo.object_type),
        data_type=data_type,
        bit_length=bit_length,
        access=access,
        default_value=default_value,
        subindices=subindices,
        coe_read=sdo.coe_read if sdo.coe_read is not None else esi.coe_read,
        coe_write=sdo.coe_write if sdo.coe_write is not None else esi.coe_write,
        backup=sdo.backup or esi.backup,
        setting=sdo.setting or esi.setting,
        pdo_mapping=sdo.pdo_mapping or esi.pdo_mapping,
        source="merged",
    )
