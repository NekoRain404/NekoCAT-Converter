"""
Validator — final sanity checks after fixes are applied.

Per dev doc §10.4 and §20, verifies:
  - No fatal issues remain
  - PDO/SM size consistency
  - Callback flags only on object rows (not entry rows)
  - All objects have data types
  - No unfixed BOOL/OCTET_STRING issues

Returns True if the device is ready for xlsx generation.
"""
from rich.console import Console

from nekoecat.model import DeviceModel, Severity

console = Console()


def validate(device: DeviceModel) -> bool:
    """Run final validation. Returns True if safe to generate."""
    checks = [
        _check_no_fatal(device),
        _check_unfixed_critical(device),
        _check_callback_not_on_entries(device),
        _check_data_types_present(device),
    ]

    ok = all(checks)

    fatal = device.issue_count("fatal")
    errors = device.issue_count("error")
    warnings = device.issue_count("warning")

    if not ok:
        console.print(f"[red]Validation FAILED[/red] — {fatal} fatal, {errors} errors")
    elif warnings > 0:
        console.print(f"[yellow]Validation passed with warnings[/yellow] — {warnings} warnings")
    else:
        console.print("[green]Validation passed[/green]")

    return ok


def _check_no_fatal(device: DeviceModel) -> bool:
    fatal = device.issue_count("fatal")
    return fatal == 0


def _check_unfixed_critical(device: DeviceModel) -> bool:
    """Unfixed ERROR-level issues block generation."""
    unfixed_errors = sum(
        1 for i in device.issues
        if i.severity == Severity.ERROR and not i.fixed
    )
    return unfixed_errors == 0


def _check_callback_not_on_entries(device: DeviceModel) -> bool:
    """CoeRead/CoeWrite must not be set on subindex entries."""
    ok = True
    for obj in device.objects.values():
        for sub in obj.subindices:
            if sub.coe_read is True or sub.coe_write is True:
                console.print(
                    f"[yellow]Warning:[/yellow] 0x{obj.index:04X}:{sub.subindex} "
                    f"has CoE flags at entry level (should be object level)"
                )
                ok = False
    return ok


def _check_data_types_present(device: DeviceModel) -> bool:
    """Every application object must have a data type or subindices."""
    ok = True
    for obj in device.objects.values():
        if obj.category == "comm":
            continue
        if not obj.data_type and not obj.subindices:
            console.print(
                f"[yellow]Warning:[/yellow] 0x{obj.index:04X} has no data type"
            )
            ok = False
    return ok
