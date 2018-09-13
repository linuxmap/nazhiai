/*!
 * \copyright NISE
 */
#ifdef FACESDK_EXPORTS
	#ifdef _WIN32
		#define FACESDK_API extern "C" __declspec(dllexport)
	#else
		#define FACESDK_API extern "C" __attribute__ ((visibility ("default")))
	#endif
#else
	#define FACESDK_API extern "C"
#endif

#include <string>
#include <vector>

namespace cv
{
	class Mat;

	namespace cuda
	{
		class GpuMat;
	}
}

enum FaceSdkResult {
	FaceSdkOk = 0,
	FaceSdkInternalError = 1000,
	FaceSdkLicenseError = 1200,
	FaceSdkTrialLimitation,
	FaceSdkInterfaceUnauthorized,
	FaceSdkChannelExcess,
	FaceSdkChannelUnready,
	FaceSdkUndefinedError = 1300
};

struct FaceSdkImage;
struct FaceSdkChannel;

/*!
 * \brief 通道参数
 */
struct FaceSdkParam {
	bool enableDetectFace = false;			//!< 启用检测人脸功能。
	bool enableTrack = false;				//!< 启用跟踪功能。
	bool enableEvaluate = false;			//!< 启用评估功能。
	bool enableDetectKeypoint = false;		//!< 启用检测关键点功能。
	bool enableAlign = false;				//!< 启用对齐功能。
	bool enableExtract = false;				//!< 启用提取功能。
	bool enableCompare = false;				//!< 启用比对功能。
	bool enableCluster = false;				//!< 启用聚类功能。
	bool enableAnalyzeLiveness = false;		//!< 启用分析活体功能。
	bool enableAnalyzeAgeGender = false;	//!< 启用分析年龄性别功能。
	bool enableAnalyzeGlasses = false;		//!< 启用分析眼镜功能。
	bool enableAnalyzeMask = false;			//!< 启用分析口罩功能。
	bool enableAnalyzeAgeEthnic = false;	//!< 启用分析年龄民族功能。
	bool enableAnalyzeGGHB = false;			//!< 启用分析性别眼镜帽子胡子功能。
	bool enableAnalyzeMGSH = false;			//!< 启用分析口罩眼镜墨镜帽子功能。
	std::string modelDir = "model/";		//!< 模型目录。
	std::string faceModel = "default";		//!< 人脸模型。default/idphoto。影响：检测人脸。
	int maxShortEdge = 512;					//!< 最大短边尺寸，为0则不缩小。影响：检测人脸。
	float thresholdDF = 0.90f;				//!< 阈值。影响：检测人脸。
	float thresholdT = 0.50f;				//!< 阈值。影响：跟踪。CPU推荐阈值0.50，GPU推荐阈值0.40。
	std::string featureModel = "resnext";	//!< 特征模型。mobilenet/resnext/resnet/shufflenet/dual。影响：提取、比对、聚类。
	float distance = 0.10f;					//!< 距离。影响：聚类。
};

/*!
 * \brief 人脸框
 */
struct FaceSdkBox {
	int number = -1;						//!< 编号。
	int x = 0;								//!< 横坐标。
	int y = 0;								//!< 纵坐标。
	int width = 0;							//!< 宽度。
	int height = 0;							//!< 高度。
	float confidence = 0.0f;				//!< 人脸置信度。0.00~1.00。
	float badness = 0.0f;						//!< 人脸质量劣度。0.00~1.00。
	float keypointsConfidence = 0.0f;		//!< 关键点置信度。0.00~1.00。
	std::vector<float> keypoints = {};		//!< 68个关键点坐标（相对于人脸框）。包含x1、x2……y68。
	std::vector<float> visibles = {};		//!< 68个关键点可见性。0/1。
	std::vector<float> angles = {};			//!< 姿态角。包含俯仰角pitch、偏航角yaw、翻滚角roll。-180~180。
	FaceSdkImage *aligned = nullptr;		//!< 对齐后的人脸数据。
};

FACESDK_API FaceSdkResult GetGpuCount(int &count);
FACESDK_API FaceSdkResult SetGpuForThread(int gpuIndex, int ctxIndex = 0);

FACESDK_API FaceSdkResult CreateImage(FaceSdkImage *&instance, std::vector<char> &image);
FACESDK_API FaceSdkResult CreateImageByMat(FaceSdkImage *&instance, cv::Mat &image);
FACESDK_API FaceSdkResult CreateImageByGpuMat(FaceSdkImage *&instance, cv::cuda::GpuMat &image);
FACESDK_API FaceSdkResult DestroyImage(FaceSdkImage *instance);

