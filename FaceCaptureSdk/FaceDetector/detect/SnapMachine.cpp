
#include "SnapMachine.h"
#include "opencv2/opencv.hpp"

#include "FaceDetectorImpl.h"

#include "TimeStamp.h"
#include "Performance.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#define HC_IN_BUFFER_SIZE 512
#define HC_OUT_BUFFER_SIZE 1024*4

bool SnapMachine::DecodeFrame(const std::vector<unsigned char>& scence, cv::Mat& mat, const std::vector<FaceInfo>& faceInfos, std::vector<cv::Mat>& mats)
{
    if (!scence.empty())
    {
        unsigned char* image = (unsigned char*)scence.data();
        size_t imageLen = scence.size();
        START_EVALUATE(IMDecodeScence);
        mat = cv::imdecode(std::vector<unsigned char>{image, image + imageLen}, cv::IMREAD_COLOR);
        PRINT_COSTS(IMDecodeScence);
    }

    for each (FaceInfo face in faceInfos)
    {
        cv::Mat fmat;
        if (!face.faceImage.empty())
        {
            unsigned char* image = (unsigned char*)face.faceImage.data();
            size_t imageLen = face.faceImage.size();
            START_EVALUATE(IMDecodeFace);
            fmat = cv::imdecode(std::vector<unsigned char>{image, image + imageLen}, cv::IMREAD_COLOR);
            PRINT_COSTS(IMDecodeFace);
        }
        mats.push_back(fmat);
    }
    return !mat.empty();
}

const int SnapMachine::GetGpuIndex() const
{
    return _faceDetector->GetDeviceIndex();
}

bool SnapMachine::Snap(const ImageParam& imageParam)
{
    if (imageParam.scenceImage.size() > 0)
    {
        cv::Mat scenceMat;
        std::vector<cv::Mat> faceMats;
        if (DecodeFrame(imageParam.scenceImage, scenceMat, imageParam.faceInfos, faceMats))
        {
            const std::vector<FaceInfo>& faceInfos = imageParam.faceInfos;
            if (faceInfos.size() > 0)
            {
                cv::Mat canvasMat;
                FaceRects faceRects;
                FaceBoxIds faceBoxIds;
                for (size_t idx = 0; idx < faceInfos.size(); ++idx)
                {
                    const FaceInfo& faceInfo = faceInfos[idx];

                    cv::Mat facemat;
                    // based on scence image
                    if (faceInfo.faceImage.empty())
                    {
                        facemat = scenceMat(cv::Rect(faceInfo.x, faceInfo.y, faceInfo.width, faceInfo.height)).clone();
                    }
                    else // based on face image
                    {
                        facemat = faceMats[idx];                        
                    }

                    if (!facemat.empty())
                    {
                        if (_faceDetector->GetDeviceIndex() < 0)
                        {
                            ApiImagePtr apiImageIPtr = ApiImagePtr(new ApiMat(_faceParam, _id, imageParam.frameId, facemat, _faceDetector->GetDeviceIndex(), imageParam.timestamp, false, scenceMat));
                            if (apiImageIPtr)
                            {
                                apiImageIPtr->portrait = true;
                                apiImageIPtr->faceBoxIds.push_back(faceInfo.id);
                                _faceDetector->PushOneDetect(ApiImagePtrBuffer{ apiImageIPtr }, 1);
                            }
                        }
                        else
                        {
                            // GPU mode, we use scence image size canvas
                            if (canvasMat.empty())
                            {
                                canvasMat = scenceMat.clone();
                                canvasMat.setTo(cv::Scalar(0, 0, 0));
                            }
                            if (!canvasMat.empty() && facemat.cols <= canvasMat.cols && facemat.rows <= canvasMat.rows)
                            {
                                cv::Rect rect(faceInfo.x >= 0 ? faceInfo.x : 0, faceInfo.y >= 0 ? faceInfo.y : 0, facemat.cols, facemat.rows);
                                facemat.copyTo(canvasMat(rect));
                                faceRects.push_back(rect);
                                faceBoxIds.push_back(faceInfo.id);
                            }
                        }
                    }
                }

                if (!canvasMat.empty() && !faceRects.empty())
                {
                    ApiImagePtr apiImageIPtr = ApiImagePtr(new ApiMat(_faceParam, _id, imageParam.frameId, canvasMat, _faceDetector->GetDeviceIndex(), imageParam.timestamp, false, scenceMat, faceRects));
                    if (apiImageIPtr)
                    {
                        apiImageIPtr->portrait = true;
                        apiImageIPtr->faceBoxIds.swap(faceBoxIds);
                        _faceDetector->PushOneDetect(ApiImagePtrBuffer{ apiImageIPtr }, 1);
                    }
                }
            } 
            else
            {
                ApiImagePtr apiImageIPtr = ApiImagePtr(new ApiMat(_faceParam, _id, imageParam.frameId, scenceMat, _faceDetector->GetDeviceIndex(), imageParam.timestamp, false));
                if (apiImageIPtr)
                {
                    _faceDetector->PushOneDetect(ApiImagePtrBuffer{ apiImageIPtr }, 1);
                }
            }
            
            return true;
        }
    }
    return false;
}

