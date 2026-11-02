"""Materialize dev-only fixtures using the unchanged official worked examples."""
import importlib.util
from pathlib import Path
import random
import sys

SUB = Path(__file__).resolve().parents[1]
OFFICIAL = SUB.parent / "official/speed-demon-2026"


def answer(track, label, expected):
    folder = SUB / ".work/fixture-answers" / track
    folder.mkdir(parents=True, exist_ok=True)
    (folder / label).write_bytes(expected.encode() if isinstance(expected, str) else expected)


def load(track, module):
    spec = importlib.util.spec_from_file_location(module, OFFICIAL / "tests" / f"test_{module}.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def make_a():
    mod = load("a", "a_ingest")
    cases = {"worked": mod.WORKED_INPUT, "edges": mod.edge_input(), "empty": ""}
    lines = []
    for n in (1, 2, 100, 101):
        for i in range(n):
            lines.append(f"2026-11-02T17:00:00.000000Z GET /rank{n} 200 {i}\n")
    for path in ("/x", "/X", "/x/a", "/x-a", "/x.a", "/x_a", "/" + "z"*124):
        for status in (100, 498, 499, 500, 599):
            lines.append(f"2026-10-30T18:00:00.000000Z OPTIONS {path} {status} 4294967295\n")
    cases["targeted"] = "".join(reversed(lines))
    answer("a", "worked", mod.WORKED_OUTPUT)
    answer("a", "edges", mod.EDGE_OUTPUT)
    answer("a", "empty", "")
    # Exact packed-latency escapes across append-chunk boundaries, plus a
    # large all-equal group exercising degenerate percentile partitions.
    rows, expected = [], []
    for n in (255, 256, 257, 1023, 1024, 1025, 4097):
        path = f"/packed/{n}"
        values = ([7] * n if n == 4097 else
                  [(0, 524286, 524287, 4294967295, 7, 7, 7)[i % 7] for i in range(n)])
        errors = sum(i % 4 in (1, 2) for i in range(n))
        for i, value in enumerate(values):
            rows.append(f"2026-10-30T18:00:00.000000Z GET {path} {(499,500,599,200)[i % 4]} {value}\n")
        ordered = sorted(values)
        ranks = [ordered[(p * n + 99) // 100 - 1] for p in (50, 95, 99)]
        expected.append((path, f"2026-10-30T18:00:00Z {path} {n} {' '.join(map(str, ranks))} {errors} {errors * 1000000 // n}\n"))
    cases["packed-chunk-boundaries"] = "".join(reversed(rows))
    answer("a", "packed-chunk-boundaries", "".join(text for _, text in sorted(expected)))
    rows, expected = [], []
    for n in range(1, 33):
        path = f"/small/{n}"
        values = [((i % 7) * 1103515245 + n * 12345) & 65535 for i in range(n)]
        ordered = sorted(values)
        ranks = [ordered[(p * n + 99) // 100 - 1] for p in (50, 95, 99)]
        for value in reversed(values):
            rows.append(f"2026-10-30T18:00:00.000000Z GET {path} 200 {value}\n")
        expected.append((path, f"2026-10-30T18:00:00Z {path} {n} {' '.join(map(str, ranks))} 0 0\n"))
    cases["small-group-ranks"] = "".join(rows)
    answer("a", "small-group-ranks", "".join(text for _, text in sorted(expected)))
    for label, text in cases.items():
        data = SUB / ".work/fixtures/a" / label
        data.mkdir(parents=True, exist_ok=True)
        (data / "requests.log").write_text(text)
    return cases


def make_c():
    mod = load("c", "c_join")
    cases = {"worked": (mod.WORKED_RECORDS, mod.WORKED_REGIONS),
             "rules": (mod.RULES_RECORDS, mod.RULES_REGIONS),
             "rules-final-lf": (mod.RULES_RECORDS + b"\n", mod.RULES_REGIONS),
             "rules-leading-bom": (mod.BOM + mod.RULES_RECORDS, mod.RULES_REGIONS),
             "empty": (b"id,name,note,region_id,amount\n", mod.WORKED_REGIONS)}
    for label in cases:
        answer("c", label, b"" if label == "empty" else
               mod.WORKED_OUTPUT if label == "worked" else mod.RULES_OUTPUT)
    # Values are aggregated directly before CSV encoding. Repeated names and
    # name/country pairs cross all parser chunks; publication and first-bit
    # counting must remain exact under contention and overflow buckets.
    values = ["", "", "Shared", "Shared ", 'Ann "A"', "shared"]
    records = [b"\xef\xbb\xbfid,name,note,region_id,amount\r\n"]
    groups = {}
    def money(value):
        return ("-" if value < 0 else "") + f"{abs(value) // 100}.{abs(value) % 100:02d}"
    for i in range(12000):
        ident = 30000000 + i * 2
        name, note = values[i % len(values)], "line\nnote" if i % 11 == 0 else ""
        region, amount = (999 if i % 7 == 0 else 1 + i % 3), (-1, 0, 1234)[i % 3]
        country = {1: "AAA", 2: "BBB", 3: "CCC"}.get(region, "_UNMATCHED")
        a = groups.setdefault(country, [0, 0, 0, amount, amount, set(), 0, 0])
        a[0] += 1; a[1] += ident; a[2] += amount
        a[3] = min(a[3], amount); a[4] = max(a[4], amount)
        a[5].add(name); a[6] += len(name.encode()); a[7] += len(note.encode())
        field = "" if i % len(values) == 0 else '"' + name.replace('"', '""') + '"'
        text = f'{ident},{field},"{note}",{region},{money(amount)}' + ("\r\n" if i % 2 else "\n")
        records.append((b"\xef\xbb\xbf" if i and i % 500 == 0 else b"") + text.encode())
    label = "concurrent-name-membership"
    cases[label] = (b"".join(records), b"region_id,country\n1,AAA\n2,BBB\n3,CCC\n4,DDD\n")
    answer("c", label, "".join(
        f"{key} {a[0]} {a[1]} {money(a[2])} {money(a[3])} {money(a[4])} {len(a[5])} {a[6]} {a[7]}\n"
        for key, a in sorted(groups.items())))
    # Country indices on both sides of each atomic-word boundary. Every
    # repeated name is shared across workers and across country words.
    codes = [chr(65 + i // 676) + chr(65 + i // 26 % 26) + chr(65 + i % 26) for i in range(240)]
    records, groups = [b"id,name,note,region_id,amount\n"], {}
    regions = ("region_id,country\n" + "".join(f"{i+1},{code}\n" for i, code in enumerate(codes))).encode()
    for i in range(16384):
        ident, region = 1000 + i, (1, 64, 65, 128, 129, 192, 193, 240, 999)[i % 9]
        name, amount = ("", 'Ann "A"', "Shared ")[i % 3], (i % 101) - 50
        country = codes[region - 1] if region <= 240 else "_UNMATCHED"
        a = groups.setdefault(country, [0, 0, 0, amount, amount, set(), 0, 0])
        a[0] += 1; a[1] += ident; a[2] += amount
        a[3] = min(a[3], amount); a[4] = max(a[4], amount)
        a[5].add(name); a[6] += len(name.encode())
        field = "" if not name and i % 2 else '"' + name.replace('"', '""') + '"'
        records.append(f'{ident},{field},"",{region},{money(amount)}\n'.encode())
    label = "country-bit-boundaries"
    cases[label] = (b"".join(records), regions)
    answer("c", label, "".join(
        f"{key} {a[0]} {a[1]} {money(a[2])} {money(a[3])} {money(a[4])} {len(a[5])} {a[6]} {a[7]}\n"
        for key, a in sorted(groups.items())))
    # Put an escaped quote pair and a quoted LF at every vector offset.
    # Derive aggregates from values, independently of structural masks.
    records = [b"\xef\xbb\xbfid,name,note,region_id,amount\r\n"]
    groups = {}
    for i in range(2048):
        ident = i + 1
        name = ("", "Ann \"A\"", "Shared ", "Z")[i % 4]
        note = "a" * (i % 64) + '"x"\nend'
        region, amount = (1, 2, 999)[i % 3], -i - 1
        country = {1: "AAA", 2: "BBB"}.get(region, "_UNMATCHED")
        a = groups.setdefault(country, [0, 0, 0, amount, amount, set(), 0, 0])
        a[0] += 1; a[1] += ident; a[2] += amount
        a[3] = min(a[3], amount); a[4] = max(a[4], amount)
        a[5].add(name); a[6] += len(name.encode()); a[7] += len(note.encode())
        quote = lambda s: '"' + s.replace('"', '""') + '"'
        field = "" if not name and i % 8 == 0 else quote(name)
        row = f'{ident},{field},{quote(note)},{region},{money(amount)}'
        records.append((b"\xef\xbb\xbf" if i % 17 == 0 else b"") + row.encode()
                       + (b"" if i == 2047 else b"\r\n" if i % 2 else b"\n"))
    label = "structural-vector-boundaries"
    cases[label] = (b"".join(records), b"region_id,country\n1,AAA\n2,BBB\n")
    answer("c", label, "".join(
        f"{key} {a[0]} {a[1]} {money(a[2])} {money(a[3])} {money(a[4])} {len(a[5])} {a[6]} {a[7]}\n"
        for key, a in sorted(groups.items())))
    for label, (records, regions) in cases.items():
        data = SUB / ".work/fixtures/c" / label
        data.mkdir(parents=True, exist_ok=True)
        (data / "records.csv").write_bytes(records)
        (data / "regions.csv").write_bytes(regions)
    return cases


def make_b():
    mod = load("b", "b_index")
    cases = {"worked": (mod.EXAMPLE_DOCS, mod.EXAMPLE_QUERIES)}
    answer("b", "worked", mod.EXAMPLE_RESULTS)
    class Capture(mod.EdgeCases):
        def check(self, texts, checks):
            cases[self.label] = (mod.docs_file(texts), "".join(q + "\n" for q, _ in checks).encode())
            answer("b", self.label, "".join(a + "\n" for _, a in checks))
    capture = Capture()
    for method in dir(mod.EdgeCases):
        if method.startswith("test_"):
            capture.label = method[5:]
            getattr(capture, method)()
    # Exercise revival after an empty intermediate, duplicates, maximal length
    # tokens, a real gap between phrase terms, and an ID sum exceeding uint32.
    cases["targeted"] = (
        mod.docs_file(["a", "b", "x x x x x x x x", "a gap b", "z"*32, ""]),
        b'BOOL missing AND a OR b\nBOOL a NOT a OR b\nBOOL a NOT a OR b\n'
        b'PHRASE "x x x x x x x x"\nPHRASE "a b"\nBOOL ' + b'z'*32 + b'\n')
    cases["wide-sum"] = (mod.docs_file(["common"]*100000), b"BOOL common\nBOOL common NOT missing\n")
    cases["kmp-overlap"] = (
        mod.docs_file(["a a a b a a b", "a b a b a b c", "a b x a b a", "a b a b", "a gap b"]),
        b'PHRASE "a a b"\nPHRASE "a b a b c"\nPHRASE "a b a b a b a"\n'
        b'PHRASE "a b a"\n')
    answer("b", "kmp-overlap", b"1 0 0\n1 1 1\n0 0\n4 6 0 1 2 3\n")
    cases["token-block-boundaries"] = (
        mod.docs_file(["." * i + "Z" * 32 + "-aa—BÜrich" for i in range(32)]),
        b'BOOL ' + b'z' * 32 + b'\nPHRASE "' + b'z' * 32 + b' aa b rich"\n')
    answer("b", "token-block-boundaries", b"32 496 0 1 2 3 4 5 6 7 8 9\n" * 2)
    cases["dense-phrase-anchor"] = (
        mod.docs_file(["common common" if i % 3 == 0 else "common gap common" for i in range(130)]),
        b'PHRASE "common common"\nPHRASE "common gap common"\nBOOL common NOT gap\n')
    answer("b", "dense-phrase-anchor",
           b"44 2838 0 3 6 9 12 15 18 21 24 27\n"
           b"86 5547 1 2 4 5 7 8 10 11 13 14\n"
           b"44 2838 0 3 6 9 12 15 18 21 24 27\n")
    words = ["a", "a0", "abcdefg", "abcdefgh", "abcdefghi", "abcdefghj",
             "abcdefgh0", "12345678", "123456789", "z" * 32]
    rows = [[w, w] for w in words] + [["abcdefgh", "abcdefghi"],
            ["abcdefghj", "abcdefgh"], ["a", "a0", "abcdefgh0"], ["z" * 32, "abcdefgh"]]
    sets = {w: {i for i, row in enumerate(rows) if w in row} for w in words}
    checks = [("BOOL " + w, sets[w]) for w in words]
    checks += [("BOOL abcdefgh AND abcdefghi", sets["abcdefgh"] & sets["abcdefghi"]),
               ("BOOL abcdefgh NOT abcdefghi OR abcdefghj", (sets["abcdefgh"] - sets["abcdefghi"]) | sets["abcdefghj"]),
               ('PHRASE "abcdefgh abcdefghi"', {10}),
               ('PHRASE "abcdefgh abcdefgh"', {3}),
               ('PHRASE "abcdefghi abcdefghi"', {4})]
    cases["short-key-prefixes"] = (mod.docs_file([" ".join(row) for row in rows]),
                                   "".join(q + "\n" for q, _ in checks).encode())
    answer("b", "short-key-prefixes", "".join(
        " ".join(map(str, [len(ids), sum(ids), *sorted(ids)[:10]])) + "\n" for _, ids in checks))
    # A partial final bitmap word plus every six-bit id-position boundary.
    # Expected sets are built from fixture values independently of bitmap math.
    populations = {
        "even": set(range(0, 513, 2)), "odd": set(range(1, 513, 2)),
        "edge": {0, 63, 64, 127, 128, 255, 256, 511, 512},
    }
    checks = [("BOOL even", populations["even"]),
              ("BOOL odd", populations["odd"]),
              ("BOOL even OR odd", set(range(513))),
              ("BOOL even OR odd NOT edge", set(range(513)) - populations["edge"]),
              ("BOOL even OR odd AND edge", populations["edge"]),
              ("BOOL even NOT even OR odd", populations["odd"])]
    cases["bitmap-summary"] = (mod.docs_file([
        " ".join(term for term, ids in populations.items() if doc in ids)
        for doc in range(513)]), "".join(q + "\n" for q, _ in checks).encode())
    answer("b", "bitmap-summary", "".join(
        " ".join(map(str, [len(ids), sum(ids), *sorted(ids)[:10]])) + "\n"
        for _, ids in checks))
    # Large monotone gaps, including exactly65535 and a large first id.
    # One-worker runs exercise every escape; multiple workers check range bases.
    positions = {0, 65534, 65535, 65536, 131071, 200000}
    text = ["" for _ in range(200001)]
    for doc in positions: text[doc] = "rare rare marker"
    text[65535] += " first"
    text[200000] += " late"
    cases["posting-gap-boundaries"] = (mod.docs_file(text),
        b'BOOL rare\nBOOL first\nBOOL late\nPHRASE "rare marker"\n')
    line = " ".join(map(str, [len(positions), sum(positions), *sorted(positions)])) + "\n"
    answer("b", "posting-gap-boundaries", line + "1 65535 65535\n1 200000 200000\n" + line)
    # Later rare AND operands, intervening NOT, and OR revival. Sets are
    # derived from fixture values independently of query evaluation/index code.
    rare = {0, 63, 64, 4096}
    wide = set(range(0, 4097, 50)) | rare
    ban, other = {63, 2500}, {63, 64, 1000}
    checks = [("BOOL wide AND rare", rare),
              ("BOOL wide NOT ban AND rare", rare - ban),
              ("BOOL wide AND rare NOT rare", set()),
              ("BOOL wide NOT rare AND rare", set()),
              ("BOOL wide AND rare NOT ban NOT other", rare - ban - other),
              ("BOOL missing AND rare", set()),
              ("BOOL wide AND missing OR rare", rare),
              ("BOOL wide OR other AND missing", set()),
              ("BOOL wide OR other AND missing OR rare", rare),
              ("BOOL wide AND missing OR other NOT other OR rare", rare),
              ("BOOL wide AND missing NOT ban OR other AND rare", other & rare),
              ("BOOL wide AND missing OR missing NOT ban OR other", other),
              ("BOOL missing OR missing", set()),
              ("BOOL missing OR rare AND other", rare & other),
              ("BOOL wide NOT ban OR other AND rare", ((wide - ban) | other) & rare)]
    values = {"wide": wide, "rare": rare, "ban": ban, "other": other}
    cases["rare-and-anchor"] = (mod.docs_file([
        " ".join(["universe", *(term for term, ids in values.items() if doc in ids)])
        for doc in range(4097)]), "".join(q + "\n" for q, _ in checks).encode())
    answer("b", "rare-and-anchor", "".join(
        " ".join(map(str, [len(ids), sum(ids), *sorted(ids)[:10]])) + "\n" for _, ids in checks))
    answer("b", "targeted", b"2 4 1 3\n2 4 1 3\n2 4 1 3\n1 2 2\n0 0\n1 4 4\n")
    answer("b", "wide-sum", b"100000 4999950000 0 1 2 3 4 5 6 7 8 9\n"*2)
    for label, (docs, queries) in cases.items():
        data = SUB / ".work/fixtures/b" / label
        data.mkdir(parents=True, exist_ok=True)
        (data / "docs.txt").write_bytes(docs)
        (data / "queries.txt").write_bytes(queries)
    return cases


def make_d():
    mod = load("d", "d_traverse")
    cases = {}
    class Capture(mod.HandWrittenCases):
        def check(self, graph, queries, expected):
            cases[self.label] = (graph, queries)
            answer("d", self.label, expected)
    capture = Capture()
    for method in dir(mod.HandWrittenCases):
        if method.startswith("test_"):
            capture.label = method[5:]
            getattr(capture, method)()
    cases["empty-edges"] = ("3 0\n", "0 0\n0 1\n2 2\n2 0\n")
    answer("d", "empty-edges", "0\n-1\n0\n-1\n")
    cases["first-meeting-trap"] = (
        "4 4\n0 1 1\n1 3 100\n0 2 50\n2 3 2\n", "0 3\n3 0\n0 3\n")
    answer("d", "first-meeting-trap", "52\n-1\n52\n")
    # Independent all-pairs oracle, not another Dijkstra implementation. Join
    # random components into one fixture so sanitizer compilation happens once.
    rng = random.Random(0xD2026)
    edges, checks, starts = [], [], []
    nodes = 0
    for _ in range(40):
        n = rng.randrange(1, 25)
        starts.append(nodes)
        dist = [[None] * n for _ in range(n)]
        for u in range(n):
            dist[u][u] = 0
        for _ in range(rng.randrange(n * 5 + 1)):
            u, v = rng.randrange(n), rng.randrange(n)
            w = rng.choice((0, 1, 2, 4294967295, rng.randrange(1000)))
            edges.append((nodes + u, nodes + v, w))
            if dist[u][v] is None or w < dist[u][v]:
                dist[u][v] = w
            if rng.randrange(5) == 0:
                edges.append(edges[-1])
        for k in range(n):
            for u in range(n):
                for v in range(n):
                    if dist[u][k] is None or dist[k][v] is None:
                        continue
                    candidate = dist[u][k] + dist[k][v]
                    if dist[u][v] is None or candidate < dist[u][v]:
                        dist[u][v] = candidate
        checks.extend((nodes + u, nodes + v, -1 if dist[u][v] is None else dist[u][v])
                      for u in range(n) for v in range(n))
        nodes += n
    checks.extend((a, b, -1) for a, b in zip(starts, starts[1:]))
    checks += checks[:100]
    rng.shuffle(edges)
    rng.shuffle(checks)
    cases["random-all-pairs"] = (
        f"{nodes} {len(edges)}\n" + "".join(f"{u} {v} {w}\n" for u, v, w in edges),
        "".join(f"{s} {t}\n" for s, t, d in checks))
    answer("d", "random-all-pairs", "".join(f"{d}\n" for s, t, d in checks))
    for label, (graph, queries) in cases.items():
        data = SUB / ".work/fixtures/d" / label
        data.mkdir(parents=True, exist_ok=True)
        (data / "graph.txt").write_text(graph)
        (data / "queries.txt").write_text(queries)
    return cases


if __name__ == "__main__":
    if sys.argv[1] == "a":
        print(list(make_a()))
    elif sys.argv[1] == "c":
        print(list(make_c()))
    elif sys.argv[1] == "b":
        print(list(make_b()))
    elif sys.argv[1] == "d":
        print(list(make_d()))
