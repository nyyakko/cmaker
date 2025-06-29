add_subdirectory(source)
add_subdirectory(include/${PROJECT_NAME})

add_executable(${PROJECT_NAME} "${!PROJECT!_SourceFiles}")

if (ENABLE_CLANGTIDY)
    enable_clang_tidy(${PROJECT_NAME})
endif()

if (ENABLE_CPPCHECK)
    enable_cppcheck(${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

%IF [<|ENV:LANGUAGE|> EQUALS <c++>]:
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_!STANDARD!)
%ELSE:
target_compile_features(${PROJECT_NAME} PRIVATE c_std_!STANDARD!)
%END

%IF [<|ENV:FEATURES|> CONTAINS <installable>]:
install(TARGETS     ${PROJECT_NAME}
        RUNTIME
        FILE_SET    ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
%END

target_link_options(${PROJECT_NAME} PRIVATE ${!PROJECT!_LinkerOptions})
target_compile_options(${PROJECT_NAME} PRIVATE ${!PROJECT!_CompilerOptions})
target_link_libraries(${PROJECT_NAME} PRIVATE ${!PROJECT!_ExternalLibraries})

