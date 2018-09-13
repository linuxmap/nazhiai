
#ifndef _DECODETASK_HEADER_H_
#define _DECODETASK_HEADER_H_

#include "BaseDecoder.h"

class DecodeTask
{
public:
    static BaseDecoder* OpenRTSP(const std::string&, const DecoderParam&, const DecodeParam&, const std::string& = "") throw(BaseException);
    static void CloseRTSP(BaseDecoder*) throw(BaseException);

    static BaseDecoder* OpenVideo(const std::string&, const DecoderParam&, const DecodeParam&, const std::string& = "") throw(BaseException);
    static void CloseVideo(BaseDecoder*) throw(BaseException);

    static BaseDecoder* OpenUSB(const std::string&, const DecoderParam&, const DecodeParam&, const std::string& = "") throw(BaseException);
    static void CloseUSB(BaseDecoder*) throw(BaseException);

    static BaseDecoder* OpenDirectory(const std::string&, const DecoderParam&, const DecodeParam&, const std::string& = "") throw(BaseException);
    static void CloseDirectory(BaseDecoder*) throw(BaseException);

public:
    static DecodeTask* CreateTask();
    static void DestroyTask(DecodeTask*);

    bool GetFrames(std::vector<DecodedFramePtr>&);

    void AddDecoder(BaseDecoder* baseDecoder);
    void DelDecoder(BaseDecoder* baseDecoder);

private:
    void ClearDecoders();

private:
    static void TryCreateDecoder(BaseDecoder* baseDecoder) throw(BaseException);
    static void TryDestoryDecoder(BaseDecoder* baseDecoder) throw(BaseException);
    static void DestoryDecoder(BaseDecoder* baseDecoder) throw();

private:
    std::mutex _locker;
    std::vector<BaseDecoder*> _decoders;

private:
    DecodeTask();
    DecodeTask(const DecodeTask&);
    DecodeTask& operator=(const DecodeTask&);
};

#endif


