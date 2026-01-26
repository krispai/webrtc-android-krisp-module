#include "krisp_processor.hpp"

#include <cstdio>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>

#include <syslog.h>
#include "krisp-audio-api-definitions-c.h"
#include "krisp-audio-sdk-nc-c.h"
#include "rtc_base/time_utils.h"

#include "krisp_sdk.h"


namespace Krisp
{

static void logCallback(const char* message, KrispLogLevel level)
{
    syslog(LOG_INFO, "KrispProcessor::logCallback: %s", message);
    switch (level) {
        case LogLevelTrace:
            syslog(LOG_DEBUG, "KrispProcessor::logCallback: %s", message);
            break;
        case LogLevelDebug:
            syslog(LOG_DEBUG, "KrispProcessor::logCallback: %s", message);
            break;
        case LogLevelInfo:
            syslog(LOG_INFO, "KrispProcessor::logCallback: %s", message);
            break;
        case LogLevelWarn:
            syslog(LOG_WARNING, "KrispProcessor::logCallback: %s", message);
            break;
        case LogLevelErr:
            syslog(LOG_ERR, "KrispProcessor::logCallback: %s", message);
            break;
        case LogLevelCritical:
            syslog(LOG_CRIT, "KrispProcessor::logCallback: %s", message);
            break;
        case LogLevelOff:
            break;
    }
}

bool LoadKrisp(const char* krispDllPath)
{
    if (!KrispSDK::LoadDll(krispDllPath))
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Unable to load Krisp DLL");
        return false;
    }

    if (!KrispSDK::GlobalInit(nullptr, logCallback, KrispLogLevel::LogLevelTrace))
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to initialize Krisp globals");
        return false;
	}
    return true;
}

bool UnloadKrisp() 
{
    if (KrispSDK::GlobalDestroy() != KrispRetValSuccess)
    {
        syslog(LOG_ERR, "KrispProcessor::Unload: Failed to destroy Krisp globals");
        return false;
    }
    KrispSDK::UnloadDll();
    return true;
}

static KrispSamplingRate GetSampleRate(size_t sampleRate)
{
    switch (sampleRate)
	{
    case 8000:
        return KrispSamplingRate::Sr8000Hz;
    case 16000:
        return KrispSamplingRate::Sr16000Hz;
    case 24000:
        return KrispSamplingRate::Sr24000Hz;
    case 32000:
        return KrispSamplingRate::Sr32000Hz;
    case 44100:
        return KrispSamplingRate::Sr44100Hz;
    case 48000:
        return KrispSamplingRate::Sr48000Hz;
    case 88200:
        return KrispSamplingRate::Sr88200Hz;
    case 96000:
        return KrispSamplingRate::Sr96000Hz;
    default:
		syslog(LOG_INFO, "KrispProcessor::GetSampleRate: The input sampling rate: %zu \
             is not supported. Using default 48khz.", sampleRate);
        return KrispSamplingRate::Sr48000Hz;
    }
}

static bool IsModelSet(const KrispModelInfo& modelInfo)
{
    const bool hasPath = modelInfo.path != nullptr && modelInfo.path[0] != L'\0';
    const bool hasBlob = modelInfo.blob.data != nullptr && modelInfo.blob.size > 0;
    return hasPath || hasBlob;
}

static bool ValidateModelPath(const char* modelPath)
{
    if (!modelPath || modelPath[0] == '\0') {
        syslog(LOG_ERR, "KrispProcessor::Init: model path is empty");
        return false;
    }

    syslog(LOG_INFO, "KrispProcessor::Init: model path: %s", modelPath);

    struct stat st;
    if (stat(modelPath, &st) != 0) {
        syslog(LOG_ERR, "KrispProcessor::Init: stat failed for %s: %s",
               modelPath, strerror(errno));
        return false;
    }

    if (st.st_size <= 0) {
        syslog(LOG_ERR, "KrispProcessor::Init: model file is empty: %s", modelPath);
        return false;
    }

    FILE* file = std::fopen(modelPath, "rb");
    if (!file) {
        syslog(LOG_ERR, "KrispProcessor::Init: fopen failed for %s: %s",
               modelPath, strerror(errno));
        return false;
    }
    unsigned char byte = 0;
    size_t read = std::fread(&byte, 1, 1, file);
    std::fclose(file);
    if (read != 1) {
        syslog(LOG_ERR, "KrispProcessor::Init: fread failed for %s: %s",
               modelPath, strerror(errno));
        return false;
    }

    syslog(LOG_INFO, "KrispProcessor::Init: model file size: %lld bytes",
           static_cast<long long>(st.st_size));
    return true;
}

