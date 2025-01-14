import pytest

import ert3

# The inverse cumulative normal distribution function evaluated at 0.995
CNORM_INV = 2.57582930

# The inverse cumulative uniform distribution function evaluated at 0.995
CUNI_INV = 0.005


def test_no_parameters():
    with pytest.raises(ValueError):
        ert3.algorithms.one_at_a_time([])


distributions_and_sens = pytest.mark.parametrize(
    ("distribution", "a", "b", "sens_low", "sens_high"),
    (
        (ert3.stats.Gaussian, 0, 1, -CNORM_INV, CNORM_INV),
        (ert3.stats.Uniform, 0, 1, CUNI_INV, 1 - CUNI_INV),
    ),
)


@distributions_and_sens
def test_scalar_parameter(distribution, a, b, sens_low, sens_high):
    single_dist = distribution(a, b)

    evaluations = ert3.algorithms.one_at_a_time({"single": single_dist})

    assert 2 == len(evaluations)
    for idx, parameter_value in enumerate([sens_low, sens_high]):
        evali = evaluations[idx]
        assert ["single"] == list(evali.keys())
        assert evali["single"].data == pytest.approx(parameter_value)


@distributions_and_sens
def test_size_1_parameter(distribution, a, b, sens_low, sens_high):
    single_dist = distribution(a, b, size=1)
    evaluations = ert3.algorithms.one_at_a_time({"single": single_dist})

    assert 2 == len(evaluations)
    for idx, parameter_value in enumerate([sens_low, sens_high]):
        evali = evaluations[idx]
        assert ["single"] == list(evali.keys())
        assert 1 == len(evali["single"].data)
        for val in evali["single"].data:
            assert parameter_value == pytest.approx(val)


def test_parameter_array():
    size = 10
    gauss_array = ert3.stats.Gaussian(0, 1, size=size)
    evaluations = ert3.algorithms.one_at_a_time({"array": gauss_array})

    assert 2 * size == len(evaluations)
    for eidx, evali in enumerate(evaluations):
        parameter_value = -CNORM_INV if eidx % 2 == 0 else CNORM_INV
        assert ["array"] == list(evali.keys())
        assert size == len(evali["array"].data)
        for vidx, val in enumerate(evali["array"].data):
            expected_value = parameter_value if vidx == eidx // 2 else 0
            assert expected_value == pytest.approx(val)


def test_parameter_index():
    index = ["a" * i + str(i) for i in range(5)]
    gauss_index = ert3.stats.Gaussian(0, 1, index=index)
    evaluations = ert3.algorithms.one_at_a_time({"indexed_gauss": gauss_index})

    assert 2 * len(index) == len(evaluations)
    for eidx, evali in enumerate(evaluations):
        parameter_value = -CNORM_INV if eidx % 2 == 0 else CNORM_INV
        assert ["indexed_gauss"] == list(evali.keys())
        assert sorted(index) == sorted(evali["indexed_gauss"].index)
        for kidx, key in enumerate(index):
            expected_value = parameter_value if kidx == eidx // 2 else 0
            assert expected_value == pytest.approx(evali["indexed_gauss"].data[key])


def test_multi_parameter_singletons():
    expected_evaluations = [
        {"a": [-CNORM_INV], "b": [0]},
        {"a": [CNORM_INV], "b": [0]},
        {"a": [0], "b": [-CNORM_INV]},
        {"a": [0], "b": [CNORM_INV]},
    ]

    records = {
        "a": ert3.stats.Gaussian(0, 1, size=1),
        "b": ert3.stats.Gaussian(0, 1, size=1),
    }
    evaluations = ert3.algorithms.one_at_a_time(records)

    assert len(expected_evaluations) == len(evaluations)
    for expected, result in zip(expected_evaluations, evaluations):
        assert expected.keys() == result.keys()
        for key in expected.keys():
            assert len(expected[key]) == len(result[key].data)
            for e, r in zip(expected[key], result[key].data):
                assert e == pytest.approx(r)


