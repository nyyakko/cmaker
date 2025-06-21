add_subdirectory(source)
add_subdirectory(include/${PROJECT_NAME})

add_executable(${PROJECT_NAME} "${cmaker_SourceFiles}")

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
        PROJECT_NAME="${PROJECT_NAME}"
)

if (ENABLE_CLANGTIDY)
    enable_clang_tidy(${PROJECT_NAME})
endif()

if (ENABLE_CPPCHECK)
    enable_cppcheck(${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

install(TARGETS     ${PROJECT_NAME}
        RUNTIME
        FILE_SET    ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

target_link_options(${PROJECT_NAME} PRIVATE ${cmaker_LinkerOptions})
target_compile_options(${PROJECT_NAME} PRIVATE ${cmaker_CompilerOptions})
target_link_libraries(${PROJECT_NAME} PRIVATE ${cmaker_ExternalLibraries})

