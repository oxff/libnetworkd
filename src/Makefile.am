# networkd automake input
# $Id: Makefile.am 19 2008-02-21 02:16:50Z lostrace $
# (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I../include/ -Werror -Wno-deprecated


library_includedir = $(includedir)/libnetworkd/
library_include_HEADERS  = ../include/libnetworkd/libnetworkd.hpp
library_include_HEADERS += ../include/libnetworkd/Configuration.hpp
library_include_HEADERS += ../include/libnetworkd/Event.hpp
library_include_HEADERS += ../include/libnetworkd/EventManager.hpp
library_include_HEADERS += ../include/libnetworkd/IO.hpp
library_include_HEADERS += ../include/libnetworkd/LogFacility.hpp
library_include_HEADERS += ../include/libnetworkd/LogManager.hpp
library_include_HEADERS += ../include/libnetworkd/ModuleManager.hpp
library_include_HEADERS += ../include/libnetworkd/NameResolution.hpp
library_include_HEADERS += ../include/libnetworkd/Network.hpp
library_include_HEADERS += ../include/libnetworkd/ProxiedNetwork.hpp
library_include_HEADERS += ../include/libnetworkd/TimeoutManager.hpp


lib_LTLIBRARIES = libnetworkd.la
libnetworkd_la_SOURCES  = Configuration.cpp ConfigParser.yacc.cpp ConfigParser.lex.cpp
libnetworkd_la_SOURCES += EventManager.cpp
libnetworkd_la_SOURCES += IOManager.cpp
libnetworkd_la_SOURCES += LogManager.cpp
libnetworkd_la_SOURCES += ModuleManager.cpp
libnetworkd_la_SOURCES += NetworkManager.cpp
libnetworkd_la_SOURCES += ProxiedNetworkManager.cpp
libnetworkd_la_SOURCES += PosixResolvingFacility.cpp
libnetworkd_la_SOURCES += TimeoutManager.cpp
libnetworkd_la_SOURCES += TcpSocket.cpp
libnetworkd_la_SOURCES += ProxiedTcpSocket.cpp
libnetworkd_la_SOURCES += UdnsResolvingFacility.cpp
libnetworkd_la_SOURCES += UdpSocket.cpp
libnetworkd_la_SOURCES += UnixSocket.cpp
libnetworkd_la_LDFLAGS = -version-info 1:0:0 --no-undefined --no-allow-shlib-undefined -ldl

noinst_HEADERS  = ConfigParser.yacc.hpp

