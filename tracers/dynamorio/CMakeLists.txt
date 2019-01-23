cmake_minimum_required(VERSION 2.8)

add_library(villoc_tracer SHARED villoc_tracer.c)
find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
  message(FATAL_ERROR "DynamoRIO package required to build")
endif(NOT DynamoRIO_FOUND)
use_DynamoRIO_extension(villoc_tracer drwrap)
use_DynamoRIO_extension(villoc_tracer drmgr)


set(DynamoRIO_USE_LIBC OFF)
configure_DynamoRIO_client(villoc_tracer)
