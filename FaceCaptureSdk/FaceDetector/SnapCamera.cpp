#include "SnapCamera.h"
#include "FaceDetectorImpl.h"
#include "SnapMachine.h"

SNAPMACHINE_API SnapMachine* CreateSnapMachine(FaceDetector* faceDetector, int cameraType, const FaceParam& faceParam, const std::string& ip, unsigned short port, const std::string& username, const std::string& password)
{
    return LoginSnapMachine::GenerateLoginSnapMachine(faceDetector, faceParam, cameraType, ip, port, username, password);
}

SNAPMACHINE_API SnapMachine* CreateSnapMachine(FaceDetector* faceDetector, const FaceParam& faceParam, const std::string& id)
{
    return new ImageMachine(faceDetector, id, faceParam);
}

SNAPMACHINE_API bool StartSnapMachine(SnapMachine* snapMachine)
{
    if (snapMachine)
    {
        return snapMachine->Start();
    }
    return false;
}

SNAPMACHINE_API bool StopSnapMachine(SnapMachine* snapMachine)
{
    if (snapMachine)
    {
        return snapMachine->Stop();
    }
    return false;
}

SNAPMACHINE_API const char* GetSnapMachineId(SnapMachine* snapMachine)
{
    if (snapMachine)
    {
        return snapMachine->GetId().c_str();
    }
    return nullptr;
}

SNAPMACHINE_API const int GetSnapMachineDeviceIndex(SnapMachine* snapMachine)
{
    if (snapMachine)
    {
        return snapMachine->GetGpuIndex();
    }
    return -1;
}

SNAPMACHINE_API bool DestroySnapMachine(SnapMachine* snapMachine)
{
    bool result = false;
    if (snapMachine)
    {
        delete snapMachine;
    }
    return result;
}

SNAPMACHINE_API bool FeedSnapMachine(SnapMachine* snapMachine, const ImageParam& imageParam)
{
    if (snapMachine)
    {
        return snapMachine->Snap(imageParam);
    }
    return false;
}


