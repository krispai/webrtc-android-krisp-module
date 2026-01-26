#include <memory>
#include <atomic>
#include <string>
#include <vector>

#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/audio_buffer.h"

#include "krisp-audio-sdk-nc-c.h"

namespace Krisp
{

// Load Krisp DLL before using Krisp API
bool LoadKrisp(const char* krispDllPath);

// Unload Krisp DLL only after disposing all KrispNoiseFilter instances
bool UnloadKrisp();

class KrispNoiseFilter
{
public:
    KrispNoiseFilter();
    KrispNoiseFilter(const KrispNoiseFilter&) = delete;
    KrispNoiseFilter(KrispNoiseFilter&&) = delete;
    KrispNoiseFilter& operator=(const KrispNoiseFilter&) = delete;
    KrispNoiseFilter& operator=(KrispNoiseFilter&&) = delete;
    virtual ~KrispNoiseFilter();

    bool Init(const char* modelPath);
    bool Init(const void* modelAddr, unsigned int modelSize);
    void DeInit();
    void Enable(bool isEnable);
    bool IsEnabled() const;

    // Call this when sample rate changes.
    // Call this when audio stream changes.
    // Call this after the end of the call, or before the next call.
    void InitializeSession(int sampleRate, int numberOfChannels);
    void ProcessFrame(webrtc::AudioBuffer* audioBuffer);

private:
    std::atomic<bool> m_isEnabled;
    int m_numberOfChannels;
    long m_lastTimeStamp;
    std::wstring m_modelPath;
    std::vector<uint8_t> m_modelData;
    std::vector<float> m_bufferIn;
    std::vector<float> m_bufferOut;
    KrispModelInfo m_modelInfo;
    KrispNcSessionConfig m_sessionConfig;
    krispNcHandle m_ncCachedHandle;
};

class KrispAdapter : public webrtc::CustomProcessing
{
public:
    explicit KrispAdapter(const std::shared_ptr<KrispNoiseFilter>& krispProcessor);
    // Do not allow copy
    KrispAdapter(const KrispAdapter&) = delete;
    KrispAdapter& operator=(const KrispAdapter&) = delete;
    // Allow move
    KrispAdapter(KrispAdapter&&) = default;
    KrispAdapter& operator=(KrispAdapter&&) = default;
    virtual ~KrispAdapter() = default;
private:
    void Initialize(int sampleRate, int numOfChannels) override ;
    void Process(webrtc::AudioBuffer* audioBuffer) override;
    std::string ToString() const override;
    void SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting) override;

    std::shared_ptr<KrispNoiseFilter> m_krispProcessor;
};

struct NativeKrispModule {
    std::shared_ptr<KrispNoiseFilter> proc;
    rtc::scoped_refptr<webrtc::AudioProcessing> apm;

    static std::unique_ptr<NativeKrispModule> Create();
    static std::unique_ptr<NativeKrispModule> CreateWithModelPath(const char* modelPath);
    static std::unique_ptr<NativeKrispModule> CreateWithModelData(
        const void* modelData, unsigned int modelSize);
};


}
