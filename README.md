# Market Simulator

This project simulates a market using C++, Redis Streams, and a React dashboard for real-time visualization. The aim is to reproduce patterns we've seen in real world stock market. From there we can make all sort of interesting hypothesis and measure data to test it. And most importantly, have fun!

## Components
- C++20 market simulator (matching engine, traders)
- Redis Stream (`trades`) for persistent messaging
- WebSocket server (C++) for real-time push to frontend
- React + Plotly dashboard (frontend)
- Python script for PnL analysis using flushed trade data

## Run the Project

### With Docker Compose

```bash
docker-compose up --build
```

Then open the frontend at `http://localhost:5173`

### Post-Simulation Analysis

```bash
python3 analyze_pnl_from_file.py
```

## Project Layout

```
market_simulator/
├── src/                  # C++ source files
├── include/              # C++ headers
├── data/                 # Simulation output
├── frontend/             # React dashboard
├── analyze_pnl_from_file.py
├── Dockerfile
├── docker-compose.yml
└── README.md
```
