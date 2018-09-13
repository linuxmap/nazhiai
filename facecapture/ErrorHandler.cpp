#include "ErrorHandler.h"

std::string ErrorHandler::_error_message;
std::stringstream ErrorHandler::ErrorStream;

void ErrorHandler::SetLastError(const std::string& err)
{
    _error_message = err;
}

const char* ErrorHandler::GetLastError()
{
    return _error_message.c_str();
}

void ErrorHandler::FlushLastErrorStream()
{
    ErrorStream >> _error_message;
    ErrorStream.clear();
    ErrorStream.str("");
}
