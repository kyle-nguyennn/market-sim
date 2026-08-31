- conda activate agenthon (using python 3.13)
- The quick-start describes a cycle: reproduce the correct baseline, measure it, then optimize without changing its outputs.

The key clarification is that there are two Python environments:

- Host tools: Python 3.13+, used for `qfbench2-common`, regression checking, and timing.
- ABIDES baseline: Python 3.11 inside Docker because its pinned pandas stack is incompatible with Python 3.13.

That separation explains your earlier Python 3.10 error and avoids trying to install incompatible packages together.

## Step 1 — Prepare ABIDES and the scoring toolkit

Conceptually, this provides:

- ABIDES source code—the simulator you will optimize.
- `qfbench2-common`—the organizer’s validation toolkit.

The README shows both being installed in one environment, but the reliable workflow is:

```text
Host Python 3.13             Docker image
─────────────────            ───────────────
qfbench2-common              Python 3.11
regression runner            pinned ABIDES
throughput timer             pinned pandas/numpy
```

You already have a Python 3.13 Conda environment named `agenthon`:

```bash
cd /home/dnguyen3/code/market-sim/agenthon/track3-simulation-public

source /home/dnguyen3/miniforge3/etc/profile.d/conda.sh
conda activate agenthon

python --version
# Must report Python 3.13.x

python -m pip install \
  "qfbench2-common @ git+https://github.com/Agenthon-2026/Agenthon2026-public.git@v2.3.1#subdirectory=common"
```

Use `python -m pip`, not bare `pip`, so the package definitely installs into the active Python 3.13 environment.

For the ABIDES baseline, let Docker build the pinned version:

```bash
docker build --platform=linux/amd64 -t track3-abides-baseline:latest baselines/
```

This is safer than the quick-start’s unpinned local clone because the Dockerfile uses the correct commit and patches.

If you want an editable local ABIDES checkout later, use a separate Python 3.11 environment. Do not install `qfbench2-common` into that environment.

## Step 2 — Build the reference-trace cache

The repository stores reference Parquet files inside `units/`. The regression runner expects them organized by scenario UUID under `regression_suite/reference_traces/`.

These commands download the real Git LFS content and build that mapping:

```bash
git lfs pull
python regression_suite/build_reference_cache.py
```

This does not generate new answers. It rearranges or links the shipped reference data into the layout the regression runner expects.

If Git LFS was not pulled, the Parquet files may only be tiny pointer files and the builder will reject them.

## Step 3 — Verify the baseline

Now run the baseline Docker image against all 65 public single-scenario cases:

```bash
python -m regression_suite.run_regression \
  --candidate-image track3-abides-baseline:latest \
  --scenarios-dir regression_suite/scenarios/ \
  --reference-dir regression_suite/reference_traces/ \
  --output-dir run_outputs/ \
  --workers 4
```

This establishes that:

- Docker and bind mounts work.
- The reference cache is correct.
- Your host Python environment has the scoring dependencies.
- The baseline reproduces the supplied reference traces.

All 65 should pass. If the unchanged baseline fails, do not optimize yet—the environment or setup is wrong.

The helper script combines the Docker build and regression run:

```bash
./baselines/build_and_validate.sh
```

Run Step 2 before using it because it does not build the reference cache itself.

## Step 4 — Inspect the realism metrics

Step 3 calculates realism statistics for each scenario and writes them under `run_outputs/<scenario UUID>/`.

For example:

```bash
find run_outputs -name stylized_fact_report.json | head
```

Then inspect one:

```bash
cat run_outputs/<scenario-uuid>/stylized_fact_report.json
```

The report includes:

- `ks`: return-distribution difference.
- `acf_abs_l2`: volatility-clustering difference.
- `hill_abs`: tail-exponent difference.
- `depth_js`: order-book-depth difference.

When the baseline is compared with its own reference output, these should normally be `0.0`. Later, these reports tell you whether an optimization changed market behavior.

There is no separate “stylized-facts command”; the regression run already computes them.

## Step 5 — Measure baseline speed

Run the local timer on the public throughput scenario:

```bash
python throughput/timer.py \
  --image track3-abides-baseline:latest \
  --scenario regression_suite/scenarios/as06_throughput_fast.json
```

