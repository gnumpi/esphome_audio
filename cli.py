import argparse
import colorama
from colorama import Fore, Style
import os

from esphome.const import __version__ as ESPHOME_VERSION

from .components import (
    ExternalComponent,
    get_components_from_repository,
    list_component_git_files,
)

from .rules.linter_cpp import ESPHomeExtCLinter
from .tests import CompileTest
from .constants import VERSION_CHECK


def print_component_info_line(component: ExternalComponent) -> int:
    version_check = component.check_esphome_version(ESPHOME_VERSION)
    s = str(component)
    s += (
        Fore.GREEN
        if version_check == VERSION_CHECK.VERSION_IN_RANGE
        else (
            Fore.RED if version_check == VERSION_CHECK.EARLIER_VERSION else Fore.YELLOW
        )
    )
    s += f"  ESPHome: {component.esphome_support[0]} - {component.esphome_support[1]}"
    s += Style.RESET_ALL
    print(s)
    return 0


def print_components_list(repo_path: str) -> int:
    components = get_components_from_repository(repo_path)
    for comp in components:
        print_component_info_line(comp)
    return 0


def lint_components(repo_path: str) -> int:
    components = get_components_from_repository(repo_path)
    for comp in components:
        lint_esphome_rules(comp)
    return 0


def lint_esphome_rules(component: ExternalComponent):
    print_component_info_line(component)
    linter = ESPHomeExtCLinter(component)
    linter.print_rules()
    files = list_component_git_files(component)
    # print( "\n".join(list_component_git_files(component)) )
    for check in linter.run_iterate(files):
        print(check)


def run_component_tests(repo_path: str) -> int:
    components = get_components_from_repository(repo_path)
    retCode = 0
    for comp in components:
        build = CompileTest(comp)
        retCode += build.run_tests()

    return retCode


def main():
    colorama.init()
    parser = argparse.ArgumentParser("CI-Tool for external ESPHome components")
    parser.add_argument("--verbose", action="store_true", help="Verbose mode")
    parser.add_argument(
        "--local-path",
        type=str,
        help="Local path to external components repository. (Project root of this file is default)",
        default=os.path.join(os.path.dirname(__file__), ".."),
    )

    subparsers = parser.add_subparsers(dest="command")

    # List found components
    subparsers.add_parser("list", help="list found components")

    # ESPHome Linter
    lint = subparsers.add_parser("lint", help="run ESPHome Linter")

    lint.add_argument(
        "-c", "--changed", action="store_true", help="only run on changed files"
    )

    # Tests defined by external components
    subparsers.add_parser(
        "test",
        help="run compilation and custom tests from the external components repository",
    )

    args = parser.parse_args()
    print(f"ESPHome: {ESPHOME_VERSION}")
    local_path = args.local_path
    if args.command == "list":
        return print_components_list(local_path)
    elif args.command == "lint":
        return lint_components(local_path)
    elif args.command == "test":
        return run_component_tests(local_path)
    else:
        parser.print_usage()
        return -1
