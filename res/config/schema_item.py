#  Copyright (C) 2016  Equinor ASA, Norway.
#
#  This file  is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.

from cwrap import BaseCClass

from res import ResPrototype
from res.config.content_type_enum import ContentTypeEnum


class SchemaItem(BaseCClass):
    TYPE_NAME = "schema_item"

    _alloc = ResPrototype("void* config_schema_item_alloc( char* , bool )", bind=False)
    _free = ResPrototype("void config_schema_item_free( schema_item )")
    _iget_type = ResPrototype(
        "config_content_type_enum config_schema_item_iget_type( schema_item, int)"
    )
    _iset_type = ResPrototype(
        "void config_schema_item_iset_type( schema_item , int , config_content_type_enum)"  # noqa
    )
    _set_argc_minmax = ResPrototype(
        "void config_schema_item_set_argc_minmax( schema_item , int , int)"
    )
    _add_alternative = ResPrototype(
        "void config_schema_item_add_indexed_alternative(schema_item , int , char*)"
    )
    _set_deprecated = ResPrototype(
        "void config_schema_item_set_deprecated(schema_item ,  char*)"
    )

    def __init__(self, keyword, required=False):
        c_ptr = self._alloc(keyword, required)
        super().__init__(c_ptr)

    def iget_type(self, index) -> ContentTypeEnum:
        """@rtype: ContentTypeEnum"""
        return self._iget_type(index)

    def iset_type(self, index, schema_type):
        """
        @type schema_type: ContentTypeEnum
        """
        assert isinstance(schema_type, ContentTypeEnum)
        self._iset_type(index, schema_type)

    def set_argc_minmax(self, minimum, maximum):
        self._set_argc_minmax(minimum, maximum)

    def initSelection(self, index, alternatives):
        for alt in alternatives:
            self.addAlternative(index, alt)

    def addAlternative(self, index, alt):
        self._add_alternative(index, alt)

    def setDeprecated(self, msg):
        """This method can be used to mark this item as deprecated.

        If the deprecated item is used in a configuration file the
        @msg will be added to the warnings of the ConfigContent
        object,
        """
        self._set_deprecated(msg)

    def free(self):
        self._free()