def test_multi_parameter_doubles():
    expected_evaluations = [
        {"a": [-CNORM_INV, 0], "b": [0, 0]},
        {"a": [CNORM_INV, 0], "b": [0, 0]},
        {"a": [0, -CNORM_INV], "b": [0, 0]},
        {"a": [0, CNORM_INV], "b": [0, 0]},
        {"a": [0, 0], "b": [-CNORM_INV, 0]},
        {"a": [0, 0], "b": [CNORM_INV, 0]},
        {"a": [0, 0], "b": [0, -CNORM_INV]},
        {"a": [0, 0], "b": [0, CNORM_INV]},
    ]

    records = {
        "a": ert3.stats.Gaussian(0, 1, size=2),
        "b": ert3.stats.Gaussian(0, 1, size=2),
    }
    evaluations = ert3.algorithms.one_at_a_time(records)

    assert len(expected_evaluations) == len(evaluations)
    for expected, result in zip(expected_evaluations, evaluations):
        assert expected.keys() == result.keys()
        for key in expected.keys():
            assert len(expected[key]) == len(result[key].data)
            for e, r in zip(expected[key], result[key].data):
                assert e == pytest.approx(r)


def test_uni_and_norm():
    expected_evaluations = [
        {"a": [-CNORM_INV, 0], "b": [0.5, 0.5]},
        {"a": [CNORM_INV, 0], "b": [0.5, 0.5]},
        {"a": [0, -CNORM_INV], "b": [0.5, 0.5]},
        {"a": [0, CNORM_INV], "b": [0.5, 0.5]},
        {"a": [0, 0], "b": [CUNI_INV, 0.5]},
        {"a": [0, 0], "b": [1 - CUNI_INV, 0.5]},
        {"a": [0, 0], "b": [0.5, CUNI_INV]},
        {"a": [0, 0], "b": [0.5, 1 - CUNI_INV]},
    ]

    records = {
        "a": ert3.stats.Gaussian(0, 1, size=2),
        "b": ert3.stats.Uniform(0, 1, size=2),
    }
    evaluations = ert3.algorithms.one_at_a_time(records)

    assert len(expected_evaluations) == len(evaluations)
    for expected, result in zip(expected_evaluations, evaluations):
        assert expected.keys() == result.keys()

        for key in expected.keys():
            assert len(expected[key]) == len(result[key].data)
            for e, r in zip(expected[key], result[key].data):
                assert e == pytest.approx(r)


def test_multi_parameters():
    records = {}

    size = 10
    records["array"] = ert3.stats.Gaussian(0, 1, size=size)

    index = ["a" * i + str(i) for i in range(5)]
    records["indexed"] = ert3.stats.Gaussian(0, 1, index=index)

    evaluations = ert3.algorithms.one_at_a_time(records)

    assert 2 * (size + len(index)) == len(evaluations)
    for eidx, evali in enumerate(evaluations):
        parameter_value = -CNORM_INV if eidx % 2 == 0 else CNORM_INV
        assert ["array", "indexed"] == sorted(evali.keys())

        assert size == len(evali["array"].data)
        for vidx, val in enumerate(evali["array"].data):
            expected_value = (
                parameter_value if eidx < 2 * size and vidx == eidx // 2 else 0
            )
            assert expected_value == pytest.approx(val)

        assert sorted(index) == sorted(evali["indexed"].index)
        for kidx, key in enumerate(index):
            expected_value = (
                parameter_value
                if eidx >= 2 * size and kidx == (eidx - 2 * size) // 2
                else 0
            )
            assert expected_value == pytest.approx(evali["indexed"].data[key])


@pytest.mark.parametrize(
    ("tail", "expected_evaluations"),
    (
        (
            0.99,
            [
                {"a": [-CNORM_INV], "b": [0.5]},
                {"a": [CNORM_INV], "b": [0.5]},
                {"a": [0], "b": [CUNI_INV]},
                {"a": [0], "b": [1 - CUNI_INV]},
            ],
        ),
        (
            0.75,
            [
                {"a": [-1.15034938], "b": [0.5]},
                {"a": [1.15034938], "b": [0.5]},
                {"a": [0], "b": [0.125]},
                {"a": [0], "b": [1 - 0.125]},
            ],
        ),
        (
            0.5,
            [
                {"a": [-0.67448975], "b": [0.5]},
                {"a": [0.67448975], "b": [0.5]},
                {"a": [0], "b": [0.25]},
                {"a": [0], "b": [1 - 0.25]},
            ],
        ),
    ),
)
def test_tail(tail, expected_evaluations):
    records = {
        "a": ert3.stats.Gaussian(0, 1, size=1),
        "b": ert3.stats.Uniform(0, 1, size=1),
    }
    evaluations = ert3.algorithms.one_at_a_time(records, tail)

    for expected, result in zip(expected_evaluations, evaluations):
        for key in expected.keys():
            for e, r in zip(expected[key], result[key].data):
                assert e == pytest.approx(r)
