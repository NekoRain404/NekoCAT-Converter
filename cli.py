"""
CLI entry point for NekoECAT Converter.

Usage:
    python cli.py convert --esi ESI.XML --sdo SDO.XML --out out/d507
    python cli.py convert --esi ESI.XML --template digital_io.xlsx
    python cli.py convert --esi ESI.XML --sdo SDO.XML --name D507 --mode product
"""
from typing import Optional
import typer
from rich.console import Console

app = typer.Typer(
    name="nekoecat",
    help="NekoECAT Converter — ESI/SDO → SSC Tool xlsx",
    no_args_is_help=True,
)
console = Console()


@app.command()
def convert(
    esi: Optional[str] = typer.Option(None, "--esi", "-e", help="Path to ESI.XML"),
    sdo: Optional[str] = typer.Option(None, "--sdo", "-s", help="Path to SDO.XML"),
    template: Optional[str] = typer.Option(None, "--template", "-t", help="SSC template xlsx"),
    out: str = typer.Option("out", "--out", "-o", help="Output directory"),
    name: Optional[str] = typer.Option(None, "--name", "-n", help="Device name override"),
    vendor_id: Optional[int] = typer.Option(None, "--vendor-id", help="Vendor ID override"),
    product_code: Optional[str] = typer.Option(None, "--product-code", help="Product code (hex)"),
    revision: Optional[str] = typer.Option(None, "--revision", help="Revision number (hex)"),
    mode: str = typer.Option("learning", "--mode", "-m",
                              help="Mode: learning / lab / product"),
    no_report: bool = typer.Option(False, "--no-report", help="Skip report generation"),
    no_json: bool = typer.Option(False, "--no-json", help="Skip normalized JSON"),
):
    """Convert ESI/SDO files to SSC Tool xlsx."""
    if not esi and not sdo:
        console.print("[red]Error:[/red] Provide at least --esi or --sdo")
        raise typer.Exit(code=1)

    from nekoecat.config import ConvertConfig
    from nekoecat.core import convert as run_convert

    config = ConvertConfig(
        esi_path=esi,
        sdo_path=sdo,
        template_xlsx_path=template,
        output_dir=out,
        device_name=name,
        vendor_id=vendor_id,
        product_code=int(product_code, 16) if product_code else None,
        revision_number=int(revision, 16) if revision else None,
        mode=mode,
        generate_report=not no_report,
        generate_json=not no_json,
    )

    result = run_convert(config)

    if result.success:
        console.print(f"\n[bold green]✓ Success[/bold green]")
        console.print(f"  xlsx:    {result.xlsx_path}")
        console.print(f"  reports: {result.output_dir}")
        console.print(f"  log:     {result.log_path}")
        console.print(f"  objects: {len(result.device.objects)}")
        console.print(f"  issues:  {result.device.issue_count()}")
        console.print(f"  fixed:   {result.device.fixed_count()}")
    else:
        console.print(f"\n[bold red]✗ Failed:[/bold red] {result.error}")
        raise typer.Exit(code=1)


@app.command()
def info():
    """Show version and architecture info."""
    from nekoecat import __version__
    console.print(f"NekoECAT Converter v{__version__}")
    console.print("Architecture: model → parser → engine → generator")
    console.print("CLI usage: python cli.py convert --esi ESI.XML --sdo SDO.XML")


if __name__ == "__main__":
    app()
