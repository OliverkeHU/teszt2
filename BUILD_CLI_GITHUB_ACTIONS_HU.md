# ARH CLI (arh_anpr_cli.exe) build GitHub Actions-szel – Windows (Qt telepítés nélkül)

Ez a projekt tartalmaz egy GitHub Actions workflow-t, ami **Windows-on lefordítja** a `DesktopANPR_source_29032018/src_cli/ArhCli.pro` projektet,
majd elkészít egy hordozható csomagot (exe + szükséges Qt DLL-ek) artifactként.

## 1) Feltöltés GitHub-ra
1. Hozz létre egy új repository-t GitHub-on (pl. `ZsiguliANPR`).
2. Töltsd fel ebbe a repo-ba a projekt teljes tartalmát (ez a mappa).
   - (GitHub Desktop / git push / web UI – mindegy.)

## 2) Workflow futtatása
1. Nyisd meg a repo oldalát GitHub-on.
2. Menj ide: **Actions**
3. A bal oldali listában válaszd: **Build ARH CLI (Windows)**
4. Kattints: **Run workflow** (workflow_dispatch)

## 3) Kész csomag letöltése
1. Nyisd meg a lefutott workflow run-t.
2. A lap alján: **Artifacts**
3. Töltsd le: `arh_anpr_cli_windows_mingw`

A letöltött ZIP-ben lesz egy `arh_anpr_cli.exe` és a hozzá tartozó Qt DLL-ek.

## 4) Behelyezés a web backendbe
Másold a tartalmat ide:

`backend/bin/`

Tehát ez legyen meg:

`backend/bin/arh_anpr_cli.exe`

## 5) Indítás
- Windows: `run.bat`
- Böngésző: `http://127.0.0.1:8000`

## Megjegyzés
- A workflow Qt **Open Source** telepítést használ (jurplel/install-qt-action).
- A build MinGW 64-bit (win64_mingw81).
- Ha neked 32-bit kellene (x86), szólj – akkor külön beállítjuk (más Qt csomag/arch kell).
