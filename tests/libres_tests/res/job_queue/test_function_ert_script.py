from ecl.util.test import TestAreaContext
from libres_utils import ResTest
from workflow_common import WorkflowCommon

from res.job_queue import WorkflowJob


class FunctionErtScriptTest(ResTest):
    def test_compare(self):
        with TestAreaContext("python/job_queue/workflow_job"):
            WorkflowCommon.createInternalFunctionJob()

            parser = WorkflowJob.configParser()
            with self.assertRaises(IOError):
                workflow_job = WorkflowJob.fromFile("no/such/file")

            workflow_job = WorkflowJob.fromFile(
                "compare_job", name="COMPARE", parser=parser
            )
            self.assertEqual(workflow_job.name(), "COMPARE")

            result = workflow_job.run(None, ["String", "string"])
            self.assertNotEqual(result, 0)

            result = workflow_job.run(None, ["String", "String"])
            # result is returned as c_void_p -> automatic conversion to None if
            # value is 0
            self.assertIsNone(result)

            workflow_job = WorkflowJob.fromFile("compare_job")
            self.assertEqual(workflow_job.name(), "compare_job")
