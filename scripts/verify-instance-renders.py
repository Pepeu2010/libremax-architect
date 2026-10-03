"""Compare shared Cycles meshes with the archived 0.12 reader through the native app."""
import argparse
import json
import pathlib
import subprocess

from PIL import Image, ImageChops, ImageStat


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", required=True)
    parser.add_argument("--blender", required=True)
    parser.add_argument("--evidence", required=True)
    parser.add_argument("--model", choices=("light", "authored"), default="light")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent
    app = str(pathlib.Path(args.app).resolve())
    blender = str(pathlib.Path(args.blender).resolve())
    evidence = pathlib.Path(args.evidence).resolve()
    evidence.mkdir(parents=True, exist_ok=True)

    def run(name, arguments):
        with (evidence / (name + ".log")).open("w", encoding="utf-8") as log:
            result = subprocess.run([app, *arguments], stdout=log, stderr=subprocess.STDOUT,
                                    encoding="utf-8", timeout=600, cwd=root)
        if result.returncode:
            raise RuntimeError(f"{name} failed; see {evidence / (name + '.log')}")

    run("fixture", ["--instances-fixture", str(evidence), "--instances-model", args.model])
    expected = json.loads((evidence / "fixture.json").read_text(encoding="utf-8"))
    for mode in ("compact", "legacy"):
        arguments = ["--render-smoke", str(evidence / mode), "--blender", blender,
                     "--render-project", str(evidence / "repeated-models.lmx"),
                     "--render-size", "480x270", "--render-samples", "32"]
        if mode == "legacy":
            arguments.extend(["--render-script", str(root / "tests/fixtures/cycles_schema1.py")])
        run(mode, arguments)
    log = (evidence / "compact.log").read_text(encoding="utf-8")
    marker = "LIBREMAX_INSTANCES "
    stats = next(json.loads(line.split(marker, 1)[1]) for line in log.splitlines() if marker in line)
    if any(stats[key] != expected[key] for key in ("objects", "definitions", "linkedObjects")) or stats["linkedObjects"] < 31:
        raise RuntimeError(f"Repeated geometry was not shared: {stats}")
    for mode in ("compact", "legacy"):
        text = (evidence / (mode + ".log")).read_text(encoding="utf-8")
        if "RENDER_SMOKE_PASS" not in text or "RENDER_FAILURE_PRESERVES_PREVIOUS_PASS" not in text:
            raise RuntimeError(f"{mode} did not finish the real image and negative preservation test")
    images = [Image.open(evidence / mode / "cycles-kitchen.png").convert("RGB")
              for mode in ("compact", "legacy")]
    if any(image.size != (480, 270) for image in images):
        raise RuntimeError("Unexpected render size")
    if min(sum(ImageStat.Stat(image).stddev) for image in images) < 15:
        raise RuntimeError("Render lacks visible scene variation")
    delta = ImageChops.difference(*images)
    mean_error = sum(ImageStat.Stat(delta).mean) / 3
    channels = delta.tobytes()
    outliers = sum(max(channels[i:i + 3]) > 24 for i in range(0, len(channels), 3)) / (480 * 270)
    if mean_error > 2 or outliers > 0.01:
        raise RuntimeError(f"Shared mesh appearance changed: mean={mean_error}, outliers={outliers}")
    report = {**expected, **stats, "model": args.model, "width": 480, "height": 270, "samples": 32, "device": "CPU",
              "meanChannelDifference": mean_error, "pixelsAbove24Fraction": outliers,
              "legacyReaderCommit": "f9cffec8020cd882c2d12c6987f622829ac8c9ef"}
    (evidence / "comparison.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("INSTANCE_RENDER_PARITY_PASS: " + json.dumps(report))


if __name__ == "__main__":
    main()
