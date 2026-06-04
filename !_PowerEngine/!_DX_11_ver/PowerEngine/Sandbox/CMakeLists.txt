add_executable(Sandbox main.cpp)

target_link_libraries(Sandbox PRIVATE Engine)

if(WIN32)
    add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:Sandbox>
    )
endif()