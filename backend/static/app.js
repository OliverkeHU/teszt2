const $ = (id) => document.getElementById(id);

let camStream = null;
let scanInterval = null;
let inFlight = false;

// Forenzikus mód (MANUAL): alapból OFF a sebesség miatt
let forensicEnabled = (localStorage.getItem('forensic') || '0') === '1';

function updateForensicBtn() {
  const b = $("btnForensic");
  if (!b) return;
  if (forensicEnabled) {
    b.textContent = "FORENSIC: ON";
    b.classList.add("btn--primary");
    b.title = "Forenzikus mód: bekapcsolva (lassabb, de több mentőöv)";
  } else {
    b.textContent = "FORENSIC: OFF";
    b.classList.remove("btn--primary");
    b.title = "Forenzikus mód: kikapcsolva (gyorsabb). Bekapcsolható gombbal.";
  }
}

function speak(text) {
  if (!('speechSynthesis' in window)) return;
  window.speechSynthesis.cancel();
  const u = new SpeechSynthesisUtterance(text);
  u.lang = 'hu-HU';
  u.pitch = 0.9;
  u.rate = 1.0;
  window.speechSynthesis.speak(u);
}

function log(msg, type = "info") {
  const box = $("systemLog");
  if (!box) return;
  const div = document.createElement("div");
  const time = new Date().toLocaleTimeString('hu-HU', { hour12: false });
  const color = type === "error" ? "#ff4747" : (type === "success" ? "var(--accent)" : "var(--muted)");
  div.innerHTML = `<span style="opacity:0.5">[${time}]</span> <span style="color:${color}">> ${msg}</span>`;
  box.prepend(div);
}

function setScanning(on) {
  const line = $("scannerLine");
  if (!line) return;
  if (on) line.classList.add("scanner-active");
  else line.classList.remove("scanner-active");
}

/* ---------------- PREVIEW (kép+videó) ---------------- */

function showPreviewDataUrl(dataUrl) {
  const img = $("imgPreview");
  const vid = $("videoPreview");
  const ph = $("previewPlaceholder");
  if (ph) ph.style.display = "none";

  if (vid) {
    vid.pause();
    vid.removeAttribute("src");
    vid.load();
    vid.style.display = "none";
  }
  if (img) {
    img.src = dataUrl;
    img.style.display = "block";
  }
}

function showPreviewVideoFile(file) {
  const img = $("imgPreview");
  const vid = $("videoPreview");
  const ph = $("previewPlaceholder");
  if (ph) ph.style.display = "none";

  if (img) {
    img.src = "";
    img.style.display = "none";
  }
  if (vid) {
    const url = URL.createObjectURL(file);
    vid.src = url;
    vid.style.display = "block";
  }
}

function clearPreview() {
  const img = $("imgPreview");
  const vid = $("videoPreview");
  const ph = $("previewPlaceholder");

  if (img) {
    img.src = "";
    img.style.display = "none";
  }
  if (vid) {
    vid.pause();
    vid.removeAttribute("src");
    vid.load();
    vid.style.display = "none";
  }
  if (ph) ph.style.display = "block";
}

/* ---------------- OUTPUT (annotált) ---------------- */

function clearOutput() {
  const imgO = $("imgOutput");
  const vidO = $("videoOutput");
  if (imgO) {
    imgO.src = "";
    imgO.style.display = "none";
  }
  if (vidO) {
    vidO.pause();
    vidO.removeAttribute("src");
    vidO.load();
    vidO.style.display = "none";
  }
}

function showOutputImage(dataUrl) {
  const imgO = $("imgOutput");
  const vidO = $("videoOutput");
  if (vidO) {
    vidO.pause();
    vidO.removeAttribute("src");
    vidO.load();
    vidO.style.display = "none";
  }
  if (imgO) {
    imgO.src = dataUrl;
    imgO.style.display = "block";
  }
}

function showOutputVideo(url) {
  const imgO = $("imgOutput");
  const vidO = $("videoOutput");
  if (imgO) {
    imgO.src = "";
    imgO.style.display = "none";
  }
  if (vidO) {
    vidO.src = url;
    vidO.style.display = "block";
  }
}

