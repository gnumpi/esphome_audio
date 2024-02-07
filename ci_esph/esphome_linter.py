from __future__ import annotations
from typing import Callable

import codecs
from dataclasses import dataclass
import functools
import re
from collections.abc import Generator

from .constants import (
    CHECK_RET,
)


@dataclass
class CheckResult:
    ret: CHECK_RET
    text: str
    func_name: str | None = None
    descr_line: str | None = None
    file: str | None = None
    row: int | None = None
    col: int | None = None

    @classmethod
    def success(cls, text: str = ""):
        return cls(ret=CHECK_RET.SUCCESS, text=text)

    @classmethod
    def failed(cls, text: str):
        return cls(ret=CHECK_RET.ERROR, text=text)

    @property
    def status(self) -> str:
        return (
            "passed"
            if self.ret == CHECK_RET.SUCCESS
            else ("warning" if self.ret == CHECK_RET.WARNING else "error")
        )


class CheckSummary(CheckResult):
    def __init__(self, **kwargs):
        ret = kwargs.get("ret", CHECK_RET.UNKNOWN)
        text = kwargs.get("text", "")
        super().__init__(ret=ret, text=text, **kwargs)
        self.results = []

    def add(self, result: CheckResult):
        self.results.append(result)
        self._update_state()

    def _update_state(self):
        for res in self.results:
            if res.ret == CHECK_RET.ERROR:
                self.ret = CHECK_RET.ERROR
                break
        else:
            self.ret = CHECK_RET.SUCCESS
        self.text = "\n".join(map(lambda r: r.text, self.results))
        files = list(dict.fromkeys(r.file for r in self.results if r.file))
        assert len(files) == 1
        self.file = files[0]

    def __repr__(self):
        s = f"Summary - {self.descr_line} ({self.file})\n"
        for res in self.results:
            s += f"  {res.descr_line}: func=`{res.func_name}` ({res.status})\n"
        return s


class FileRule:
    """Linter rule applied to a file."""

    def __init__(
        self,
        func: Callable[[str], CheckResult],
        include: list[str] = [],
        exclude: list[str] = [],
    ):
        self.func = func
        self.include = include
        self.exclude = exclude
        self.verbose = True
        self._incl = re.compile(
            "|".join(
                map(lambda x: x.replace(".", r"\.").replace("*", ".+") + "$", include)
            )
        )
        self._excl = re.compile(
            "|".join(
                map(lambda x: x.replace(".", r"\.").replace("*", ".+") + "$", exclude)
            )
        )

    def skip_file(self, fname: str) -> bool:
        return self._excl.match(fname)

    def take_file(self, fname: str) -> bool:
        return self._incl.match(fname)

    def check(self, file: str) -> CheckResult:
        return self.func(file)

    def __str__(self):
        s = self.func.__doc__
        s += " Applies to: " + ", ".join(self.include)
        # s += " excl: " + ",".join(self.exclude)
        return s


class MatchRegExFileRule(FileRule):
    """FileRule: Checks regular expression rules against the file."""

    def __init__(self, include: list[str] = [], exclude: list[str] = []):
        self.matchers: list[re.Pattern] = []
        self.rules: list[Callable[[str, re.Match], CheckResult]] = []
        super().__init__(self.check_matches, include, exclude)

    def add_regEx_rule(
        self, regExp: str, func: Callable[[str, re.Match], CheckResult]
    ) -> None:
        self.matchers.append(re.compile(regExp, re.MULTILINE))
        self.rules.append(func)

    def check_matches_iter(self, file: str) -> Generator[CheckResult, None, None]:
        try:
            with codecs.open(file, "r", encoding="utf-8") as f:
                content = f.read()
        except UnicodeDecodeError:
            yield CheckResult.failed(
                f"File {file} is not readable as UTF-8. Please set your editor to UTF-8 mode.",
            )
            return

        for mId, matcher in enumerate(self.matchers):
            found = 0
            for match in matcher.finditer(content):
                if "NOLINT" in match.group(0):
                    continue
                found += 1
                yield self.rules[mId](file, match)

            if found == 0:
                yield self.rules[mId](file, None)

    def check_matches(self, file: str) -> tuple[CHECK_RET, str]:
        """Regular expression checks."""
        sumRes = CheckSummary(descr_line="RexExFileRules")
        for res in self.check_matches_iter(file):
            sumRes.add(res)

        return sumRes

    def __str__(self):
        s = super().__str__() + "\n    "
        s += "\n    ".join(map(lambda r: r.__doc__ or "", self.rules))
        return s


