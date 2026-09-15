"""E01: the scalar latency clamp must exactly preserve the NumPy reference path."""

from __future__ import annotations

import unittest

import numpy as np

from abides_fork.config import ScenarioLatencyModel


def _reference_latency(
    random_state: np.random.RandomState,
    latency_config: dict[str, object],
) -> int:
    params = latency_config.get("params", {})
    assert isinstance(params, dict)
    model = str(latency_config.get("model", "deterministic"))
    mean_ns = float(params.get("mean_ns", 0.0))
    sigma = float(params.get("sigma", 0.0))
    min_ns = float(params.get("min_ns", 0.0))
    max_ns = float(params.get("max_ns", 1e12))
    alpha = float(params.get("alpha", 1.5))

    if model == "log_normal":
        mu = float(np.log(mean_ns)) if mean_ns > 0 else 0.0
        value = random_state.lognormal(mean=mu, sigma=sigma)
    elif model == "uniform":
        value = random_state.uniform(min_ns, max_ns)
    elif model == "pareto":
        base = min_ns if min_ns > 0 else 1.0
        value = base * (1.0 + random_state.pareto(alpha))
    else:
        value = mean_ns
    return int(round(float(np.clip(value, min_ns, max_ns))))


class ScalarLatencyClipTest(unittest.TestCase):
    def assert_model_matches_reference(
        self, latency_config: dict[str, object], draws: int = 512
    ) -> None:
        reference_state = np.random.RandomState(20260916)
        candidate_state = np.random.RandomState(20260916)
        candidate = ScenarioLatencyModel(3, latency_config, candidate_state)

        for _ in range(draws):
            self.assertEqual(
                candidate.get_latency(0, 1),
                _reference_latency(reference_state, latency_config),
            )

        reference_rng = reference_state.get_state()
        candidate_rng = candidate_state.get_state()
        self.assertEqual(reference_rng[0], candidate_rng[0])
        np.testing.assert_array_equal(reference_rng[1], candidate_rng[1])
        self.assertEqual(reference_rng[2:], candidate_rng[2:])

    def test_deterministic_boundaries_and_clamping(self) -> None:
        for mean_ns, min_ns, max_ns in (
            (500.0, 100.0, 2_000.0),
            (100.0, 100.0, 2_000.0),
            (2_000.0, 100.0, 2_000.0),
            (50.0, 100.0, 2_000.0),
            (3_000.0, 100.0, 2_000.0),
            (500.0, 2_000.0, 100.0),
        ):
            with self.subTest(mean_ns=mean_ns, min_ns=min_ns, max_ns=max_ns):
                self.assert_model_matches_reference(
                    {
                        "model": "deterministic",
                        "params": {
                            "mean_ns": mean_ns,
                            "min_ns": min_ns,
                            "max_ns": max_ns,
                        },
                    },
                    draws=1,
                )

    def test_stochastic_models_preserve_values_and_rng_state(self) -> None:
        for latency_config in (
            {
                "model": "log_normal",
                "params": {
                    "mean_ns": 500.0,
                    "sigma": 1.4,
                    "min_ns": 100.0,
                    "max_ns": 2_000.0,
                },
            },
            {
                "model": "uniform",
                "params": {"min_ns": 100.0, "max_ns": 2_000.0},
            },
            {
                "model": "pareto",
                "params": {
                    "alpha": 1.2,
                    "min_ns": 100.0,
                    "max_ns": 2_000.0,
                },
            },
        ):
            with self.subTest(model=latency_config["model"]):
                self.assert_model_matches_reference(latency_config)

    def test_self_message_returns_zero_without_consuming_rng(self) -> None:
        random_state = np.random.RandomState(20260916)
        untouched_state = np.random.RandomState(20260916)
        model = ScenarioLatencyModel(
            3,
            {
                "model": "log_normal",
                "params": {
                    "mean_ns": 500.0,
                    "sigma": 0.3,
                    "min_ns": 100.0,
                    "max_ns": 2_000.0,
                },
            },
            random_state,
        )

        self.assertEqual(model.get_latency(1, 1), 0)
        actual = random_state.get_state()
        expected = untouched_state.get_state()
        np.testing.assert_array_equal(actual[1], expected[1])
        self.assertEqual(actual[2:], expected[2:])


if __name__ == "__main__":
    unittest.main()