/* ---------------- CROPS ---------------- */

function clearCrops() {
  const strip = $("cropStrip");
  if (strip) strip.innerHTML = "";
}

function renderCrops(data) {
  const strip = $("cropStrip");
  if (!strip) return;

  strip.innerHTML = "";

  const crops = Array.isArray(data.plate_crops) ? data.plate_crops : [];
  if (crops.length === 0) return;

  crops.forEach((c) => {
    const wrap = document.createElement("div");
    wrap.className = "cropItem";

    const img = document.createElement("img");
    img.src = c.image;

    const label = document.createElement("div");
    label.className = "cropLabel";
    label.textContent = `PLATE ${c.plate_index}`;

    wrap.appendChild(img);
    wrap.appendChild(label);
    strip.appendChild(wrap);
  });
}

/* ---------------- RESULTS ---------------- */

function renderResults(data) {
  const resultList = $("resultList");
  const meta = $("resultMeta");
  if (!resultList || !meta) return;

  resultList.innerHTML = "";

  const plates = Array.isArray(data.plates) ? data.plates : [];
  const info = data.car_info || {};

  if (plates.length === 0) {
    meta.textContent = "AZONOSÍTÁS SIKERTELEN";
    resultList.innerHTML = `<div class="empty-state">Nincs találat.</div>`;
    return;
  }

  let best = "N/A";
  if (plates[0].candidates && plates[0].candidates.length > 0) best = plates[0].candidates[0].text;

  meta.textContent = "ANALÍZIS EREDMÉNYE:";
  log(`Sikeres azonosítás: ${best}`, "success");

  const infoBox = document.createElement("div");
  infoBox.style =
    "background: rgba(58, 163, 255, 0.10); border: 1px solid var(--border); border-radius: 14px; padding: 14px; " +
    "font-family: monospace; animation: fadeIn 0.35s ease-in;";
  infoBox.innerHTML = `
    <div style="font-weight:900; letter-spacing:1px; color:var(--accent2); margin-bottom:10px;">
      JÁRMŰ ADATLAP: <span style="color:var(--text)">${best}</span>
    </div>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px;">
      <div style="font-size:11px;">GYÁRTMÁNY:<br><span style="font-size:14px; color:var(--text)">${info.make || "N/A"}</span></div>
      <div style="font-size:11px;">TÍPUS:<br><span style="font-size:14px; color:var(--text)">${info.model || "N/A"}</span></div>
      <div style="font-size:11px;">SZÍN:<br><span style="font-size:14px; color:var(--text)">${info.color || "N/A"}</span></div>
      <div style="font-size:11px;">STÁTUSZ:<br><span style="font-size:14px; color:var(--text)">${info.status || "N/A"}</span></div>
    </div>
  `;
  resultList.appendChild(infoBox);

  plates.forEach((p) => {
    const title = document.createElement("div");
    title.style = "margin-top:12px; color:var(--muted); font-family: monospace;";
    title.innerHTML = `<b>PLATE ${p.plate_index}</b> (det: ${((p.det_confidence || 0) * 100).toFixed(1)}%)`;
    resultList.appendChild(title);

    const cands = Array.isArray(p.candidates) ? p.candidates : [];
    if (cands.length === 0) {
      const none = document.createElement("div");
      none.className = "empty-state";
      none.textContent = "Nincs OCR jelölt.";
      resultList.appendChild(none);
      return;
    }

    cands.forEach((c, i) => {
      const row = document.createElement("div");
      row.style =
        "padding:10px; border:1px solid var(--border); border-radius:14px; " +
        "background: rgba(7,11,18,.15); font-family: monospace; display:flex; justify-content:space-between;";
      row.innerHTML = `<span>[${String(i + 1).padStart(2, "0")}] <b>${c.text}</b></span><span style="color:var(--muted)">${((c.confidence || 0) * 100).toFixed(1)}%</span>`;
      resultList.appendChild(row);
    });
  });

  speak(`Azonosítás kész. Rendszám: ${best.split("").join(" ")}.`);
}

