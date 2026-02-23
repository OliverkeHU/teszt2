# Headless ARH CLI (Windows) – build jegyzet

A webes FastAPI backend **nem** tud a GUI-s `DesktopAnpr.exe`-vel megbízhatóan dolgozni, ezért készítünk egy **konzol (headless) CLI-t**, ami:

- betölt egy képet
- lefuttatja az ARH motort
- JSON-t ír stdout-ra

## 1) Előfeltételek

- Qt 5.x (a csomagban Qt 5.3-hoz van prebuilt runtime, de fordítani bármely 5.x-szel lehet)
- MinGW (ajánlott) vagy MSVC
- Qt Creator (opcionális, de kényelmes)

## 2) Fordítás (Qt Creator)

1. Nyisd meg: `DesktopANPR_source_29032018/src_cli/ArhCli.pro`
2. Build (Release)
3. A kapott `arh_anpr_cli.exe`-t másold ide:
   - `backend/bin/arh_anpr_cli.exe`

## 3) Futtatás teszt (kézzel)

A prebuilt `lpr.lprdat` itt van:
- `DesktopANPR_source_29032018/DesktopANPR_prebuilt_mingw32_530_x86/lpr.lprdat`

Példa:
```bat
backend\bin\arh_anpr_cli.exe --dat DesktopANPR_source_29032018\DesktopANPR_prebuilt_mingw32_530_x86\lpr.lprdat --image sample.jpg
```

JSON kimenet:
```json
{"ok":true,"plates":[{"text":"ABC123","score":1.0,"quad":[[x,y],[x,y],[x,y],[x,y]]}]}
```
