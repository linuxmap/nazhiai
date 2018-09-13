#include "FaceCaptureContext.h"
#include "ErrorHandler.h"

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include "opencv2/opencv.hpp"
#pragma warning(default:4819)
#pragma warning(default:4996)

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include <windows.h>

#ifdef _DEBUG
#pragma comment(lib, "opencv_core320d.lib")
#pragma comment(lib, "opencv_imgcodecs320d.lib")
#pragma comment(lib, "opencv_imgproc320d.lib")
#pragma comment(lib, "opencv_highgui320d.lib")
#pragma comment(lib, "opencv_videoio320d.lib")
#else
#pragma comment(lib, "opencv_core320.lib")
#pragma comment(lib, "opencv_imgcodecs320.lib")
#pragma comment(lib, "opencv_imgproc320.lib")
#pragma comment(lib, "opencv_highgui320.lib")
#pragma comment(lib, "opencv_videoio320.lib")
#endif

#pragma comment(lib, "FaceDetector.lib")

#pragma warning(disable:4244)

std::vector<FaceCaptureContext::FromParam> FaceCaptureContext::FromParams;
std::vector<FaceCaptureContext::ToParam> FaceCaptureContext::ToParams;

std::mutex FaceCaptureContext::SnapMachinesLocker;
std::map<std::string, SnapMachine*> FaceCaptureContext::SnapMachines;

std::mutex FaceCaptureContext::SyncBaseDecodersLocker;
std::map<std::string, BaseDecoder*> FaceCaptureContext::SyncBaseDecoders;

std::mutex FaceCaptureContext::AsyncContextsLocker;
std::map<int, AsyncContext*> FaceCaptureContext::AsyncContexts;

bool FaceCaptureContext::ReadDetect(cJSON* parent, DetectParam& detectParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        detectParam.deviceIndex = gpu_index->valueint;
    }
    if (detectParam.deviceIndex < 0)
    {
        detectParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        detectParam.threadCount = thread_count->valueint;
    }

    cJSON* face_model = cJSON_GetObjectItem(parent, "face_model");
    if (face_model && face_model->type == cJSON_String)
    {
        detectParam.faceModel = face_model->valuestring;
    }
    else
    {
        detectParam.faceModel = "default";
    }

    cJSON* face_threshold = cJSON_GetObjectItem(parent, "face_threshold");
    if (face_threshold && face_threshold->type == cJSON_Number)
    {
        detectParam.faceThreshold = face_threshold->valueint;
    }

    cJSON* threshold = cJSON_GetObjectItem(parent, "threshold");
    if (threshold && threshold->type == cJSON_Number)
    {
        detectParam.threshold = threshold->valuedouble;
    }

    cJSON* max_short_edge = cJSON_GetObjectItem(parent, "max_short_edge");
    if (max_short_edge && max_short_edge->type == cJSON_Number)
    {
        detectParam.maxShortEdge = max_short_edge->valueint;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        detectParam.bufferSize = buffer_size->valueint;
    }

    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        detectParam.batchTimeout = batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        detectParam.batchSize = batch_size->valueint;
    }
    return true;
}

bool FaceCaptureContext::ReadTrack(cJSON* parent, TrackParam& trackParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        trackParam.deviceIndex = gpu_index->valueint;
    }
    if (trackParam.deviceIndex < 0)
    {
        trackParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        trackParam.threadCount = thread_count->valueint;
    }

    cJSON* threshold = cJSON_GetObjectItem(parent, "threshold");
    if (threshold && threshold->type == cJSON_Number)
    {
        trackParam.threshold = threshold->valuedouble;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        trackParam.bufferSize = buffer_size->valueint;
    }
    
    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        trackParam.batchTimeout = batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        trackParam.batchSize = batch_size->valueint;
    }
    return true;
}

bool FaceCaptureContext::ReadEvaluate(cJSON* parent, EvaluateParam& evaluateParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        evaluateParam.deviceIndex = gpu_index->valueint;
    }
    if (evaluateParam.deviceIndex < 0)
    {
        evaluateParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        evaluateParam.threadCount = thread_count->valueint;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        evaluateParam.bufferSize = buffer_size->valueint;
    }

    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        evaluateParam.batchTimeout = batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        evaluateParam.batchSize = batch_size->valueint;
    }
    return true;
}

