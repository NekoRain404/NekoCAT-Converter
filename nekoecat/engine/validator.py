"""
Validator — final sanity checks after fixes are applied.

Returns True if the device is ready for xlsx generation,
False if fatal issues remain.
"""
from rich.console import Console

from nekoecat.model import DeviceModel, Severity

console = Console()


def validate(device: DeviceModel) -> bool:
    """Run final validation. Returns True if safe to generate."""
    ok = True

    fatal = device.issue_count("fatal")
    errors = device.issue_count("error")

    if fatal > 0:
        console.print(f"[red]Validation FAILED:[/red] {fatal} fatal issues")
        ok = False
    elif errors > 0:
        console.print(f"[yellow]Validation WARNING:[/yellow] {errors} errors (will proceed)")
    else:
        console.print("[green]Validation passed[/green]")

    return ok
