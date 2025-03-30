# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-src"
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-build"
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix"
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix/tmp"
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp"
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src"
  "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/kostyantin/Desktop/web_server_cpp/cmake-build-debug/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
