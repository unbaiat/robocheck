# **********************************************************
# Copyright (c) 2010-2012 Google, Inc.  All rights reserved.
# Copyright (c) 2009-2010 VMware, Inc.  All rights reserved.
# **********************************************************

# Dr. Memory: the memory debugger
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; 
# version 2.1 of the License, and no later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

cmake_minimum_required(VERSION 2.6)

include(CheckCCompilerFlag)

if (UNIX)
  CHECK_C_COMPILER_FLAG("-Wno-unused-but-set-variable" nounused_avail)
endif(UNIX)

# tests are always built w/o optimizations and with symbols,
# regardless of DrMem library settings
set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  # we don't support the Debug and Release subdirs
  foreach (config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper}
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper}
      "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper}
      "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  endforeach ()
endif ()

# For their annotations.
include_directories("../third_party/valgrind")

if (WIN32)
  # if MinGW gcc is available, we do some extra tests
  # (we do not yet support Cygwin)
  find_program(GCC mingw32-g++.exe DOC "path to MinGW g++.exe" PATHS "c:/mingw/bin")
  if (NOT GCC)
    message(STATUS "MinGW gcc not found: some tests will be disabled")
  else (NOT GCC)
    message(STATUS "Found MinGW gcc at ${GCC}")
  endif (NOT GCC)
endif (WIN32)

function(append_compile_flags target newflags)
  get_target_property(cur_flags ${target} COMPILE_FLAGS)
  # cmake should add an APPEND option
  if (NOT cur_flags)
    set(cur_flags "")
  endif (NOT cur_flags)
  set_target_properties(${target} PROPERTIES
    COMPILE_FLAGS "${cur_flags} ${newflags}")
endfunction(append_compile_flags)

function(set_props target)
  if (UNIX)
    set(defs "-DLINUX")
    if (HAVE_FVISIBILITY)
      set(defs "${defs} -DUSE_VISIBILITY_ATTRIBUTES")
    endif (HAVE_FVISIBILITY)
  else (UNIX)
    set(defs "-DWINDOWS")
  endif (UNIX)
  if (VMKERNEL)
    set(defs "${defs} -DVMX86_SERVER")
  endif (VMKERNEL)
  set_target_properties(${target} PROPERTIES COMPILE_FLAGS "${defs}")
  # enough tests need this (malloc, free, registers, cs2bug, float, etc.)
  # that we add to all tests
  if (nounused_avail)
    append_compile_flags(${target} "-Wno-unused-but-set-variable")
  endif (nounused_avail)
endfunction(set_props)

function(tobuild name source)
  add_executable(${name} ${source})
  set_props(${name})
endfunction(tobuild)

function(append_link_flags target newflags)
  get_target_property(cur_ldflags ${target} LINK_FLAGS)
  # cmake should add an APPEND option
  if (NOT cur_ldflags)
    set(cur_ldflags "")
  endif (NOT cur_ldflags)
  set_target_properties(${target} PROPERTIES
    LINK_FLAGS "${cur_ldflags} ${newflags}")
endfunction(append_link_flags)

function(tobuild_lib name source cust_flags cust_link)
  add_library(${name} SHARED ${source})
  set_props(${name})
  get_target_property(cur_flags ${name} COMPILE_FLAGS)
  set_target_properties(${name} PROPERTIES COMPILE_FLAGS "${cur_flags} ${cust_flags}")
  append_link_flags(${name} "${cust_link}")
endfunction(tobuild_lib)

# tools for nudge tests
# to avoid conflict w/ DR's run_in_bg target we use a different target name
tobuild(run_in_bg_tgt run_in_bg.c)
set_target_properties(run_in_bg_tgt PROPERTIES OUTPUT_NAME run_in_bg)
if (WIN32)
  target_link_libraries(run_in_bg_tgt ntdll)
endif (WIN32)

