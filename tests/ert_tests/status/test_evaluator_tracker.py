from typing import Any, List, Tuple, Union
from unittest.mock import MagicMock, patch

import pytest
from cloudevents.http.event import CloudEvent

import ert.ensemble_evaluator.identifiers as ids
from ert.ensemble_evaluator.snapshot import (
    PartialSnapshot,
    SnapshotBuilder,
    Step,
)
from ert_shared.ensemble_evaluator.config import EvaluatorServerConfig
from ert_shared.models.base_run_model import BaseRunModel
from ert3.evaluator._evaluator import ERT3RunModel
from ert.ensemble_evaluator import state
from ert.ensemble_evaluator.event import EndEvent, SnapshotUpdateEvent
from ert.ensemble_evaluator import EvaluatorTracker


def build_snapshot(real_list: List[str] = ["0"]):
    # passing ["0"] is required
    return (
        SnapshotBuilder()
        .add_step(step_id="0", status=state.STEP_STATE_UNKNOWN)
        .build(real_list, state.REALIZATION_STATE_UNKNOWN)
    )


def build_partial(real_list: List[str] = ["0"]):
    return PartialSnapshot(build_snapshot(real_list))


@pytest.fixture
def make_mock_ee_monitor():
    def _mock_ee_monitor(events):
        def _track():
            while True:
                try:
                    event = events.pop(0)
                    yield event
                except IndexError:
                    return

        return MagicMock(track=MagicMock(side_effect=_track))

    return _mock_ee_monitor


@pytest.mark.timeout(60)
@pytest.mark.parametrize(
    "run_model, monitor_events,brm_mutations,expected_progress",
    [
        pytest.param(
            ERT3RunModel,
            [
                CloudEvent(
                    # zero realizations completed
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1", "2"]).to_dict()),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0"])
                            # Complete one realizations completed
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            ).to_dict()
                        ),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            # Complete two realizations
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "1", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "2"])
                            # Complete all three realizations
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "2", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 0,
                    },
                ),
            ],
            [("_phase_count", 1)],
            # Expected progress matches combination of yielded snapshotevents
            # It is calculated as sum i=1:#phases of
            # (phase i completed realizations / phase i #realizations) / #phases.
            # When a phase is in the past (current phase > phase),
            # phase progress is set to 1.0
            [
                (0 / 3) / 1,
                (1 / 3) / 1,
                (2 / 3) / 1,
                (3 / 3) / 1,
            ],
            id="ensemble_experiment_100",
        ),
        pytest.param(
            BaseRunModel,
            [
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 0,
                    },
                ),
            ],
            [("_phase_count", 1)],
            [(0 / 2) / 1, (1 / 2) / 1],
            id="ensemble_experiment_50",
        ),
        pytest.param(
            BaseRunModel,
            [
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 0,
                    },
                ),
            ],
            [("_phase_count", 2)],
            [(0 / 2 + 0) / 2, (1 / 2 + 0) / 2],
            id="ensemble_smoother_25",
        ),
        pytest.param(
            BaseRunModel,
            [
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "1", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 1,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 1,
                    },
                ),
            ],
            [("_phase_count", 2)],
            [
                (0 / 2 + 0 / 2) / 2,
                (2 / 2 + 0 / 2) / 2,
                (2 / 2 + 0 / 2) / 2,
                (2 / 2 + 1 / 2) / 2,
            ],
            id="ensemble_smoother_75",
        ),
        pytest.param(
            BaseRunModel,
            [
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "1", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 0,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 1,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "1", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 1,
                    },
                ),
            ],
            [("_phase_count", 2)],
            [
                (0 / 2 + 0 / 2) / 2,
                (2 / 2 + 0 / 2) / 2,
                (2 / 2 + 0 / 2) / 2,
                (2 / 2 + 2 / 2) / 2,
            ],
            id="ensemble_smoother_100",
        ),
        pytest.param(
            BaseRunModel,
            [
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 1,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "1", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 1,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT},
                    data={
                        **(build_snapshot(["0", "1"]).to_dict()),
                        "iter": 2,
                    },
                ),
                CloudEvent(
                    {"source": "/", "type": ids.EVTYPE_EE_SNAPSHOT_UPDATE},
                    data={
                        **(
                            build_partial(["0", "1"])
                            .update_step(
                                "0", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .update_step(
                                "1", "0", Step(status=state.STEP_STATE_SUCCESS)
                            )
                            .to_dict()
                        ),
                        "iter": 2,
                    },
                ),
            ],
            [("_phase_count", 3)],
            [
                (1 + 0 / 2 + 0 / 2) / 3,
                (1 + 2 / 2 + 0 / 2) / 3,
                (1 + 2 / 2 + 0 / 2) / 3,
                (1 + 2 / 2 + 2 / 2) / 3,
            ],
            id="ensemble_smoother_100",
        ),
    ],
)
def test_tracking_progress(
    run_model: Union[BaseRunModel, ERT3RunModel],
    monitor_events: List[CloudEvent],
    brm_mutations: List[Tuple[str, Any]],
    expected_progress: float,
    make_mock_ee_monitor,
):
    """Tests progress by providing a list of CloudEvent and a list of
    arguments to apply to setattr(brm) where brm is an actual BaseRunModel
    instance.

    The CloudEvent are provided to the tracker via mocking an Ensemble
    Evaluator Monitor.

    PartialSnapshots allow realizations to progress, while iterating "iter" in
    CloudEvents allows phases to progress. Such progress should happen
    when events are yielded by the tracker. This combined progress is tested.

    The final update event and end event is also tested."""

    if issubclass(run_model, ERT3RunModel):
        brm = run_model()
    else:
        brm = run_model(None, None, None)
    ee_config = EvaluatorServerConfig(
        custom_port_range=range(1024, 65535), custom_host="127.0.0.1"
    )
    with patch(
        "ert.ensemble_evaluator.tracker.evaluator_tracker.create_ee_monitor"
    ) as mock_ee:
        mock_ee.return_value.__enter__.return_value = make_mock_ee_monitor(
            monitor_events.copy()
        )
        tracker = EvaluatorTracker(
            brm, ee_config.get_connection_info(), next_ensemble_evaluator_wait_time=0.1
        )
        for attr, val in brm_mutations:
            setattr(brm, attr, val)
        tracker_gen = tracker.track()
        update_event = None
        for i in range(len(monitor_events)):
            update_event = next(tracker_gen)
            assert update_event.progress == expected_progress[i]
        assert isinstance(update_event, SnapshotUpdateEvent)
        brm._phase = brm._phase_count
        assert isinstance(next(tracker_gen), EndEvent)
