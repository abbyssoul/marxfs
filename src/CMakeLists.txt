
set(SOURCE_FILES
        errorDomain.cpp
        model.cpp
    )


add_library(l${PROJECT_NAME} OBJECT ${SOURCE_FILES})

target_compile_definitions(l${PROJECT_NAME} PRIVATE "-DASIO_STANDALONE")

target_include_directories(l${PROJECT_NAME} SYSTEM PRIVATE ../${EXTERNAL_DEP_ASIO_DIR})
target_include_directories(l${PROJECT_NAME} SYSTEM PRIVATE ../${EXTERNAL_DEP_SAJSON_DIR})

add_executable(${PROJECT_NAME} main.cpp $<TARGET_OBJECTS:l${PROJECT_NAME}>)
target_compile_definitions(${PROJECT_NAME} PRIVATE "-DASIO_STANDALONE")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ../${EXTERNAL_DEP_ASIO_DIR})


install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${CONAN_LIBS}
    Threads::Threads
    $<$<BOOL:${COVERALLS}>:--coverage>
    )