# For more portable testing (PR 544430):
# 1) use relative dirs within parent build dir
if (USE_DRSYMS)
  file(RELATIVE_PATH bin_relative "${CMAKE_CURRENT_BINARY_DIR}"
    "${PROJECT_BINARY_DIR}/${BUILD_BIN_PREFIX}/bin")
else (USE_DRSYMS)
  file(RELATIVE_PATH bin_relative "${CMAKE_CURRENT_BINARY_DIR}"
    "${PROJECT_BINARY_DIR}/${BUILD_BIN_PREFIX}")
endif (USE_DRSYMS)
# 2) parameterize dirs outside of build dir
set(src_param_pattern "{DRMEMORY_CTEST_SRC_DIR}")
set(DR_param_pattern "{DRMEMORY_CTEST_DR_DIR}")

function (get_relative_location target outvar)
  get_target_property(tgtpath ${target} LOCATION${location_suffix})
  # For more portable testing (PR 544430) use relative dirs within parent build dir
  file(RELATIVE_PATH relpath "${CMAKE_CURRENT_BINARY_DIR}" "${tgtpath}")
  if (NOT "${relpath}" MATCHES "^\\.\\.")
    set(relpath "./${relpath}")
  endif()
  set(${outvar} "${relpath}" PARENT_SCOPE)
endfunction (get_relative_location)

if (UNIX)
  set(cmd_shell ${bin_relative}/${toolname}.pl)
else (UNIX)
  if (PERL_TO_EXE OR USE_DRSYMS)
    set(cmd_shell ${bin_relative}/${toolname}.exe)
  else (PERL_TO_EXE OR USE_DRSYMS)
    find_program(PERL perl)
    if (NOT PERL)
      message(FATAL_ERROR "cannot find perl.exe")
    endif (NOT PERL)
    set(cmd_shell ${PERL} ${bin_relative}/${toolname}.pl)
  endif (PERL_TO_EXE OR USE_DRSYMS)

  # FIXME PR 406287: with /MDd, the app somehow gets an exception, and
  # though the app's output looks normal, Dr. Memory only reports
  # a bunch of ininit accesses by NtContinue.  Disabling /MDd for now.
  # Also, /MTd results in the debug heap code throwing an exception
  # when malloc.exe does free(0x1230).
  # We also avoid msvcrt*.dll differences by using static libc (/MT).
  # But for cygwin to find operators we need /MD (until we have online
  # syms)
  if (USE_DRSYMS)
    set(LIBC_FLAG "/MT")
  else (USE_DRSYMS)
    set(LIBC_FLAG "/MD")
  endif (USE_DRSYMS)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
  string(REGEX REPLACE "/MDd?" "${LIBC_FLAG}" CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}
    "${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  string(REGEX REPLACE "/MDd?" "${LIBC_FLAG}" CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}
    "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  # With /MT, cs2bug.cpp gets link errors with libcpmtd.lib so we remove _DEBUG:
  string(REGEX REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}
    "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  string(REGEX REPLACE "/O2" "/Od" CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}
    "${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  # removing RTC initially just for tests/unload but on app_suite it's thwarting
  # drmem's checks while not detecting many simple uninits through its own checks
  string(REGEX REPLACE "/RTC." "" CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}
    "${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  string(REGEX REPLACE "/RTC." "" CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}
    "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  if (NOT DEBUG_BUILD)
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
      foreach (var C;CXX;EXE_LINKER;MODULE_LINKER;SHARED_LINKER)
        set(CMAKE_${var}_FLAGS_RELEASE ${CMAKE_${var}_FLAGS_DEBUG})
        set(CMAKE_${var}_FLAGS_RELWITHDEBINFO ${CMAKE_${var}_FLAGS_DEBUG})
      endforeach ()
    endif ()
  endif (NOT DEBUG_BUILD)
endif (UNIX)
if (DEBUG_BUILD)
  # use debug DR as well to catch asserts there
  set(dbg_args "-debug;-dr_debug")
else (DEBUG_BUILD)
  set(dbg_args "")
