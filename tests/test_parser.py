"""测试 nekoecat/parser/ 模块。"""
import pytest
from pathlib import Path
from nekoecat.parser import parse_esi, parse_sdo

DEMO_DIR = Path(__file__).parent.parent / "demo"

class TestEsiParser:
    def test_parse_esi(self):
        """测试 ESI XML 解析。"""
        device = parse_esi(str(DEMO_DIR / "ESI.XML"))
        assert device is not None
        assert device.identity.vendor_id == 162
        assert len(device.objects) > 0

    def test_parse_esi_with_objects(self):
        """测试 ESI 解析包含对象。"""
        device = parse_esi(str(DEMO_DIR / "ESI.XML"))
        indices = list(device.objects.keys())
        assert len(indices) > 10

class TestSdoParser:
    def test_parse_sdo_xml(self):
        """测试 SDO XML 解析。"""
        device = parse_sdo(str(DEMO_DIR / "SDO.XML"))
        assert device is not None
        assert len(device.objects) > 0