/* ---------------- IMAGE OCR ---------------- */

async function sendToOcr(blobOrFile, fastPreviewDataUrl = null) {
  if (inFlight) return;
  inFlight = true;

  try {
    clearOutput();

    if (fastPreviewDataUrl) showPreviewDataUrl(fastPreviewDataUrl);

    setScanning(true);
    $("resultMeta").textContent = "ELEKTRONIKUS AZONOSÍTÁS...";
    log("Analízis folyamatban...", "info");

    const form = new FormData();
    form.append("file", blobOrFile);
    form.append("forensic", forensicEnabled ? "1" : "0");

    const res = await fetch("/api/recognize", { method: "POST", body: form });
    const data = await res.json();

    setScanning(false);

    if (!data.ok) {
      log("Hálózati hiba az analízis során!", "error");
      log(String(data.error || "Ismeretlen hiba"), "error");
      $("resultMeta").textContent = "HIBA";
      clearCrops();
      clearOutput();
      return;
    }

    // Preview: a processed_image (bbox rajzzal) kerül a bemeneti preview-ra
    if (data.processed_image) {
      showPreviewDataUrl(data.processed_image);
    }

    // Output: ugyanazt is megmutatjuk külön kimenet panelen (ha akarod, itt hagyható)
    if (data.processed_image) {
      showOutputImage(data.processed_image);
    }

    renderCrops(data);
    renderResults(data);
    log("Analízis kész. Találatok szűrése...", "info");
  } catch (e) {
    setScanning(false);
    log("Hiba: " + (e && e.message ? e.message : String(e)), "error");
    $("resultMeta").textContent = "HIBA";
    clearCrops();
    clearOutput();
  } finally {
    inFlight = false;
  }
}

/* ---------------- VIDEO OCR (annotált mp4) ---------------- */

async function sendToVideo(file) {
  if (inFlight) return;
  inFlight = true;

  try {
    clearCrops();
    clearOutput();

    showPreviewVideoFile(file);

    setScanning(true);
    $("resultMeta").textContent = "VIDEÓ FELDOLGOZÁS...";
    log("Videó feltöltés és feldolgozás...", "info");

    const strideEl = $("videoStride");
    const maxEl = $("videoMaxSeconds");
    const stride = Math.max(1, parseInt((strideEl && strideEl.value) ? strideEl.value : "3", 10));
    const maxSeconds = Math.max(0, parseInt((maxEl && maxEl.value) ? maxEl.value : "0", 10));

    const form = new FormData();
    form.append("file", file);
    form.append("forensic", forensicEnabled ? "1" : "0");
    form.append("detect_stride", String(stride));
    form.append("max_seconds", String(maxSeconds));

    const res = await fetch("/api/recognize_video", { method: "POST", body: form });
    const data = await res.json();

    setScanning(false);

    if (!data.ok) {
      log("Hálózati hiba videó feldolgozás közben!", "error");
      log(String(data.error || "Ismeretlen hiba"), "error");
      $("resultMeta").textContent = "HIBA";
      clearOutput();
      return;
    }

    if (data.video_url) {
      showOutputVideo(data.video_url);
      log("Annotált videó elkészült és betöltve.", "success");
    }

    // “best” eredmény kiírás a results panelbe (video endpoint nem ad plates listát)
    const best = data.best || {};
    const fake = {
      plates: best.text ? [{
        plate_index: 1,
        det_confidence: 1.0,
        candidates: [{ text: best.text, confidence: best.conf || 0.0, engine: "video" }]
      }] : [],
      car_info: {
        make: "N/A",
        model: "N/A",
        color: "N/A",
        status: `VIDEO | stride=${(data.meta && data.meta.detect_stride) ? data.meta.detect_stride : "?"} | ${forensicEnabled ? "FORENSIC" : "NORMAL"}`
      },
      plate_crops: []
    };
    renderResults(fake);

    if (best.text) {
      speak(`Videó kész. Legjobb rendszám: ${String(best.text).split("").join(" ")}.`);
    } else {
      speak("Videó kész, de nem volt megbízható találat.");
    }
  } catch (e) {
    setScanning(false);
    log("Hiba: " + (e && e.message ? e.message : String(e)), "error");
    $("resultMeta").textContent = "HIBA";
    clearOutput();
  } finally {
    inFlight = false;
  }
}

