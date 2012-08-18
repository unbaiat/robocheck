@echo off
@if [%1] == [] goto usage

	@if "%1" == "build" goto build
	@if "%1" == "clean" goto clean
	@goto usage

:build
	@echo "building..."
	
	@for /f %%i in ('dir /d /b modules') do (
		@cd modules\%%i
		if exist "NMakefile" (
			@echo Building: %%i
			@nmake /F NMakefile clean
			@nmake /F NMakefile
			@if %errorlevel% neq 0 (
				cd ../..
				@exit /B 1
			)
			cp /*.dll ../.. 2>NUL
			cp /*.lib ../.. 2>NUL
			cp /*.exp ../.. 2>NUL
			cp /*.jar ../.. 2>NUL
		)
		@cd ../..
	)
	
	@exit /B 0

:clean
	@echo "cleaning..."
	
	@for /f %%i in ('dir /d /b modules') do (
		@cd modules\%%i
		if exist "NMakefile" (
			@echo Cleaning: %%i
			@nmake /F NMakefile clean
		)
		@cd ../..
	)
	
	@exit /B 0
	
:usage
	@echo Insufficient number of parameters.
	@echo Try %0 [build] or [clean]
	@goto :eof
