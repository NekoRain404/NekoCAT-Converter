"""
ESI XML parser — reads EtherCAT Slave Information XML into DeviceModel.

Only depends on: model/, utils, constants.  No imports from engine/generator.
"""
from pathlib import Path
from typing import Optional
from rich.console import Console

from nekoecat.model import (
    DeviceModel, ObjectEntry, SubIndex,
    PdoDefinition, PdoEntry, SmDefinition,
    FmmuDefinition, DcDefinition, DeviceIdentity,
    Issue, Severity, RuleCode,
)
from nekoecat import utils
from nekoecat.constants import SM_INDEX_TO_TYPE

console = Console()


# ──────────────────────────────────────────────
# Parser
# ──────────────────────────────────────────────

class EsiParser:
    """Stateful ESI parser — call ``parse(path)`` to get a DeviceModel."""

    def parse(self, filepath: str) -> DeviceModel:
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"ESI file not found: {path}")

        root = utils.parse_xml_file(str(path))
        device = DeviceModel()

        device_el = self._find_device(root)
        if device_el is None:
            device.add_issue(Issue(
                code=RuleCode.PARSE_ERROR,
                severity=Severity.FATAL,
                message="No <Device> element found in ESI XML",
            ))
            return device

        self._fill_identity(device_el, device)
        self._fill_sm_configs(device_el, device)
        self._fill_pdos(device_el, device)
        self._fill_dc(device_el, device)
        self._fill_fmmu(device_el, device)

        console.print(
            f"[green]ESI parsed:[/green] "
            f"{len(device.objects)} objects, "
            f"{len(device.sm_configs)} SMs, "
            f"{len(device.issues)} issues"
        )
        return device

    # ── locate <Device> ────────────────────────

    @staticmethod
    def _find_device(root):
        for candidate in utils.xml_findall(root, ".//Device"):
            return candidate
        return None

    # ── identity ───────────────────────────────

    @staticmethod
    def _fill_identity(device_el, device: DeviceModel):
        ident = device.identity

        vendor_el = utils.xml_find(device_el, "Vendor")
        if vendor_el is not None:
            ident.vendor_id = utils.to_int(utils.xml_text(utils.xml_find(vendor_el, "Id")))
            ident.vendor_name = utils.xml_text(utils.xml_find(vendor_el, "Name"))

        type_el = utils.xml_find(device_el, "Type")
        if type_el is not None:
            ident.product_code = utils.to_int(utils.xml_attr(type_el, "ProductCode"))
            ident.revision_number = utils.to_int(utils.xml_attr(type_el, "RevisionNo"))
            ident.device_type = ident.product_code
            ident.device_name = utils.xml_text(type_el)

        name_el = utils.xml_find(device_el, "Name")
        if name_el is not None and utils.xml_text(name_el):
            ident.device_name = utils.xml_text(name_el)

    # ── Sync Managers ──────────────────────────

    @staticmethod
    def _fill_sm_configs(device_el, device: DeviceModel):
        seen_indices: set[int] = set()

        # SMs can appear under <DL> or directly under <Device>
        for container in [device_el, utils.xml_find(device_el, "DL")]:
            if container is None:
                continue
            for sm_el in utils.xml_findall(container, "Sm"):
                sm_idx = utils.to_int(utils.xml_attr(sm_el, "SmIndex"), -1)
                if sm_idx < 0:
                    continue
                if sm_idx in seen_indices:
                    continue
                seen_indices.add(sm_idx)

                device.sm_configs.append(SmDefinition(
                    index=sm_idx,
                    start_address=utils.to_int(utils.xml_attr(sm_el, "StartAddress")),
                    control_byte=utils.to_int(utils.xml_attr(sm_el, "ControlByte")),
                    default_size=utils.to_int(utils.xml_attr(sm_el, "DefaultSize")),
                    enable=utils.xml_attr(sm_el, "Enable", "1") != "0",
                    pdo_type=SM_INDEX_TO_TYPE.get(sm_idx, ""),
                ))

    # ── PDOs ───────────────────────────────────

    def _fill_pdos(self, device_el, device: DeviceModel):
        for tag, direction in [("RxPDO", "rx"), ("TxPDO", "tx")]:
            for pdo_el in utils.xml_findall(device_el, tag):
                pdo = self._parse_pdo(pdo_el)
                if pdo is None:
                    continue
                if direction == "rx":
                    device.rxpdo_assign.append(pdo.index)
                else:
                    device.txpdo_assign.append(pdo.index)
                # Populate object entries referenced by this PDO
                self._register_pdo_entries(pdo, device)

    @staticmethod
    def _parse_pdo(pdo_el) -> Optional[PdoDefinition]:
        idx_el = utils.xml_find(pdo_el, "Index")
        pdo_idx = utils.to_int(
            utils.xml_text(idx_el) if idx_el is not None else utils.xml_attr(pdo_el, "Index")
        )
        if pdo_idx == 0:
            return None

        name = utils.xml_text(utils.xml_find(pdo_el, "Name"))
        sm_ref = utils.to_int(utils.xml_attr(pdo_el, "Sm"), -1)
        fixed = utils.xml_attr(pdo_el, "Fixed") == "1"
        mandatory = utils.xml_attr(pdo_el, "Mandatory") == "1"

        pdo = PdoDefinition(
            index=pdo_idx, name=name,
            sm_index=sm_ref if sm_ref >= 0 else None,
            fixed=fixed, mandatory=mandatory,
        )

        for entry_el in utils.xml_findall(pdo_el, "Entry"):
            pdo.entries.append(PdoEntry(
                index=utils.to_int(utils.xml_text(utils.xml_find(entry_el, "Index"))),
                subindex=utils.to_int(utils.xml_text(utils.xml_find(entry_el, "SubIndex"))),
                name=utils.xml_text(utils.xml_find(entry_el, "Name")),
                bit_length=utils.to_int(utils.xml_text(utils.xml_find(entry_el, "BitLen"))),
                data_type=utils.xml_text(utils.xml_find(entry_el, "DataType")),
            ))

        return pdo

    @staticmethod
    def _register_pdo_entries(pdo: PdoDefinition, device: DeviceModel):
        """Ensure every object referenced by the PDO exists in the dictionary."""
        for entry in pdo.entries:
            if entry.index == 0:
                continue
            if entry.index not in device.objects:
                device.objects[entry.index] = ObjectEntry(
                    index=entry.index,
                    name=entry.name,
                    data_type=entry.data_type,
                    bit_length=entry.bit_length,
                    source="esi",
                )

    # ── Distributed Clock ──────────────────────

    @staticmethod
    def _fill_dc(device_el, device: DeviceModel):
        dc_el = utils.xml_find(device_el, "Dc")
        if dc_el is None:
            return

        opmode = utils.xml_find(dc_el, "OpMode")
        source = opmode if opmode is not None else dc_el

        device.dc_config = DcDefinition(
            assign_activate=utils.to_int(utils.xml_text(utils.xml_find(source, "AssignActivate"))),
            sync0_cycle=utils.to_int(utils.xml_text(utils.xml_find(source, "CycleTimeSync0"))),
            sync0_shift=utils.to_int(utils.xml_text(utils.xml_find(source, "ShiftTimeSync0"))),
            sync1_cycle=utils.to_int(utils.xml_text(utils.xml_find(source, "CycleTimeSync1"))),
            sync1_shift=utils.to_int(utils.xml_text(utils.xml_find(source, "ShiftTimeSync1"))),
        )

    # ── FMMU ───────────────────────────────────

    @staticmethod
    def _fill_fmmu(device_el, device: DeviceModel):
        for i, fmmu_el in enumerate(utils.xml_findall(device_el, "Fmmu")):
            usage = utils.xml_attr(fmmu_el, "Usage") or utils.xml_text(fmmu_el)
            device.fmmu_configs.append(FmmuDefinition(index=i, usage=usage))


# ──────────────────────────────────────────────
# Public convenience function
# ──────────────────────────────────────────────

def parse_esi(filepath: str) -> DeviceModel:
    """Parse an ESI XML file and return a populated DeviceModel."""
    return EsiParser().parse(filepath)
