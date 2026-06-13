"""
nekoecat.core — 公共门面层 (Facade)。

GUI 和 CLI 只调用本模块，不直接导入 parser/engine/generator。
这是解耦的核心：上层不感知下层实现。

Public API:
    parse_only(esi_path, sdo_path) → DeviceModel   # 仅解析，供 GUI "Parse" 按钮
    convert(config) → ConvertResult                  # 完整转换管线
"""
import json
import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional
from datetime import datetime
from rich.console import Console

from nekoecat.config import ConvertConfig
from nekoecat.parser import parse_esi, parse_sdo
from nekoecat.engine import classify_objects, merge, run_rules, auto_fix, validate
from nekoecat.generator import generate_ssc_xlsx, generate_report
from nekoecat.model import DeviceModel

console = Console()


# ──────────────────────────────────────────────
# 结果容器
# ──────────────────────────────────────────────

@dataclass
class ConvertResult:
    """一次完整转换的结果。"""
    success: bool = False
    output_dir: str = ""
    xlsx_path: str = ""
    report_files: dict[str, str] = field(default_factory=dict)
    device: Optional[DeviceModel] = None
    error: Optional[str] = None
    log_path: str = ""


# ──────────────────────────────────────────────
# 仅解析（供 GUI "Parse" 按钮调用）
# ──────────────────────────────────────────────

def parse_only(
    esi_path: Optional[str] = None,
    sdo_path: Optional[str] = None,
) -> DeviceModel:
    """只做解析 + 合并 + 分类，不做规则检查和修复。

    GUI 的 "Parse" 按钮调用此函数，拿到 DeviceModel 后
    更新各个页面的显示。

    Args:
        esi_path: ESI XML 文件路径，可为 None
        sdo_path: SDO 文件路径 (XML 或文本格式)，可为 None

    Returns:
        合并后的 DeviceModel

    Raises:
        FileNotFoundError: 文件不存在
        ValueError: 两个路径都为 None
    """
    if not esi_path and not sdo_path:
        raise ValueError("至少提供一个 ESI 或 SDO 文件路径")

    esi_device = parse_esi(esi_path) if esi_path else None
    sdo_device = parse_sdo(sdo_path) if sdo_path else None

    # 合并策略：两个都有则 merge，否则取有数据的那个
    if esi_device and sdo_device:
        device = merge(esi_device, sdo_device)
    else:
        device = esi_device or sdo_device

    # 分类（设置 category、is_pdo_mapping_object 等标志）
    classify_objects(device)

    return device


# ──────────────────────────────────────────────
# 完整转换管线
# ──────────────────────────────────────────────

