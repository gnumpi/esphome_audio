"""Definition of linting rules for all files."""
from __future__ import annotations
import re

from ..esphome_linter import ESPHomeExtLinter, CheckResult


class ESPHomeExtAllLinter(ESPHomeExtLinter):
    rules = []
    default_include = ["*.*"]
    default_exclude = []


add_file_rule = ESPHomeExtAllLinter.file_rule_decorator
add_matched_line_rule = ESPHomeExtAllLinter.matched_line_rule_decorator


@add_matched_line_rule(
    r"(whitelist|blacklist|slave)",
    exclude=[__file__],
    flags=re.IGNORECASE | re.MULTILINE,
)
def lint_inclusive_language(fname: str, match: re.Match | None) -> CheckResult:
    # From https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=49decddd39e5f6132ccd7d9fdc3d7c470b0061bb
    if match is None:
        return CheckResult.success("Pattern not found")
    return CheckResult.failed(
        "Avoid the use of whitelist/blacklist/slave.\n"
        "Recommended replacements for 'master / slave' are:\n"
        "    '{primary,main} / {secondary,replica,subordinate}\n"
        "    '{initiator,requester} / {target,responder}'\n"
        "    '{controller,host} / {device,worker,proxy}'\n"
        "    'leader / follower'\n"
        "    'director / performer'\n"
        "\n"
        "Recommended replacements for 'blacklist/whitelist' are:\n"
        "    'denylist / allowlist'\n"
        "    'blocklist / passlist'"
    )