int SnapMachine::Init()
{
    return 0;
}

void SnapMachine::Uninit()
{
}

std::string LoginSnapMachine::GenerateId(const std::string& ip, unsigned short port)
{
    return ip + "_" + std::to_string(port);
}

SnapMachine* LoginSnapMachine::GenerateLoginSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, int cameraType, const std::string& ip, unsigned short port, const std::string& username, const std::string& password)
{
    switch (cameraType)
    {
    case CAMERA_HC:
        return new HCSnapMachine(faceDetector, faceParam, ip, port, username, password);
    default:
        break;
    }
    return NULL;
}

LoginSnapMachine::LoginSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, const std::string& ip, unsigned short port, const std::string& username, const std::string& password)
    : SnapMachine(faceDetector, GenerateId(ip, port), faceParam), _ip(ip), _port(port), _username(username), _password(password)
{
}

LoginSnapMachine::~LoginSnapMachine()
{
}

HCSnapMachine::HCSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, const std::string& ip, unsigned short port, const std::string& username, const std::string& password)
    : LoginSnapMachine(faceDetector, faceParam, ip, port, username, password)
    , _session(-1), _event(-1), _snapcfg()
{
}

HCSnapMachine::~HCSnapMachine()
{
}

BOOL CALLBACK HCSnapCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
    static volatile unsigned int FRAME_ID = 0;
    if (!pUser)
    {
        LOG(ERROR) << __FUNCTION__ << " has not context";
        return FALSE;
    }

    SnapMachine* snapMachine = (SnapMachine*)pUser;

    switch (lCommand)
    {
    case COMM_UPLOAD_FACESNAP_RESULT:
    {
        NET_VCA_FACESNAP_RESULT struFaceSnap = { 0 };
        memcpy(&struFaceSnap, pAlarmInfo, sizeof(NET_VCA_FACESNAP_RESULT));

        if (struFaceSnap.dwBackgroundPicLen > 0 && struFaceSnap.pBuffer2 != NULL)
        {
            ImageParam imageParam;
            imageParam.frameId = FRAME_ID++;
            imageParam.imageType = 0;
            imageParam.timestamp = TimeStamp<MILLISECONDS>::Now();
            imageParam.scenceImage.swap(std::vector<unsigned char>{struFaceSnap.pBuffer2, struFaceSnap.pBuffer2 + struFaceSnap.dwBackgroundPicLen});
            cv::Mat mat = cv::imdecode(imageParam.scenceImage, cv::IMREAD_COLOR);;
            if (!mat.empty())
            {
                FaceInfo faceInfo;
                faceInfo.id = (int)struFaceSnap.dwFacePicID;
                faceInfo.x = (int)(struFaceSnap.struRect.fX*mat.cols);
                faceInfo.y = (int)(struFaceSnap.struRect.fY*mat.rows);

                // choose the shorter one as the face range
                faceInfo.width = (int)((struFaceSnap.struRect.fWidth > struFaceSnap.struRect.fHeight ? struFaceSnap.struRect.fHeight : struFaceSnap.struRect.fWidth) * mat.cols);
                faceInfo.height = faceInfo.width;

                faceInfo.confidence = (float)struFaceSnap.dwFaceScore;

                imageParam.faceInfos.push_back(faceInfo);

                snapMachine->Snap(imageParam);
            }
        }
    }
    break;
    default:
        LOG(WARNING) << __FUNCTION__ << " other alarm type: " << lCommand;
        break;
    }
    return TRUE;
}

