"""
Shared constants: data type maps, index ranges, default configs.

No external dependencies — pure Python.
"""

# ──────────────────────────────────────────────
# EtherCAT CoE data type mappings
# ──────────────────────────────────────────────

# Hex code → standard name
COE_TYPE_HEX_TO_NAME: dict[str, str] = {
    "0x0001": "BOOLEAN",
    "0x0002": "INTEGER8",
    "0x0003": "INTEGER16",
    "0x0004": "INTEGER32",
    "0x0005": "UNSIGNED8",
    "0x0006": "UNSIGNED16",
    "0x0007": "UNSIGNED32",
    "0x0008": "REAL32",
    "0x0009": "VISIBLE_STRING",
    "0x000A": "OCTET_STRING",
    "0x000B": "UNICODE_STRING",
    "0x000C": "TIME_OF_DAY",
    "0x000D": "TIME_DIFFERENCE",
    "0x000F": "DOMAIN",
    "0x0010": "INTEGER24",
    "0x0011": "REAL64",
    "0x0012": "INTEGER40",
    "0x0013": "INTEGER48",
    "0x0014": "INTEGER56",
    "0x0015": "INTEGER64",
    "0x0016": "UNSIGNED24",
    "0x0017": "UNSIGNED40",
    "0x0018": "UNSIGNED48",
    "0x0019": "UNSIGNED56",
    "0x001A": "UNSIGNED64",
    "0x001B": "GUID",
    "0x001C": "BYTE",
    "0x001D": "WORD",
    "0x001E": "DWORD",
}

# Standard name → bit size (for fixed-size types)
COE_TYPE_BIT_SIZE: dict[str, int] = {
    "BOOLEAN": 1,
    "INTEGER8": 8,
    "INTEGER16": 16,
    "INTEGER24": 24,
    "INTEGER32": 32,
    "INTEGER40": 40,
    "INTEGER48": 48,
    "INTEGER56": 56,
    "INTEGER64": 64,
    "UNSIGNED8": 8,
    "UNSIGNED16": 16,
    "UNSIGNED24": 24,
    "UNSIGNED32": 32,
    "UNSIGNED40": 40,
    "UNSIGNED48": 48,
    "UNSIGNED56": 56,
    "UNSIGNED64": 64,
    "REAL32": 32,
    "REAL64": 64,
    "BYTE": 8,
    "WORD": 16,
    "DWORD": 32,
    "GUID": 128,
}

# Normalized alias map (lowercase → standard name)
COE_TYPE_ALIASES: dict[str, str] = {
    "bool": "BOOLEAN",
    "boolean": "BOOLEAN",
    "uint8": "UNSIGNED8",
    "uint16": "UNSIGNED16",
    "uint32": "UNSIGNED32",
    "uint64": "UNSIGNED64",
    "int8": "INTEGER8",
    "int16": "INTEGER16",
    "int32": "INTEGER32",
    "int64": "INTEGER64",
    "float": "REAL32",
    "double": "REAL64",
    "string": "VISIBLE_STRING",
    "visible_string": "VISIBLE_STRING",
    "octet_string": "OCTET_STRING",
    "unicode_string": "UNICODE_STRING",
    "domain": "DOMAIN",
    "time_of_day": "TIME_OF_DAY",
    "time_difference": "TIME_DIFFERENCE",
    "byte": "BYTE",
    "word": "WORD",
    "dword": "DWORD",
}

# Types that need CoE callback in SSC Tool (not directly supported)
SSC_CALLBACK_TYPES: set[str] = {"OCTET_STRING", "UNICODE_STRING", "DOMAIN"}

# ──────────────────────────────────────────────
# Object index ranges per EtherCAT spec
# ──────────────────────────────────────────────

# Communication profile objects (0x1000-0x1FFF)
COMM_OBJECT_RANGE = range(0x1000, 0x2000)

# Manufacturer-specific profile (0x2000-0x5FFF)
MFG_OBJECT_RANGE = range(0x2000, 0x6000)

# Standardized device profile (0x6000-0x9FFF)
DEVICE_PROFILE_RANGE = range(0x6000, 0xA000)

# ──────────────────────────────────────────────
# Sync Manager defaults
# ──────────────────────────────────────────────

SM_INDEX_TO_TYPE: dict[int, str] = {
    0: "mboxout",
    1: "mboxin",
    2: "outputs",
    3: "inputs",
}

SM_DEFAULT_ADDRESSES: dict[int, int] = {
    0: 0x1000,
    1: 0x1400,
    2: 0x1800,
    3: 0x1C00,
}

# ──────────────────────────────────────────────
# SSC Tool xlsx column names (for generator)
# ──────────────────────────────────────────────

SSC_OBJ_COLUMNS = [
    "Index", "Name", "ObjectCode", "DataType", "BitLength",
    "Access", "PdoMapping", "DefaultValue", "Value",
    "CoeRead", "CoeWrite", "Backup", "Setting",
]

SSC_SUB_COLUMNS = [
    "Index", "SubIndex", "Name", "DataType", "BitLength",
    "Access", "PdoMapping", "DefaultValue", "Value",
    "CoeRead", "CoeWrite", "Backup", "Setting",
]
