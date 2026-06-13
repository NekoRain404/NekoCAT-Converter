"""
Shared utility functions for XML traversal and number conversion.

No domain logic — only generic helpers used by parsers and generators.
"""
from typing import Optional
from lxml import etree

from .constants import COE_TYPE_HEX_TO_NAME, COE_TYPE_ALIASES


# ──────────────────────────────────────────────
# XML helpers
# ──────────────────────────────────────────────

# EtherCAT namespace (some ESI files use it, some don't)
_NS_MAP = {"e": "EtherCATInfo.xsd"}


def xml_find(element: etree._Element, tag: str) -> Optional[etree._Element]:
    """Find child element, trying with and without EtherCAT namespace."""
    el = element.find(tag, _NS_MAP)
    if el is None:
        el = element.find(tag)
    return el


def xml_findall(element: etree._Element, tag: str) -> list[etree._Element]:
    """Find all child elements, trying with and without namespace."""
    els = element.findall(tag, _NS_MAP)
    if not els:
        els = element.findall(tag)
    return els


def xml_text(element: Optional[etree._Element], default: str = "") -> str:
    """Extract stripped text content from an XML element."""
    if element is None:
        return default
    return (element.text or "").strip()


def xml_attr(element: Optional[etree._Element], name: str, default: str = "") -> str:
    """Extract an attribute value from an XML element."""
    if element is None:
        return default
    return element.get(name, default)


def parse_xml_file(filepath: str) -> etree._Element:
    """Parse an XML file and return its root element."""
    tree = etree.parse(filepath)
    return tree.getroot()


# ──────────────────────────────────────────────
# Number conversion helpers
# ──────────────────────────────────────────────

def to_int(value, default: int = 0) -> int:
    """Convert a value to int, handling hex strings (#x, 0x) and None."""
    if value is None:
        return default
    s = str(value).strip()
    if not s:
        return default
    try:
        if s.startswith("#x") or s.startswith("#X"):
            return int(s[2:], 16)
        if s.startswith("0x") or s.startswith("0X"):
            return int(s, 16)
        return int(s)
    except (ValueError, TypeError):
        return default


def to_hex(value: int, prefix: str = "0x", width: int = 4) -> str:
    """Format an integer as hex string, e.g. 0x1000."""
    return f"{prefix}{value:0{width}X}"


# ──────────────────────────────────────────────
# Data type helpers
# ──────────────────────────────────────────────

def normalize_data_type(raw: str) -> str:
    """Normalize a raw data type string to standard CoE type name.

    Handles hex codes (0x0006), aliases (uint16), and passthrough (UNSIGNED16).
    Also preserves parameterized types like OCTET_STRING(10).
    """
    if not raw:
        return raw

    s = raw.strip()

    # Handle parameterized types: "OCTET_STRING(10)" → check base
    base = s.split("(")[0].strip()

    # Hex code lookup
    if base in COE_TYPE_HEX_TO_NAME:
        return COE_TYPE_HEX_TO_NAME[base]

    # Alias lookup (case-insensitive)
    lower = base.lower()
    if lower in COE_TYPE_ALIASES:
        return COE_TYPE_ALIASES[lower]

    # Already a standard name or unknown — return as-is
    return s


def classify_object_index(index: int) -> str:
    """Classify an object index into a category."""
    if 0x1000 <= index < 0x2000:
        return "comm"
    if 0x2000 <= index < 0x6000:
        return "mfg"
    if 0x6000 <= index < 0xA000:
        return "standard"
    return "other"
