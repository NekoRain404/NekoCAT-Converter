"""
nekoecat.generator — Output writers: xlsx, ESI, report, code, CSV.

Public API:
    generate_ssc_xlsx(device, output_path, template_path) → str
    generate_esi(device, output_path) → str
    generate_report(device, output_dir) → dict[str, str]
    generate_code(device, output_dir) → dict[str, str]
"""
from .ssc_xlsx_generator import generate_ssc_xlsx
from .esi_generator import generate_esi
from .report_generator import generate_report
from .code_generator import generate_code

__all__ = [
    "generate_ssc_xlsx",
    "generate_esi",
    "generate_report",
    "generate_code",
]