/* ---------------- KAMERA (élő stream) ---------------- */

async function startCamera() {
  const video = $("video");
  if (!video) return;

  try {
    camStream = await navigator.mediaDevices.getUserMedia({
      video: {
        facingMode: "environment",
        width: { ideal: 1280 },
        height: { ideal: 720 }
      },
      audio: false
    });

    video.srcObject = camStream;
    log("Élő videócsatorna megnyitva.", "success");
    speak("Kamera aktív.");

    $("btnCamStart").textContent = "KAMERA LEÁLLÍTÁSA";
  } catch (e) {
    log("Kamera hiba: " + (e && e.message ? e.message : String(e)), "error");
    speak("Kamera hiba.");
  }
}

function stopCamera() {
  if (scanInterval) stopScan();
  const video = $("video");

  if (camStream) {
    camStream.getTracks().forEach(t => t.stop());
    camStream = null;
  }
  if (video) video.srcObject = null;

  $("btnCamStart").textContent = "KAMERA AKTIVÁLÁSA";
  log("Kamera leállítva.", "info");
}

function captureVideoFrameToBlob(quality = 0.92) {
  const video = $("video");
  const canvas = $("canvas");
  if (!video || !canvas) return null;

  const w = video.videoWidth || 1280;
  const h = video.videoHeight || 720;

  canvas.width = w;
  canvas.height = h;

  const ctx = canvas.getContext("2d");
  ctx.drawImage(video, 0, 0, w, h);

  const dataUrl = canvas.toDataURL("image/jpeg", quality);
  return { dataUrl, canvas };
}

function dataURLtoBlob(dataurl) {
  const arr = dataurl.split(',');
  const mime = arr[0].match(/:(.*?);/)[1];
  const bstr = atob(arr[1]);
  let n = bstr.length;
  const u8arr = new Uint8Array(n);
  while (n--) u8arr[n] = bstr.charCodeAt(n);
  return new Blob([u8arr], { type: mime });
}

function startScan() {
  if (!camStream) {
    log("Nincs aktív kamera jel!", "error");
    return;
  }
  if (scanInterval) return;

  $("btnCapture").textContent = "SCAN LEÁLLÍTÁSA";
  log("Scan indítva...", "info");
  speak("Scan indítva.");

  scanInterval = setInterval(() => {
    if (inFlight) return;

    const cap = captureVideoFrameToBlob(0.86);
    if (!cap) return;

    const blob = dataURLtoBlob(cap.dataUrl);
    sendToOcr(blob, cap.dataUrl);
  }, 1400);
}

function stopScan() {
  if (!scanInterval) return;
  clearInterval(scanInterval);
  scanInterval = null;

  $("btnCapture").textContent = "RÖGZÍTÉS + SCAN";
  log("Scan leállítva.", "info");
  speak("Scan leállítva.");
}

/* ---------------- THEME ---------------- */

function initTheme() {
  const btn = $("themeToggle");
  if (!btn) return;

  const saved = localStorage.getItem("theme") || "dark";
  document.documentElement.setAttribute("data-theme", saved);
  btn.textContent = saved === "dark" ? "🌙" : "☀️";

  btn.addEventListener("click", () => {
    const cur = document.documentElement.getAttribute("data-theme") || "dark";
    const next = cur === "dark" ? "light" : "dark";
    document.documentElement.setAttribute("data-theme", next);
    localStorage.setItem("theme", next);
    btn.textContent = next === "dark" ? "🌙" : "☀️";
  });
}

/* ---------------- RAIN ---------------- */

