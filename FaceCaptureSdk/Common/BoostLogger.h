
#ifndef _BOOSTLOGGER_HEADER_H_
#define _BOOSTLOGGER_HEADER_H_

#include <string>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/keywords/channel.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

#pragma comment(lib, "Psapi.lib")

// -------------- local static log ------------------
#define USING_LOCAL_LOG(module) \
    static boost::log::sources::severity_channel_logger<boost::log::trivial::severity_level, std::string> module##_sclogger(boost::log::keywords::channel = #module)

#define LOCAL_LOG(module, lvl) BOOST_LOG_SEV(module##_sclogger, boost::log::trivial::lvl)

// ----------------- logger initialization --------------------
#define INITIALIZE_BOOST_LOGGER() boost::log::add_common_attributes();\
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");\
    boost::log::register_simple_filter_factory<boost::log::trivial::severity_level, char>("Severity");\
    try\
    {\
        boost::log::init_from_stream(std::ifstream("boost_log_setup.dll"));\
    }\
    catch (const std::exception& e)\
    {\
        std::cerr << "Log initialize failed, reason: " << e.what() << std::endl;\
    }

#endif
