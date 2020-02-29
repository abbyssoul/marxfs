set(TEST_SOURCE_FILES
        main_gtest.cpp
        ci/teamcity_messages.cpp
        ci/teamcity_gtest.cpp

        test_model.cpp
    )


enable_testing()

add_executable(test_${PROJECT_NAME} EXCLUDE_FROM_ALL ${TEST_SOURCE_FILES} $<TARGET_OBJECTS:l${PROJECT_NAME}>)

target_include_directories(test_${PROJECT_NAME} PRIVATE ../src)


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

target_link_libraries(test_${PROJECT_NAME}
    ${CONAN_LIBS}
    Threads::Threads
    $<$<NOT:$<PLATFORM_ID:Darwin>>:rt>
    )

add_test(NAME test_${PROJECT_NAME}
    COMMAND test_${PROJECT_NAME}
    )
