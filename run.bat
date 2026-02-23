@echo off
setlocal
cd /d %~dp0
python -m venv .venv
call .venv\Scripts\activate
pip install -r requirements.txt
uvicorn backend.app.main:app --host 127.0.0.1 --port 8000
