import json

with open("./gh-pages/results.json", 'r') as f:
    results = json.load(f)   

for name, tests in results.items():
    print(f"# {name}")
    print("<details>")
    print("<summary>Table</summary>")
    print("")
    print("Test Name | Current, ns | Prev, ns | Ratio")
    print("--- | --- | --- | ---")
    for test_name, data in tests.items():
        new_value = f"{float(data[-1]['val']):.2f}ns"
        old_value = f"{float(data[-2]['val']):.2f}ns" if len(data)> 1 else '.'
        ratio     = f"{float(data[-1]['val'])/float(data[-2]['val']):.2f}" if len(data)> 1 else '.'
        print(f"{test_name} | { new_value } | { old_value } | {ratio}")
    print("")
    print("</details>")
    print("")
