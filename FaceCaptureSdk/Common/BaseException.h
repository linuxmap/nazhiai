
#ifndef _BASEEXCEPTION_HEADER_H_
#define _BASEEXCEPTION_HEADER_H_

#include <string>

class BaseException
{
public:
    BaseException(int code, const std::string& desc)
        : _code(code), _description(desc)
    {}

    ~BaseException()
    {}

    int Code() const
    {
        return _code;
    }

    const std::string& Description() const
    {
        return _description;
    }

protected:
    int _code;
    std::string _description;
};

#endif

