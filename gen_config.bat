@echo off
set newline= ^& echo

echo ^<?xml version="1.0" encoding="utf-8"?^> > rbc_config.xml
echo ^<appSettings^> >> rbc_config.xml
echo   ^<init output="NULL"^> >> rbc_config.xml
echo     ^<tools count="0"^> >> rbc_config.xml
echo     ^</tools^> >> rbc_config.xml
echo     ^<input/^> >> rbc_config.xml
echo     ^<penalty load="false" lib_path="libpenalty.dll"/^> >> rbc_config.xml
echo     ^<err_count value="19"/^> >> rbc_config.xml
echo   ^</init^> >> rbc_config.xml
echo   ^<installed_tools count="0"^> >> rbc_config.xml
echo   ^</installed_tools^> >> rbc_config.xml
echo   ^<errors^> >> rbc_config.xml
echo     ^<err_1 id="1" name="Memory leak"/^> >> rbc_config.xml
echo     ^<err_2 id="2" name="Invalid access exception"/^> >> rbc_config.xml
echo     ^<err_3 id="3" name="Uninitialized"/^> >> rbc_config.xml
echo     ^<err_4 id="4" name="File descriptors error"/^> >> rbc_config.xml
echo     ^<err_5 id="5" name="Data race"/^> >> rbc_config.xml
echo     ^<err_6 id="6" name="Dead lock"/^> >> rbc_config.xml
echo     ^<err_7 id="7" name="Unlock"/^> >> rbc_config.xml
echo     ^<err_8 id="8" name="Destroy"/^> >> rbc_config.xml
echo     ^<err_9 id="9" name="Condition variable"/^> >> rbc_config.xml
echo     ^<err_10 id="10" name="Hold lock"/^> >> rbc_config.xml
echo     ^<err_11 id="11" name="Duplicate code"/^> >> rbc_config.xml
echo     ^<err_12 id="12" name="Static variable"/^> >> rbc_config.xml
echo     ^<err_13 id="13" name="Signed unsigned"/^> >> rbc_config.xml
echo     ^<err_14 id="14" name="Unverified function return call"/^> >> rbc_config.xml
echo     ^<err_15 id="15" name="Function line count exceeds maximum admited"/^> >> rbc_config.xml
echo     ^<err_16 id="16" name="Function indentation exceeds maximum admited"/^> >> rbc_config.xml
echo     ^<err_17 id="17" name="Trailing whitespace"/^> >> rbc_config.xml
echo     ^<err_18 id="18" name="Mixed tabs with spaces"/^> >> rbc_config.xml
echo     ^<err_19 id="19" name="Invalid free"/^> >> rbc_config.xml
echo   ^</errors^> >> rbc_config.xml
echo ^</appSettings^> >> rbc_config.xml

configure.exe --add-error-details 1 "New memory leak modification" 1 0.2 float
configure.exe --add-error-details 2 "Invalid accesses" INF 0.3 float
configure.exe --add-error-details 3 "Use of unitialized variable" INF 0.2 float
configure.exe --add-error-details 4 "Not closing file descriptors at the end of the program" INF 0.1 float
configure.exe --add-error-details 5 "Data races" INF 0.1 float
configure.exe --add-error-details 6 "Dead lock" INF 0.1 float
configure.exe --add-error-details 7 "Unlock error" INF 0.1 float
configure.exe --add-error-details 8 "Destroy" INF 0.1 float
configure.exe --add-error-details 9 "Missusage of condition variables" INF 0.1 float
configure.exe --add-error-details 10 "Hold lock" INF 0.1 float
configure.exe --add-error-details 11 "Duplicate code in sources" INF 0.2 float
configure.exe --add-error-details 12 "Use of non-static variables or functions in a single module" INF 0.1 float
configure.exe --add-error-details 13 "Assignment from signed to unsigned" INF 0.1 float
configure.exe --add-error-details 14 "Verification of return value for black listed function call is omited" INF 0.3 float
configure.exe --add-error-details 15 "Function line count overlaps the maximum admited limit" INF 0.2 float
configure.exe --add-error-details 16 "Indentation level overlaps the maximum admited limit" INF 0.2 float
configure.exe --add-error-details 17 "Trailing whitespace at the end of lines" INF 0.2 float
configure.exe --add-error-details 18 "Mixed tabs with spaces at the begining of each line" INF 0.2 float
configure.exe --add-error-details 19 "Invalid memory free" INF 0.1 float

configure.exe --create-tool drmemory .\modules\drmemory\libdrmemory.dll dynamic
configure.exe --register-tool drmemory
configure.exe --register-error 1 drmemory
configure.exe --register-error 2 drmemory
configure.exe --register-error 3 drmemory
configure.exe --register-error 19 drmemory

configure.exe --set-executable .\tests\simple.exe
configure.exe --add-static-parameter tests\simple.c
configure.exe --set-penalty-info true libpenalty.dll