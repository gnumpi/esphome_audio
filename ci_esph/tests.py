import os

from esphome.core import EsphomeError
from esphome.__main__ import run_esphome

from .components import ExternalComponent


class CompileTest:
    def __init__(self, component: ExternalComponent):
        self.comp = component

    def run_tests(self) -> int:
        ret = 0
        for cfgFile in os.listdir(self.comp.testsRoot):
            if not cfgFile.endswith(".yaml"):
                continue
            print( "Testing config: ", cfgFile )
            cfgFile = os.path.join(self.comp.testsRoot, cfgFile)
            try:
                ret += run_esphome(["esphome", "-q", "compile", cfgFile])
            except EsphomeError:
                ret = 1
        return ret
