# Verification — 2026-11-02 session

Independent full-scale seed 7 inputs, generated with the unchanged official generators. Expected outputs come from the unchanged official references. Candidates were built locally (GCC 13, `-O3 -mavx2 [-mbmi2]`) from the sources at the commit that adds this file and run at 4 and 12 workers; both matched byte for byte.

| Track | Input sha256 | Reference output sha256 |
|---|---|---|
| a | requests.log 983f105465072f30<br> | summary.txt: b384b9ddf5fa776ac8b9bbd10f935b5847f3b2cb52bcd2f7111fa8d00903d8c1 |
| b | docs.txt ba9344eb48a58fb3<br>queries.txt 0cf6425e9b4db712<br> | results.txt: a5a568281ee327b54011c43ac63e971b7a344948f58be60ee57ff07ef50f9c0a |
| c | records.csv 68b7c2d9adf38cc0<br>regions.csv aa5ceb246fb1b577<br> | aggregates.txt: da8b48aef3c54504a45cd4a8952916f4968ff5319a35fe2664e57b30655043db |
| d | graph.txt 925139d08965c0b9<br>queries.txt b9266e6df0c5a054<br> | distances.txt: 66d63e1c9c1f80583914b835d82fd24e1bd78fba3bdc1cf73efb7f60ed4fe646 |

Also run for every pushed change: the official fixture suites (worked examples, edge cases, targeted cases) under AddressSanitizer + UndefinedBehaviorSanitizer at several worker counts, sample seeds 2 and 3, and the full seed 1 published hash.
