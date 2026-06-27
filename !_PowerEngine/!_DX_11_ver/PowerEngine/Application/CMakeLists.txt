add_executable(PowerEngine "main.cpp")

target_link_libraries(PowerEngine PRIVATE Engine)

if(WIN32)
    add_custom_command(TARGET PowerEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:PowerEngine>
    )
endif()