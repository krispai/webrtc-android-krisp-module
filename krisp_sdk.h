// required by KrispRetVal, KrispLogLevel, KrispVersionInfo
#include "krisp-audio-sdk-c.h"
// required by KrispNcSessionConfig, KrispNcHandle, KrispNcPerFrameStats
#include "krisp-audio-sdk-nc-c.h"


namespace KrispSDK
{
  // Loads the Krisp SDK shared library from the provided path.
  bool LoadDll(const char* dllPath);
  void UnloadDll();

  // Global lifecycle.
  KrispRetVal GlobalInit(const wchar_t* workingPath,
                         void (*logCallback)(const char*, KrispLogLevel),
                         KrispLogLevel logLevel);
  KrispRetVal GlobalDestroy();

  // Noise cancellation session management.
  krispNcHandle CreateNcFloat(const KrispNcSessionConfig* config);
  KrispRetVal DestroyNcFloat(const krispNcHandle session);

  // Per-frame processing.
  KrispRetVal ProcessNcFloat(const krispNcHandle session,
                             const float* inputSamples,
                             size_t numInputSamples,
                             float* outputSamples,
                             size_t numOutputSamples,
                             float noiseSuppressionLevel = 100.0f,
                             KrispNcPerFrameStats* frameStats = nullptr);
}