BOOL CALLBACK HCFaceDetectionCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
    if (!pUser)
    {
        LOG(ERROR) << __FUNCTION__ << " has not context";
        return FALSE;
    }

    SnapMachine* snapMachine = (SnapMachine*)pUser;

    switch (lCommand)
    {
    case COMM_ALARM_FACE_DETECTION:
    {
        NET_DVR_FACE_DETECTION struFaceDetectionAlarm = { 0 };
        memcpy(&struFaceDetectionAlarm, pAlarmInfo, sizeof(NET_DVR_FACE_DETECTION));

        LOG(WARNING) << __FUNCTION__ << " COMM_ALARM_FACE_DETECTION alarm type: " << lCommand;
    }
    break;
    default:
        LOG(WARNING) << __FUNCTION__ << " other alarm type: " << lCommand;
        break;
    }
    return TRUE;
}


bool HCSnapMachine::Start()
{
    if (Init() == 0)
    {
        NET_DVR_SETUPALARM_PARAM  struAlarmParam = { 0 };
        struAlarmParam.dwSize = sizeof(struAlarmParam);
        struAlarmParam.byFaceAlarmDetection = 0;

        _event = NET_DVR_SetupAlarmChan_V41(_session, &struAlarmParam);
        if (_event < 0)
        {
            LOG(ERROR) << __FUNCTION__ << " start face snap for snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError();
            NET_DVR_Logout(_session);
            NET_DVR_Cleanup();
            return false;
        }
        LOG(INFO) << __FUNCTION__ << " start snap camera(" << _ip << ":" << _port << ") success";
        return true;
    }
    return false;
}

bool HCSnapMachine::Stop()
{
    NET_DVR_CloseAlarmChan_V30(_event);
    Uninit();
    LOG(INFO) << __FUNCTION__ << " stop snap camera(" << _ip << ":" << _port << ") success";
    return true;
}