endif (DEBUG_BUILD)
if (WIN32)
  # we need the nudge notification msgs
  # we want core dumps on asserts, etc.
  set(default_dr_ops "-dumpcore_mask 0x87fd -stderr_mask 15 -msgbox_mask 0")
else (WIN32)
  # stderr_mask 15 is the default for debug build, and we don't want to go
  # over option buffer limit on esxi
  set(default_dr_ops "-dumpcore_mask 0")
endif (WIN32)
set(cmd_base ${cmd_shell} ${dbg_args} -dr ${DR_param_pattern}/..)
if (NOT TOOL_DR_HEAPSTAT)
  set(cmd_base ${cmd_base} -batch)
endif (NOT TOOL_DR_HEAPSTAT)
# there's no easy way to set a cmake or env var for running
# from build dir to make runtest.cmake have a relative path so we
# copy to build dir
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtest.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/runtest.cmake" COPYONLY)
set(cmd_script -P "./runtest.cmake")

# test_ops and drmem_ops should have words separated by ; so they'll
# be separate arguments on the cmdline, while dr_ops should be
# separated by spaces and will remain a single arg.
# be wary of trailing spaces or extra spaces: could cause problems.
# Note: For args with spaces (or multiple args) passed via drmem_ops a ;
# should be used instead of each space.  Otherwise, the whole of
# drmem_ops is passed in as a single argument, which results in
# options parsing errors in drmemory.pl and drmem client.
# The postprocess arg is a bool that applies only for Dr. Memory:
# whether to run the front-end with -results after the app is run.
# The resbase arg support specifying a different .res and .out name
# to use.
function(newtest_nobuild test exe test_ops drmem_ops dr_ops postprocess resbase)
  if (${exe} MATCHES ".exe$")
    set(exepath ${exe})
  else ()
    get_relative_location(${exe} exepath)
  endif ()

  set(cmd ${cmd_base})
  if (NOT "${dr_ops}" STREQUAL "")
    set(cmd ${cmd} -dr_ops "${default_dr_ops} ${dr_ops}")
  else()
    set(cmd ${cmd} -dr_ops "${default_dr_ops}")
  endif ()
  # script ops last since using as hack for nudge test app
  # we request callstacks w/ file:line on separate line for matching our templates
  # we include absaddr and mod+offs for easier debugging of problems
  if (USE_DRSYMS)
    set(drmem_ops -callstack_style 0x27 -no_results_to_stderr ${drmem_ops})
  else (USE_DRSYMS)
    set(drmem_ops -callstack_style 0x27 ${drmem_ops})
  endif (USE_DRSYMS)
  if (NOT "${drmem_ops}" STREQUAL "")
    set(cmd ${cmd} ${drmem_ops})
  endif ()
  if ("${exepath}" MATCHES "run_in_bg")
    # unfortunately we have to hardcode support here: if we run drmemory.pl
    # as the command, cmake waits for its direct children to exit (including
    # postprocess.pl): instead we run run_in_bg as the primary app, which
    # makes it much easier to run drmemory.pl in the background.
    set(cmd ${exepath} ${test_ops} ${cmd})
    if (WIN32)
      # run_in_bg spawns perl.exe and perl then re-parses the args, so we need
      # extra quotes.  '' works for cygwin perl but not ActiveState: "" works
      # for both (PR 506111)
      string(REGEX REPLACE ";-dr_ops;([^;]+);" ";-dr_ops;\"\\1\";" cmd "${cmd}")
    endif (WIN32)
  else ()
    set(cmd ${cmd} -- ${exepath})
    if (NOT "${test_ops}" STREQUAL "")
      set(cmd ${cmd} ${test_ops})
    endif ()
  endif ()
  # pass intra-arg spaces via @@ and inter-arg via @
  # to get around the pain of trying to quote everything just right:
  # much simpler this way.
  string(REGEX REPLACE " " "@@" cmd "${cmd}")
  string(REGEX REPLACE ";" "@" cmd "${cmd}")
  string(REGEX REPLACE " " "@@" nudge "${cmd_base}")
  string(REGEX REPLACE ";" "@" nudge "${nudge}")
  if (TOOL_DR_HEAPSTAT)
    # XXX: this works manually but doesn't seem to get through ctest: but
    # the test is passing so not taking the time to figure it out
    set(postcmd "${cmd_base};-view_leaks;-callstack_modname_hide '';-callstack_style 0x21;-x;${exepath};-profdir")
  else (TOOL_DR_HEAPSTAT)
    if (postprocess)
      # test -skip_results + -results (PR 575481)
      set(postcmd "${cmd_base};-results")
    else (postprocess)
      set(postcmd "")
    endif (postprocess)
  endif (TOOL_DR_HEAPSTAT)
  string(REGEX REPLACE " " "@@" postcmd "${postcmd}")
  string(REGEX REPLACE ";" "@" postcmd "${postcmd}")
  if ("${resbase}" STREQUAL "")
    set(resbase "${test}") 
    string(REGEX REPLACE "_FLAKY$" "" resbase "${resbase}")
  endif ("${resbase}" STREQUAL "")
  add_test(${test} ${CMAKE_COMMAND}
    -D cmd:STRING=${cmd}
    -D TOOL_DR_HEAPSTAT:BOOL=${TOOL_DR_HEAPSTAT}
    -D outpat:STRING=${src_param_pattern}/${resbase}.out
    -D respat:STRING=${src_param_pattern}/${resbase}.res
    -D nudge:STRING=${nudge}
    -D VMKERNEL:BOOL=${VMKERNEL}
    -D USE_DRSYMS:BOOL=${USE_DRSYMS}
    -D toolbindir:STRING=${DR_param_pattern}/../bin32
    -D DRMEMORY_CTEST_SRC_DIR:STRING=${CMAKE_CURRENT_SOURCE_DIR}
    -D DRMEMORY_CTEST_DR_DIR:STRING=${DynamoRIO_DIR}
    -D CMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}
    # runtest.cmake will add the -profdir arg
    -D postcmd:STRING=${postcmd}
    ${cmd_script})
