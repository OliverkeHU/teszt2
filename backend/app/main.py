# backend/app/main.py
import base64
import json
import os
import subprocess
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Tuple

from fastapi import FastAPI, File, UploadFile, Form
from fastapi.responses import JSONResponse
from fastapi.staticfiles import StaticFiles
from PIL import Image, ImageDraw

app = FastAPI()

CURRENT_FILE = Path(__file__).resolve()
BACKEND_DIR = CURRENT_FILE.parents[1]
STATIC_DIR = BACKEND_DIR / "static"
OUTPUTS_DIR = STATIC_DIR / "outputs"
OUTPUTS_DIR.mkdir(parents=True, exist_ok=True)

PROJECT_ROOT = CURRENT_FILE.parents[2]
ARH_PACK_DIR = PROJECT_ROOT / "DesktopANPR_source_29032018"
PREBUILT_DIR = ARH_PACK_DIR / "DesktopANPR_prebuilt_mingw32_530_x86"
DEFAULT_DAT = PREBUILT_DIR / "lpr.lprdat"

# Ide tedd a lefordított headless CLI-t:
#   backend/bin/arh_anpr_cli.exe
CLI_PATH = BACKEND_DIR / "bin" / "arh_anpr_cli.exe"


def _img_to_dataurl_png(img: Image.Image) -> str:
    buf = tempfile.SpooledTemporaryFile(max_size=10_000_000)
    img.save(buf, format="PNG")
    buf.seek(0)
    b64 = base64.b64encode(buf.read()).decode("ascii")
    return "data:image/png;base64," + b64


def _run_arh_cli(image_path: Path, dat_path: Path) -> Dict[str, Any]:
    if not CLI_PATH.exists():
        raise FileNotFoundError(
            f"Hiányzik a headless ARH CLI: {CLI_PATH}. "
            f"Ezt le kell fordítani a DesktopANPR_source_29032018/src_cli alatt lévő forrásból."
        )
    if not dat_path.exists():
        raise FileNotFoundError(f"Hiányzik az adatfájl (lpr.lprdat): {dat_path}")

    # A CLI JSON-t ír stdout-ra
    cmd = [
        str(CLI_PATH),
        "--dat", str(dat_path),
        "--image", str(image_path),
    ]
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError(
            "ARH CLI hiba\n"
            f"CMD: {' '.join(cmd)}\n"
            f"STDOUT: {p.stdout[:2000]}\n"
            f"STDERR: {p.stderr[:2000]}"
        )
    try:
        return json.loads(p.stdout)
    except Exception as e:
        raise RuntimeError(f"Nem JSON a CLI kimenete. Első 2000 karakter:\n{p.stdout[:2000]}") from e


def _bbox_from_quad(quad: List[List[float]]) -> Tuple[int,int,int,int]:
    xs = [p[0] for p in quad]
    ys = [p[1] for p in quad]
    x0 = int(max(0, min(xs)))
    y0 = int(max(0, min(ys)))
    x1 = int(max(xs))
    y1 = int(max(ys))
    return x0, y0, x1, y1


@app.post("/api/recognize")
async def recognize(file: UploadFile = File(...), forensic: str = Form("0")):
    # forensic flag jelenleg nem befolyásolja az ARH motort, de a frontend miatt megtartjuk.
    try:
        suffix = Path(file.filename or "upload").suffix or ".jpg"
        with tempfile.NamedTemporaryFile(delete=False, suffix=suffix) as tmp:
            tmp.write(await file.read())
            tmp_path = Path(tmp.name)

        data = _run_arh_cli(tmp_path, DEFAULT_DAT)

        # Load image for crop + bbox draw
        img = Image.open(tmp_path).convert("RGB")
        draw = ImageDraw.Draw(img)

        plates_out: List[Dict[str, Any]] = []
        crops_out: List[Dict[str, Any]] = []

        plates = data.get("plates") or []
        for i, p in enumerate(plates):
            text = (p.get("text") or "").strip()
            quad = p.get("quad") or []
            if len(quad) == 4:
                # draw polygon
                poly = [(float(x), float(y)) for x, y in quad]
                draw.line(poly + [poly[0]], width=3)
                x0, y0, x1, y1 = _bbox_from_quad(quad)
            else:
                x0=y0=0; x1=img.width; y1=img.height

            # plate object schema expected by frontend
            plates_out.append({
                "plate_index": i,
                "candidates": [{"text": text, "score": float(p.get("score", 1.0))}],
                "quad": quad,
            })

            # crop
            if x1 > x0 and y1 > y0:
                crop = img.crop((x0, y0, x1, y1))
                crops_out.append({
                    "plate_index": i,
                    "image": _img_to_dataurl_png(crop)
                })

        processed = _img_to_dataurl_png(img)

        return JSONResponse({
            "ok": True,
            "plates": plates_out,
            "plate_crops": crops_out,
            "processed_image": processed,
            "engine": "ARH (headless CLI wrapper)"
        })
    except Exception as e:
        return JSONResponse({"ok": False, "error": str(e)})


@app.post("/api/recognize_video")
async def recognize_video(file: UploadFile = File(...), forensic: str = Form("0")):
    # ARH demo motor alapból GUI + videó Qt backend; a headless videó pipeline külön fejlesztés.
    return JSONResponse({
        "ok": False,
        "error": "A /api/recognize_video jelenleg nincs implementálva ARH-hoz. "
                 "Első lépés: /api/recognize (állókép) headless CLI-vel."
    })


# statikus frontend kiszolgálás
app.mount("/", StaticFiles(directory=str(STATIC_DIR), html=True), name="static")
