@echo off
setlocal DisableDelayedExpansion
set "r=%__CD__%"
for /r . %%f in (*.c) do (
	set "p=%%f"
	setlocal EnableDelayedExpansion
	echo !p:%r%=!
	endlocal
)