Record the resulting host-measured `events_per_sec`. That is your local comparison point.

Do not rely heavily on the throughput figures written in the documentation: they were measured under different or unknown hardware conditions. Compare your candidate against the baseline on the same machine using the same command.

## Step 6 — Implement and optimize

Now create your own image, initially preserving the baseline interface:

```text
simulate --config /input/scenario.json --out /output/trace.parquet
simulate-batch --batch-dir /input/scenarios --out-dir /output
```

Likely optimization paths include:

- Replacing Python order-book structures with compact arrays.
- Compiling matching loops with Numba, Cython, Rust, or C++.
- Reducing event-object allocation and Python callback overhead.
- Optimizing trace construction and Parquet serialization.
- Running independent batch sub-scenarios concurrently.

After every meaningful optimization:

1. Build your image.
2. Run all regressions.
3. Check the stylized-fact reports.
4. Measure throughput.
5. Keep the change only if correctness still passes and speed improves.

The governing rule is: an arbitrarily fast simulator receives no useful score if its event semantics diverge.

## Practical command sequence

For your setup, I would use this version of the quick-start:

```bash
cd /home/dnguyen3/code/market-sim/agenthon/track3-simulation-public

source /home/dnguyen3/miniforge3/etc/profile.d/conda.sh
conda activate agenthon
python --version

python -m pip install \
  "qfbench2-common @ git+https://github.com/Agenthon-2026/Agenthon2026-public.git@v2.3.1#subdirectory=common"

git lfs pull
python regression_suite/build_reference_cache.py

docker build --platform=linux/amd64 \
  -t track3-abides-baseline:latest \
  baselines/

python -m regression_suite.run_regression \
  --candidate-image track3-abides-baseline:latest \
  --scenarios-dir regression_suite/scenarios \
  --reference-dir regression_suite/reference_traces \
  --output-dir run_outputs \
  --workers 4

python -m throughput.timer \
  --image track3-abides-baseline:latest \
  --scenario regression_suite/scenarios/as06_throughput_fast.json
```

The quick-start itself is at [README.md](/home/dnguyen3/code/market-sim/agenthon/track3-simulation-public/README.md:388).


## Logs

