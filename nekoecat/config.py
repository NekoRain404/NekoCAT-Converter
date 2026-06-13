"""
ConvertConfig — configuration for a conversion run.

Follows dev doc §14: strategy options, identity overrides, mode selection.
"""
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class ConvertConfig:
    """Configuration for a single conversion run."""

    # ── input files ────────────────────────────
    esi_path: Optional[str] = None
    sdo_path: Optional[str] = None
    template_xlsx_path: Optional[str] = None
    output_dir: str = "out"

    # ── identity overrides ─────────────────────
    vendor_id: Optional[int] = None
    product_code: Optional[int] = None
    revision_number: Optional[int] = None
    device_name: Optional[str] = None
    physics: str = "YY"

    # ── mode (dev doc §12.2) ──────────────────
    mode: str = "learning"   # "learning" | "lab" | "product"

    # ── strategy options (dev doc §12.3) ──────
    octet_string_strategy: str = "string_with_callback"
    # "string_with_callback" | "skip" | "manual_callback"

    unaligned_strategy: str = "coe_callback"
    # "coe_callback" | "insert_padding" | "skip"

    communication_object_strategy: str = "ssc_auto"
    # "ssc_auto" | "force_import" | "skip"

    include_fxxx: bool = True

    # ── output options ─────────────────────────
    generate_xlsx: bool = True
    generate_esi: bool = False     # TwinCAT ESI (future)
    generate_report: bool = True
    generate_json: bool = True     # device.normalized.json
    generate_code: bool = False    # C code skeleton (future)
