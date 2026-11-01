"""Materialize dev-only fixtures using the unchanged official worked examples."""
import importlib.util
from pathlib import Path
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
    answer("b", "targeted", b"2 4 1 3\n2 4 1 3\n2 4 1 3\n1 2 2\n0 0\n1 4 4\n")
    answer("b", "wide-sum", b"100000 4999950000 0 1 2 3 4 5 6 7 8 9\n"*2)
    for label, (docs, queries) in cases.items():
        data = SUB / ".work/fixtures/b" / label
        data.mkdir(parents=True, exist_ok=True)
        (data / "docs.txt").write_bytes(docs)
        (data / "queries.txt").write_bytes(queries)
    return cases


if __name__ == "__main__":
    if sys.argv[1] == "a":
        print(list(make_a()))
    elif sys.argv[1] == "c":
        print(list(make_c()))
    elif sys.argv[1] == "b":
        print(list(make_b()))
