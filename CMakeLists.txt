cmake_minimum_required(VERSION 3.27)
project(MyJsonServer LANGUAGES CXX)

# 查找依赖库
find_path(MYSQL_INCLUDE_DIR mysql.h PATHS /usr/include/mysql)
find_library(MYSQL_LIBRARY NAMES mariadb)
find_package(Qt6 REQUIRED COMPONENTS Core Network)
find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system thread)
find_package(absl REQUIRED)
find_package(Protobuf REQUIRED)
find_library(HIREDIS_LIB hiredis)
find_package(OpenSSL REQUIRED)

# 包含目录
include_directories(
    ${MYSQL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    /usr/include
    /usr/include/jsoncpp
    ${CMAKE_SOURCE_DIR}/include
)

# 自动处理Qt元对象代码
set(CMAKE_AUTOMOC ON)

# 添加可执行文件
add_executable(
  MyJsonServer
  main.cpp
  cserver.h
  cserver.cpp
  defines.h
  csession.h
  csession.cpp
  msgnode.h
  msgnode.cpp
  Singleton.h
  logicsystem.h
  logicsystem.cpp
  logicnode.h
  logicnode.cpp
  servicepool.h
  servicepool.cpp
  file.h
  file.cpp
  filemanagement.h
  filemanagement.cpp
  hashmd5.h
  hashmd5.cpp
  client.h
  client.cpp
  base64_code.h
  base64_code.cpp
  message.pb.h
  message.pb.cc
  message.grpc.pb.h
  message.grpc.pb.cc
  config.ini
  message.proto
  ConfigMgr.h
  ConfigMgr.cpp
  VerifyGrpcClient.h
  VerifyGrpcClient.cpp
  HttpMgr.h
  HttpMgr.cpp
  RedisMgr.cpp
  RedisMgr.h
)

# 编译特性
target_compile_features(MyJsonServer PRIVATE cxx_std_20)
set_target_properties(MyJsonServer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接库
target_link_libraries(MyJsonServer PRIVATE
    ${MYSQL_LIBRARY}
    Qt6::Core
    Qt6::Network
    ${CURL_LIBRARIES}
    ${HIREDIS_LIB}
    Boost::filesystem
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    absl::log_internal_check_op
    absl::cord
    absl::strings
    absl::base
    absl::synchronization
    protobuf::libprotobuf
    protobuf::libprotoc
    protobuf::libprotobuf-lite
    libgpr.so
    libgrpc.so
    libgrpc++.so
    libgrpc++_reflection.so
    libaddress_sorting.so
    /usr/lib/libjsoncpp.so
)
