
#ifndef _MANAGER_HEADER_H_
#define _MANAGER_HEADER_H_

#include "StreamDecoder.h"
#include "FaceDetector.h"
#include "SnapCamera.h"
#include "FaceCaptureStruct.h"
#include "AsyncContext.h"

#include "cJSON.h"

#include <map>
#include <mutex>

class FaceCaptureContext
{
private:
    struct FromParam
    {
        std::string extractName = {};
        ModelParam modelParam = {};
        DetectParam detectParam = {};
        TrackParam trackParam = {};
        EvaluateParam evaluateParam = {};
        KeypointParam keypointParam = {};
        AlignParam alignParam = {};
        AnalyzeParam analyzeParam = {};
        ResultParam resultParam = {};

        FaceDetector* detector = nullptr;
    };

    struct ToParam
    {
        std::string name = {};
        ModelParam modelParam = {};
        ExtractParam extractParam = {};
        ResultParam resultParam = {};

        FaceExtractor* extractor = nullptr;
    };

public:
    static bool ReadConfiguration(const char* path);
    static void ShowConfiguration();

    static FaceDetector* ChooseFaceDetector(int deviceIndex);

    static bool InitializeFaceExtractor(ToParam& toParam);
    static bool InitializeFaceExtractors();
    static void UninitializeFaceExtractors();

    static bool InitializeFaceDetector(FromParam& from, FaceExtractor* extractor);
    static bool InitializeFaceDetectors();
    static void UninitializeFaceDetectors();

    static void AddSnapMachine(const std::string& id, SnapMachine*);
    static bool FindSnapMachine(const std::string& id);
    static bool FeedImageMachine(const ImageParam&);
    static bool DelSnapMachine(const std::string& id);
    static void UninitializeAllSnapMachines();

    static void AddDecoder(const std::string& id, BaseDecoder*);
    static BaseDecoder* FindDecoder(const std::string& id);
    static bool DelDecoder(const std::string& id);
    static void UninitializeAllDecoders();

    static void AddAsyncDecoders(const std::vector<int>& types, const std::vector<std::string>& urls, const std::vector<DecoderParam>& decoderParams, const std::vector<DecodeParam>& decodeParams, const std::vector<FaceParam>& faceParams, const std::vector<std::string>& ids, void(*asyncCallback)(const std::string&, const std::string&));
    static void DelAsyncDecoders(const std::vector<std::string>& ids, void(*asyncCallback)(const std::string&, const std::string&));
    static void UninitializeAllAsyncDecoders();

    static bool GetCaptureResults(std::vector<std::shared_ptr<CaptureResult>>& captureResults);

    static void StreamStoppedCallback(const std::string& id, bool async);

private:
    static bool ReadDetect(cJSON* parent, DetectParam& detectParam);
    static bool ReadTrack(cJSON* parent, TrackParam& trackParam);
    static bool ReadEvaluate(cJSON* parent, EvaluateParam& evaluateParam);
    static bool ReadKeypointe(cJSON* parent, KeypointParam& keypointParam);
    static bool ReadAligne(cJSON* parent, AlignParam& alignParam);
    static bool ReadAnalyze(cJSON* parent, AnalyzeParam& analyzeParam);
    static bool ReadExtract(cJSON* parent, ExtractParam& extractParam);

    static bool ReadCaptures(cJSON* parent, const std::string& modelPath, const std::string& path);
    static bool ReadExtracts(cJSON* parent, const std::string& modelPath, const std::string& path);

private:
    // initialization fields
    static std::vector<FromParam> FromParams;
    static std::vector<ToParam> ToParams;

    // operation fields
    static std::mutex SnapMachinesLocker;
    static std::map<std::string, SnapMachine*> SnapMachines;

    static std::mutex SyncBaseDecodersLocker;
    static std::map<std::string, BaseDecoder*> SyncBaseDecoders;

    static std::mutex AsyncContextsLocker;
    static std::map<int, AsyncContext*> AsyncContexts;

private:
    FaceCaptureContext();
    FaceCaptureContext(const FaceCaptureContext&);
    FaceCaptureContext& operator=(const FaceCaptureContext&);
};

#endif

