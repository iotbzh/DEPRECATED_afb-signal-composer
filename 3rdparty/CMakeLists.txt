###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Romain Forlot <romain@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

set(LUA external-lua)
set(LUA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
ExternalProject_Add(${LUA}
    URL https://www.lua.org/ftp/lua-5.3.4.tar.gz
    SOURCE_DIR ${LUA_SOURCE_DIR}
    PATCH_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/crosscompile_lua.patch
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cd src && make all
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

PROJECT_TARGET_ADD(lua-lib)

add_library(${TARGET_NAME} STATIC IMPORTED GLOBAL)

SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
    IMPORTED_LOCATION ${LUA_SOURCE_DIR}/src/liblua.a
    INTERFACE_INCLUDE_DIRECTORIES ${LUA_SOURCE_DIR}/src
)

add_dependencies(${TARGET_NAME} ${LUA})

PROJECT_TARGET_ADD(lua)

add_executable(${TARGET_NAME} IMPORTED GLOBAL)

SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	LABELS EXECUTABLE
	IMPORTED_LOCATION ${LUA_SOURCE_DIR}/src/lua
)

add_dependencies(${TARGET_NAME} ${LUA})

PROJECT_TARGET_ADD(lua-compiler)

add_executable(${TARGET_NAME} IMPORTED GLOBAL)

SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	LABELS EXECUTABLE
	IMPORTED_LOCATION ${LUA_SOURCE_DIR}/src/luac
)

add_dependencies(${TARGET_NAME} ${LUA})
