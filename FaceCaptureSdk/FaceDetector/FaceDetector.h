
#ifndef _FACEDETECTOR_HEADER_H
#define _FACEDETECTOR_HEADER_H

#ifdef FACEDETECTOR_EXPORTS
#define FACEDETECTOR_C_API extern "C" __declspec(dllexport)
#define FACEDETECTOR_API extern __declspec(dllexport)
#define FACEDETECTOR_CLASS __declspec(dllexport)
#else
#define FACEDETECTOR_C_API extern "C" __declspec(dllimport)
#define FACEDETECTOR_API extern __declspec(dllimport)
#define FACEDETECTOR_CLASS __declspec(dllimport)
#endif

#include "FaceDetectCore.h"
#include "FaceDetectStruct.h"
#include "FaceCaptureStruct.h"
#include "SnapStruct.h"

class FaceDetector;
class FaceExtractor;
class BaseDecoder;

FACEDETECTOR_API bool DetectInit();
FACEDETECTOR_API void DetectDestroy();

FACEDETECTOR_API const char* GetLastDetectError();

FACEDETECTOR_API FaceDetector* CreateDetector(const ModelParam& modelParam, const DetectParam& detectParam, const TrackParam& trackParam,
    const EvaluateParam& evaluateParam, const KeypointParam& keypointParam, const AlignParam& alignParam,
    const AnalyzeParam& analyzerParam, const ResultParam& resultParam, FaceExtractor*);
FACEDETECTOR_API void DestroyDetector(FaceDetector*);

FACEDETECTOR_API FaceExtractor* CreateExtractor(const ModelParam& modelParam, const ExtractParam& extractParam, const ResultParam& resultParam);
FACEDETECTOR_API void DestroyExtractor(FaceExtractor*);

FACEDETECTOR_API void AddSource(FaceDetector*, BaseDecoder*, const FaceParam& faceParam);
FACEDETECTOR_API void DelSource(FaceDetector*, BaseDecoder*);

FACEDETECTOR_API bool GetCapture(FaceDetector*, std::vector<std::shared_ptr<CaptureResult>>& captureResults);
FACEDETECTOR_API bool GetCapture(FaceExtractor*, std::vector<std::shared_ptr<CaptureResult>>& captureResults);

#endif
