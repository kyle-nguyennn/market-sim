- Set up candidate implementation in `candidate`:
    - Created candidate/ as a byte-for-byte copy of baselines/.
    - Built track3-candidate-control:latest successfully.
    - AS01 outputs matched exactly: 24,695 trace rows and 30,087 ledger rows.
    - AS06 outputs matched exactly: 74,502 trace rows and 83,337 ledger rows.
    - Schemas, dtypes, row ordering, semantic metadata, and hashes matched.
    - baselines/ remains unchanged.
    
- TO DO: following the plan in `agenthon/track3-simulation-public/notes/T3_Optimization_Experiments.md`
    - Create each branch for each control experiment.