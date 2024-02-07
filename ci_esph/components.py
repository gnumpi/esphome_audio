from __future__ import annotations
from dataclasses import dataclass
import json
import os
import subprocess

from .excecptions import (
    ComponentNotFound,
    UnsupportedComponentPath,
    ManifestNotFound,
    ManifestNameMismatch,
)

from .constants import COMPONENT_ROOTS, MANIFEST_FILE_NAME, VERSION_CHECK


@dataclass
class ExternalComponent:
    """Class representing a single external component"""

    name: str
    version: str
    repository_root: str
    components_path: str
    esphome_support: tuple[str, str] | None = None  # min,max version

    @property
    def path(self) -> str:
        return os.path.join(self.repository_root, self.components_path, self.name)

    @property
    def relpath(self) -> str:
        return os.path.join(self.components_path, self.name)

    @property
    def testsRoot(self) -> str:
        return os.path.join(self.repository_root, "tests", "components", self.name)

    def check_esphome_version(self, esphome_version: str) -> VERSION_CHECK:
        if esphome_version < self.esphome_support[0]:
            return VERSION_CHECK.EARLIER_VERSION
        elif esphome_version > self.esphome_support[1]:
            return VERSION_CHECK.LATER_VERSION
        else:
            return VERSION_CHECK.VERSION_IN_RANGE

    @classmethod
    def from_manifest(cls, repository_root: str, manifest: str):
        if not os.path.exists(manifest):
            raise ManifestNotFound

        path_to_component = os.path.dirname(manifest)
        components_abs_path, component_name = os.path.split(path_to_component)
        components_rel_path = os.path.relpath(components_abs_path, repository_root)
        if components_rel_path not in COMPONENT_ROOTS:
            raise UnsupportedComponentPath

        with open(manifest) as f:
            manifest_data = json.load(f)

        if manifest_data["name"] != component_name:
            raise ManifestNameMismatch

        return cls(
            name=component_name,
            repository_root=repository_root,
            components_path=components_rel_path,
            version=manifest_data["version"],
            esphome_support=tuple(manifest_data["esphome"][k] for k in ["min", "max"]),
        )

    @classmethod
    def from_local_repository(cls, repo_path: str, component_path: str):
        """_summary_

        :param repo_path: Path to the local repository.
        :type repo_path: str
        :param component_path: Relative path to the component.
        :type component_path: str
        :raises UnsupportedComponentPath: _description_
        :raises ComponentNotFound: _description_
        :return: _description_
        :rtype: ExternalComponent
        """
        absPath = os.path.join(repo_path, component_path)

        if not os.path.exists(absPath):
            raise ComponentNotFound

        components_path, name = os.path.split(component_path)
        if components_path not in COMPONENT_ROOTS:
            raise UnsupportedComponentPath

        return cls(
            name=name,
            repository_root=repo_path,
            components_path=components_path,
        )

    def __str__(self):
        return f"{self.name}-{self.version} ({self.components_path})"


def get_components_from_repository(path: str) -> list[ExternalComponent]:
    components : list[ExternalComponent] = []
    for cPath in COMPONENT_ROOTS:
        absPath = os.path.join(path, cPath)
        if os.path.exists(absPath):
            components += [
                ExternalComponent.from_manifest(
                    repository_root=path,
                    manifest=os.path.join(cPath, comp, MANIFEST_FILE_NAME),
                )
                for comp in os.listdir(absPath)
                if os.path.exists(os.path.join(absPath, comp, MANIFEST_FILE_NAME))
            ]

    return components


def list_component_git_files(component: ExternalComponent) -> list[str]:
    command = ["git", "ls-files", "-s", os.path.join(component.relpath, "*")]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE)
    output, err = proc.communicate()
    lines = [x.split()[3].strip() for x in output.decode("utf-8").splitlines()]
    return lines
