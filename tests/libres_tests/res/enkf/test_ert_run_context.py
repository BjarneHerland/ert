#  Copyright (C) 2018  Equinor ASA, Norway.
#
#  The file 'test_ert_run_context.py' is part of ERT - Ensemble based Reservoir Tool.
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


from libres_utils import ResTest

from res.enkf import ErtRunContext


class ErtRunContextTest(ResTest):
    def test_case_init(self):
        mask = [True] * 100
        ErtRunContext.case_init(None, mask)