bool FaceCaptureContext::ReadKeypointe(cJSON* parent, KeypointParam& keypointParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        keypointParam.deviceIndex = gpu_index->valueint;
    }
    if (keypointParam.deviceIndex < 0)
    {
        keypointParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        keypointParam.threadCount = thread_count->valueint;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        keypointParam.bufferSize = buffer_size->valueint;
    }

    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        keypointParam.batchTimeout = batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        keypointParam.batchSize = batch_size->valueint;
    }
    return true;
}

bool FaceCaptureContext::ReadAligne(cJSON* parent, AlignParam& alignParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        alignParam.deviceIndex = gpu_index->valueint;
    }
    if (alignParam.deviceIndex < 0)
    {
        alignParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        alignParam.threadCount = thread_count->valueint;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        alignParam.bufferSize = buffer_size->valueint;
    }

    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        alignParam.batchTimeout = batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        alignParam.batchSize = batch_size->valueint;
    }
    return true;
}

bool FaceCaptureContext::ReadAnalyze(cJSON* parent, AnalyzeParam& analyzeParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        analyzeParam.deviceIndex = gpu_index->valueint;
    }
    if (analyzeParam.deviceIndex < 0)
    {
        analyzeParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        analyzeParam.threadCount = thread_count->valueint;
    }

    cJSON* analyze_clarity = cJSON_GetObjectItem(parent, "analyze_clarity");
    if (analyze_clarity)
    {
        analyzeParam.analyze_clarity = analyze_clarity->type == cJSON_True ? true : false;
    }
    cJSON* analyze_brightness = cJSON_GetObjectItem(parent, "analyze_brightness");
    if (analyze_brightness)
    {
        analyzeParam.analyze_brightness = analyze_brightness->type == cJSON_True ? true : false;
    }
    cJSON* analyze_glasses = cJSON_GetObjectItem(parent, "analyze_glasses");
    if (analyze_glasses)
    {
        analyzeParam.analyze_glasses = analyze_glasses->type == cJSON_True ? true : false;
    }
    cJSON* analyze_mask = cJSON_GetObjectItem(parent, "analyze_mask");
    if (analyze_mask)
    {
        analyzeParam.analyze_mask = analyze_mask->type == cJSON_True ? true : false;
    }
    cJSON* analyze_age_gender = cJSON_GetObjectItem(parent, "analyze_age_gender");
    if (analyze_age_gender)
    {
        analyzeParam.analyze_age_gender = analyze_age_gender->type == cJSON_True ? true : false;
    }
    cJSON* analyze_age_ethnic = cJSON_GetObjectItem(parent, "analyze_age_ethnic");
    if (analyze_age_ethnic)
    {
        analyzeParam.analyze_age_ethnic = analyze_age_ethnic->type == cJSON_True ? true : false;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        analyzeParam.bufferSize = buffer_size->valueint;
    }

    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        analyzeParam.batchTimeout = batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        analyzeParam.batchSize = batch_size->valueint;
    }
    return true;
}

bool FaceCaptureContext::ReadExtract(cJSON* parent, ExtractParam& extractParam)
{
    if (!parent)
    {
        return false;
    }

    cJSON* gpu_index = cJSON_GetObjectItem(parent, "device_index");
    if (gpu_index && gpu_index->type == cJSON_Number)
    {
        extractParam.deviceIndex = gpu_index->valueint;
    }

    if (extractParam.deviceIndex < 0)
    {
        extractParam.deviceIndex = -1;
    }

    cJSON* thread_count = cJSON_GetObjectItem(parent, "thread_count");
    if (thread_count && thread_count->type == cJSON_Number)
    {
        extractParam.threadCount = thread_count->valueint;
    }

    cJSON* buffer_size = cJSON_GetObjectItem(parent, "buffer_size");
    if (buffer_size && buffer_size->type == cJSON_Number)
    {
        extractParam.bufferSize = buffer_size->valueint;
    }

    cJSON* batch_timeout = cJSON_GetObjectItem(parent, "batch_timeout");
    if (batch_timeout && batch_timeout->type == cJSON_Number)
    {
        extractParam.batchTimeout = batch_timeout->valueint ? 40 : batch_timeout->valueint;
    }

    cJSON* batch_size = cJSON_GetObjectItem(parent, "batch_size");
    if (batch_size && batch_size->type == cJSON_Number)
    {
        extractParam.batchSize = batch_size->valueint <= 0 ? 1 : batch_size->valueint;
    }

    return true;
}

