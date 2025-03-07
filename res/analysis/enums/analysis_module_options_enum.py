#  Copyright (C) 2013  Equinor ASA, Norway.
#
#  The file 'analysis_module_options_enum.py' is part of ERT.
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
from cwrap import BaseCEnum


class AnalysisModuleOptionsEnum(BaseCEnum):
    TYPE_NAME = "analysis_module_options_enum"
    ANALYSIS_USE_A = None
    ANALYSIS_UPDATE_A = None
    ANALYSIS_ITERABLE = None


AnalysisModuleOptionsEnum.addEnum("ANALYSIS_USE_A", 4)
AnalysisModuleOptionsEnum.addEnum("ANALYSIS_UPDATE_A", 8)
AnalysisModuleOptionsEnum.addEnum("ANALYSIS_ITERABLE", 32)
