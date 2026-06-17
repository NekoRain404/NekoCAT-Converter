"""测试 nekoecat/engine/ 模块。"""
import pytest
from pathlib import Path
from nekoecat.parser import parse_esi
from nekoecat.engine import classify_objects, run_rules, auto_fix, validate

DEMO_DIR = Path(__file__).parent.parent / "demo"

class TestClassifier:
    def test_classify_objects(self):
        """测试对象分类。"""
        device = parse_esi(str(DEMO_DIR / "ESI.XML"))
        classify_objects(device)
        for obj in device.objects.values():
            assert obj.category != ""

class TestRuleEngine:
    def test_run_rules(self):
        """测试规则检查。"""
        device = parse_esi(str(DEMO_DIR / "ESI.XML"))
        classify_objects(device)
        run_rules(device)

class TestFixEngine:
    def test_auto_fix(self):
        """测试自动修复。"""
        device = parse_esi(str(DEMO_DIR / "ESI.XML"))
        classify_objects(device)
        run_rules(device)
        auto_fix(device)

class TestValidator:
    def test_validate(self):
        """测试校验。"""
        device = parse_esi(str(DEMO_DIR / "ESI.XML"))
        classify_objects(device)
        run_rules(device)
        auto_fix(device)
        result = validate(device)
        assert isinstance(result, bool)