FACESDK_API FaceSdkResult GetImage(FaceSdkImage *instance, std::vector<char> &image);
FACESDK_API FaceSdkResult GetImageByMat(FaceSdkImage *instance, cv::Mat &image);

FACESDK_API FaceSdkResult CreateChannel(FaceSdkChannel *&instance, FaceSdkParam &param);
FACESDK_API FaceSdkResult DestroyChannel(FaceSdkChannel *instance);

FACESDK_API FaceSdkResult DetectFaces(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &images, std::vector<std::vector<FaceSdkBox>> &faces, int top);
FACESDK_API FaceSdkResult Track(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &images, std::vector<std::vector<FaceSdkBox>> &faces, int top, std::vector<std::string> &sources);
FACESDK_API FaceSdkResult EvaluateBadness(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &images, std::vector<std::vector<FaceSdkBox>> &faces);
FACESDK_API FaceSdkResult DetectKeypoints(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &images, std::vector<std::vector<FaceSdkBox>> &faces);
FACESDK_API FaceSdkResult Align(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &images, std::vector<std::vector<FaceSdkBox>> &faces);

FACESDK_API FaceSdkResult EvaluateClarity(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<float> &scores);
FACESDK_API FaceSdkResult EvaluateBrightness(FaceSdkChannel *instance, FaceSdkImage *aligned, float &score);

FACESDK_API FaceSdkResult ExtractFeatures(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<char> &features, std::vector<int> &ages, std::vector<float> &genders);

FACESDK_API FaceSdkResult CompareFeatures1v1(FaceSdkChannel *instance, std::vector<char> &featureA, std::vector<char> &featureB, float &score);
FACESDK_API FaceSdkResult AddFeaturesN(FaceSdkChannel *instance, std::vector<char> &featuresN, std::vector<unsigned int> &idVectorN);
FACESDK_API FaceSdkResult CompareToFeaturesN(FaceSdkChannel *instance, std::vector<char> &featuresM, std::vector<float> &scoreMatrix, unsigned int top, std::vector<unsigned int> &idMatrixN);
FACESDK_API FaceSdkResult ClearFeaturesN(FaceSdkChannel *instance);
FACESDK_API FaceSdkResult GetFeaturesNCount(FaceSdkChannel *instance, unsigned int &count);
FACESDK_API FaceSdkResult SelectFeaturesN(FaceSdkChannel *instance, std::vector<unsigned int> &idVectorN, bool enabled, bool keep);
FACESDK_API FaceSdkResult SelectFeaturesNByIndex(FaceSdkChannel *instance, std::vector<unsigned int> &indexVectorN, bool enabled, bool keep);
FACESDK_API FaceSdkResult GetProcessedFeaturesN(FaceSdkChannel *instance, unsigned int index, unsigned int count, std::vector<char> &featuresNP, std::vector<char> &featuresNQ, std::vector<unsigned int> &idVectorN);
FACESDK_API FaceSdkResult AddProcessedFeaturesN(FaceSdkChannel *instance, std::vector<char> &featuresNP, std::vector<char> &featuresNQ, std::vector<unsigned int> &idVectorN);
FACESDK_API FaceSdkResult _SetWeightsOfFeaturesN(FaceSdkChannel *instance, std::vector<unsigned int> &idVectorN, std::vector<float> &weights);

#ifdef _WIN32
FACESDK_API FaceSdkResult Cluster(FaceSdkChannel *instance, std::vector<char> &features, std::vector<int> &labels);
#endif

FACESDK_API FaceSdkResult AnalyzeLiveness(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<float> &livenesss);
FACESDK_API FaceSdkResult AnalyzeAgeGender(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<int> &ages, std::vector<int> &genders);
FACESDK_API FaceSdkResult AnalyzeGlasses(FaceSdkChannel *instance, FaceSdkImage *aligned, bool &wearGlasses);
FACESDK_API FaceSdkResult AnalyzeMask(FaceSdkChannel *instance, FaceSdkImage *aligned, bool &wearMask);
FACESDK_API FaceSdkResult AnalyzeAgeEthnic(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<int> &ageGroups, std::vector<int> &ethnicGroups);
FACESDK_API FaceSdkResult AnalyzeGGHB(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<float> &genders, std::vector<float> &glassess, std::vector<float> &hats, std::vector<float> &beards);
FACESDK_API FaceSdkResult AnalyzeMGSH(FaceSdkChannel *instance, std::vector<FaceSdkImage *> &aligneds, std::vector<float> &masks, std::vector<float> &glassess, std::vector<float> &sunglassess, std::vector<float> &hats);

FACESDK_API FaceSdkResult _GetReservedInfo(std::vector<unsigned char> &info);