int HCSnapMachine::Init()
{
    NET_DVR_Init();
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);

    NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
    struLoginInfo.bUseAsynLogin = 0;
    strcpy(struLoginInfo.sDeviceAddress, _ip.c_str());
    struLoginInfo.wPort = _port;
    strcpy(struLoginInfo.sUserName, _username.c_str());
    strcpy(struLoginInfo.sPassword, _password.c_str());

    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = { 0 };
    _session = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
    if (_session < 0)
    {
        LOG(WARNING) << __FUNCTION__ << " login snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError();
        NET_DVR_Cleanup();
        return NET_DVR_GetLastError();
    }
    LOG(INFO) << __FUNCTION__ << " login snap camera(" << _ip << ":" << _port << ") success";

    // get device ability
    /*char reqXml[HC_IN_BUFFER_SIZE] = { 0 };
    char abilityXml[HC_OUT_BUFFER_SIZE] = { 0 };
    sprintf(reqXml, "<VcaChanAbility version='2.0'><channelNO>%d</channelNO></VcaChanAbility>", struDeviceInfoV40.struDeviceV30.byStartChan);
    if (!NET_DVR_GetDeviceAbility(_session, DEVICE_ABILITY_INFO, reqXml, (DWORD)strlen(reqXml), abilityXml, HC_OUT_BUFFER_SIZE))
    {
        LOG(error) << "get channel face snap ability of snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError();
        NET_DVR_Logout(_session);
        NET_DVR_Cleanup();
        return SdkApiFailed;
    }

    char* abilityXmlEnd = abilityXml + strlen(abilityXml);
    std::string abilityXmlStr(abilityXml, abilityXmlEnd);
    if (abilityXmlStr.find("<FaceSnap>") == std::string::npos)
    {
        LOG(error) << "snap camera(" << _ip << ":" << _port << ") does not have face snap ability";
        NET_DVR_Logout(_session);
        NET_DVR_Cleanup();
        return SdkApiFailed;
    }*/

    // get device ability
    /*memset(reqXml, 0, HC_IN_BUFFER_SIZE);
    memset(abilityXml, 0, HC_OUT_BUFFER_SIZE);
    sprintf(reqXml, "<EventAbility version='2.0'><channelNO>%d</channelNO></EventAbility>", struDeviceInfoV40.struDeviceV30.byStartChan);
    if (!NET_DVR_GetDeviceAbility(_session, DEVICE_ABILITY_INFO, reqXml, (DWORD)strlen(reqXml), abilityXml, HC_OUT_BUFFER_SIZE))
    {
        LOG(error) << "get channel face detection ability of snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError(); 
        NET_DVR_Logout(_session);
        NET_DVR_Cleanup();
        return SdkApiFailed;
    }

    abilityXmlEnd = abilityXml + strlen(abilityXml);
    abilityXmlStr.swap(std::string(abilityXml, abilityXmlEnd));
    if (abilityXmlStr.find("<FaceDetection>") == std::string::npos)
    {
        LOG(error) << "snap camera(" << _ip << ":" << _port << ") does not have face detection ability";
        NET_DVR_Logout(_session);
        NET_DVR_Cleanup();
        return SdkApiFailed;
    }

    DWORD retLen = 0;
    if (!NET_DVR_GetDVRConfig(_session, NET_DVR_GET_FACESNAPCFG,
        struDeviceInfoV40.struDeviceV30.byStartChan, &_snapcfg, sizeof(_snapcfg), &retLen))
    {
        LOG(warning) << "get face snap configuration of snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError();
    }
    else
    {
        _snapcfg.bySnapInterval = (BYTE)_snapParam.interval;
        _snapcfg.byMatchType = 1;
        if (!NET_DVR_SetDVRConfig(_session, NET_DVR_GET_FACESNAPCFG,
            struDeviceInfoV40.struDeviceV30.byStartChan, &_snapcfg, sizeof(_snapcfg)))
        {
            LOG(warning) << "set face snap configuration of snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError();
        }
    }*/

    if (!NET_DVR_SetDVRMessageCallBack_V31(HCSnapCallback, this))
    {
        LOG(ERROR) << __FUNCTION__ << " set face snap callback for snap camera(" << _ip << ":" << _port << ") failed, error code: " << NET_DVR_GetLastError();
        NET_DVR_Logout(_session);
        NET_DVR_Cleanup();
        return NET_DVR_GetLastError();
    }
    LOG(INFO) << __FUNCTION__ << " initialize snap camera(" << _ip << ":" << _port << ") success";

    int result = SnapMachine::Init();
    if (result)
    {
        LOG(ERROR) << __FUNCTION__ << " create JPEG decoder failed, error code: " << result;
        NET_DVR_Logout(_session);
        NET_DVR_Cleanup();
        return NET_DVR_GetLastError();
    }

    return result;
}

void HCSnapMachine::Uninit()
{
    NET_DVR_Logout(_session);
    NET_DVR_Cleanup();

    SnapMachine::Uninit();
}

ImageMachine::ImageMachine(FaceDetector* faceDetector, const std::string& id, const FaceParam& faceParam)
    : SnapMachine(faceDetector, id, faceParam)
{
}

ImageMachine::~ImageMachine()
{
}

bool ImageMachine::Start()
{
    int res = Init();
    if (res)
    {
        LOG(INFO) << __FUNCTION__ << " (" << _id << ") failed, error reason: " << res;
        return true;
    } 
    else
    {
        LOG(INFO) << __FUNCTION__ << " (" << _id << ") success";
        return true;
    }
}

bool ImageMachine::Stop()
{
    Uninit();
    LOG(INFO) << __FUNCTION__ << " (" << _id << ") success";
    return true;
}