endfunction(newtest_nobuild)

function(newtest_ex test source test_ops drmem_ops dr_ops postprocess resbase)
  tobuild(${test} ${source})
  newtest_nobuild(${test} ${test} "${test_ops}" "${drmem_ops}" "${dr_ops}"
    ${postprocess} "${resbase}")
endfunction(newtest_ex)

function(newtest test source)
  newtest_ex(${test} ${source} "" "" "" OFF "")
endfunction(newtest)

function(newtest_custbuild test source cust_flags resbase)
  newtest_ex(${test} ${source} "" "" "" OFF "${resbase}")
  get_target_property(cur_flags ${test} COMPILE_FLAGS)
  set_target_properties(${test} PROPERTIES COMPILE_FLAGS "${cur_flags} ${cust_flags}")
endfunction(newtest_custbuild)

function(newtest_gcc test source depender test_ops drmem_ops dr_ops postprocess resbase)
  add_custom_target(target_${test} DEPENDS ${test}.exe)
  set(exepath ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${test}.exe)
  add_custom_command(OUTPUT ${test}.exe
    DEPENDS ${source} ${CMAKE_CURRENT_SOURCE_DIR}/rungcc.cmake
    COMMAND ${CMAKE_COMMAND}
    # to work around i#84 be sure to put a space after -D for 1st arg at least
    ARGS -D exename=${exepath}
       -D gcc=${GCC}
       -D source=${CMAKE_CURRENT_SOURCE_DIR}/${source}
       -D args=
       -P ${CMAKE_CURRENT_SOURCE_DIR}/rungcc.cmake
    VERBATIM # recommended: p260
    )
  newtest_nobuild(${test} ${exepath} "${test_ops}" "${drmem_ops}" "${dr_ops}"
    "${postprocess}" "${resbase}")
  add_dependencies(${depender} target_${test})
endfunction(newtest_gcc)

