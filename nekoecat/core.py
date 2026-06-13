"""
nekoecat.core — Public facade for the conversion pipeline.

Orchestrates:  parser → engine → generator.
CLI/GUI call into this module only.

Usage:
    from nekoecat.core import convert, ConvertConfig

    config = ConvertConfig(esi_path="ESI.XML", sdo_path="SDO.XML")
    result = convert(config)
"""
import json
import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional
from datetime import datetime
from rich.console import Console

from nekoecat.config import ConvertConfig
from nekoecat.parser import parse_esi, parse_sdo
from nekoecat.engine import classify_objects, merge, run_rules, auto_fix, validate
from nekoecat.generator import generate_ssc_xlsx, generate_report
from nekoecat.model import DeviceModel

console = Console()


# ──────────────────────────────────────────────
# Result container
# ──────────────────────────────────────────────

@dataclass
class ConvertResult:
    """Outcome of a full conversion run."""
    success: bool = False
    output_dir: str = ""
    xlsx_path: str = ""
    report_files: dict[str, str] = field(default_factory=dict)
    device: Optional[DeviceModel] = None
    error: Optional[str] = None
    log_path: str = ""


# ──────────────────────────────────────────────
# Main pipeline
# ──────────────────────────────────────────────

def convert(config: ConvertConfig) -> ConvertResult:
    """Run the full conversion pipeline.

    Steps (dev doc §14):
        1. Parse ESI / SDO
        2. Merge
        3. Apply identity overrides
        4. Classify objects
        5. Run rule checks
        6. Auto-fix
        7. Validate
        8. Generate outputs (xlsx, report, json, log)
    """
    result = ConvertResult()

    if not config.esi_path and not config.sdo_path:
        result.error = "At least one of esi_path or sdo_path is required"
        return result

    # ── set up output dir ──────────────────────
    ts = datetime.now().strftime("%Y%m%d_%H%M")
    device_label = config.device_name or "device"
    out = Path(config.output_dir) / f"{device_label}_{ts}"
    out.mkdir(parents=True, exist_ok=True)
    result.output_dir = str(out)

    # ── set up logging ─────────────────────────
    log_path = out / "logs" / "convert.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    result.log_path = str(log_path)

    logger = _setup_logger(log_path)
    logger.info("NekoECAT Converter — conversion started")
    logger.info(f"Config: esi={config.esi_path}, sdo={config.sdo_path}")

    try:
        # ── 1. Parse ───────────────────────────
        console.rule("[bold blue]Parsing[/bold blue]")
        logger.info("Parsing ESI...")
        esi_device = parse_esi(config.esi_path) if config.esi_path else DeviceModel()
        logger.info(f"  ESI: {len(esi_device.objects)} objects")

        logger.info("Parsing SDO...")
        sdo_device = parse_sdo(config.sdo_path) if config.sdo_path else DeviceModel()
        logger.info(f"  SDO: {len(sdo_device.objects)} objects")

        # ── 2. Merge ───────────────────────────
        console.rule("[bold blue]Merging[/bold blue]")
        if config.esi_path and config.sdo_path:
            device = merge(esi_device, sdo_device)
        elif config.esi_path:
            device = esi_device
        else:
            device = sdo_device
        logger.info(f"Merged: {len(device.objects)} objects")

        # ── 3. Apply identity overrides ────────
        _apply_identity_overrides(device, config)

        # ── 4. Classify ────────────────────────
        classify_objects(device)

        # ── 5. Rules ───────────────────────────
        console.rule("[bold blue]Rule Engine[/bold blue]")
        run_rules(device)
        logger.info(f"Rules: {device.issue_count()} issues")

        # ── 6. Auto-fix ────────────────────────
        console.rule("[bold blue]Auto-Fix[/bold blue]")
        auto_fix(device)
        logger.info(f"Auto-fix: {device.fixed_count()} issues fixed")

        # ── 7. Validate ────────────────────────
        console.rule("[bold blue]Validation[/bold blue]")
        if not validate(device):
            result.error = "Validation failed — see issues in report"
            result.device = device
            logger.error("Validation failed")
            return result

        # ── 8. Generate outputs ────────────────
        console.rule("[bold blue]Generating outputs[/bold blue]")

        # SSC xlsx
        if config.generate_xlsx:
            xlsx_path = generate_ssc_xlsx(
                device,
                str(out / f"{device_label}_SSC_DESCRIPTION.xlsx"),
                template_path=config.template_xlsx_path,
            )
            result.xlsx_path = xlsx_path
            logger.info(f"Generated xlsx: {xlsx_path}")

        # Reports (always generate)
        if config.generate_report:
            report_files = generate_report(device, str(out))
            result.report_files = report_files
            logger.info(f"Reports: {list(report_files.keys())}")

        # device.normalized.json (dev doc §18)
        if config.generate_json:
            json_path = _write_normalized_json(device, out, device_label)
            result.report_files["normalized_json"] = json_path
            logger.info(f"Normalized JSON: {json_path}")

        result.success = True
        result.device = device

        console.rule("[bold green]Done[/bold green]")
        console.print(f"[green]Output:[/green] {out}")
        logger.info("Conversion completed successfully")

    except Exception as exc:
        result.error = str(exc)
        console.print_exception()
        logger.exception(f"Conversion failed: {exc}")

    return result


# ──────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────

def _apply_identity_overrides(device: DeviceModel, config: ConvertConfig):
    """Override device identity from config."""
    if config.vendor_id is not None:
        device.identity.vendor_id = config.vendor_id
    if config.product_code is not None:
        device.identity.product_code = config.product_code
    if config.revision_number is not None:
        device.identity.revision_number = config.revision_number
    if config.device_name is not None:
        device.identity.device_name = config.device_name


def _write_normalized_json(device: DeviceModel, out: Path, label: str) -> str:
    """Write device.normalized.json — used to reproduce issues (dev doc §18)."""
    path = out / f"{label}.normalized.json"
    data = device.model_dump(mode="json")
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    return str(path)


def _setup_logger(log_path: Path) -> logging.Logger:
    """Set up file logger for the conversion."""
    logger = logging.getLogger("nekoecat.convert")
    logger.setLevel(logging.DEBUG)
    # Clear existing handlers
    logger.handlers.clear()
    fh = logging.FileHandler(str(log_path), encoding="utf-8")
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(logging.Formatter(
        "%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    ))
    logger.addHandler(fh)
    return logger
