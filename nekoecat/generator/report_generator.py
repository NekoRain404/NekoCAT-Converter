"""
Report generator — writes Markdown + CSV reports about the conversion.

Only depends on: model/.  No parser/engine imports.
"""
import csv
from datetime import datetime
from pathlib import Path
from rich.console import Console

from nekoecat.model import DeviceModel, Severity
from nekoecat.constants import COMM_OBJECT_RANGE

console = Console()


def generate_report(device: DeviceModel, output_dir: str) -> dict[str, str]:
    """Generate all report files. Returns dict of {name: path}."""
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)

    files: dict[str, str] = {}
    files["report"] = _write_report_md(device, out)
    files["pdo_layout"] = _write_pdo_csv(device, out)
    files["callback_list"] = _write_callback_csv(device, out)
    files["skipped_objects"] = _write_skipped_csv(device, out)

    console.print(f"[green]Reports generated:[/green] {len(files)} files in {out}")
    return files


# ──────────────────────────────────────────────
# report.md
# ──────────────────────────────────────────────

def _write_report_md(device: DeviceModel, out: Path) -> str:
    path = out / "report.md"
    ident = device.identity
    lines = [
        f"# NekoECAT Conversion Report",
        f"",
        f"Generated: {datetime.now().isoformat()}",
        f"",
        f"## Device Identity",
        f"",
        f"| Field | Value |",
        f"|-------|-------|",
        f"| Vendor | {ident.vendor_name} (0x{ident.vendor_id:08X}) |",
        f"| Product Code | 0x{ident.product_code:08X} |",
        f"| Revision | 0x{ident.revision_number:08X} |",
        f"| Name | {ident.device_name} |",
        f"",
        f"## Statistics",
        f"",
        f"| Metric | Count |",
        f"|--------|-------|",
        f"| Total objects | {len(device.objects)} |",
        f"| Comm objects (filtered) | {_count_comm(device)} |",
        f"| Application objects | {len(device.objects) - _count_comm(device)} |",
        f"| SM configs | {len(device.sm_configs)} |",
        f"| RxPDO assignments | {len(device.rxpdo_assign)} |",
        f"| TxPDO assignments | {len(device.txpdo_assign)} |",
        f"| Issues total | {device.issue_count()} |",
        f"| Issues fixed | {device.fixed_count()} |",
        f"",
        f"## Issues",
        f"",
        f"| Severity | Code | Object | Message | Fixed |",
        f"|----------|------|--------|---------|-------|",
    ]
    for issue in device.issues:
        idx = f"0x{issue.object_index:04X}" if issue.object_index is not None else "-"
        lines.append(
            f"| {issue.severity.value} | {issue.code.value} | {idx} | "
            f"{issue.message} | {'✓' if issue.fixed else ''} |"
        )

    path.write_text("\n".join(lines), encoding="utf-8")
    return str(path)


# ──────────────────────────────────────────────
# pdo_layout.csv
# ──────────────────────────────────────────────

def _write_pdo_csv(device: DeviceModel, out: Path) -> str:
    path = out / "pdo_layout.csv"
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Type", "PDO_Index", "PDO_Name", "Entry_Index", "SubIndex",
                     "Entry_Name", "BitLength", "DataType"])
        for sm in device.sm_configs:
            for pdo in sm.pdos:
                for entry in pdo.entries:
                    w.writerow([
                        sm.pdo_type, f"0x{pdo.index:04X}", pdo.name,
                        f"0x{entry.index:04X}", entry.subindex,
                        entry.name, entry.bit_length, entry.data_type,
                    ])
    return str(path)


# ──────────────────────────────────────────────
# callback_list.csv
# ──────────────────────────────────────────────

def _write_callback_csv(device: DeviceModel, out: Path) -> str:
    path = out / "callback_list.csv"
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Index", "Name", "DataType", "CoeRead", "CoeWrite"])
        for obj in device.sorted_objects():
            if obj.coe_read is True or obj.coe_write is True:
                w.writerow([
                    f"0x{obj.index:04X}", obj.name, obj.data_type,
                    obj.coe_read, obj.coe_write,
                ])
    return str(path)


# ──────────────────────────────────────────────
# skipped_objects.csv
# ──────────────────────────────────────────────

def _write_skipped_csv(device: DeviceModel, out: Path) -> str:
    path = out / "skipped_objects.csv"
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Index", "Name", "Reason"])
        for issue in device.issues:
            if issue.code.value in ("OBJECT_SKIPPED", "COMM_OBJECT_FILTERED"):
                idx = f"0x{issue.object_index:04X}" if issue.object_index is not None else ""
                w.writerow([idx, "", issue.message])
    return str(path)


# ──────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────

def _count_comm(device: DeviceModel) -> int:
    return sum(1 for idx in device.objects if idx in COMM_OBJECT_RANGE)
