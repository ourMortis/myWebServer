cmake_minimum_required(VERSION 4.0)

project(myWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include_directories(./include ./config ./test)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/test_space)

#add_executable(async_logger_test ./test/unit/async_logger_test.cpp ./src/buffer.cpp ./src/async_logger.cpp ./src/log_file.cpp)

#add_executable(test_async_logger_2 ./test/unit/test_async_logger_2.cpp ./src/buffer.cpp ./src/async_logger.cpp ./src/log_file.cpp)

#add_executable(mytest ./test/unit/mytest.cpp ./src/buffer.cpp ./src/async_logger.cpp ./src/log_file.cpp)

#add_executable(test_thread_pool ./test/unit/test_thread_pool.cpp ./src/thread_pool.cpp)

#add_executable(buffer_test ./test/buffer_test.cpp ./src/buffer.cpp)

#add_executable(log_buffer_test ./test/log_buffer_test.cpp ./src/buffer.cpp)

#add_executable(block_queue_test ./test/block_queue_test.cpp)

#add_executable(log_file_test ./test/log_file_test.cpp ./src/log_file.cpp)

add_executable(test_mysql ./test/unit/test_sql.cpp)
target_link_libraries(test_mysql mysqlclient)