@echo off
cd /D "%~dp0"
Scripts\win-bash\bash.exe scripts/generate_project_files.sh vs2022 d3d12
exit