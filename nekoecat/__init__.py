"""
NekoECAT Converter — EtherCAT ESI/SDO → SSC Tool xlsx.

Architecture:
    model/      Pure data types (no logic)
    parser/     File readers → models
    engine/     Transform / validate / fix (models → models)
    generator/  Models → output files
    core.py     Public facade (orchestrates parser→engine→generator)
"""
__version__ = "0.1.0"
