cmake_minimum_required(VERSION 3.27)
project(
    MyJsonServer
    VERSION 0.1
    LANGUAGES CXX)

# 包含目录
include_directories(
    ${MYSQL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    /usr/include
    /usr/include/jsoncpp
    ${CMAKE_SOURCE_DIR}/include
)

find_path(MYSQL_INCLUDE_DIR mysql.h PATHS /usr/include/mysql)
include_directories(${MYSQL_INCLUDE_DIR})
# find_package(Qt6 REQUIRED COMPONENTS Core Network Qml Gui)
# find_package(Qt6 REQUIRED COMPONENTS Widgets Quick)
find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system thread)
find_package(absl REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(MYSQL_LIBRARY NAMES mariadb)
find_library(HIREDIS_LIB hiredis)
# qt_standard_project_setup()



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
    Main.qml
    Window.qml
    Firstlogin.qml
    Content.qml
    Controler.js
    register.qml
    resources.qrc
    delayedthread.h
    Register.qml
    code.qml
    Connectdb.h Connectdb.cpp
    ServereMain.qml
    MysqlDao.cpp MysqlDao.h
    MysqlMgr.cpp MysqlMgr.h
    filetimer.h
    # ServerLogin.cpp ServerLogin.h
    # ServerRegister.cpp ServerRegister.h
    # CServerStart.h CServerStart.cpp
)

# 编译特性
target_compile_features(MyJsonServer PRIVATE cxx_std_20)

find_package(OpenSSL REQUIRED)

# target_include_directories(MyJsonServer PRIVATE /usr/include)
# target_include_directories(MyJsonServer PRIVATE /usr/include/jsoncpp)
target_include_directories(MyJsonServer PRIVATE
    ${MYSQL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    /usr/include
    /usr/include/jsoncpp
    ${CMAKE_SOURCE_DIR}/include
    # ${Qt6Qml_DIR}/../../include/QtQml
)

# 链接库
target_link_libraries(MyJsonServer PRIVATE
    ${MYSQL_LIBRARY}
    # Qt6::Core
    # Qt6::Network
    # Qt6::Qml
    # Qt6::Gui
    # Qt6::Widgets
    # Qt6::Quick
    ${CURL_LIBRARIES}
    ${HIREDIS_LIB}
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::boost
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

set_target_properties(MyJsonServer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # AUTORCC TRUE
    # MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    # MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    # MACOSX_BUNDLE_SHORT_VERSION_STRING
    # ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    # MACOSX_BUNDLE TRUE
    # WIN32_EXECUTABLE TRUE
)


# include(GNUInstallDirs)
# install(
#   TARGETS MyJsonServer
#   BUNDLE DESTINATION .
#   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# qt_finalize_executable(MyJsonServer)

