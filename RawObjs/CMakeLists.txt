cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "RawObjs")
project(${PROJECT_NAME})

message(STATUS "Building ${PROJECT_NAME} standalone on macOS...")

# ============================================================================
# ROOT 설정
# ============================================================================
find_package(ROOT REQUIRED COMPONENTS Core)
include(${ROOT_USE_FILE})

# ============================================================================
# 프로젝트 디렉토리 설정
# ============================================================================
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(PROJECT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

# ============================================================================
# 소스 및 헤더 파일 수집
# ============================================================================
file(GLOB ${PROJECT_NAME}_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cc")
file(GLOB ${PROJECT_NAME}_HEADERS "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/*.hh")

# 포함 디렉토리 생성
file(MAKE_DIRECTORY ${PROJECT_INCLUDE_DIR}/${PROJECT_NAME})

# 헤더 파일 복사
foreach(HEADER_i ${${PROJECT_NAME}_HEADERS})
  configure_file(${HEADER_i} ${PROJECT_INCLUDE_DIR}/${PROJECT_NAME} COPYONLY)
endforeach(HEADER_i)

# LinkDef.h 복사
configure_file(
  ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/LinkDef.h
  ${PROJECT_INCLUDE_DIR}/${PROJECT_NAME}
  COPYONLY
)

# 업데이트된 헤더 파일 목록
file(GLOB ${PROJECT_NAME}_HEADERS ${PROJECT_INCLUDE_DIR}/${PROJECT_NAME}/*.hh)
set(${PROJECT_NAME}_DICT_FILE ${PROJECT_INCLUDE_DIR}/${PROJECT_NAME}/LinkDef.h)

# ============================================================================
# ROOT 딕셔너리 생성
# ============================================================================
root_generate_dictionary(
  G__${PROJECT_NAME}
  ${${PROJECT_NAME}_HEADERS}
  LINKDEF ${${PROJECT_NAME}_DICT_FILE}
)

# ============================================================================
# 라이브러리 빌드
# ============================================================================
add_library(${PROJECT_NAME} SHARED
  ${${PROJECT_NAME}_SOURCES}
  G__${PROJECT_NAME}.cxx
)

# 포함 디렉토리 설정
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${PROJECT_INCLUDE_DIR}
    ${ROOT_INCLUDE_DIRS}
)

# ROOT 라이브러리 링크
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    ${ROOT_LIBRARIES}
)

# ============================================================================
# 설치 설정
# ============================================================================
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)

install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_rdict.pcm
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.rootmap
  DESTINATION lib
)

install(
  FILES ${${PROJECT_NAME}_HEADERS}
  DESTINATION include/${PROJECT_NAME}
)

# ============================================================================
# 빌드 정보
# ============================================================================
message(STATUS "ROOT version: ${ROOT_VERSION}")
message(STATUS "ROOT libraries: ${ROOT_LIBRARIES}")
message(STATUS "Source files: ${${PROJECT_NAME}_SOURCES}")
message(STATUS "Header files: ${${PROJECT_NAME}_HEADERS}")
message(STATUS "Install directory: ${CMAKE_INSTALL_PREFIX}")
