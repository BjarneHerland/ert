from argparse import ArgumentParser

from requests import Response
import uuid

from ert_shared.cli import ENSEMBLE_SMOOTHER_MODE
from ert_shared.cli.main import run_cli
from ert_shared.main import ert_parser

import pandas as pd
import io


def test_get_experiment(poly_example_tmp_dir, dark_storage_client):
    resp: Response = dark_storage_client.get("/experiments")
    answer_json = resp.json()
    assert len(answer_json) == 1
    assert "ensemble_ids" in answer_json[0]
    assert len(answer_json[0]["ensemble_ids"]) == 1
    assert "name" in answer_json[0]
    assert answer_json[0]["name"] == "default"


def test_get_ensemble(poly_example_tmp_dir, dark_storage_client):
    resp: Response = dark_storage_client.get("/experiments")
    experiment_json = resp.json()
    assert len(experiment_json) == 1
    assert len(experiment_json[0]["ensemble_ids"]) == 1

    ensemble_id = experiment_json[0]["ensemble_ids"][0]

    resp: Response = dark_storage_client.get(f"/ensembles/{ensemble_id}")
    ensemble_json = resp.json()

    assert ensemble_json["experiment_id"] == experiment_json[0]["id"]
    assert ensemble_json["userdata"]["name"] == "default"


def test_get_experiment_ensemble(poly_example_tmp_dir, dark_storage_client):
    resp: Response = dark_storage_client.get("/experiments")
    experiment_json = resp.json()
    assert len(experiment_json) == 1
    assert len(experiment_json[0]["ensemble_ids"]) == 1

    experiment_id = experiment_json[0]["id"]

    resp: Response = dark_storage_client.get(f"/experiments/{experiment_id}/ensembles")
    ensembles_json = resp.json()

    assert len(ensembles_json) == 1
    assert ensembles_json[0]["experiment_id"] == experiment_json[0]["id"]
    assert ensembles_json[0]["userdata"]["name"] == "default"


def test_get_response(poly_example_tmp_dir, dark_storage_client):
    parser = ArgumentParser(prog="test_main")
    parsed = ert_parser(
        parser,
        [
            ENSEMBLE_SMOOTHER_MODE,
            "--target-case",
            "poly_runpath_file",
            "--realizations",
            "1,2,4",
            "poly.ert",
        ],
    )

    run_cli(parsed)

    resp: Response = dark_storage_client.get("/experiments")
    experiment_json = resp.json()

    ensemble_id1 = experiment_json[0]["ensemble_ids"][0]
    ensemble_id2 = experiment_json[0]["ensemble_ids"][1]

    resp: Response = dark_storage_client.get(f"/ensembles/{ensemble_id1}")
    ensemble_json = resp.json()
    assert ensemble_json["response_names"][0] == "POLY_RES@0"

    resp: Response = dark_storage_client.get(f"/ensembles/{ensemble_id2}")
    ensemble_json2 = resp.json()
    assert ensemble_json2["response_names"][0] == "POLY_RES@0"

    resp: Response = dark_storage_client.get(
        f"/ensembles/{ensemble_id1}/responses/POLY_RES@0/data"
    )
    stream = io.BytesIO(resp.content)
    response_df1 = pd.read_csv(stream, index_col=0, float_precision="round_trip")

    resp: Response = dark_storage_client.get(
        f"/ensembles/{ensemble_id2}/responses/POLY_RES@0/data"
    )
    stream = io.BytesIO(resp.content)
    response_df2 = pd.read_csv(stream, index_col=0, float_precision="round_trip")

    assert len(response_df1.columns) == 10
    assert len(response_df1.index) == 3

    assert len(response_df2.columns) == 10
    assert len(response_df2.index) == 3


def test_get_ensemble_parameters(poly_example_tmp_dir, dark_storage_client):
    ensemble_id = uuid.uuid4()
    resp: Response = dark_storage_client.get(f"/ensembles/{ensemble_id}/parameters")
    parameters_json = resp.json()

    assert len(parameters_json) == 3
    assert parameters_json[0]["name"] == "COEFFS:COEFF_A"
    assert parameters_json[1]["name"] == "COEFFS:COEFF_B"
    assert parameters_json[2]["name"] == "COEFFS:COEFF_C"


def test_get_experiment_observations(poly_example_tmp_dir, dark_storage_client):
    experiment_id = uuid.uuid4()
    resp: Response = dark_storage_client.get(
        f"/experiments/{experiment_id}/observations"
    )
    response_json = resp.json()

    assert len(response_json) == 1
    assert response_json[0]["name"] == "POLY_OBS"
    assert len(response_json[0]["errors"]) == 5
    assert len(response_json[0]["values"]) == 5
    assert len(response_json[0]["x_axis"]) == 5