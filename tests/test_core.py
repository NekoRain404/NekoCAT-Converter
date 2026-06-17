"""测试 nekoecat/core.py 模块。"""
import pytest
from pathlib import Path
from nekoecat.core import parse_only, convert
from nekoecat.config import ConvertConfig

DEMO_DIR = Path(__file__).parent.parent / "demo"
ESI_PATH = str(DEMO_DIR / "ESI.XML")
SDO_PATH = str(DEMO_DIR / "SDO.XML")

class TestParseOnly:
    def test_parse_esi_only(self):
        """测试仅解析 ESI 文件。"""
        device = parse_only(esi_path=ESI_PATH)
        assert device is not None
        assert len(device.objects) > 0
        assert device.identity.vendor_id > 0

    def test_parse_sdo_only(self):
        """测试仅解析 SDO 文件。"""
        device = parse_only(sdo_path=SDO_PATH)
        assert device is not None
        assert len(device.objects) > 0

    def test_parse_both(self):
        """测试同时解析 ESI 和 SDO。"""
        device = parse_only(esi_path=ESI_PATH, sdo_path=SDO_PATH)
        assert device is not None
        assert len(device.objects) > 0

    def test_parse_no_files(self):
        """测试未提供文件时抛出异常。"""
        with pytest.raises(ValueError):
            parse_only()

    def test_parse_nonexistent_file(self):
        """测试解析不存在的文件。"""
        with pytest.raises(FileNotFoundError):
            parse_only(esi_path="/nonexistent/ESI.XML")

class TestConvert:
    def test_convert_basic(self, tmp_path):
        """测试基本转换功能。"""
        config = ConvertConfig(
            esi_path=ESI_PATH,
            sdo_path=SDO_PATH,
            output_dir=str(tmp_path),
            generate_report=True,
            generate_json=True,
        )
        result = convert(config)
        assert result.success
        assert result.xlsx_path != ""
        assert Path(result.xlsx_path).exists()