if (NOT X64)
# FIXME i#111, i#217: 64-bit is only supported in pattern mode
# Leaving indentation as-is to avoid code churn
newtest(hello hello.c)
newtest(malloc malloc.c)
newtest(free free.c)
newtest(registers registers.c)
newtest(bitfield bitfield.cpp)
# linux glibc backtrace on hashtable curiosities causing problems:
# * making syscalls => hit sysenter hook
# * w/ -sysenter_is_int80 => calls abort() for some reason
newtest_ex(cs2bug cs2bug.cpp "" "" "" OFF "")
newtest(float float.c)
newtest(selfmod selfmod.c)
newtest(patterns patterns.c)

if (UNIX)
  newtest(signal signal.c)
  newtest(unix_syscalls unix_syscalls.c)
  target_link_libraries(signal m)
  if (NOT VMKERNEL)
    # PR 468352: SYS_clone w/o CLONE_SETTLS not supported on ESXi
    newtest(clone clone.c)
  endif (NOT VMKERNEL)
  get_relative_location(malloc malloc_path)
  newtest_ex(execve execve.c "${malloc_path}" "" "" OFF "")
  newtest(pthreads pthreads.c)
  target_link_libraries(pthreads pthread)
  tobuild_lib(loaderlib loader.lib.c "" "")
  get_relative_location(loaderlib loaderlib_path)
  newtest_ex(loader loader.c "${loaderlib_path}" "" "" OFF "")
  target_link_libraries(loader dl)
else (UNIX)
  newtest(winthreads winthreads.c)
  if (NOT TOOL_DR_HEAPSTAT)
    newtest(procterm procterm.c)
  endif (NOT TOOL_DR_HEAPSTAT)
endif (UNIX)

if (UNIX)
  tobuild_lib(unloadlib unload.lib.c "-fno-builtin" "")
else (UNIX)
  tobuild_lib(unloadlib unload.lib.c "/DUSE_CUSTOM_MALLOC" "/nodefaultlib /noentry")
endif (UNIX)
get_relative_location(unloadlib unloadlib_path)
newtest_ex(unload unload.c "${unloadlib_path}" "" "" OFF "")
if (UNIX)
  target_link_libraries(unload dl)
endif (UNIX)
if (WIN32)
  tobuild_lib(unloadlibMTd unload.lib.c "/D_DEBUG /MTd" "")
  get_relative_location(unloadlibMTd unloadlibMTd_path)
  newtest_nobuild(unloadMTd unload "${unloadlibMTd_path}" "" "" OFF "unload")
endif (WIN32)