def convert(config: ConvertConfig) -> ConvertResult:
    """运行完整的转换管线。

    步骤 (dev doc §14):
        1. 解析 ESI / SDO
        2. 合并
        3. 应用身份覆盖
        4. 对象分类
        5. 规则检查
        6. 自动修复
        7. 校验
        8. 生成输出 (xlsx, report, json, log)
    """
    result = ConvertResult()

    if not config.esi_path and not config.sdo_path:
        result.error = "至少提供一个 ESI 或 SDO 文件路径"
        return result

    # ── 创建输出目录 ──────────────────────────
    ts = datetime.now().strftime("%Y%m%d_%H%M")
    device_label = config.device_name or "device"
    out = Path(config.output_dir) / f"{device_label}_{ts}"
    out.mkdir(parents=True, exist_ok=True)
    result.output_dir = str(out)

    # ── 设置日志 ──────────────────────────────
    log_path = out / "logs" / "convert.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    result.log_path = str(log_path)

    logger = _setup_logger(log_path)
    logger.info("NekoECAT Converter — 转换开始")
    logger.info(f"配置: esi={config.esi_path}, sdo={config.sdo_path}")

    try:
        # ── 1. 解析 ───────────────────────────
        console.rule("[bold blue]解析[/bold blue]")
        logger.info("解析 ESI...")
        esi_device = parse_esi(config.esi_path) if config.esi_path else DeviceModel()
        logger.info(f"  ESI: {len(esi_device.objects)} 个对象")

        logger.info("解析 SDO...")
        sdo_device = parse_sdo(config.sdo_path) if config.sdo_path else DeviceModel()
        logger.info(f"  SDO: {len(sdo_device.objects)} 个对象")

        # ── 2. 合并 ───────────────────────────
        console.rule("[bold blue]合并[/bold blue]")
        if config.esi_path and config.sdo_path:
            device = merge(esi_device, sdo_device)
        elif config.esi_path:
            device = esi_device
        else:
            device = sdo_device
        logger.info(f"合并后: {len(device.objects)} 个对象")

        # ── 3. 应用身份覆盖 ───────────────────
        _apply_identity_overrides(device, config)

        # ── 4. 分类 ───────────────────────────
        classify_objects(device)

        # ── 5. 规则检查 ───────────────────────
        console.rule("[bold blue]规则检查[/bold blue]")
        run_rules(device)
        logger.info(f"规则: {device.issue_count()} 个问题")

        # ── 6. 自动修复 ───────────────────────
        console.rule("[bold blue]自动修复[/bold blue]")
        auto_fix(device)
        logger.info(f"修复: {device.fixed_count()} 个已修复")

        # ── 7. 校验 ───────────────────────────
        console.rule("[bold blue]校验[/bold blue]")
        if not validate(device):
            result.error = "校验失败 — 请查看报告中的问题列表"
            result.device = device
            logger.error("校验失败")
            return result

        # ── 8. 生成输出 ───────────────────────
        console.rule("[bold blue]生成输出[/bold blue]")

        if config.generate_xlsx:
            xlsx_path = generate_ssc_xlsx(
                device,
                str(out / f"{device_label}_SSC_DESCRIPTION.xlsx"),
                template_path=config.template_xlsx_path,
            )
            result.xlsx_path = xlsx_path
            logger.info(f"生成 xlsx: {xlsx_path}")

        if config.generate_report:
            report_files = generate_report(device, str(out))
            result.report_files = report_files
            logger.info(f"报告: {list(report_files.keys())}")

        if config.generate_json:
            json_path = _write_normalized_json(device, out, device_label)
            result.report_files["normalized_json"] = json_path
            logger.info(f"归一化 JSON: {json_path}")

        result.success = True
        result.device = device

        console.rule("[bold green]完成[/bold green]")
        console.print(f"[green]输出目录:[/green] {out}")
        logger.info("转换成功完成")

    except Exception as exc:
        result.error = str(exc)
        console.print_exception()
        logger.exception(f"转换失败: {exc}")

    return result


# ──────────────────────────────────────────────
# 内部辅助函数
# ──────────────────────────────────────────────

def _apply_identity_overrides(device: DeviceModel, config: ConvertConfig):
    """将配置中的身份覆盖应用到设备模型。"""
    if config.vendor_id is not None:
        device.identity.vendor_id = config.vendor_id
    if config.product_code is not None:
        device.identity.product_code = config.product_code
    if config.revision_number is not None:
        device.identity.revision_number = config.revision_number
    if config.device_name is not None:
        device.identity.device_name = config.device_name


def _write_normalized_json(device: DeviceModel, out: Path, label: str) -> str:
    """写入 device.normalized.json — 用于问题复现 (dev doc §18)。"""
    path = out / f"{label}.normalized.json"
    data = device.model_dump(mode="json")
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    return str(path)


def _setup_logger(log_path: Path) -> logging.Logger:
    """创建文件日志记录器。"""
    logger = logging.getLogger("nekoecat.convert")
    logger.setLevel(logging.DEBUG)
    logger.handlers.clear()
    fh = logging.FileHandler(str(log_path), encoding="utf-8")
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(logging.Formatter(
        "%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    ))
    logger.addHandler(fh)
    return logger
