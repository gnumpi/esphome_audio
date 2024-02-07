import os

from esphome.core import EsphomeError
from esphome.__main__ import run_esphome

from .components import ExternalComponent


class CompileTest:
    def __init__(self, component: ExternalComponent):
        self.comp = component

    def run_tests(self) -> int:
        for cfgFile in os.listdir(self.comp.testsRoot):
            if not cfgFile.endswith(".yaml"):
                continue
            cfgFile = os.path.join(self.comp.testsRoot, cfgFile)
            try:
                return run_esphome(["esphome", "-q", "compile", cfgFile])
            except EsphomeError:
                return 1
