///
/// Copyright Krisp, Inc
///
#pragma once

#include "krisp-audio-api-definitions-c.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Ringtone configuration used with inbound NC models to keep ringtones.
typedef struct
{
    /// @brief Ringtone model configuration.
    KrispModelInfo modelInfo;
} KrispNcRingtoneCfg;

/// @brief NC session configuration.
typedef struct
{
    /// @brief Sampling frequency of the input data.
    KrispSamplingRate inputSampleRate;

    /// @brief Input audio frame duration.
    KrispFrameDuration inputFrameDuration;

    /// @brief Sampling frequency of the output data.
    KrispSamplingRate outputSampleRate;

    /// @brief NC model configuration.
    KrispModelInfo* modelInfo;

    /// @brief Set true to enable collection of NC session statistics
    bool enableSessionStats;

    /// @brief Optional: Ringtone configuration that may be provided with inbound NC models to retain ringtones.
    ///        Pass NULL to skip the ringtone retention feature.
    KrispNcRingtoneCfg* ringtoneCfg;
} KrispNcSessionConfig;

/// @brief Audio frame energy information struct describing noise/voice energy values
typedef struct
{
    /// @brief Voice energy level, range [0,100]
    uint8_t voiceEnergy;

    /// @brief Noise energy level, range [0,100]
    uint8_t noiseEnergy;
} KrispNcEnergyInfo;

/// @brief Cleaned secondary speech status enum
typedef enum
{
    /// @brief Cleaned secondary speech algorithm is not available (if non BVC model provided)
    Undefined = 0,

    /// @brief Cleaned secondary speech detected in the processed frame
    Detected = 1,

    /// @brief Cleaned secondary speech is not detected in the processed frame
    NotDetected = 2
} KrispNcCleanedSecondarySpeechStatus;

/// @brief Per-frame information returned after NC processing of the given frame
typedef struct
{
    /// @brief Voice and noise energy info.
    KrispNcEnergyInfo energy;

    /// @brief BVC specific feature.
    /// Returns the state of the removed secondary speech.
    /// If secondary speech is detected and removed, it returns Detected otherwise, it returns NotDetected.
    //  Undefined will be returned in case of running the NC.
    KrispNcCleanedSecondarySpeechStatus cleanedSecondarySpeechStatus;
} KrispNcPerFrameStats;

/// @brief Voice stats
typedef struct
{
    /// @brief Voice duration in ms
    uint32_t talkTimeMs;
} KrispNcVoiceStats;

/// @brief Noise stats based on the noise intensity levels
typedef struct
{
    /// @brief No noise duration in ms
    uint32_t noNoiseMs;

    /// @brief Low intensity noise duration in ms
    uint32_t lowNoiseMs;

    /// @brief Medium intensity noise duration in ms
    uint32_t mediumNoiseMs;

    /// @brief High intensity noise duration in ms
    uint32_t highNoiseMs;

    /// @brief Cleaned secondary speech detected duration in ms
    uint32_t cleanedSecondarySpeechMs;

    /// @brief Cleaned secondary speech not detected duration in ms
    uint32_t cleanedSecondarySpeechNotDetectedMs;

    /// @brief Cleaned secondary speech undefined duration in ms (non BVC use-case)
    uint32_t cleanedSecondarySpeechUndefinedMs;
} KrispNcNoiseStats;

/// @brief NC stats containing noise and voice information
typedef struct
{
    /// @brief Voice stats
    KrispNcVoiceStats voiceStats;

    /// @brief Noise stats
    KrispNcNoiseStats noiseStats;
} KrispNcSessionStats;

typedef uint64_t krispNcHandle;

/// @brief Creates a new instance of Nc session for int16 stream processing.
///        AI technology removes background noises, reverb, and background voices from the main speaker's voice
///        in real-time, while also providing noise and voice statistics for the audio stream and frame
/// @param[in] config Configuration for the Nc Session.
/// @retval Valid pointer on success, otherwise NULL.
KRISP_AUDIO_API krispNcHandle krispCreateNcInt16(const KrispNcSessionConfig* config);

