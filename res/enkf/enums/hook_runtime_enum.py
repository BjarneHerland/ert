#  Copyright (C) 2016  Equinor ASA, Norway.
#
#  The file 'hook_runtime_enum.py' is part of ERT - Ensemble based Reservoir Tool.
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


class HookRuntime(BaseCEnum):
    TYPE_NAME = "hook_runtime_enum"
    PRE_SIMULATION = 0
    POST_SIMULATION = 1
    PRE_UPDATE = 2
    POST_UPDATE = 3
    PRE_FIRST_UPDATE = 4


HookRuntime.addEnum("PRE_SIMULATION", 0)
HookRuntime.addEnum("POST_SIMULATION", 1)
HookRuntime.addEnum("PRE_UPDATE", 2)
HookRuntime.addEnum("POST_UPDATE", 3)
HookRuntime.addEnum("PRE_FIRST_UPDATE", 4)