class ESPHomeExtLinter:
    file_rules: list[FileRule] = []
    default_include: list[str] = []
    default_exclude: list[str] = []
    file_matcher: dict[str, tuple[Callable[[str], bool], list[FileRule]]] = {}

    def __init__(self, component):
        pass

    def print_rules(self):
        print("File rules:")
        for rule in self.file_rules:
            print(rule)

    def run_iterate(self, files):
        for rule in self.file_rules:
            for file in filter(rule.take_file, files):
                yield rule.check(file)

    def run(
        self, stop_on_error: bool = False, stop_on_warning: bool = False
    ) -> CheckResult:
        summary = CheckSummary(descr_line="Linting summary.")
        for rule in self.rules:
            for file in rule.get_files(self):
                check = rule.check(file)
                summary.add(check)
                if stop_on_error and check.ret == CHECK_RET.ERROR:
                    summary.ret = CHECK_RET.ERROR
                    return summary
                elif stop_on_warning and check.ret == CHECK_RET.WARNING:
                    summary.ret = CHECK_RET.WARNING
                    return summary
        return summary

    @classmethod
    def add_file_rule(
        cls,
        func: Callable[[str], CheckResult],
        include: list[str] | None = None,
        exclude: list[str] | None = None,
        **kwargs,
    ) -> None:
        cls.file_rules.append(
            FileRule(
                func=func,
                include=include or cls.default_include,
                exclude=exclude or cls.default_exclude,
            )
        )

    @classmethod
    def add_matched_line_rule(
        cls,
        match_str: str,
        func: Callable[[str, re.Match | None], CheckResult],
        include: list[str] | None = None,
        exclude: list[str] | None = None,
        **kwargs,
    ) -> None:
        include = include or cls.default_include
        exclude = exclude or cls.default_exclude
        fRule = None
        for fileRule in cls.file_rules:
            if (
                isinstance(fileRule, MatchRegExFileRule)
                and fileRule.include == include
                and fileRule.exclude == exclude
            ):
                fRule = fileRule
                break
        if fRule is None:
            fRule = MatchRegExFileRule(
                include,
                exclude,
            )
            cls.file_rules.append(fRule)

        fRule.add_regEx_rule(
            regExp=match_str,
            func=func,
        )

    @classmethod
    def file_rule_decorator(
        cls, include: list[str] | None = None, exclude: list[str] | None = None
    ) -> Callable[[str], CheckResult]:
        def decorator(
            func: Callable[[str], CheckResult]
        ) -> Callable[[str], CheckResult]:
            @functools.wraps(func)
            def set_doc_string_and_name(fname: str) -> CheckResult:
                res = func(fname)
                res.descr_line = func.__doc__ or ""
                res.func_name = func.__name__
                res.file = fname
                return res

            cls.add_file_rule(set_doc_string_and_name, include, exclude)
            return set_doc_string_and_name

        return decorator

    @classmethod
    def matched_line_rule_decorator(
        cls,
        regEx: str,
        include: list[str] | None = None,
        exclude: list[str] | None = None,
    ) -> Callable[[str, re.Match | None], CheckResult]:
        def decorator(
            func: Callable[[str, re.Match | None], CheckResult]
        ) -> Callable[[str, re.Match | None], CheckResult]:
            @functools.wraps(func)
            def set_doc_string_and_name(fname: str, regEx: str) -> CheckResult:
                res = func(fname, regEx)
                res.descr_line = func.__doc__ or ""
                res.func_name = func.__name__
                res.file = fname
                return res

            cls.add_matched_line_rule(regEx, set_doc_string_and_name, include, exclude)
            return set_doc_string_and_name

        return decorator