inline std::wstring convertMBString2WString(const std::string& str)
{
  std::wstring w(str.begin(), str.end());
  return w;
}

KrispNoiseFilter::KrispNoiseFilter() :
    m_isEnabled(false),
	m_numberOfChannels(1),
    m_lastTimeStamp(0),
    m_modelPath(),
    m_modelData(),
	m_bufferIn(),
	m_bufferOut()
{
    m_modelInfo.path = L"";
    m_modelInfo.blob.data = nullptr;
    m_modelInfo.blob.size = 0;
    m_sessionConfig.enableSessionStats = false;
    m_sessionConfig.inputSampleRate = KrispSamplingRate::Sr16000Hz;
    m_sessionConfig.inputFrameDuration = KrispFrameDuration::Fd10ms;
    m_sessionConfig.outputSampleRate = KrispSamplingRate::Sr16000Hz;
    m_sessionConfig.modelInfo = &m_modelInfo;
    m_sessionConfig.ringtoneCfg = nullptr;
    m_ncCachedHandle = 0;
}

KrispNoiseFilter::~KrispNoiseFilter()
{
    syslog(LOG_INFO,"KrispProcessor::~KrispProcessor()");
    DeInit();
}

void KrispNoiseFilter::DeInit() {
    if (m_ncCachedHandle) 
    {
        KrispSDK::DestroyNcFloat(m_ncCachedHandle);
        m_ncCachedHandle = 0;
    }
    m_modelPath.clear();
    m_modelPath.shrink_to_fit();
    m_modelData.clear();
    m_modelData.shrink_to_fit();
    m_modelInfo.path = nullptr;
    m_modelInfo.blob.data = nullptr;
    m_modelInfo.blob.size = 0;
}

bool KrispNoiseFilter::Init(const char* modelPath)
{
    if (!ValidateModelPath(modelPath)) {
        return false;
    }
    m_modelPath = convertMBString2WString(modelPath);
    m_modelInfo.path = m_modelPath.c_str();
    m_modelInfo.blob.data = nullptr;
    m_modelInfo.blob.size = 0;
    m_modelData.clear();
    if (m_ncCachedHandle)
    {
        KrispSDK::DestroyNcFloat(m_ncCachedHandle);
        m_ncCachedHandle = 0;
    }
    m_ncCachedHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
    if (m_ncCachedHandle == 0)
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to create Krisp NC session");
        return false;
    }
    return true;
}

bool KrispNoiseFilter::Init(const void* modelAddr, unsigned int modelSize)
{
    m_modelData.resize(modelSize);
    std::memcpy(m_modelData.data(), modelAddr, modelSize);
    m_modelInfo.path = L"";
    m_modelInfo.blob.data = m_modelData.data();
    m_modelInfo.blob.size = modelSize;

    m_ncCachedHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
    if (m_ncCachedHandle == 0)
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to create Krisp NC session");
        return false;
    }
	return true;
}

void KrispNoiseFilter::Enable(bool isEnable)
{
	m_isEnabled.store(isEnable, std::memory_order_release);
}

bool KrispNoiseFilter::IsEnabled() const
{
	return m_isEnabled.load(std::memory_order_acquire);
}

void KrispNoiseFilter::InitializeSession(int sampleRate, int numberOfChannels)
{
	syslog(LOG_INFO, "KrispProcessor::Initialize: sampleRate: %i\
        numberOfChannels: %i", sampleRate, numberOfChannels);

    m_numberOfChannels = numberOfChannels;
    m_sessionConfig.inputSampleRate = GetSampleRate(sampleRate);
    m_sessionConfig.outputSampleRate = m_sessionConfig.inputSampleRate;

    if (!IsModelSet(m_modelInfo)) {
        syslog(LOG_INFO, "KrispProcessor::Initialize: model not loaded yet");
        return;
    }

    krispNcHandle newNcHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
    if (newNcHandle == 0)
    {
        syslog(LOG_ERR, "KrispProcessor::Initialize: Failed to create Krisp NC session");
        return;
    }
    if (m_ncCachedHandle && KrispSDK::DestroyNcFloat(m_ncCachedHandle) != KrispRetValSuccess) {
        syslog(LOG_ERR, "KrispProcessor::Initialize: Failed to destroy Krisp NC session");
        // TODO: handle memory leak
    }
    m_ncCachedHandle = newNcHandle;
}