if (NOT TOOL_DR_HEAPSTAT)
  # PR 525807: test malloc stacks
  newtest(varstack varstack.c)

  # PR 464804: test runtime options
  # FIXME: we should set up a suite like DR uses.  For now hand-picking
  # a few to run w/ options.
  newtest_nobuild(leaks-only malloc "" "-leaks_only" "" OFF "")
  newtest_nobuild(slowpath registers "" "-no_fastpath" "" OFF "registers")
  newtest_nobuild(slowesp registers "" "-no_esp_fastpath" "" OFF "registers")
  newtest_nobuild(addronly free "" "-light" "" OFF "")
  newtest_nobuild(addronly-reg registers "" "-no_check_uninitialized" "" OFF "")
  newtest_nobuild(reachable cs2bug "" "-show_reachable" "" OFF "")
  if (USE_DRSYMS)
    newtest_nobuild(nosymcache malloc "" "-no_use_symcache" "" OFF malloc)
  endif (USE_DRSYMS)
  newtest_nobuild(strict_bitops bitfield "" "-strict_bitops" "" OFF "bitfield.strict")
  # test this option to exercise the realloc handling code.
  # note that we can't run the realloc test b/c the races will result in unaddrs.
  newtest_nobuild(malloc_noreplace malloc "" "-no_replace_realloc" "" OFF "malloc")
  # test redzone sizes
  newtest_nobuild(redzone8 malloc "" "-redzone_size;8" "" OFF "malloc")
  newtest_nobuild(redzone1024 malloc "" "-redzone_size;1024" "" OFF "malloc")

  # shared by all suppress tests
  tobuild(suppress suppress.c)
  if (WIN32)
    tobuild_lib(suppress-mod-foo suppress-mod-foo.c "" "/nodefaultlib /noentry")
    tobuild_lib(suppress-mod-bar suppress-mod-bar.c "" "/nodefaultlib /noentry")
  else ()
    tobuild_lib(suppress-mod-foo suppress-mod-foo.c "" "")
    tobuild_lib(suppress-mod-bar suppress-mod-bar.c "" "")
    target_link_libraries(suppress dl)  # Needed for dlopen.
  endif ()

  # test suppress wildcards
  if (UNIX)
    if (VMKERNEL)
      set(supp_fileA "{DRMEMORY_CTEST_SRC_DIR}/suppress.vmk.suppress")
      set(supp_fileB "{DRMEMORY_CTEST_SRC_DIR}/suppress.vmk.suppress")
    else (VMKERNEL)
      set(supp_fileA "{DRMEMORY_CTEST_SRC_DIR}/suppress.lin.suppress")
      set(supp_fileB "{DRMEMORY_CTEST_SRC_DIR}/suppressB.lin.suppress")
    endif (VMKERNEL)
  else (UNIX)
    set(supp_fileA "{DRMEMORY_CTEST_SRC_DIR}/suppress.win.suppress")
    set(supp_fileB "{DRMEMORY_CTEST_SRC_DIR}/suppressB.win.suppress")
  endif (UNIX)
  # test multiple supp files (i#574)
  if (USE_DRSYMS)
    newtest_nobuild(suppress suppress ""
      "-suppress;${supp_fileA};-suppress;${supp_fileB};-callstack_modname_hide;;"
      "" OFF "")
  else (USE_DRSYMS)
    newtest_nobuild(suppress suppress ""
      "-suppress;${supp_fileA};-suppress;${supp_fileB}" "" OFF "")
  endif (USE_DRSYMS)

  # i#80: test suppression file generation and use via multiple runs
  # since we need the name of the suppress file, runtest.cmake must do
  # the second run.  it looks for "suppress" with no "-suppress" option,
  # rather than taking explicit params, and for the 2nd run uses the
  # suppress output files
  if (USE_DRSYMS)
    newtest_nobuild(suppress-genoffs suppress ""
      "-no_gen_suppress_syms;-callstack_modname_hide;;" "" OFF "")
    newtest_nobuild(suppress-gensyms suppress ""
      "-no_gen_suppress_offs;-callstack_modname_hide;;" "" OFF "")
  else (USE_DRSYMS)
    newtest_nobuild(suppress-genoffs suppress "" "-no_gen_suppress_syms" "" OFF "")
    newtest_nobuild(suppress-gensyms suppress "" "-no_gen_suppress_offs" "" OFF "")
  endif (USE_DRSYMS)

  set(nudge_test_args "")

  if (UNIX)
    # test -skip_results + -results (PR 575481)
    newtest_nobuild(skip-results malloc "" "-skip_results" "" ON "")
  endif (UNIX)

  # For -perturb_only we do not have a .res file so that runtest.cmake
  # won't wait for a message showing that results are ready
  # FIXME i#715: perturb is flaky on the bots.
  if (UNIX)
    newtest_nobuild(perturb_FLAKY pthreads "" "-perturb_only" "" OFF "")
  else (UNIX)
    newtest_nobuild(perturb_FLAKY winthreads  "" "-perturb_only" "" OFF "")
  endif (UNIX)

  if (WIN32)
    newtest(wincrt wincrt.cpp)
    # FIXME: Re-enable when it doesn't break the build.
    #newtest(rtl_memory_zones rtl_memory_zones.c)
    #target_link_libraries(rtl_memory_zones ntdll)
    if (USE_DRSYMS) # cygwin build can't see static malloc routines
      newtest_custbuild(wincrtdbg wincrt.cpp "/MTd" "")
      newtest_custbuild(mallocMTd malloc.c "/MTd" malloc)
      newtest_custbuild(mallocMD malloc.c "/MD" malloc)

      # C++ tests with other libc configs
      # for /MTd, we skip the destructor mismatches, as they end up raising
      # heap assertions that we can't recover from
      newtest_custbuild(cs2bugMTd cs2bug.cpp "/MTd /DSKIP_MISMATCH_DTR" cs2bugMTd)
      newtest_custbuild(cs2bugMD cs2bug.cpp "/MD" cs2bug)
    endif (USE_DRSYMS)

    if (GCC)
      # We have issues w/ static libstdc++ FPO so we ask for better callstacks (i#783)
      # With i#805, -no_callstack_use_top_fp is set automatically
      newtest_gcc(cs2bugMinGW cs2bug.cpp cs2bug "" "" "" "" cs2bug)
    endif (GCC)

    newtest(gdi gdi.cpp)
  endif (WIN32)

  # FIXME i#657: realloc is flaky on the Windows bots.
  set(realloc "realloc")
  if (WIN32)
    set(realloc "realloc_FLAKY")
  endif ()
  newtest(${realloc} realloc.c)
  if (UNIX)
    target_link_libraries(${realloc} pthread)
  endif (UNIX)

  newtest(annotations annotations.c)

  if (UNIX)
    newtest(memalign memalign.c)
  endif (UNIX)

  # persistent cache tests: currently only light mode is supported
  # XXX: would be nice to ensure pcaches are actually generated and used,
  # but how?  annotations would be the cleanest way but that requires
  # a bunch of annotations that are only used for tests.
  newtest_ex(pcache free.c "" "-light;-persist_code"
    # do not load any stale pcaches from prior runs, in first run
    "-no_use_persisted -no_coarse_disk_merge -no_coarse_lone_merge" OFF "addronly")
  if (WIN32)
    # we make a separate executable to avoid ASLR
    append_link_flags(pcache "/dynamicbase:no")
  endif ()
  newtest_nobuild(pcache-use pcache "" "-light;-persist_code" "" OFF "addronly")
  # when running tests in parallel, have to generate pcaches first
  set_property(TEST pcache-use APPEND PROPERTY DEPENDS pcache)
  # pattern mode testing.
  newtest_nobuild(free.pattern free "" "-pattern;0xf1fd;-no_count_leaks" "" OFF "addronly")
  newtest_nobuild(malloc.pattern malloc "" "-pattern;0xf1fd;-no_count_leaks" "" OFF "")
  newtest_nobuild(registers.pattern registers "" "-pattern;0xf1fd" "" OFF "addronly-reg")

