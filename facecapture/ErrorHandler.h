
#ifndef _ERRORHANDLER_HEADER_H_
#define _ERRORHANDLER_HEADER_H_

#include <string>
#include <sstream>

class ErrorHandler
{
public:
    static void SetLastError(const std::string& err);
    static const char* GetLastError();

    static std::stringstream ErrorStream;
    static void FlushLastErrorStream();

private:
    static std::string _error_message;

private:
    ErrorHandler();
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);
};

#endif