void KrispNoiseFilter::ProcessFrame(webrtc::AudioBuffer* audioBuffer)
{

	if(!KrispNoiseFilter::IsEnabled())
    {
        syslog(LOG_DEBUG, "KrispProcessor::Process: Bypassing NoiseSuppressor::Process");
	    return;
	}

    auto now = rtc::TimeMillis();
    if (now - m_lastTimeStamp > 10000)
    {
        syslog(LOG_INFO,"KrispProcessor::Process: Num Frames: %zu\
             num Bands: %zu  Num Channels: %zu ", audioBuffer->num_frames(),
             audioBuffer->num_bands(), audioBuffer->num_channels());
      m_lastTimeStamp = now;
    }

    int audioBufferSampleRate = audioBuffer->num_frames() * 100;
	if(audioBufferSampleRate != static_cast<int>(m_sessionConfig.inputSampleRate))
    {
        m_sessionConfig.inputSampleRate = GetSampleRate(audioBufferSampleRate);
        m_sessionConfig.outputSampleRate = m_sessionConfig.inputSampleRate;
        krispNcHandle newNcHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
        if (newNcHandle == 0)
        {
            syslog(LOG_ERR, "KrispProcessor::Process: Failed to create Krisp NC session");
            return;
        }
        if (m_ncCachedHandle)
        {
            if (KrispSDK::DestroyNcFloat(m_ncCachedHandle) != KrispRetValSuccess) {
                syslog(LOG_ERR, "KrispProcessor::Process: Failed to destroy Krisp NC session");
                // TODO: handle memory leak
            }
        }
        m_ncCachedHandle = newNcHandle;
	}

    if (!m_ncCachedHandle) {
        syslog(LOG_DEBUG, "KrispProcessor::Process: Krisp session is not initialized");
        return;
    }

    size_t bufferSize = audioBuffer->num_frames();
    if (m_bufferIn.size() != bufferSize) {
        m_bufferIn.resize(bufferSize);
    }
    if (m_bufferOut.size() != bufferSize) {
        m_bufferOut.resize(bufferSize);
    }

    for (size_t i = 0; i < bufferSize; ++i)
    {
        m_bufferIn[i] = audioBuffer->channels()[0][i] / 32768.f;
    }

    auto returnCode = KrispSDK::ProcessNcFloat(
        m_ncCachedHandle,
        m_bufferIn.data(), bufferSize,
        m_bufferOut.data(), bufferSize, 100.0f, nullptr);

    if (returnCode != KrispRetValSuccess)
    {
        syslog(LOG_INFO, "KrispProcessor::Process: Krisp noise cleanup error");
        return;
    }

    for (size_t i = 0; i < bufferSize; ++i)
    {
        audioBuffer->channels()[0][i] = m_bufferOut[i] * 32768.f;
    }
}


KrispAdapter::KrispAdapter(const std::shared_ptr<KrispNoiseFilter>& krispProcessor) :
    m_krispProcessor(krispProcessor)
{
}

void KrispAdapter::Initialize(int sampleRate, int numOfChannels)
{
    m_krispProcessor->InitializeSession(sampleRate, numOfChannels);
}

void KrispAdapter::Process(webrtc::AudioBuffer* audioBuffer)
{
    m_krispProcessor->ProcessFrame(audioBuffer);
}

std::string KrispAdapter::ToString() const
{
    return "KrispAudioProcessor";
}

void KrispAdapter::SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting)
{
    if (setting.type() ==
        webrtc::AudioProcessing::RuntimeSetting::Type::kCaptureOutputUsed)
    {
        bool enable = false;
        setting.GetBool(&enable);
        m_krispProcessor->Enable(enable);
    }
}

static std::unique_ptr<NativeKrispModule> BuildModule(
    const std::shared_ptr<KrispNoiseFilter>& proc)
{
    auto m = std::make_unique<NativeKrispModule>();
    m->proc = proc;
    m->apm = webrtc::AudioProcessingBuilder()
        .SetCapturePostProcessing(std::make_unique<KrispAdapter>(m->proc))
        .Create();
    webrtc::AudioProcessing::Config config;
    config.echo_canceller.enabled = false;
    config.echo_canceller.mobile_mode = true;
    m->apm->ApplyConfig(config);
    return m;
}

std::unique_ptr<NativeKrispModule> NativeKrispModule::Create()
{
    return BuildModule(std::make_shared<KrispNoiseFilter>());
}

std::unique_ptr<NativeKrispModule> NativeKrispModule::CreateWithModelPath(
    const char* modelPath)
{
    auto proc = std::make_shared<KrispNoiseFilter>();
    if (!proc->Init(modelPath)) {
        return nullptr;
    }
    return BuildModule(proc);
}

std::unique_ptr<NativeKrispModule> NativeKrispModule::CreateWithModelData(
    const void* modelData, unsigned int modelSize)
{
    auto proc = std::make_shared<KrispNoiseFilter>();
    if (!proc->Init(modelData, modelSize)) {
        return nullptr;
    }
    return BuildModule(proc);
}


}
