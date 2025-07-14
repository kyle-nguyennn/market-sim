import json
import matplotlib.pyplot as plt
from collections import defaultdict

pnl = defaultdict(lambda: {'cash': 0.0, 'inv': 0})

with open("data/trades_dump.jsonl") as f:
    for line in f:
        data = json.loads(line)
        try:
            buyer = int(data['buyer_id'])
            seller = int(data['seller_id'])
            price = float(data['price'])
            qty = int(data['quantity'])
            notional = price * qty

            pnl[buyer]['cash'] -= notional
            pnl[buyer]['inv'] += qty

            pnl[seller]['cash'] += notional
            pnl[seller]['inv'] -= qty

        except Exception as e:
            print("Invalid entry:", data, e)

last_price = price if 'price' in locals() else 100.0
pnl_values = [state['cash'] + state['inv'] * last_price for state in pnl.values()]

initial_capital = 100000 * len(pnl)
final_capital = sum(pnl_values)

print(f"Initial capital: {initial_capital:.2f}")
print(f"Final capital: {final_capital:.2f}")
print(f"Net change: {final_capital - initial_capital:.2f}")

plt.hist(pnl_values, bins=30)
plt.title("Trader PnL Distribution (from trades_dump.jsonl)")
plt.xlabel("Final Value")
plt.ylabel("Number of Traders")
plt.grid(True)
plt.tight_layout()
plt.show()
