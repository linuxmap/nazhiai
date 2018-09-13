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
 * \brief ͨ������
 */
struct FaceSdkParam {
	bool enableDetectFace = false;			//!< ���ü���������ܡ�
	bool enableTrack = false;				//!< ���ø��ٹ��ܡ�
	bool enableEvaluate = false;			//!< �����������ܡ�
	bool enableDetectKeypoint = false;		//!< ���ü��ؼ��㹦�ܡ�
	bool enableAlign = false;				//!< ���ö��빦�ܡ�
	bool enableExtract = false;				//!< ������ȡ���ܡ�
	bool enableCompare = false;				//!< ���ñȶԹ��ܡ�
	bool enableCluster = false;				//!< ���þ��๦�ܡ�
	bool enableAnalyzeLiveness = false;		//!< ���÷������幦�ܡ�
	bool enableAnalyzeAgeGender = false;	//!< ���÷��������Ա��ܡ�
	bool enableAnalyzeGlasses = false;		//!< ���÷����۾����ܡ�
	bool enableAnalyzeMask = false;			//!< ���÷������ֹ��ܡ�
	bool enableAnalyzeAgeEthnic = false;	//!< ���÷����������幦�ܡ�
	bool enableAnalyzeGGHB = false;			//!< ���÷����Ա��۾�ñ�Ӻ��ӹ��ܡ�
	bool enableAnalyzeMGSH = false;			//!< ���÷��������۾�ī��ñ�ӹ��ܡ�
	std::string modelDir = "model/";		//!< ģ��Ŀ¼��
	std::string faceModel = "default";		//!< ����ģ�͡�default/idphoto��Ӱ�죺���������
	int maxShortEdge = 512;					//!< ���̱߳ߴ磬Ϊ0����С��Ӱ�죺���������
	float thresholdDF = 0.90f;				//!< ��ֵ��Ӱ�죺���������
	float thresholdT = 0.50f;				//!< ��ֵ��Ӱ�죺���١�CPU�Ƽ���ֵ0.50��GPU�Ƽ���ֵ0.40��
	std::string featureModel = "resnext";	//!< ����ģ�͡�mobilenet/resnext/resnet/shufflenet/dual��Ӱ�죺��ȡ���ȶԡ����ࡣ
	float distance = 0.10f;					//!< ���롣Ӱ�죺���ࡣ
};

/*!
 * \brief ������
 */
struct FaceSdkBox {
	int number = -1;						//!< ��š�
	int x = 0;								//!< �����ꡣ
	int y = 0;								//!< �����ꡣ
	int width = 0;							//!< ��ȡ�
	int height = 0;							//!< �߶ȡ�
	float confidence = 0.0f;				//!< �������Ŷȡ�0.00~1.00��
	float badness = 0.0f;						//!< ���������Ӷȡ�0.00~1.00��
	float keypointsConfidence = 0.0f;		//!< �ؼ������Ŷȡ�0.00~1.00��
	std::vector<float> keypoints = {};		//!< 68���ؼ������꣨����������򣩡�����x1��x2����y68��
	std::vector<float> visibles = {};		//!< 68���ؼ���ɼ��ԡ�0/1��
	std::vector<float> angles = {};			//!< ��̬�ǡ�����������pitch��ƫ����yaw��������roll��-180~180��
	FaceSdkImage *aligned = nullptr;		//!< �������������ݡ�
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


