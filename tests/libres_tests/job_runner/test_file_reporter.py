import os
import os.path
from unittest import TestCase

from libres_utils import tmpdir

from job_runner.job import Job
from job_runner.reporting import File
from job_runner.reporting.message import Exited, Finish, Init, Running, Start


class FileReporterTests(TestCase):
    def setUp(self):
        self.reporter = File(sync_disc_timeout=0)

    @tmpdir(None)
    def test_report_with_init_message_argument(self):
        r = self.reporter
        job1 = Job({"name": "job1", "stdout": "/stdout", "stderr": "/stderr"}, 0)

        r.report(Init([job1], 1, 19))

        with open(self.reporter.STATUS_file, "r") as f:
            self.assertIn(
                "Current host", f.readline(), "STATUS file missing expected value"
            )
        with open(self.reporter.STATUS_json, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn('"name": "job1"', contents, "status.json missing job1")
            self.assertIn(
                '"status": "Waiting"', contents, "status.json missing Waiting status"
            )

    @tmpdir(None)
    def test_report_with_successful_start_message_argument(self):
        msg = Start(
            Job(
                {
                    "name": "job1",
                    "stdout": "/stdout.0",
                    "stderr": "/stderr.0",
                    "argList": ["--foo", "1", "--bar", "2"],
                    "executable": "/bin/bash",
                },
                0,
            )
        )
        self.reporter.status_dict = self.reporter._init_job_status_dict(
            msg.timestamp, 0, [msg.job]
        )

        self.reporter.report(msg)

        with open(self.reporter.STATUS_file, "r") as f:
            self.assertIn("job1", f.readline(), "STATUS file missing job1")
        with open(self.reporter.LOG_file, "r") as f:
            self.assertIn(
                "Calling: /bin/bash --foo 1 --bar 2",
                f.readline(),
                """JOB_LOG file missing executable and arguments""",
            )

        with open(self.reporter.STATUS_json, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn(
                '"status": "Running"', contents, "status.json missing Running status"
            )
            self.assertNotIn('"start_time": null', contents, "start_time not set")

    @tmpdir(None)
    def test_report_with_failed_start_message_argument(self):
        msg = Start(Job({"name": "job1"}, 0)).with_error("massive_failure")
        self.reporter.status_dict = self.reporter._init_job_status_dict(
            msg.timestamp, 0, [msg.job]
        )

        self.reporter.report(msg)

        with open(self.reporter.STATUS_file, "r") as f:
            self.assertIn(
                "EXIT: -10/massive_failure",
                f.readline(),
                "STATUS file missing EXIT message",
            )
        with open(self.reporter.STATUS_json, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn(
                '"status": "Failure"', contents, "status.json missing Failure status"
            )
            self.assertIn(
                '"error": "massive_failure"',
                contents,
                "status.json missing error message",
            )
        self.assertIsNotNone(
            self.reporter.status_dict["jobs"][0]["end_time"],
            "end_time not set for job1",
        )

    @tmpdir(None)
    def test_report_with_successful_exit_message_argument(self):
        msg = Exited(Job({"name": "job1"}, 0), 0)
        self.reporter.status_dict = self.reporter._init_job_status_dict(
            msg.timestamp, 0, [msg.job]
        )

        self.reporter.report(msg)

        with open(self.reporter.STATUS_json, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn(
                '"status": "Success"', contents, "status.json missing Success status"
            )

    @tmpdir(None)
    def test_report_with_failed_exit_message_argument(self):
        msg = Exited(Job({"name": "job1"}, 0), 1).with_error("massive_failure")
        self.reporter.status_dict = self.reporter._init_job_status_dict(
            msg.timestamp, 0, [msg.job]
        )

        self.reporter.report(msg)

        with open(self.reporter.STATUS_file, "r") as f:
            self.assertIn("EXIT: 1/massive_failure", f.readline())
        with open(self.reporter.ERROR_file, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn("<job>job1</job>", contents, "ERROR file missing job")
            self.assertIn(
                "<reason>massive_failure</reason>",
                contents,
                "ERROR file missing reason",
            )
            self.assertIn(
                "<stderr: Not redirected>",
                contents,
                "ERROR had invalid stderr information",
            )
        with open(self.reporter.STATUS_json, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn(
                '"status": "Failure"', contents, "status.json missing Failure status"
            )
            self.assertIn(
                '"error": "massive_failure"',
                contents,
                "status.json missing error message",
            )
        self.assertIsNotNone(self.reporter.status_dict["jobs"][0]["end_time"])

    @tmpdir(None)
    def test_report_with_running_message_argument(self):
        msg = Running(Job({"name": "job1"}, 0), 100, 10)
        self.reporter.status_dict = self.reporter._init_job_status_dict(
            msg.timestamp, 0, [msg.job]
        )

        self.reporter.report(msg)

        with open(self.reporter.STATUS_json, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn('"status": "Running"', contents, "status.json missing status")
            self.assertIn(
                '"max_memory_usage": 100',
                contents,
                "status.json missing max_memory_usage",
            )
            self.assertIn(
                '"current_memory_usage": 10',
                contents,
                "status.json missing current_memory_usage",
            )

    @tmpdir(None)
    def test_report_with_successful_finish_message_argument(self):
        msg = Finish()
        self.reporter.status_dict = self.reporter._init_job_status_dict(
            msg.timestamp, 0, []
        )

        self.reporter.report(msg)

        with open(self.reporter.OK_file, "r") as f:
            self.assertIn(
                "All jobs complete", f.readline(), "OK file missing expected value"
            )

    @tmpdir(None)
    def test_dump_error_file_with_stderr(self):
        """
        Assert that, in the case of an stderr file, it is included in the XML
        that constitutes the ERROR file.
        The report system is left out, since this was tested in the fail case.
        """
        with open("stderr.out.0", "a") as stderr:
            stderr.write("Error:\n")
            stderr.write("E_MASSIVE_FAILURE\n")

        self.reporter._dump_error_file(
            Job({"name": "job1", "stderr": "stderr.out.0"}, 0), "massive_failure"
        )

        with open(self.reporter.ERROR_file, "r") as f:
            contents = "".join(f.readlines())
            self.assertIn(
                "E_MASSIVE_FAILURE", contents, "ERROR file missing stderr content"
            )
            self.assertIn("<stderr_file>", contents, "ERROR missing stderr_file part")

    @tmpdir(None)
    def test_old_file_deletion(self):
        r = self.reporter
        # touch all files that are to be removed
        for f in [r.ERROR_file, r.STATUS_file, r.OK_file]:
            open(f, "a").close()

        r._delete_old_status_files()

        for f in [r.ERROR_file, r.STATUS_file, r.OK_file]:
            self.assertFalse(os.path.isfile(f), f"{r} was not deleted")

    @tmpdir(None)
    def test_status_file_is_correct(self):
        """The STATUS file is a file to which we append data about jobs as they
        are run. So this involves multiple reports, and should be tested as
        such.
        See https://github.com/equinor/libres/issues/764
        """
        j_1 = Job({"name": "j_1", "executable": "", "argList": []}, 0)
        j_2 = Job({"name": "j_2", "executable": "", "argList": []}, 0)
        init = Init([j_1, j_2], 1, 1)
        start_j_1 = Start(j_1)
        exited_j_1 = Exited(j_1, 0)
        start_j_2 = Start(j_2)
        exited_j_2 = Exited(j_2, 9).with_error("failed horribly")

        for msg in [init, start_j_1, exited_j_1, start_j_2, exited_j_2]:
            self.reporter.report(msg)

        expected_j1_line = (
            f"{j_1.name():32}: {start_j_1.timestamp:%H:%M:%S} .... "
            f"{exited_j_1.timestamp:%H:%M:%S}  \n"
        )
        expected_j2_line = (
            f"{j_2.name():32}: "
            f"{start_j_2.timestamp:%H:%M:%S} .... "
            f"{exited_j_2.timestamp:%H:%M:%S}   "
            f"EXIT: {exited_j_2.exit_code}/{exited_j_2.error_message}\n"
        )

        with open(self.reporter.STATUS_file, "r") as f:
            for expected in [
                "Current host",
                expected_j1_line,
                expected_j2_line,
            ]:  # noqa
                self.assertIn(expected, f.readline())

            # EOF
            self.assertEqual("", f.readline())