bool FaceCaptureContext::ReadCaptures(cJSON* parent, const std::string& modelPath, const std::string& path)
{
    bool success = true;
    // read captures
    cJSON *captures = cJSON_GetObjectItem(parent, "captures");
    if (captures)
    {
        if (captures->type == cJSON_Array)
        {
            int arrSize = cJSON_GetArraySize(captures);
            if (arrSize > 0)
            {
                for (int idx = 0; success && idx < arrSize; ++idx)
                {
                    cJSON *capture = cJSON_GetArrayItem(captures, idx);

                    FromParam fromParam;
                    // read model name
                    cJSON *model_name = cJSON_GetObjectItem(capture, "model_name");
                    if (model_name && model_name->type == cJSON_String && strlen(model_name->valuestring) > 0)
                    {
                        fromParam.modelParam.name = model_name->valuestring;
                        fromParam.modelParam.path = modelPath;
                    }
                    else
                    {
                        fromParam.modelParam.name = "dual";
                        fromParam.modelParam.path = modelPath;
                    }

                    // read extract name
                    cJSON *extract_name = cJSON_GetObjectItem(capture, "extract_name");
                    if (extract_name && extract_name->type == cJSON_String && strlen(extract_name->valuestring) > 0)
                    {
                        fromParam.extractName = extract_name->valuestring;
                    }
                    else
                    {
                        success = false;
                        ErrorHandler::ErrorStream << "extract_name configuration not found or is not valid: " << path;
                        ErrorHandler::FlushLastErrorStream();

                        LOG(ERROR) << ErrorHandler::GetLastError();
                        break;
                    }

                    // read result parameter
                    cJSON *result_buffer_size = cJSON_GetObjectItem(capture, "result_buffer_size");
                    if (result_buffer_size && result_buffer_size->type == cJSON_Number)
                    {
                        fromParam.resultParam.bufferSize = result_buffer_size->valueint;
                    }
                    else
                    {
                        fromParam.resultParam.bufferSize = 100;
                    }

                    do
                    {
                        cJSON *detect = cJSON_GetObjectItem(capture, "detect");
                        if (!detect || !ReadDetect(detect, fromParam.detectParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "detect configuration not found or is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        cJSON *track = cJSON_GetObjectItem(capture, "track");
                        if (!track || !ReadTrack(track, fromParam.trackParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "track configuration not found or is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        cJSON *evaluate = cJSON_GetObjectItem(capture, "evaluate");
                        if (!evaluate || !ReadEvaluate(evaluate, fromParam.evaluateParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "evaluate configuration not found or is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        cJSON *keypoint = cJSON_GetObjectItem(capture, "keypoint");
                        if (!keypoint || !ReadKeypointe(keypoint, fromParam.keypointParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "keypoint configuration not found or is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        cJSON *align = cJSON_GetObjectItem(capture, "align");
                        if (!align || !ReadAligne(align, fromParam.alignParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "align configuration not found or is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        cJSON *analyze = cJSON_GetObjectItem(capture, "analyze");
                        if (!analyze || !ReadAnalyze(analyze, fromParam.analyzeParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "analyze configuration not found or is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        FromParams.push_back(fromParam);
                    } while (false);
                }
            }
            else
            {
                success = false;
                ErrorHandler::ErrorStream << "empty captures configuration: " << path;
                ErrorHandler::FlushLastErrorStream();

                LOG(ERROR) << ErrorHandler::GetLastError();
            }
        }
        else
        {
            success = false;
            ErrorHandler::ErrorStream << "captures configuration entity should be array: " << path;
            ErrorHandler::FlushLastErrorStream();

            LOG(ERROR) << ErrorHandler::GetLastError();
        }
    }
    else
    {
        success = false;
        ErrorHandler::ErrorStream << "captures configuration not found: " << path;
        ErrorHandler::FlushLastErrorStream();

        LOG(ERROR) << ErrorHandler::GetLastError();
    }
    return success;
}

bool FaceCaptureContext::ReadExtracts(cJSON* parent, const std::string& modelPath, const std::string& path)
{
    bool success = true;
    // read extracts
    cJSON *extracts = cJSON_GetObjectItem(parent, "extracts");
    if (extracts)
    {
        if (extracts->type == cJSON_Array)
        {
            int arrSize = cJSON_GetArraySize(extracts);
            if (arrSize > 0)
            {
                for (int idx = 0; success && idx < arrSize; ++idx)
                {
                    cJSON *extract = cJSON_GetArrayItem(extracts, idx);

                    ToParam toParam;
                    // read model name
                    cJSON *model_name = cJSON_GetObjectItem(extract, "model_name");
                    if (model_name && model_name->type == cJSON_String && strlen(model_name->valuestring) > 0)
                    {
                        toParam.modelParam.name = model_name->valuestring;
                        toParam.modelParam.path = modelPath;
                    }
                    else
                    {
                        toParam.modelParam.name = "dual";
                        toParam.modelParam.path = modelPath;
                    }

                    // read extract name
                    cJSON *extract_name = cJSON_GetObjectItem(extract, "name");
                    if (extract_name && extract_name->type == cJSON_String && strlen(extract_name->valuestring) > 0)
                    {
                        toParam.name = extract_name->valuestring;
                    }
                    else
                    {
                        success = false;
                        ErrorHandler::ErrorStream << "extract name configuration not found or is not valid: " << path;
                        ErrorHandler::FlushLastErrorStream();

                        LOG(ERROR) << ErrorHandler::GetLastError();
                        break;
                    }

                    // read result parameter
                    cJSON *result_buffer_size = cJSON_GetObjectItem(extract, "result_buffer_size");
                    if (result_buffer_size && result_buffer_size->type == cJSON_Number)
                    {
                        toParam.resultParam.bufferSize = result_buffer_size->valueint;
                    }
                    else
                    {
                        toParam.resultParam.bufferSize = 100;
                    }

                    do
                    {
                        if (!ReadExtract(extract, toParam.extractParam))
                        {
                            success = false;
                            ErrorHandler::ErrorStream << "extract configuration is not valid: " << path;
                            ErrorHandler::FlushLastErrorStream();

                            LOG(ERROR) << ErrorHandler::GetLastError();
                            break;
                        }

                        ToParams.push_back(toParam);
                    } while (false);
                }
            }
            else
            {
                success = false;
                ErrorHandler::ErrorStream << "empty extracts configuration: " << path;
                ErrorHandler::FlushLastErrorStream();

                LOG(ERROR) << ErrorHandler::GetLastError();
            }
        }
        else
        {
            success = false;
            ErrorHandler::ErrorStream << "extracts configuration entity should be array: " << path;
            ErrorHandler::FlushLastErrorStream();

            LOG(ERROR) << ErrorHandler::GetLastError();
        }
    }
    else
    {
        success = false;
        ErrorHandler::ErrorStream << "extracts configuration not found: " << path;
        ErrorHandler::FlushLastErrorStream();

        LOG(ERROR) << ErrorHandler::GetLastError();
    }
    return success;
}

bool FaceCaptureContext::ReadConfiguration(const char* path)
{
    bool success = true;
    char* content = NULL;
    FILE* cfg = fopen(path, "r");
    if (cfg)
    {
        fseek(cfg, 0, SEEK_END);
        long size = ftell(cfg);
        fseek(cfg, 0, SEEK_SET);
        content = (char*)malloc(size + 1);
        fread(content, size, 1, cfg);
        content[size] = 0;
        fclose(cfg);
    }
    else
    {
        success = false;
        ErrorHandler::SetLastError("Open configuration file failed");
        LOG(ERROR) << "Open configuration file failed: " << path;
    }

    if (success)
    {
        cJSON *jroot = cJSON_Parse(content);
        if (jroot)
        {
            // read model path
            cJSON *model_path = cJSON_GetObjectItem(jroot, "model_path");
            if (model_path && model_path->type == cJSON_String && strlen(model_path->valuestring) > 0)
            {
                success = ReadCaptures(jroot, model_path->valuestring, path) && ReadExtracts(jroot, model_path->valuestring, path);
            }
            else
            {
                success = false;
                ErrorHandler::ErrorStream << "model_path configuration not found or is not valid: " << path;
                ErrorHandler::FlushLastErrorStream();

                LOG(ERROR) << ErrorHandler::GetLastError();
            }

            cJSON_Delete(jroot);
        }
        else
        {
            success = false;
            ErrorHandler::ErrorStream << "invalid format of configuration file: " << path;
            ErrorHandler::FlushLastErrorStream();

            LOG(ERROR) << ErrorHandler::GetLastError();
        }
    }

    if (content)
    {
        free(content);
        content = NULL;
    }

    return success;
}

bool FaceCaptureContext::InitializeFaceExtractor(ToParam& toParam)
{
    if (toParam.extractParam.threadCount > 0)
    {
        FaceExtractor* faceExtractor = CreateExtractor(toParam.modelParam, toParam.extractParam, toParam.resultParam);
        if (faceExtractor)
        {
            toParam.extractor = faceExtractor;
        }
        else
        {
            ErrorHandler::ErrorStream << "create face extractor failed, reason: " << GetLastDetectError();
            ErrorHandler::FlushLastErrorStream();

            LOG(ERROR) << ErrorHandler::GetLastError();
            return false;
        }
    }
    return true;
}

bool FaceCaptureContext::InitializeFaceExtractors()
{
    bool success = true;
    for (size_t idx = 0; idx < ToParams.size() && success; ++idx)
    {
        ToParam& to = ToParams[idx];
        bool found = false;
        for each (FromParam from in FromParams)
        {
            if (from.extractName == to.name)
            {
                if (from.modelParam.name == to.modelParam.name)
                {
                    found = true;
                    break;
                }
                else
                {
                    LOG(ERROR) << "initialize extract instance failed: model name is not the same between capture and extract[" << to.name << "]";
                }
            }
        }

        if (found)
        {
            success = InitializeFaceExtractor(to);
        }
    }
    return success;
}

void FaceCaptureContext::UninitializeFaceExtractors()
{
    for each (ToParam to in ToParams)
    {
        if (to.extractor)
        {
            DestroyExtractor(to.extractor);
            to.extractor = nullptr;
        }
    }
    ToParams.clear();
}

bool FaceCaptureContext::InitializeFaceDetector(FromParam& from, FaceExtractor* extractor)
{
    FaceDetector* faceDetector = CreateDetector(from.modelParam, from.detectParam, from.trackParam, from.evaluateParam,
        from.keypointParam, from.alignParam, from.analyzeParam, from.resultParam, extractor);
    if (faceDetector)
    {
        from.detector = faceDetector;
        return true;
    }
    else
    {
        ErrorHandler::ErrorStream << "create face detector failed, reason: " << GetLastDetectError();
        ErrorHandler::FlushLastErrorStream();
        LOG(ERROR) << ErrorHandler::GetLastError();
        return false;
    }
}

bool FaceCaptureContext::InitializeFaceDetectors()
{
    bool success = true;
    for (size_t idx = 0; idx < FromParams.size() && success; ++idx)
    {
        FromParam& from = FromParams[idx];
        FaceExtractor* extractor = nullptr;
        for each (ToParam to in ToParams)
        {
            if (to.name == from.extractName)
            {
                extractor = to.extractor;
                break;
            }
        }
        success = InitializeFaceDetector(from, extractor);
    }
    return success;
}

void FaceCaptureContext::UninitializeFaceDetectors()
{
    for (size_t idx = 0; idx < FromParams.size(); ++idx)
    {
        FromParam& from = FromParams[idx];
        if (from.detector)
        {
            DestroyDetector(from.detector);
            from.detector = nullptr;
        }
    }
    FromParams.clear();
}

FaceDetector* FaceCaptureContext::ChooseFaceDetector(int deviceIndex)
{
    for each (FromParam from in FromParams)
    {
        int largestIndex = -1;
        if (from.detectParam.deviceIndex > largestIndex && from.detectParam.threadCount > 0)
        {
            largestIndex = from.detectParam.deviceIndex;
        }
        else if (from.trackParam.deviceIndex > largestIndex && from.trackParam.threadCount > 0)
        {
            largestIndex = from.trackParam.deviceIndex;
        }
        else if (from.evaluateParam.deviceIndex > largestIndex && from.evaluateParam.threadCount > 0)
        {
            largestIndex = from.evaluateParam.deviceIndex;
        }
        else if (from.keypointParam.deviceIndex > largestIndex && from.keypointParam.threadCount > 0)
        {
            largestIndex = from.keypointParam.deviceIndex;
        }
        else if (from.alignParam.deviceIndex > largestIndex && from.alignParam.threadCount > 0)
        {
            largestIndex = from.alignParam.deviceIndex;
        }
        else if (from.analyzeParam.deviceIndex > largestIndex && from.analyzeParam.threadCount > 0)
        {
            largestIndex = from.analyzeParam.deviceIndex;
        }

        if (deviceIndex == largestIndex)
        {
            return from.detector;
        }
    }
    return nullptr;
}

void FaceCaptureContext::AddSnapMachine(const std::string& id, SnapMachine* snapMachine)
{
    std::lock_guard<std::mutex> lg(SnapMachinesLocker);
    SnapMachines.insert(std::make_pair(id, snapMachine));
}

bool FaceCaptureContext::FindSnapMachine(const std::string& id)
{
    std::lock_guard<std::mutex> lg(SnapMachinesLocker);
    return SnapMachines.find(id) != SnapMachines.end();
}

bool FaceCaptureContext::FeedImageMachine(const ImageParam& imageParam)
{
    std::lock_guard<std::mutex> lg(SnapMachinesLocker);
    std::map<std::string, SnapMachine*>::iterator it = SnapMachines.find(imageParam.sourceId);
    if (it != SnapMachines.end())
    {
        return FeedSnapMachine(it->second, imageParam);
    }
    return false;
}

bool FaceCaptureContext::DelSnapMachine(const std::string& id)
{
    std::lock_guard<std::mutex> lg(SnapMachinesLocker);
    std::map<std::string, SnapMachine*>::iterator it = SnapMachines.find(id);
    if (it != SnapMachines.end())
    {
        StopSnapMachine(it->second);
        DestroySnapMachine(it->second);
        SnapMachines.erase(it);
        return true;
    }
    return false;
}

void FaceCaptureContext::UninitializeAllSnapMachines()
{
    std::lock_guard<std::mutex> lg(SnapMachinesLocker);
    for (std::map<std::string, SnapMachine*>::iterator it = SnapMachines.begin(); it != SnapMachines.end(); ++it)
    {
        StopSnapMachine(it->second);
        DestroySnapMachine(it->second);
    }
    SnapMachines.clear();
}

void FaceCaptureContext::AddDecoder(const std::string& id, BaseDecoder* baseDecoder)
{
    std::lock_guard<std::mutex> lg(SyncBaseDecodersLocker);
    SyncBaseDecoders.insert(std::make_pair(id, baseDecoder));
}

BaseDecoder* FaceCaptureContext::FindDecoder(const std::string& id)
{
    std::lock_guard<std::mutex> lg(SyncBaseDecodersLocker);
    std::map<std::string, BaseDecoder *>::iterator it = SyncBaseDecoders.find(id);
    if (SyncBaseDecoders.end() != it)
    {
        return it->second;
    }
    return nullptr;
}

bool FaceCaptureContext::DelDecoder(const std::string& id)
{
    std::lock_guard<std::mutex> lg(SyncBaseDecodersLocker);
    std::map<std::string, BaseDecoder*>::iterator it = SyncBaseDecoders.find(id);
    if (it != SyncBaseDecoders.end())
    {
        // close decoder
        CloseDecoder(it->second);
        SyncBaseDecoders.erase(it);
        return true;
    }
    return false;
}

void FaceCaptureContext::UninitializeAllDecoders()
{
    std::lock_guard<std::mutex> lg(SyncBaseDecodersLocker);
    for (std::map<std::string, BaseDecoder*>::iterator it = SyncBaseDecoders.begin(); it != SyncBaseDecoders.end(); ++it)
    {
        int deviceIndex = GetDecoderDeviceIndex(it->second);
        FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(deviceIndex);
        if (faceDetector)
        {
            // delete from face detector
            DelSource(faceDetector, it->second);
        }
        else
        {
            LOG(WARNING) << "can not find face detector instance which work on device[" << deviceIndex << "]";
        }

        // close decoder
        CloseDecoder(it->second);
    }

    SyncBaseDecoders.clear();
}

void FaceCaptureContext::AddAsyncDecoders(const std::vector<int>& types, const std::vector<std::string>& urls, const std::vector<DecoderParam>& decoderParams, const std::vector<DecodeParam>& decodeParams, const std::vector<FaceParam>& faceParams, const std::vector<std::string>& ids, void(*asyncCallback)(const std::string&, const std::string&))
{
    for (size_t paramIndex = 0; paramIndex < decoderParams.size(); ++paramIndex)
    {
        int deviceIndex = decoderParams[paramIndex].device_index;
        FaceDetector* faceDetector = ChooseFaceDetector(deviceIndex);
        if (faceDetector)
        {
            AsyncContext* asyncContext = nullptr;
            {
                std::lock_guard<std::mutex> lg(AsyncContextsLocker);
                std::map<int, AsyncContext *>::iterator it = AsyncContexts.find(deviceIndex);
                if (it == AsyncContexts.end())
                {
                    asyncContext = new AsyncContext(deviceIndex, faceDetector);
                    asyncContext->Create();
                    AsyncContexts.insert(std::make_pair(deviceIndex, asyncContext));
                }
                else
                {
                    asyncContext = it->second;
                }
            }

            if (asyncContext)
            {
                asyncContext->AddAsyncDecoder(types[paramIndex],
                    urls[paramIndex],
                    decoderParams[paramIndex],
                    decodeParams[paramIndex],
                    ids[paramIndex],
                    faceParams[paramIndex],
                    asyncCallback,
                    StreamStoppedCallback);
            }
            else
            {
                if (asyncCallback)
                {
                    asyncCallback(ids[paramIndex], AsyncContext::FailedReason("async context create failed"));
                }
            }
        }
        else
        {
            if (asyncCallback)
            {
                std::string err = "can not find face detector instance which work on device[";
                err += std::to_string(deviceIndex);
                err += "]";

                asyncCallback(ids[paramIndex], AsyncContext::FailedReason(err));
            }
        }
    }
}

void FaceCaptureContext::DelAsyncDecoders(const std::vector<std::string>& ids, void(*asyncCallback)(const std::string&, const std::string&))
{
    for (size_t paramIndex = 0; paramIndex < ids.size(); ++paramIndex)
    {
        const std::string& id = ids[paramIndex];
        AsyncContext* found = nullptr;
        {
            std::lock_guard<std::mutex> lg(AsyncContextsLocker);
            for (std::map<int, AsyncContext *>::iterator it = AsyncContexts.begin(); it != AsyncContexts.end(); ++it)
            {
                AsyncContext* asyncContext = it->second;
                if (asyncContext && asyncContext->FindAsyncDecoder(id))
                {
                    found = asyncContext;
                    break;
                }
            }
        }

        if (found)
        {
            found->DelAsyncDecoder(id, asyncCallback);
        }
        else
        {
            if (asyncCallback)
            {
                asyncCallback(id, AsyncContext::FailedReason("decoder can not be found in all async contexts"));
            }
        }
    }
}

void FaceCaptureContext::UninitializeAllAsyncDecoders()
{
    std::lock_guard<std::mutex> lg(AsyncContextsLocker);
    for (std::map<int, AsyncContext *>::iterator it = AsyncContexts.begin(); it != AsyncContexts.end(); ++it)
    {
        AsyncContext* asyncContext = it->second;
        if (asyncContext)
        {
            asyncContext->Destroy();
            delete asyncContext;
        }
    }
    AsyncContexts.clear();
}

bool FaceCaptureContext::GetCaptureResults(std::vector<std::shared_ptr<CaptureResult>>& captureResults)
{
    for (size_t idxTo = 0; idxTo < ToParams.size(); ++idxTo)
    {
        GetCapture(ToParams[idxTo].extractor, captureResults);
    }
    for (size_t idxFrom = 0; idxFrom < FromParams.size(); ++idxFrom)
    {
        GetCapture(FromParams[idxFrom].detector, captureResults);
    }
    
    return captureResults.size() > 0;
}

void FaceCaptureContext::StreamStoppedCallback(const std::string& id, bool async)
{
    if (async)
    {
        FaceCaptureContext::DelAsyncDecoders(std::vector<std::string>{id}, nullptr);
    } 
    else
    {
        BaseDecoder* decoder = FaceCaptureContext::FindDecoder(id);
        if (decoder)
        {
            int deviceIndex = GetDecoderDeviceIndex(decoder);
            FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(deviceIndex);
            if (faceDetector)
            {
                // delete from face detector
                DelSource(faceDetector, decoder);
            }
            else
            {
                LOG(WARNING) << "can not find face detector instance which work on device[" << deviceIndex << "] for id: " << id;
            }

            DelDecoder(id);
        }
    }
}

void FaceCaptureContext::ShowConfiguration()
{
    LOG(INFO) << "======================== face capture configuration ========================";
    for (size_t idx = 0; idx < ToParams.size(); ++idx)
    {
        ModelParam& modelParam = ToParams[idx].modelParam;
        ResultParam& resultParam = ToParams[idx].resultParam;
        LOG(INFO) << "---------------------------- extract instance[" << idx + 1 << "] ---------------------------";
        LOG(INFO) << "-- model_path         : " << modelParam.path;
        LOG(INFO) << "-- model_name         : " << modelParam.name;
        LOG(INFO) << "-- result_buffer_size : " << resultParam.bufferSize;

        ExtractParam& extractParam = ToParams[idx].extractParam;
        LOG(INFO) << "-- device_index       : " << extractParam.deviceIndex;
        LOG(INFO) << "-- thread_count       : " << extractParam.threadCount;
        LOG(INFO) << "-- buffer_size        : " << extractParam.bufferSize;
        LOG(INFO) << "-- batch_timeout      : " << extractParam.batchTimeout;
        LOG(INFO) << "-- batch_size         : " << extractParam.batchSize;
    }
    for (size_t idx = 0; idx < FromParams.size(); ++idx)
    {
        ModelParam& modelParam = FromParams[idx].modelParam;
        ResultParam& resultParam = FromParams[idx].resultParam;
        LOG(INFO) << "---------------------------- capture instance[" << idx + 1 << "] ---------------------------";
        LOG(INFO) << "-- model_path         : " << modelParam.path;
        LOG(INFO) << "-- model_name         : " << modelParam.name;
        LOG(INFO) << "-- result_buffer_size : " << resultParam.bufferSize;

        DetectParam& detectParam = FromParams[idx].detectParam;
        LOG(INFO) << "------------------- detect --------------------";
        LOG(INFO) << "-- device_index   : " << detectParam.deviceIndex;
        LOG(INFO) << "-- thread_count   : " << detectParam.threadCount;
        LOG(INFO) << "-- face_model     : " << detectParam.faceModel;
        LOG(INFO) << "-- face_threshold : " << detectParam.faceThreshold;
        LOG(INFO) << "-- threshold      : " << detectParam.threshold;
        LOG(INFO) << "-- max_short_edge : " << detectParam.maxShortEdge;
        LOG(INFO) << "-- buffer_size    : " << detectParam.bufferSize;
        LOG(INFO) << "-- batch_timeout  : " << detectParam.batchTimeout;
        LOG(INFO) << "-- batch_size     : " << detectParam.batchSize;

        TrackParam& trackParam = FromParams[idx].trackParam;
        LOG(INFO) << "------------------- track --------------------";
        LOG(INFO) << "-- device_index : " << trackParam.deviceIndex;
        LOG(INFO) << "-- thread_count : " << trackParam.threadCount;
        LOG(INFO) << "-- threshold    : " << trackParam.threshold;
        LOG(INFO) << "-- buffer_size  : " << trackParam.bufferSize;
        LOG(INFO) << "-- batch_timeout: " << trackParam.batchTimeout;
        LOG(INFO) << "-- batch_size   : " << trackParam.batchSize;

        EvaluateParam& evaluateParam = FromParams[idx].evaluateParam;
        LOG(INFO) << "------------------- evaluate --------------------";
        LOG(INFO) << "-- device_index : " << evaluateParam.deviceIndex;
        LOG(INFO) << "-- thread_count : " << evaluateParam.threadCount;
        LOG(INFO) << "-- buffer_size  : " << evaluateParam.bufferSize;
        LOG(INFO) << "-- batch_timeout: " << evaluateParam.batchTimeout;
        LOG(INFO) << "-- batch_size   : " << evaluateParam.batchSize;

        KeypointParam& keypointParam = FromParams[idx].keypointParam;
        LOG(INFO) << "------------------- keypoint --------------------";
        LOG(INFO) << "-- device_index : " << keypointParam.deviceIndex;
        LOG(INFO) << "-- thread_count : " << keypointParam.threadCount;
        LOG(INFO) << "-- buffer_size  : " << keypointParam.bufferSize;
        LOG(INFO) << "-- batch_timeout: " << keypointParam.batchTimeout;
        LOG(INFO) << "-- batch_size   : " << keypointParam.batchSize;

        AlignParam& alignParam = FromParams[idx].alignParam;
        LOG(INFO) << "------------------- align --------------------";
        LOG(INFO) << "-- device_index : " << alignParam.deviceIndex;
        LOG(INFO) << "-- thread_count : " << alignParam.threadCount;
        LOG(INFO) << "-- buffer_size  : " << alignParam.bufferSize;
        LOG(INFO) << "-- batch_timeout: " << alignParam.batchTimeout;
        LOG(INFO) << "-- batch_size   : " << alignParam.batchSize;

        AnalyzeParam& analyzeParam = FromParams[idx].analyzeParam;
        LOG(INFO) << "------------------- analyze --------------------";
        LOG(INFO) << "-- device_index       : " << analyzeParam.deviceIndex;
        LOG(INFO) << "-- thread_count       : " << analyzeParam.threadCount;

        LOG(INFO) << "-- analyze_clarity    : " << analyzeParam.analyze_clarity;
        LOG(INFO) << "-- analyze_brightness : " << analyzeParam.analyze_brightness;
        LOG(INFO) << "-- analyze_glasses    : " << analyzeParam.analyze_glasses;
        LOG(INFO) << "-- analyze_mask       : " << analyzeParam.analyze_mask;
        LOG(INFO) << "-- analyze_age_gender : " << analyzeParam.analyze_age_gender;
        LOG(INFO) << "-- analyze_age_ethnic : " << analyzeParam.analyze_age_ethnic;

        LOG(INFO) << "-- buffer_size        : " << analyzeParam.bufferSize;
        LOG(INFO) << "-- batch_timeout      : " << analyzeParam.batchTimeout;
        LOG(INFO) << "-- batch_size         : " << analyzeParam.batchSize;
    }
}


