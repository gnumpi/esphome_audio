import os
import enum

COMPONENT_ROOTS = ["components", os.path.join("esphome", "components")]

MANIFEST_FILE_NAME = "manifest.json"


class VERSION_CHECK(enum.Enum):
    VERSION_IN_RANGE = 0
    EARLIER_VERSION = -1
    LATER_VERSION = 1


class CHECK_RET(enum.Enum):
    SUCCESS = 0
    ERROR = 1
    WARNING = 2
    UNKNOWN = 3
