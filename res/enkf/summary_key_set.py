from cwrap import BaseCClass
from ecl.util.util import StringList

from res import ResPrototype


class SummaryKeySet(BaseCClass):
    TYPE_NAME = "summary_key_set"

    _alloc = ResPrototype("void* summary_key_set_alloc()", bind=False)
    _alloc_from_file = ResPrototype(
        "void* summary_key_set_alloc_from_file(char*, bool)", bind=False
    )
    _free = ResPrototype("void  summary_key_set_free(summary_key_set)")
    _size = ResPrototype("int   summary_key_set_get_size(summary_key_set)")
    _add_key = ResPrototype(
        "bool  summary_key_set_add_summary_key(summary_key_set, char*)"
    )
    _has_key = ResPrototype(
        "bool  summary_key_set_has_summary_key(summary_key_set, char*)"
    )
    _keys = ResPrototype("stringlist_obj summary_key_set_alloc_keys(summary_key_set)")
    _is_read_only = ResPrototype("bool  summary_key_set_is_read_only(summary_key_set)")
    _fwrite = ResPrototype("void  summary_key_set_fwrite(summary_key_set, char*)")

    def __init__(self, filename=None, read_only=False):
        if filename is None:
            c_ptr = self._alloc()
        else:
            c_ptr = self._alloc_from_file(filename, read_only)

        super().__init__(c_ptr)

    def addSummaryKey(self, key):
        assert isinstance(key, str)
        return self._add_key(key)

    def __len__(self):
        return self._size()

    def __contains__(self, key):
        return self._has_key(key)

    def keys(self) -> StringList:
        """@rtype: StringList"""
        return self._keys()

    def isReadOnly(self):
        """@rtype: bool"""
        return self._is_read_only()

    def writeToFile(self, filename):
        assert isinstance(filename, str)
        self._fwrite(filename)

    def free(self):
        self._free()
