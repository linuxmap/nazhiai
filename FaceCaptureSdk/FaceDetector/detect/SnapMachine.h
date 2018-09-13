
#ifndef _SNAPMACHINE_HEADER_H
#define _SNAPMACHINE_HEADER_H

#include "FaceSdkApi.h"

#include "SnapStruct.h"

#include <string>

#include "HCNetSDK.h"

class FaceDetector;
class SnapMachine
{
public:
    SnapMachine(FaceDetector* faceDetector, const std::string& id, const FaceParam& faceParam)
        : _faceDetector(faceDetector), _id(id), _faceParam(faceParam)
    {
        _faceParam.detect_interval = 1;
        _faceParam.capture_interval = 0;
    }
    virtual ~SnapMachine()
    {}

    virtual bool Start() = 0;
    virtual bool Stop() = 0;

    inline const std::string& GetId() const
    {
        return _id;
    }

    const int GetGpuIndex() const;

    bool Snap(const ImageParam& imageParam);

protected:
    virtual int Init();
    virtual void Uninit();
    
    bool DecodeFrame(const std::vector<unsigned char>& scence, cv::Mat& mat, const std::vector<FaceInfo>& faceInfos, std::vector<cv::Mat>& mats);

private:
    friend BOOL CALLBACK HCSnapCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);

protected:
    FaceDetector* _faceDetector;

    std::string _id;
    FaceParam _faceParam;

private:
    SnapMachine(const SnapMachine&);
    SnapMachine& operator=(const SnapMachine&);
};

class LoginSnapMachine : public SnapMachine
{
public:
    static std::string GenerateId(const std::string& ip, unsigned short port);

    static SnapMachine* GenerateLoginSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, int cameraType, const std::string& ip, unsigned short port, const std::string& username, const std::string& password);

public:
    LoginSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, const std::string& ip, unsigned short port, const std::string& username, const std::string& password);
    virtual ~LoginSnapMachine();

    virtual bool Start() = 0;
    virtual bool Stop() = 0;

protected:
    std::string _ip;
    unsigned short _port;
    std::string _username;
    std::string _password;

private:
    LoginSnapMachine();
    LoginSnapMachine(const LoginSnapMachine&);
    LoginSnapMachine& operator=(const LoginSnapMachine&);
};

class HCSnapMachine : public LoginSnapMachine
{
public:
    HCSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, const std::string& ip, unsigned short port, const std::string& username, const std::string& password);
    ~HCSnapMachine();

    bool Start();
    bool Stop();

protected:
    virtual int Init();
    virtual void Uninit();

private:
    long _session;
    long _event;
    NET_VCA_FACESNAPCFG _snapcfg;

private:
    HCSnapMachine();
    HCSnapMachine(const HCSnapMachine&);
    HCSnapMachine& operator=(const HCSnapMachine&);

};

class ImageMachine : public SnapMachine
{
public:
    ImageMachine(FaceDetector* faceDetector, const std::string& id, const FaceParam& faceParam);
    ~ImageMachine();

    bool Start();
    bool Stop();

private:
    ImageMachine();
    ImageMachine(const ImageMachine&);
    ImageMachine& operator=(const ImageMachine&);
};

#endif

