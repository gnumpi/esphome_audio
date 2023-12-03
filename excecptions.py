class ExceptionBase(Exception):
    pass

class ComponentNotFound(ExceptionBase):
    pass

class UnsupportedComponentPath(ExceptionBase):
    pass

class ManifestNotFound(ExceptionBase):
    pass

class ManifestNameMismatch(ExceptionBase):
    pass
