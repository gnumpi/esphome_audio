"""Definition of linting rules for c/c++ files."""
from __future__ import annotations
import re

from ..esphome_linter import ESPHomeExtLinter, CheckResult


class ESPHomeExtCLinter(ESPHomeExtLinter):
    rules = []
    default_include = ["*.h", "*.c", "*.cpp", "*.tcc"]
    default_exclude = []


add_file_rule = ESPHomeExtCLinter.file_rule_decorator
add_matched_line_rule = ESPHomeExtCLinter.matched_line_rule_decorator

# FileRules


@add_file_rule(include=["*.ino"])
def lint_ino(fname: str) -> CheckResult:
    """.ino files."""
    return CheckResult.failed(f"Found {fname}. Please use either .cpp or .h")


# MatchedLineRules

CPP_RE_EOL = r"\s*?(?://.*?)?$"


@add_matched_line_rule(
    r"^#define\s+([a-zA-Z0-9_]+)\s+([0-9bx]+)" + CPP_RE_EOL,
)
def lint_no_defines(fname: str, match: re.Match | None) -> CheckResult:
    """Integer #define macros."""
    if match is None:
        return CheckResult.success("Pattern not found.")
    s = f"static const uint8_t {match.group(1)} = {match.group(2)};"
    return CheckResult.failed(
        "#define macros for integer constants are not allowed, please use "
        "{} style instead (replace uint8_t with the appropriate "
        "datatype). See also Google style guide.".format(s)
    )


@add_matched_line_rule(r"^\s*delay\((\d+)\);" + CPP_RE_EOL)
def lint_no_long_delays(fname: str, match: re.Match | None) -> CheckResult:
    """delay() calls > 50ms"""
    if match is None:
        return CheckResult.success("Pattern not found.")
    duration_ms = int(match.group(1))
    if duration_ms < 50:
        return CheckResult.success("")
    return CheckResult.failed(
        "{} - long calls to delay() are not allowed in ESPHome because everything executes "
        "in one thread. Calling delay() will block the main thread and slow down ESPHome.\n"
        "If there's no way to work around the delay() and it doesn't execute often, please add "
        "a '// NOLINT' comment to the line."
        "".format(match.group(0).strip())
    )


RAW_PIN_ACCESS_RE = (
    r"^\s(pinMode|digitalWrite|digitalRead)\((.*)->get_pin\(\),\s*([^)]+).*\)"
)


@add_matched_line_rule(RAW_PIN_ACCESS_RE)
def lint_no_raw_pin_access(fname: str, match: re.Match | None) -> CheckResult:
    """raw_pin_access"""
    if match is None:
        return CheckResult.success("Pattern not found.")
    func = match.group(1)
    pin = match.group(2)
    mode = match.group(3)
    new_func = {
        "pinMode": "pin_mode",
        "digitalWrite": "digital_write",
        "digitalRead": "digital_read",
    }[func]
    new_code = f"{pin}->{new_func}({mode})"
    return CheckResult.failed(
        f"Don't use raw {func} calls. Instead, use the `->{new_func}` function: {new_code}"
    )


# Functions from Arduino framework that are forbidden to use directly
ARDUINO_FORBIDDEN = [
    "digitalWrite",
    "digitalRead",
    "pinMode",
    "shiftOut",
    "shiftIn",
    "radians",
    "degrees",
    "interrupts",
    "noInterrupts",
    "lowByte",
    "highByte",
    "bitRead",
    "bitSet",
    "bitClear",
    "bitWrite",
    "bit",
    "analogRead",
    "analogWrite",
    "pulseIn",
    "pulseInLong",
    "tone",
]
ARDUINO_FORBIDDEN_RE = r"[^\w\d](" + r"|".join(ARDUINO_FORBIDDEN) + r")\(.*"


@add_matched_line_rule(ARDUINO_FORBIDDEN_RE)
def lint_no_arduino_framework_functions(
    fname: str, match: re.Match | None
) -> CheckResult:
    """Forbidden arduino functions"""
    if match is None:
        return CheckResult.success("Pattern not found.")
    nolint = "// NOLINT"
    return CheckResult.failed(
        f"The function {match.group(1)} from the Arduino framework is forbidden to be "
        f"used directly in the ESPHome codebase. Please use ESPHome's abstractions and equivalent "
        f"C++ instead.\n"
        f"\n"
        f"(If the function is strictly necessary, please add `{nolint}` to the end of the line)"
    )


IDF_CONVERSION_FORBIDDEN = {
    "ARDUINO_ARCH_ESP32": "USE_ESP32",
    "ARDUINO_ARCH_ESP8266": "USE_ESP8266",
    "pgm_read_byte": "progmem_read_byte",
    "ICACHE_RAM_ATTR": "IRAM_ATTR",
    "esphome/core/esphal.h": "esphome/core/hal.h",
}
IDF_CONVERSION_FORBIDDEN_RE = r"(" + r"|".join(IDF_CONVERSION_FORBIDDEN) + r").*"


@add_matched_line_rule(IDF_CONVERSION_FORBIDDEN_RE)
def lint_no_removed_in_idf_conversions(
    fname: str, match: re.Match | None
) -> CheckResult:
    """IDF conversion"""
    if match is None:
        return CheckResult.success("Pattern not found.")
    replacement = IDF_CONVERSION_FORBIDDEN[match.group(1)]
    return CheckResult.failed(
        f"The macro {match.group(1)} can no longer be used in ESPHome directly. "
        f"Please use {replacement} instead."
    )


@add_matched_line_rule(
    r"[^\w\d]byte\s+[\w\d]+\s*=",
)
def lint_no_byte_datatype(fname: str, match: re.Match | None) -> CheckResult:
    """Usage of byte data type"""
    if match is None:
        return CheckResult.success("Pattern not found.")
    return CheckResult.failed(
        f"The datatype {'byte'} is not allowed to be used in ESPHome. "
        f"Please use {'uint8_t'} instead."
    )
