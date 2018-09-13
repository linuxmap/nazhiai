
#ifndef _SNAPCAMERA_HEADER_H_
#define _SNAPCAMERA_HEADER_H_

#ifdef SNAPMACHINE_EXPORTS
#define SNAPMACHINE_C_API extern "C" __declspec(dllexport)
#define SNAPMACHINE_API extern __declspec(dllexport)
#define SNAPMACHINE_CLASS __declspec(dllexport)
#else
#define SNAPMACHINE_C_API extern "C" __declspec(dllimport)
#define SNAPMACHINE_API extern __declspec(dllimport)
#define SNAPMACHINE_CLASS __declspec(dllimport)
#endif

#include "SnapStruct.h"
#include "FaceDetectStruct.h"

class FaceDetector;
class SnapMachine;

SNAPMACHINE_API SnapMachine* CreateSnapMachine(FaceDetector*, int cameraType, const FaceParam& faceParam, const std::string& ip, unsigned short port, const std::string& username, const std::string& password);
SNAPMACHINE_API SnapMachine* CreateSnapMachine(FaceDetector*, const FaceParam& faceParam, const std::string& id);

SNAPMACHINE_API bool StartSnapMachine(SnapMachine*);
SNAPMACHINE_API bool StopSnapMachine(SnapMachine*);

SNAPMACHINE_API const char* GetSnapMachineId(SnapMachine*);
SNAPMACHINE_API const int GetSnapMachineDeviceIndex(SnapMachine*);

SNAPMACHINE_API bool FeedSnapMachine(SnapMachine*, const ImageParam& imageParam);

SNAPMACHINE_API bool DestroySnapMachine(SnapMachine*);

#endif

