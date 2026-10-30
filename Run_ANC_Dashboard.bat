@echo off
title AI Active Noise Cancellation Dashboard
echo ========================================================
echo   Launching AI-ANC Control Dashboard & Hardware Visualizer
echo ========================================================
cd /d "%~dp0dashboard"
python server.py
pause
