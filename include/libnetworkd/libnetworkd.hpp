/*
 * libnetworkd.hpp - meta header including other headers and defining version
 * $Id: libnetworkd.hpp 18 2008-02-21 02:11:39Z lostrace $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#ifndef __INCLUDE_libnetworkd_libnetworkd_hpp
#define __INCLUDE_libnetworkd_libnetworkd_hpp

#define LIBNETWORKD_VERSION			0x0100
#define LIBNETWORKD_VERSION_STRING	"1.0"


#include "Configuration.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "IO.hpp"
#include "LogFacility.hpp"
#include "LogManager.hpp"
#include "ModuleManager.hpp"
#include "NameResolution.hpp"
#include "Network.hpp"
#include "ProxiedNetwork.hpp"
#include "TimeoutManager.hpp"

#endif // #ifndef __INCLUDE_libnetworkd_libnetworkd_hpp

/**
* \mainpage
* \code
*  _ _ _                _                      _       _ 
* | (_) |__  _ __   ___| |___      _____  _ __| | ____| |
* | | | '_ \| '_ \ / _ \ __\ \ /\ / / _ \| '__| |/ / _` |
* | | | |_) | | | |  __/ |_ \ V  V / (_) | |  |   < (_| |
* |_|_|_.__/|_| |_|\___|\__| \_/\_/ \___/|_|  |_|\_\__,_|
*    
* 		version 1.0.0
* \endcode
*
* libnetworkd is the skeleton library for a configurable, modular, event driven,
* single-threaded and logging capable network daemon conforming to the POSIX
* standard and using autotools.
* 
* This code is published under the terms of the GNU Public License, see the file
* LICENSE for details. It is (c) 2007 by Georg Wicherski,
* <georg-wicherski@pixel-house.net>.
*
*/
