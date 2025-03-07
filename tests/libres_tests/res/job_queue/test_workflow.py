from ecl.util.test import TestAreaContext
from libres_utils import ResTest
from workflow_common import WorkflowCommon

from res.job_queue import Workflow, WorkflowJoblist
from res.util.substitution_list import SubstitutionList


class WorkflowTest(ResTest):
    def test_workflow(self):
        with TestAreaContext("python/job_queue/workflow"):
            WorkflowCommon.createExternalDumpJob()

            joblist = WorkflowJoblist()
            self.assertTrue(joblist.addJobFromFile("DUMP", "dump_job"))

            with self.assertRaises(UserWarning):
                joblist.addJobFromFile("KNOCK", "knock_job")

            self.assertTrue("DUMP" in joblist)

            workflow = Workflow("dump_workflow", joblist)

            self.assertEqual(len(workflow), 2)

            job, args = workflow[0]
            self.assertEqual(job, joblist["DUMP"])
            self.assertEqual(args[0], "dump1")
            self.assertEqual(args[1], "dump_text_1")

            job, args = workflow[1]
            self.assertEqual(job, joblist["DUMP"])

    def test_workflow_run(self):
        with TestAreaContext("python/job_queue/workflow"):
            WorkflowCommon.createExternalDumpJob()

            joblist = WorkflowJoblist()
            self.assertTrue(joblist.addJobFromFile("DUMP", "dump_job"))
            self.assertTrue("DUMP" in joblist)

            workflow = Workflow("dump_workflow", joblist)

            self.assertTrue(len(workflow), 2)

            context = SubstitutionList()
            context.addItem("<PARAM>", "text")

            self.assertTrue(workflow.run(None, verbose=True, context=context))

            with open("dump1", "r") as f:
                self.assertEqual(f.read(), "dump_text_1")

            with open("dump2", "r") as f:
                self.assertEqual(f.read(), "dump_text_2")

    def test_failing_workflow_run(self):
        with TestAreaContext("python/job_queue/workflow"):
            WorkflowCommon.createExternalDumpJob()

            joblist = WorkflowJoblist()
            self.assertTrue(joblist.addJobFromFile("DUMP", "dump_job"))
            self.assertTrue("DUMP" in joblist)

            workflow = Workflow("undefined", joblist)
            context = SubstitutionList()

            self.assertFalse(workflow.run(None, verbose=True, context=context))