```bash
(agenthon) dnguyen3@kylenguyen-pc:~/code/market-sim/agenthon/track3-simulation-public$ python -m regression_suite.run_regression \
  --candidate-image track3-abides-baseline:latest \
  --scenarios-dir regression_suite/scenarios/ \
  --reference-dir regression_suite/reference_traces/ \
  --output-dir run_outputs/ \
  --workers 4
2026-08-31 23:23:05,704 INFO __main__: Discovered 65 scenario(s) under regression_suite/scenarios
2026-08-31 23:23:11,017 INFO __main__: [PASS] abd7b2b8-ecba-5a6e-a7c3-dfac68beb0ad
2026-08-31 23:23:11,207 INFO __main__: [PASS] 8b1084a6-4f53-556a-9929-5afff3818858
2026-08-31 23:23:13,698 INFO __main__: [PASS] fe284cc5-c965-5cf6-9ff8-b375758c1ee6
2026-08-31 23:23:14,752 INFO __main__: [PASS] df79ad78-3db2-5770-8c82-64df5edd62b5
2026-08-31 23:23:16,407 INFO __main__: [PASS] e2bb9fee-7150-5240-af64-ee1066e55564
2026-08-31 23:23:21,092 INFO __main__: [PASS] 2bec943d-4f5f-5c8c-8ee3-b57550d936a6
2026-08-31 23:23:21,895 INFO __main__: [PASS] 418157d5-26eb-5d5f-a000-217ec60a43a3
2026-08-31 23:23:23,948 INFO __main__: [PASS] a350b90c-0348-5a64-8369-4d4d8bb3dbcd
2026-08-31 23:23:24,066 INFO __main__: [PASS] 683a0b4e-a386-5a7d-a8b2-e252cc6d9f7f
2026-08-31 23:23:27,890 INFO __main__: [PASS] 8a2f9219-014d-5d15-a7f2-efbad536db72
2026-08-31 23:23:28,944 INFO __main__: [PASS] 16892744-6a80-5300-8552-ca18872438ea
2026-08-31 23:23:31,849 INFO __main__: [PASS] e90a1b02-0001-4a3c-8b21-0f1e2d3c4b5a
2026-08-31 23:23:32,655 INFO __main__: [PASS] 14e79532-bbf4-5055-9b2f-7cb07791a6d7
2026-08-31 23:23:35,366 INFO __main__: [PASS] e32eaeba-515c-5fde-af46-44d5246d0805
2026-08-31 23:23:35,938 INFO __main__: [PASS] f1f0ca35-8c17-5843-8c31-3fca8ab5a46b
2026-08-31 23:23:36,827 INFO __main__: [PASS] 38e61d08-72ee-594b-b999-ebdd4b2ba8ab
2026-08-31 23:23:39,432 INFO __main__: [PASS] 2c9e14c7-6c6c-5288-823f-7ff82d6050fa
2026-08-31 23:23:40,008 INFO __main__: [PASS] d85a4b79-703d-54fb-8680-35d4c0059797
2026-08-31 23:23:40,989 INFO __main__: [PASS] 4ff8463e-c268-5a14-b191-dcb374933011
2026-08-31 23:23:43,567 INFO __main__: [PASS] 50076583-ba35-52ab-b3f0-a82ad328f10d
2026-08-31 23:23:47,259 INFO __main__: [PASS] 73ae330d-1e33-5418-8266-44e8deea6571
2026-08-31 23:23:51,006 INFO __main__: [PASS] e8650933-8e32-57db-8eb7-7b22231dce57
2026-08-31 23:23:52,044 INFO __main__: [PASS] 55f80db0-96b2-50d7-8d6f-448f11a626bc
2026-08-31 23:23:55,667 INFO __main__: [PASS] b73c8c0e-15cd-5ca6-9b91-abd2c18b417f
2026-08-31 23:24:46,243 INFO __main__: [PASS] 1c15fde8-5b8b-5fee-8274-a2ee8f5bedd5
2026-08-31 23:25:07,370 INFO __main__: [PASS] ed69678c-450f-5f69-83a0-b4cf31101f75
2026-08-31 23:25:17,331 INFO __main__: [PASS] a4251e7c-c760-58d9-8707-7c6d4c7d8789
2026-08-31 23:25:19,149 INFO __main__: [PASS] 2c78b9d2-b4c6-5342-88a2-c6d4cc70d2f1
2026-08-31 23:25:27,176 INFO __main__: [PASS] a86b50cb-0785-56dd-bd58-9c3d317e49e5
2026-08-31 23:25:29,205 INFO __main__: [PASS] dfba64e0-cac2-5f16-b408-41de04ff2cae
/home/dnguyen3/miniforge3/envs/agenthon/lib/python3.13/site-packages/qfbench2_common/scoring/stylized_facts.py:89: RuntimeWarning: divide by zero encountered in log
  xi = np.mean(np.log(top[:-1]) - np.log(top[-1]))
/home/dnguyen3/miniforge3/envs/agenthon/lib/python3.13/site-packages/qfbench2_common/scoring/stylized_facts.py:89: RuntimeWarning: invalid value encountered in subtract
  xi = np.mean(np.log(top[:-1]) - np.log(top[-1]))
2026-08-31 23:25:42,006 INFO __main__: [PASS] 644c3665-1e55-5672-9497-55c1368b9e18
/home/dnguyen3/miniforge3/envs/agenthon/lib/python3.13/site-packages/qfbench2_common/scoring/stylized_facts.py:89: RuntimeWarning: divide by zero encountered in log
  xi = np.mean(np.log(top[:-1]) - np.log(top[-1]))
/home/dnguyen3/miniforge3/envs/agenthon/lib/python3.13/site-packages/qfbench2_common/scoring/stylized_facts.py:89: RuntimeWarning: invalid value encountered in subtract
  xi = np.mean(np.log(top[:-1]) - np.log(top[-1]))
2026-08-31 23:25:44,283 INFO __main__: [PASS] 7fc88018-85be-50d4-9159-dbc1cc44f525
2026-08-31 23:25:54,359 INFO __main__: [PASS] 001d9e77-7666-5f5b-9e3e-723daed0cbe8
2026-08-31 23:26:03,916 INFO __main__: [PASS] 647ddddf-b30b-5323-98ce-e1b9da65bb76
2026-08-31 23:26:10,749 INFO __main__: [PASS] 077d6642-7e7c-529b-876d-f37aa5f70190
2026-08-31 23:26:31,032 INFO __main__: [PASS] c5c25713-e1b2-50e8-bb08-f0216b9d4a72
2026-08-31 23:26:38,762 INFO __main__: [PASS] 5f8fe817-da1a-56cd-b7ff-754f135c0029
2026-08-31 23:26:43,162 INFO __main__: [PASS] 60f3db6d-9305-543b-8388-b9a4a887f255
2026-08-31 23:26:50,185 INFO __main__: [PASS] 73203dc3-1a14-5976-81e8-8fcec979ae7f
2026-08-31 23:26:53,364 INFO __main__: [PASS] 998e5c53-30cb-5ad4-b4b2-4c3791edd002
2026-08-31 23:26:58,502 INFO __main__: [PASS] 9b855004-0d35-5ed1-945f-5d5d299b64d8
2026-08-31 23:26:59,261 INFO __main__: [PASS] 66ea1f06-132d-58ac-8a27-cc8d45953215
2026-08-31 23:27:00,818 INFO __main__: [PASS] 2b0ae926-c496-536c-a826-9738944667ed
2026-08-31 23:27:04,946 INFO __main__: [PASS] 4f876cd0-76bb-5369-bd07-a575b4249c12
2026-08-31 23:27:05,569 INFO __main__: [PASS] 31546dd0-026f-5db8-989f-d85a6a2dace9
2026-08-31 23:27:06,970 INFO __main__: [PASS] bcd382f5-0924-5d48-99f5-7d4075c0d674
/home/dnguyen3/miniforge3/envs/agenthon/lib/python3.13/site-packages/qfbench2_common/scoring/stylized_facts.py:89: RuntimeWarning: divide by zero encountered in log
  xi = np.mean(np.log(top[:-1]) - np.log(top[-1]))
/home/dnguyen3/miniforge3/envs/agenthon/lib/python3.13/site-packages/qfbench2_common/scoring/stylized_facts.py:89: RuntimeWarning: invalid value encountered in subtract
  xi = np.mean(np.log(top[:-1]) - np.log(top[-1]))
2026-08-31 23:27:08,677 INFO __main__: [PASS] a1b2c3d4-0001-0000-0000-000000000001
2026-08-31 23:27:11,976 INFO __main__: [PASS] ccb06368-afbd-5c1a-ac9a-2fd72ce9ebe0
2026-08-31 23:27:12,507 INFO __main__: [PASS] c85a5154-b948-5aff-89e7-d5c8ce61ea4d
2026-08-31 23:27:15,205 INFO __main__: [PASS] 8a59be07-bb4b-5eec-b8ab-7cbc27fc6735
2026-08-31 23:27:15,599 INFO __main__: [PASS] a1b2c3d4-0019-0000-0000-000000000019
2026-08-31 23:27:22,405 INFO __main__: [PASS] 7a903ca0-a2cb-5061-85be-2885929f64c1
2026-08-31 23:27:24,985 INFO __main__: [PASS] f596295c-e1e4-534d-ad0a-8cfc098bc609
2026-08-31 23:27:25,308 INFO __main__: [PASS] 8c53af6a-f534-57fa-a048-e4e591f7dfb8
2026-08-31 23:27:32,888 INFO __main__: [PASS] 056e6f9a-1891-5197-8988-e410f7884ba6
2026-08-31 23:27:34,353 INFO __main__: [PASS] a1b2c3d4-0012-0000-0000-000000000012
2026-08-31 23:27:35,183 INFO __main__: [PASS] 0bfd6969-c241-57eb-8d6e-6de1388cb183
2026-08-31 23:27:36,260 INFO __main__: [PASS] 3118b1c8-f45b-56fc-9447-a49cfa38a8b9
2026-08-31 23:27:43,288 INFO __main__: [PASS] acbaff26-109b-5856-af00-0603bde931ea
2026-08-31 23:27:44,933 INFO __main__: [PASS] 20d717b4-9f62-5c55-946d-317b281292e8
2026-08-31 23:27:45,976 INFO __main__: [PASS] 406cfe13-1d13-57a6-a6ba-707a55b2236f
2026-08-31 23:27:46,893 INFO __main__: [PASS] dec8b800-21b1-5a05-84b0-8d1474130202
2026-08-31 23:27:50,322 INFO __main__: [PASS] 493d1d62-88f8-507b-9c8e-84aff7737a49
2026-08-31 23:27:54,212 INFO __main__: [PASS] af38a808-c9db-54cb-8604-75e1bf79759c
2026-08-31 23:27:55,591 INFO __main__: [PASS] 94f530d4-8b56-5395-85a5-af0a9ef53356
2026-08-31 23:27:55,627 INFO __main__: Report written to run_outputs/report.json

Regression Report — candidate: track3-abides-baseline:latest
--------------------------------------------------------------------------------------------------------------
scenario_id                                tier  semantic_pass  stylized_pass  events_per_sec   failure_labels
--------------------------------------------------------------------------------------------------------------
001d9e77-7666-5f5b-9e3e-723daed0cbe8       A     PASS           PASS           13,476           —
056e6f9a-1891-5197-8988-e410f7884ba6       B     PASS           PASS           11,282           —
077d6642-7e7c-529b-876d-f37aa5f70190       A     PASS           PASS           12,638           —
0bfd6969-c241-57eb-8d6e-6de1388cb183       B     PASS           PASS           13,084           —
14e79532-bbf4-5055-9b2f-7cb07791a6d7       A     PASS           PASS           7,145            —
16892744-6a80-5300-8552-ca18872438ea       B     PASS           PASS           8,748            —
1c15fde8-5b8b-5fee-8274-a2ee8f5bedd5       A     PASS           PASS           9,017            —
20d717b4-9f62-5c55-946d-317b281292e8       B     PASS           PASS           11,656           —
2b0ae926-c496-536c-a826-9738944667ed       A     PASS           PASS           11,055           —
2bec943d-4f5f-5c8c-8ee3-b57550d936a6       B     PASS           PASS           10,322           —
2c78b9d2-b4c6-5342-88a2-c6d4cc70d2f1       A     PASS           PASS           9,508            —
2c9e14c7-6c6c-5288-823f-7ff82d6050fa       A     PASS           PASS           9,076            —
3118b1c8-f45b-56fc-9447-a49cfa38a8b9       B     PASS           PASS           12,619           —
31546dd0-026f-5db8-989f-d85a6a2dace9       A     PASS           PASS           10,629           —
38e61d08-72ee-594b-b999-ebdd4b2ba8ab       A     PASS           PASS           8,851            —
406cfe13-1d13-57a6-a6ba-707a55b2236f       B     PASS           PASS           11,618           —
418157d5-26eb-5d5f-a000-217ec60a43a3       B     PASS           PASS           10,819           —
493d1d62-88f8-507b-9c8e-84aff7737a49       B     PASS           PASS           9,034            —
4f876cd0-76bb-5369-bd07-a575b4249c12       A     PASS           PASS           10,037           —
4ff8463e-c268-5a14-b191-dcb374933011       A     PASS           PASS           8,044            —
50076583-ba35-52ab-b3f0-a82ad328f10d       A     PASS           PASS           8,606            —
55f80db0-96b2-50d7-8d6f-448f11a626bc       A     PASS           PASS           11,017           —
5f8fe817-da1a-56cd-b7ff-754f135c0029       A     PASS           PASS           9,821            —
60f3db6d-9305-543b-8388-b9a4a887f255       A     PASS           PASS           12,244           —
644c3665-1e55-5672-9497-55c1368b9e18       A     PASS           PASS           12,962           —
647ddddf-b30b-5323-98ce-e1b9da65bb76       A     PASS           PASS           12,375           —
66ea1f06-132d-58ac-8a27-cc8d45953215       A     PASS           PASS           12,130           —
683a0b4e-a386-5a7d-a8b2-e252cc6d9f7f       B     PASS           PASS           9,709            —
73203dc3-1a14-5976-81e8-8fcec979ae7f       A     PASS           PASS           11,491           —
73ae330d-1e33-5418-8266-44e8deea6571       A     PASS           PASS           10,624           —
7a903ca0-a2cb-5061-85be-2885929f64c1       B     PASS           PASS           11,979           —
7fc88018-85be-50d4-9159-dbc1cc44f525       A     PASS           PASS           13,163           —
8a2f9219-014d-5d15-a7f2-efbad536db72       B     PASS           PASS           8,433            —
8a59be07-bb4b-5eec-b8ab-7cbc27fc6735       A     PASS           PASS           10,408           —
8b1084a6-4f53-556a-9929-5afff3818858       B     PASS           PASS           11,253           —
8c53af6a-f534-57fa-a048-e4e591f7dfb8       B     PASS           PASS           13,149           —
94f530d4-8b56-5395-85a5-af0a9ef53356       A     PASS           PASS           12,332           —
998e5c53-30cb-5ad4-b4b2-4c3791edd002       A     PASS           PASS           11,817           —
9b855004-0d35-5ed1-945f-5d5d299b64d8       A     PASS           PASS           9,535            —
a1b2c3d4-0001-0000-0000-000000000001       A     PASS           PASS           10,631           —
a1b2c3d4-0012-0000-0000-000000000012       A     PASS           PASS           11,915           —
a1b2c3d4-0019-0000-0000-000000000019       A     PASS           PASS           10,119           —
a350b90c-0348-5a64-8369-4d4d8bb3dbcd       B     PASS           PASS           10,411           —
a4251e7c-c760-58d9-8707-7c6d4c7d8789       A     PASS           PASS           12,032           —
a86b50cb-0785-56dd-bd58-9c3d317e49e5       A     PASS           PASS           12,710           —
abd7b2b8-ecba-5a6e-a7c3-dfac68beb0ad       B     PASS           PASS           10,942           —
acbaff26-109b-5856-af00-0603bde931ea       B     PASS           PASS           11,489           —
af38a808-c9db-54cb-8604-75e1bf79759c       B     PASS           PASS           13,516           —
b73c8c0e-15cd-5ca6-9b91-abd2c18b417f       A     PASS           PASS           9,347            —
bcd382f5-0924-5d48-99f5-7d4075c0d674       A     PASS           PASS           11,011           —
c5c25713-e1b2-50e8-bb08-f0216b9d4a72       A     PASS           PASS           12,187           —
c85a5154-b948-5aff-89e7-d5c8ce61ea4d       A     PASS           PASS           8,782            —
ccb06368-afbd-5c1a-ac9a-2fd72ce9ebe0       A     PASS           PASS           8,931            —
d85a4b79-703d-54fb-8680-35d4c0059797       A     PASS           PASS           9,224            —
dec8b800-21b1-5a05-84b0-8d1474130202       B     PASS           PASS           11,910           —
df79ad78-3db2-5770-8c82-64df5edd62b5       B     PASS           PASS           10,623           —
dfba64e0-cac2-5f16-b408-41de04ff2cae       A     PASS           PASS           12,034           —
e2bb9fee-7150-5240-af64-ee1066e55564       B     PASS           PASS           9,902            —
e32eaeba-515c-5fde-af46-44d5246d0805       A     PASS           PASS           10,580           —
e8650933-8e32-57db-8eb7-7b22231dce57       A     PASS           PASS           10,352           —
e90a1b02-0001-4a3c-8b21-0f1e2d3c4b5a       A     PASS           PASS           9,465            —
ed69678c-450f-5f69-83a0-b4cf31101f75       A     PASS           PASS           9,269            —
f1f0ca35-8c17-5843-8c31-3fca8ab5a46b       A     PASS           PASS           8,706            —
f596295c-e1e4-534d-ad0a-8cfc098bc609       B     PASS           PASS           12,732           —
fe284cc5-c965-5cf6-9ff8-b375758c1ee6       B     PASS           PASS           10,400           —
--------------------------------------------------------------------------------------------------------------
Total: 65  Passed: 65  Failed: 0  Errored: 0
```