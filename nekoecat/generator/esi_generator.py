"""
TwinCAT ESI XML generator — writes a DeviceModel into ESI XML format.

Only depends on: model/.  No parser/engine imports.
"""
from pathlib import Path
from typing import Optional
from lxml import etree
from rich.console import Console

from nekoecat.model import DeviceModel

console = Console()

# EtherCAT ESI namespace
_NS = "EtherCATInfo.xsd"
_NS_MAP = {"e": _NS}


def generate_esi(device: DeviceModel, output_path: str) -> str:
    """Generate a TwinCAT-compatible ESI XML file.

    Returns: Absolute path of the generated file.
    """
    output = Path(output_path)
    output.parent.mkdir(parents=True, exist_ok=True)

    root = _build_root(device)
    tree = etree.ElementTree(root)
    tree.write(
        str(output),
        pretty_print=True,
        xml_declaration=True,
        encoding="UTF-8",
    )

    console.print(f"[green]ESI generated:[/green] {output}")
    return str(output.resolve())


def _build_root(device: DeviceModel) -> etree._Element:
    """Build the full ESI XML tree."""
    root = etree.Element("EtherCATInfo", nsmap={None: _NS})

    # ── Vendor ────────────────────────────────
    vendor = etree.SubElement(root, "Vendor")
    vid = etree.SubElement(vendor, "Id")
    vid.text = str(device.identity.vendor_id)
    vname = etree.SubElement(vendor, "Name")
    vname.text = device.identity.vendor_name

    # ── Descriptions > Devices > Device ───────
    desc = etree.SubElement(root, "Descriptions")
    devices = etree.SubElement(desc, "Devices")

    dev_el = etree.SubElement(devices, "Device")
    type_el = etree.SubElement(dev_el, "Type")
    type_el.set("ProductCode", f"#x{device.identity.product_code:08X}")
    type_el.set("RevisionNo", f"#x{device.identity.revision_number:08X}")
    type_el.text = device.identity.device_name

    name_el = etree.SubElement(dev_el, "Name")
    name_el.text = device.identity.device_name

    # ── DL (Data Link) ────────────────────────
    dl = etree.SubElement(dev_el, "DL")
    for sm in device.sm_configs:
        sm_el = etree.SubElement(dl, "Sm")
        sm_el.set("SmIndex", str(sm.index))
        sm_el.set("StartAddress", f"#x{sm.start_address:04X}")
        sm_el.set("ControlByte", str(sm.control_byte))
        sm_el.set("DefaultSize", str(sm.default_size))
        sm_el.set("Enable", "1" if sm.enable else "0")

    # ── FMMU ──────────────────────────────────
    for fmmu in device.fmmu_configs:
        fmmu_el = etree.SubElement(dev_el, "Fmmu")
        fmmu_el.set("Usage", fmmu.usage)

    # ── RxPDO / TxPDO ─────────────────────────
    _add_pdo_elements(dev_el, device, "RxPDO", device.rxpdo_assign)
    _add_pdo_elements(dev_el, device, "TxPDO", device.txpdo_assign)

    # ── DC ────────────────────────────────────
    if device.dc_config:
        dc = etree.SubElement(dev_el, "Dc")
        opmode = etree.SubElement(dc, "OpMode")
        aa = etree.SubElement(opmode, "AssignActivate")
        aa.text = f"#x{device.dc_config.assign_activate:04X}"
        if device.dc_config.sync0_cycle:
            s0c = etree.SubElement(opmode, "CycleTimeSync0")
            s0c.text = str(device.dc_config.sync0_cycle)
        if device.dc_config.sync0_shift:
            s0s = etree.SubElement(opmode, "ShiftTimeSync0")
            s0s.text = str(device.dc_config.sync0_shift)

    return root


def _add_pdo_elements(
    dev_el: etree._Element,
    device: DeviceModel,
    tag: str,
    pdo_indices: list[int],
):
    """Add RxPDO or TxPDO elements with their entry lists."""
    for pdo_idx in pdo_indices:
        obj = device.get_object(pdo_idx)
        if obj is None:
            continue

        pdo_el = etree.SubElement(dev_el, tag)
        idx_el = etree.SubElement(pdo_el, "Index")
        idx_el.text = f"#x{pdo_idx:04X}"
        name_el = etree.SubElement(pdo_el, "Name")
        name_el.text = obj.name

        for sub in obj.subindices:
            entry_el = etree.SubElement(pdo_el, "Entry")
            si = etree.SubElement(entry_el, "Index")
            si.text = f"#x{pdo_idx:04X}"  # Reference the mapping object
            ssi = etree.SubElement(entry_el, "SubIndex")
            ssi.text = str(sub.subindex)
            bl = etree.SubElement(entry_el, "BitLen")
            bl.text = str(sub.bit_length)
            dt = etree.SubElement(entry_el, "DataType")
            dt.text = sub.data_type or obj.data_type
            ne = etree.SubElement(entry_el, "Name")
            ne.text = sub.name