else (NOT TOOL_DR_HEAPSTAT)
  newtest_ex(stale stale.c "" "-staleness;-stale_granularity 100" "" OFF "")

  newtest_nobuild(time-allocs malloc "" "-time_allocs" "" OFF "")
  newtest_nobuild(time-bytes malloc "" "-time_bytes" "" OFF "")
  newtest_nobuild(time-instrs malloc "" "-time_instrs" "" OFF "")
  newtest_nobuild(dump malloc "" "-dump" "" OFF "")

  set(nudge_test_args "")
endif (NOT TOOL_DR_HEAPSTAT)

# nudge test: runs infloop in background and runtest.cmake nudges it
tobuild(infloop infloop.c)
get_relative_location(infloop infloop_path)
# we have special support to put run_in_bg and its args first, so we put
# the test to run as a final drmem arg
newtest_nobuild(nudge run_in_bg_tgt
  "-out;./nudge-out"
  "${nudge_test_args}--;${infloop_path}" "" OFF "")

newtest(leakcycle leakcycle.cpp)

add_subdirectory(app_suite)

else (NOT X64)
  # pattern mode testing.
  newtest_ex(free.pattern free.c "" "-light" "" OFF "addronly")
  newtest_ex(malloc.pattern malloc.c "" "-light" "" OFF "")
endif (NOT X64)
