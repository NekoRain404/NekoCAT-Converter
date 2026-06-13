"""
Report generator — writes Markdown + CSV reports about the conversion.

Uses Jinja2 template (templates/report.md.j2) for the main report.
Only depends on: model/.  No parser/engine imports.
"""
import csv
from datetime import datetime
from pathlib import Path
from rich.console import Console
from jinja2 import Environment, FileSystemLoader

from nekoecat.model import DeviceModel, Severity
from nekoecat.constants import COMM_OBJECT_RANGE

console = Console()

_TEMPLATE_DIR = Path(__file__).resolve().parent.parent.parent / "templates"


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
# report.md (via Jinja2 template)
# ──────────────────────────────────────────────

def _write_report_md(device: DeviceModel, out: Path) -> str:
    path = out / "report.md"

    env = Environment(
        loader=FileSystemLoader(str(_TEMPLATE_DIR)),
        keep_trailing_newline=True,
    )
    tpl = env.get_template("report.md.j2")

    # Prepare template context
    comm_count = _count_comm(device)
    app_count = len(device.objects) - comm_count
    callback_objects = [
        obj for obj in device.sorted_objects()
        if obj.coe_read is True or obj.coe_write is True
    ]

    from nekoecat import __version__
    content = tpl.render(
        device=device,
        timestamp=datetime.now().isoformat(),
        version=__version__,
        total_objects=len(device.objects),
        comm_count=comm_count,
        app_count=app_count,
        sm_count=len(device.sm_configs),
        rxpdo_count=len(device.rxpdo_assign),
        txpdo_count=len(device.txpdo_assign),
        issue_count=device.issue_count(),
        fixed_count=device.fixed_count(),
        issues=device.issues,
        callback_objects=callback_objects,
    )

    path.write_text(content, encoding="utf-8")
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