function initRain() {
  const canvas = $("digitalRainCanvas");
  if (!canvas) return;
  const ctx = canvas.getContext("2d");

  const resize = () => {
    canvas.width = canvas.parentElement.offsetWidth;
    canvas.height = canvas.parentElement.offsetHeight;
  };
  resize();
  window.addEventListener("resize", resize);

  const letters = "01ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const fontSize = 14;
  const columns = () => Math.max(10, Math.floor(canvas.width / fontSize));
  let drops = Array(columns()).fill(1);

  function draw() {
    ctx.fillStyle = "rgba(0,0,0,0.08)";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.font = `${fontSize}px monospace`;
    ctx.fillStyle = "rgba(58,163,255,0.65)";

    if (drops.length !== columns()) drops = Array(columns()).fill(1);

    for (let i = 0; i < drops.length; i++) {
      const ch = letters[Math.floor(Math.random() * letters.length)];
      ctx.fillText(ch, i * fontSize, drops[i] * fontSize);
      if (drops[i] * fontSize > canvas.height && Math.random() > 0.975) drops[i] = 0;
      drops[i]++;
    }

    requestAnimationFrame(draw);
  }
  draw();
}

/* ---------------- RESET ---------------- */

function resetSystem() {
  if (scanInterval) stopScan();
  if (camStream) stopCamera();

  clearPreview();
  clearCrops();
  clearOutput();

  const resultList = $("resultList");
  const meta = $("resultMeta");
  if (resultList) resultList.innerHTML = `<div class="empty-state">Várakozás bemeneti jelre.</div>`;
  if (meta) meta.textContent = "ANALÍZIS VÁRAKOZIK";

  log("Rendszer reset kész.", "success");
  speak("Memória törölve.");
}

/* ---------------- INIT ---------------- */

window.addEventListener("DOMContentLoaded", () => {
  initRain();
  initTheme();

  log("Műveleti központ V3.8 online.");
  log("Rendszer készenlétben.");

  updateForensicBtn();
  log(`Forenzikus mód: ${forensicEnabled ? "BE" : "KI"}.`, forensicEnabled ? "success" : "info");

  const fileInput = $("fileInput");
  const btnImport = $("btnImport");
  const btnClear = $("btnClear");
  const btnCamStart = $("btnCamStart");
  const btnCapture = $("btnCapture");
  const btnForensic = $("btnForensic");

  if (btnImport && fileInput) {
    btnImport.addEventListener("click", () => fileInput.click());
  }

  if (btnForensic) {
    btnForensic.addEventListener("click", () => {
      forensicEnabled = !forensicEnabled;
      localStorage.setItem('forensic', forensicEnabled ? '1' : '0');
      updateForensicBtn();
      log(`Forenzikus mód: ${forensicEnabled ? "BEKAPCSOLVA" : "KIKAPCSOLVA"}.`, forensicEnabled ? "success" : "info");
      speak(forensicEnabled ? "Forenzikus mód bekapcsolva." : "Forenzikus mód kikapcsolva.");
    });
  }

  if (fileInput) {
    fileInput.addEventListener("change", (e) => {
      const file = e.target.files && e.target.files[0];
      if (!file) return;

      clearCrops();
      clearOutput();

      // VIDEÓ: közvetlenül küldjük a video endpointnak
      if (file.type && file.type.startsWith("video/")) {
        showPreviewVideoFile(file);
        sendToVideo(file);
        return;
      }

      // KÉP: régi logika (FileReader + /api/recognize)
      const reader = new FileReader();
      reader.onload = (ev) => {
        showPreviewDataUrl(ev.target.result);
        sendToOcr(file, ev.target.result);
      };
      reader.readAsDataURL(file);
    });
  }

  if (btnClear) {
    btnClear.addEventListener("click", resetSystem);
  }

  if (btnCamStart) {
    btnCamStart.addEventListener("click", () => {
      if (!camStream) startCamera();
      else stopCamera();
    });
  }

  if (btnCapture) {
    btnCapture.addEventListener("click", () => {
      if (!camStream) {
        log("Előbb kapcsold be a kamerát!", "error");
        return;
      }
      if (!scanInterval) startScan();
      else stopScan();
    });
  }
});