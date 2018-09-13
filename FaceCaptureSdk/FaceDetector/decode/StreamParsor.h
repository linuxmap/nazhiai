
#ifndef _STREAMPARSOR_HEADER_H_
#define _STREAMPARSOR_HEADER_H_

#include <string>

struct StreamInfo
{
    double fps = 0.0f;
    double interval = 0.0f;
};

class StreamParsor
{
public:
    StreamParsor(const std::string& url);
    ~StreamParsor();

    bool Parse(StreamInfo& info, std::string& err);

protected:
    std::string _url;

private:
    StreamParsor();
    StreamParsor(const StreamParsor&);
    StreamParsor& operator=(const StreamParsor&);
};

#endif

