"""
SSC Tool log parser — reads SSC Tool import error logs and extracts
structured error information for automatic re-fix.

Only depends on: re (standard lib).  No nekoecat imports.
"""
import re
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class SscLogEntry:
    """A single parsed SSC Tool log error/warning."""
    line_no: int = 0
    level: str = ""          # "Error", "Warning"
    message: str = ""
    index: str = ""          # e.g. "0x6000"
    subindex: str = ""
    raw_line: str = ""


@dataclass
class SscLogParseResult:
    """Parsed SSC Tool log."""
    entries: list[SscLogEntry] = field(default_factory=list)

    @property
    def error_count(self) -> int:
        return sum(1 for e in self.entries if e.level == "Error")

    @property
    def warning_count(self) -> int:
        return sum(1 for e in self.entries if e.level == "Warning")


# Common SSC Tool error patterns
_PATTERNS = [
    # "Failed to convert '0' to DataType '1' [Index:0x6000, SubIndex:0x01]"
    re.compile(
        r"(?P<level>Error|Warning).*?(?P<msg>Failed to convert .*?to DataType .*?)"
        r".*?\[Index:(?P<idx>0x[0-9A-Fa-f]+)(?:,\s*SubIndex:(?P<sub>0x[0-9A-Fa-f]+))?\]"
    ),
    # 'Data type "OCTET_STRING(10)" not supported'
    re.compile(
        r"(?P<level>Error|Warning).*?(?P<msg>Data type .*? not supported)"
    ),
    # "Entry doesn't start at word offset"
    re.compile(
        r"(?P<level>Error|Warning).*?(?P<msg>Entry doesn't start at word offset)"
    ),
    # "A CoE callback shall be defined for an object"
    re.compile(
        r"(?P<level>Error|Warning).*?(?P<msg>A CoE callback shall be defined .*?)"
    ),
    # Generic line
    re.compile(
        r"(?P<level>Error|Warning)[:\s]+(?P<msg>.*)"
    ),
]


def parse_ssc_log(text: str) -> SscLogParseResult:
    """Parse SSC Tool log text into structured entries."""
    result = SscLogParseResult()

    for line_no, line in enumerate(text.splitlines(), start=1):
        line = line.strip()
        if not line:
            continue

        for pattern in _PATTERNS:
            m = pattern.search(line)
            if m:
                groups = m.groupdict()
                result.entries.append(SscLogEntry(
                    line_no=line_no,
                    level=groups.get("level", ""),
                    message=groups.get("msg", line),
                    index=groups.get("idx", ""),
                    subindex=groups.get("sub", ""),
                    raw_line=line,
                ))
                break

    return result


def parse_ssc_log_file(filepath: str) -> SscLogParseResult:
    """Parse an SSC Tool log file."""
    text = Path(filepath).read_text(encoding="utf-8", errors="replace")
    return parse_ssc_log(text)
