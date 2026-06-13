"""
nekoecat.parser — File readers that produce model objects.

Public API:
    parse_esi(path)           → DeviceModel
    parse_sdo(path)           → DeviceModel
    parse_template_xlsx(path) → dict
"""
from .esi_parser import parse_esi, EsiParser
from .sdo_parser import parse_sdo, SdoParser
from .template_parser import parse_template_xlsx

__all__ = [
    "parse_esi", "EsiParser",
    "parse_sdo", "SdoParser",
    "parse_template_xlsx",
]
