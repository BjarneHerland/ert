#  Copyright (C) 2018  Equinor ASA, Norway.
#
#  The file 'test_ecl.py' is part of ERT - Ensemble based Reservoir Tool.
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
import inspect
import json
import os
import re
import shutil
import stat
from unittest import mock

import pytest
import yaml
from ecl.summary import EclSum
from libres_utils import ResTest, tmpdir
from pytest import MonkeyPatch

from res.fm.ecl import Ecl100Config, EclRun, EclrunConfig, run


def find_version(output):
    return re.search(r"flow\s*([\d.]+)", output).group(1)


class EclRunTest(ResTest):
    def setUp(self):
        self.ecl_config_path = os.path.dirname(inspect.getsourcefile(Ecl100Config))
        self.monkeypatch = MonkeyPatch()

    def tearDown(self):
        self.monkeypatch.undo()

    @staticmethod
    def _eclrun_conf():
        return {
            "eclrun_env": {
                "SLBSLS_LICENSE_FILE": "7321@eclipse-lic-no.statoil.no",
                "ECLPATH": "/prog/res/ecl/grid",
                "PATH": "/prog/res/ecl/grid/macros",
                "F_UFMTENDIAN": "big",
                "LSB_JOB_ID": None,
            }
        }

    def init_eclrun_config(self):
        conf = EclRunTest._eclrun_conf()
        with open("ecl100_config.yml", "w") as f:
            f.write(yaml.dump(conf))
        self.monkeypatch.setenv("ECL100_SITE_CONFIG", "ecl100_config.yml")

    def test_get_version_raise(self):
        ecl_config = Ecl100Config()
        class_file = inspect.getfile(Ecl100Config)
        class_dir = os.path.dirname(os.path.abspath(class_file))
        msg = os.path.join(class_dir, "ecl100_config.yml")
        with pytest.raises(ValueError, match=msg):
            ecl_config._get_version(None)

    @tmpdir()
    @mock.patch.dict(os.environ, {"LSB_JOBID": "some-id"})
    def test_env(self):
        self.init_eclrun_config()
        with open("eclrun", "w") as f, open("DUMMY.DATA", "w"):
            f.write(
                """#!/usr/bin/env python
import os
import json
with open("env.json", "w") as f:
    json.dump(dict(os.environ), f)
"""
            )
        os.chmod("eclrun", os.stat("eclrun").st_mode | stat.S_IEXEC)
        ecl_config = Ecl100Config()
        eclrun_config = EclrunConfig(ecl_config, "2019.3")
        ecl_run = EclRun("DUMMY", None, check_status=False)
        with mock.patch.object(
            ecl_run, "_get_run_command", mock.MagicMock(return_value="./eclrun")
        ):
            ecl_run.runEclipse(eclrun_config=eclrun_config)
        with open("env.json") as f:
            run_env = json.load(f)

        eclrun_env = self._eclrun_conf()["eclrun_env"]
        for k, v in eclrun_env.items():
            if v is None:
                assert k not in run_env
                continue

            if k == "PATH":
                assert run_env[k].startswith(v)
            else:
                assert v == run_env[k]

    @tmpdir()
    @pytest.mark.equinor_test
    def test_run(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1.DATA"),
            "SPE1.DATA",
        )
        ecl_config = Ecl100Config()

        ecl_run = EclRun("SPE1.DATA", None)
        ecl_run.runEclipse(eclrun_config=EclrunConfig(ecl_config, "2019.1"))

        ok_path = os.path.join(ecl_run.runPath(), f"{ecl_run.baseName()}.OK")
        log_path = os.path.join(ecl_run.runPath(), f"{ecl_run.baseName()}.LOG")

        self.assertTrue(os.path.isfile(ok_path))
        self.assertTrue(os.path.isfile(log_path))
        self.assertTrue(os.path.getsize(log_path) > 0)

        errors = ecl_run.parseErrors()
        self.assertEqual(0, len(errors))

    @tmpdir()
    @pytest.mark.equinor_test
    def test_run_new_log_file(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1.DATA"),
            "SPE1.DATA",
        )
        ecl_config = Ecl100Config()

        ecl_run = EclRun("SPE1.DATA", None)
        ecl_run.runEclipse(eclrun_config=EclrunConfig(ecl_config, "2019.3"))

        ok_path = os.path.join(ecl_run.runPath(), f"{ecl_run.baseName()}.OK")
        log_path = os.path.join(ecl_run.runPath(), f"{ecl_run.baseName()}.OUT")

        self.assertTrue(os.path.isfile(ok_path))
        self.assertTrue(os.path.isfile(log_path))
        self.assertTrue(os.path.getsize(log_path) > 0)

        errors = ecl_run.parseErrors()
        self.assertEqual(0, len(errors))

    @pytest.mark.equinor_test
    @tmpdir()
    def test_run_api(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1.DATA"),
            "SPE1.DATA",
        )
        ecl_config = Ecl100Config()
        run(ecl_config, ["SPE1.DATA", "--version=2019.3"])

        self.assertTrue(os.path.isfile("SPE1.DATA"))

    @pytest.mark.equinor_test
    @tmpdir()
    def test_failed_run(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1_ERROR.DATA"),
            "SPE1_ERROR.DATA",
        )
        ecl_config = Ecl100Config()
        eclrun_config = EclrunConfig(ecl_config, "2019.3")
        ecl_run = EclRun("SPE1_ERROR", None)
        with self.assertRaises(Exception) as error_context:
            ecl_run.runEclipse(eclrun_config=eclrun_config)
        self.assertIn("ERROR", str(error_context.exception))

    @pytest.mark.equinor_test
    @tmpdir()
    def test_failed_run_OK(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1_ERROR.DATA"),
            "SPE1_ERROR.DATA",
        )
        ecl_config = Ecl100Config()
        run(ecl_config, ["SPE1_ERROR", "--version=2019.3", "--ignore-errors"])

    @pytest.mark.equinor_test
    @tmpdir()
    def test_no_hdf5_output_by_default_with_ecl100(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1.DATA"),
            "SPE1.DATA",
        )
        ecl_config = Ecl100Config()
        # check that by default .h5 file IS NOT produced
        run(ecl_config, ["SPE1.DATA", "--version=2019.3"])
        self.assertFalse(os.path.exists("SPE1.h5"))

    @pytest.mark.equinor_test
    @tmpdir()
    def test_flag_to_produce_hdf5_output_with_ecl100(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1.DATA"),
            "SPE1.DATA",
        )
        ecl_config = Ecl100Config()
        # check that with flag .h5 file IS produced
        run(ecl_config, ["SPE1.DATA", "--version=2019.3", "--summary-conversion"])
        self.assertTrue(os.path.exists("SPE1.h5"))

    @pytest.mark.equinor_test
    @tmpdir()
    def test_mpi_run(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1_PARALLELL.DATA"),
            "SPE1_PARALLELL.DATA",
        )
        ecl_config = Ecl100Config()
        run(ecl_config, ["SPE1_PARALLELL.DATA", "--version=2019.3", "--num-cpu=2"])
        self.assertTrue(os.path.isfile("SPE1_PARALLELL.OUT"))
        self.assertTrue(os.path.getsize("SPE1_PARALLELL.OUT") > 0)

    @pytest.mark.equinor_test
    @tmpdir()
    def test_summary_block(self):
        self.init_eclrun_config()
        shutil.copy(
            os.path.join(self.TESTDATA_ROOT, "local/eclipse/SPE1.DATA"),
            "SPE1.DATA",
        )
        ecl_config = Ecl100Config()
        ecl_run = EclRun("SPE1.DATA", None)
        ret_value = ecl_run.summary_block()
        self.assertTrue(ret_value is None)

        ecl_run.runEclipse(eclrun_config=EclrunConfig(ecl_config, "2019.3"))
        ecl_sum = ecl_run.summary_block()
        self.assertTrue(isinstance(ecl_sum, EclSum))

    @pytest.mark.equinor_test
    @tmpdir()
    def test_check(self):
        full_case = os.path.join(self.TESTDATA_ROOT, "Equinor/ECLIPSE/Gurbat/ECLIPSE")
        short_case = os.path.join(
            self.TESTDATA_ROOT, "Equinor/ECLIPSE/ShortSummary/ECLIPSE"
        )
        failed_case = os.path.join(
            self.SOURCE_ROOT,
            "test-data/Equinor/ECLIPSE/SummaryFail/NOR-2013A_R002_1208-0",
        )

        with self.assertRaises(IOError):
            self.assertTrue(EclRun.checkCase(full_case, failed_case))

        with self.assertRaises(IOError):
            self.assertTrue(EclRun.checkCase(full_case, "DOES-NOT-EXIST"))

        with self.assertRaises(IOError):
            self.assertTrue(EclRun.checkCase("DOES-NOT-EXIST", full_case))

        with self.assertRaises(ValueError):
            EclRun.checkCase(full_case, short_case)

        self.assertTrue(not os.path.isfile("CHECK_ECLIPSE_RUN.OK"))
        self.assertTrue(EclRun.checkCase(full_case, full_case))
        self.assertTrue(os.path.isfile("CHECK_ECLIPSE_RUN.OK"))

        os.remove("CHECK_ECLIPSE_RUN.OK")
        self.assertTrue(
            EclRun.checkCase(short_case, full_case)
        )  # Simulation is longer than refcase - OK
        self.assertTrue(os.path.isfile("CHECK_ECLIPSE_RUN.OK"))
