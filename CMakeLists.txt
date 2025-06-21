cmake_minimum_required(VERSION 3.25)

project(cmaker LANGUAGES CXX)

include(cmake/get_cpm.cmake)

CPMAddPackage(URI "gh:Dobiasd/FunctionalPlus#v0.2.24" EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:p-ranav/argparse@3.2"           EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nlohmann/json@3.11.3"           EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:fmtlib/fmt#10.2.1"              EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibError#master"        EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibPreprocessor#master" EXCLUDE_FROM_ALL YES)

include(cmake/static_analyzers.cmake)
include(GNUInstallDirs)

set(cmaker_CompilerOptions ${cmaker_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(cmaker_LinkerOptions ${cmaker_LinkerOptions})

set(cmaker_ExternalLibraries
    argparse::argparse
    FunctionalPlus::fplus
    nlohmann_json::nlohmann_json
    fmt::fmt
    LibError::LibError
    LibPreprocessor::LibPreprocessor
)

add_subdirectory(cmaker)

