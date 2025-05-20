include(GenerateExportHeader)

# In order to maintain compatibility with older platforms which may not have a
# recent version of CMake (i.e. are running CMake <3.3), we cannot simply use
# the `add_compile_options` with a generator expression.  This uses the
# `target_compile_options` with `PRIVATE` to add the flags only to the targets
# so that CMark may be used in projects with non-C languages.
function(cmark_add_compile_options target)
  if(MSVC)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(${target} PRIVATE
            -Wall -Wextra -pedantic
            $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL Profile)
    target_compile_options(${target} PRIVATE -pg)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL Ubsan)
    target_compile_options(${target} PRIVATE -fsanitize=undefined)
  endif()
  if(CMARK_LIB_FUZZER)
    if(target MATCHES fuzz)
      target_compile_options(${target} PRIVATE -fsanitize=fuzzer)
      target_link_options(${target} PRIVATE -fsanitize=fuzzer)
    else()
      target_compile_options(${target} PRIVATE -fsanitize=fuzzer-no-link)
      target_link_options(${target} PRIVATE -fsanitize=fuzzer-no-link)
    endif()
  endif()
endfunction()

set (CMARK_STATIC ON)

add_subdirectory(src)

