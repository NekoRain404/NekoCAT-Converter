"""
nekoecat.log — SSC Tool log parsing.

Public API:
    parse_ssc_log(text) → SscLogParseResult
    parse_ssc_log_file(path) → SscLogParseResult
"""
from .ssc_log_parser import parse_ssc_log, parse_ssc_log_file, SscLogEntry, SscLogParseResult

__all__ = [
    "parse_ssc_log", "parse_ssc_log_file",
    "SscLogEntry", "SscLogParseResult",
]