/// @brief Creates a new instance of Nc session for float stream processing.
///        AI technology removes background noises, reverb, and background voices from the main speaker's voice
///        in real-time, while also providing noise and voice statistics for the audio stream and frame
/// @param[in] config Configuration for the Nc Session.
/// @retval Valid pointer on success, otherwise NULL.
KRISP_AUDIO_API krispNcHandle krispCreateNcFloat(const KrispNcSessionConfig* config);

/// @brief Destroys the Nc instance.
///        Should be called if the Nc instance is no longer needed, before krispGlobalDestroy()
/// @param nc The Nc instance to destroy.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispDestroyNc(const krispNcHandle nc);

/// @brief Processes an input frame of audio data with int16 samples.
/// @param[in] nc The handle of Nc instance to process the audio data.
/// @param[in] inputSamples Pointer to the input buffer containing audio samples.
///                         The buffer should hold enough samples to fill a frame of audio data,
///                         calculated as frameDuration * inputSampleRate / 1000 of FrameDataType samples.
/// @param[in] numInputSamples The number of samples in the input buffer.
///                            Must be sufficient to match the expected input frame size.
/// @param[out] outputSamples Pointer to the buffer for the processed audio samples.
///                           The caller must allocate a buffer of sufficient size to handle
///                           a frame of output samples, calculated as frameDuration * outputSampleRate / 1000 of
///                           FrameDataType samples.
/// @param[in] numOutputSamples The number of samples the output buffer can handle.
///                             Must be sufficient to match the expected output frame size.
/// @param[in] noiseSuppressionLevel Noise suppression level in the range [0, 100]%
///                                  Used to adjust the intensity of the applied noise suppression.
///                                  - 0% indicates no noise suppression.
///                                  - 100% indicates full noise suppression.
/// @param[out] frameStats Optional: Frame statistics calculated during NC processing.
///                        Pass NULL to skip calculation, or provide a valid pointer to receive the statistics.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispProcessNcInt16(
    const krispNcHandle nc,
    const int16_t* inputSamples,
    size_t numInputSamples,
    int16_t* outputSamples,
    size_t numOutputSamples,
    float noiseSuppressionLevel,
    KrispNcPerFrameStats* frameStats);

/// @brief Processes an input frame of audio data with float samples.
/// @param[in] nc The handle of Nc instance to process the audio data.
/// @param[in] inputSamples Pointer to the input buffer containing audio samples.
///                         The buffer should hold enough samples to fill a frame of audio data,
///                         calculated as frameDuration * inputSampleRate / 1000 of FrameDataType samples.
/// @param[in] numInputSamples The number of samples in the input buffer.
///                            Must be sufficient to match the expected input frame size.
/// @param[out] outputSamples Pointer to the buffer for the processed audio samples.
///                           The caller must allocate a buffer of sufficient size to handle
///                           a frame of output samples, calculated as frameDuration * outputSampleRate / 1000 of
///                           FrameDataType samples.
/// @param[in] numOutputSamples The number of samples the output buffer can handle.
///                             Must be sufficient to match the expected output frame size.
/// @param[in] noiseSuppressionLevel Noise suppression level in the range [0, 100]%
///                                  Used to adjust the intensity of the applied noise suppression.
///                                  - 0% indicates no noise suppression.
///                                  - 100% indicates full noise suppression.
/// @param[out] frameStats Optional: Frame statistics calculated during NC processing.
///                        Pass NULL to skip calculation, or provide a valid pointer to receive the statistics.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispProcessNcFloat(
    const krispNcHandle nc,
    const float* inputSamples,
    size_t numInputSamples,
    float* outputSamples,
    size_t numOutputSamples,
    float noiseSuppressionLevel,
    KrispNcPerFrameStats* frameStats);

/// @brief Retrieves noise and voice statistics calculated from the start of NC processing.
///        To enable statistics collection, ensure that NcSessionConfig::enableStats is set when creating the NC object.
///        The recommended frequency for retrieving stats is 200ms or more.
///        If it's required only at the end of the NC session, call this function once
///        before the NC class object is destroyed.
/// @param stats Session statistics
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispGetNcSessionStats(const krispNcHandle nc, KrispNcSessionStats* stats);

#ifdef __cplusplus
} // extern "C"
#endif
