
#ifndef _DECODEMANAGER_HEADER_H_
#define _DECODEMANAGER_HEADER_H_

#include "BaseDecoder.h"

#include <atomic>

class DecodeManager
{
public:
    static int GetLicenseInformation();
    static void UseOne();
    static void ReturnOne();

    static BaseDecoder* OpenRTSP(const std::string&, const DecoderParam&, const DecodeParam&, const std::string&, bool async) throw(BaseException);

    static BaseDecoder* OpenVideo(const std::string&, const DecoderParam&, const DecodeParam&, const std::string&, bool async) throw(BaseException);

    static BaseDecoder* OpenUSB(const std::string&, const DecoderParam&, const DecodeParam&, const std::string&) throw(BaseException);

    static BaseDecoder* OpenDirectory(const std::string&, const DecoderParam&, const DecodeParam&, const std::string&) throw(BaseException);

    static void CloseDecoder(BaseDecoder*);

private:
    static void TryCreateDecoder(BaseDecoder* baseDecoder) throw(BaseException);

private:
    static std::atomic_int _license_number;

private:
    DecodeManager();
    DecodeManager(const DecodeManager&);
    DecodeManager& operator=(const DecodeManager&);
};

#endif


