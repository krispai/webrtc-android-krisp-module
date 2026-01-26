#pragma once

#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef KRISP_AUDIO_STATIC
#define KRISP_AUDIO_API
#else
#ifdef KRISP_AUDIO_EXPORTS
#ifdef __GNUC__
#define KRISP_AUDIO_API __attribute__((dllexport))
#else
#define KRISP_AUDIO_API __declspec(dllexport) // Note: actually gcc seems to also support this syntax.
#endif
#else
#ifdef __GNUC__
#define KRISP_AUDIO_API __attribute__((dllimport))
#else
#define KRISP_AUDIO_API __declspec(dllimport) // Note: actually gcc seems to also support this syntax.
#endif
#endif
#endif
#else
#if __GNUC__ >= 4 || __clang__
#define KRISP_AUDIO_API __attribute__((visibility("default")))
#else
#define KRISP_AUDIO_API
#endif
#endif

/// @brief Sampling frequency of the audio frame
typedef enum
{
    Sr8000Hz = 8000,
    Sr16000Hz = 16000,
    Sr24000Hz = 24000,
    Sr32000Hz = 32000,
    Sr44100Hz = 44100,
    Sr48000Hz = 48000,
    Sr88200Hz = 88200,
    Sr96000Hz = 96000
} KrispSamplingRate;

/// @brief Input audio frame duration in ms
typedef enum
{
    Fd10ms = 10,
    Fd15ms = 15,
    Fd20ms = 20,
    Fd30ms = 30,
    Fd32ms = 32,
} KrispFrameDuration;

/// @brief Version information
typedef struct
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t build;
} KrispVersionInfo;

/// @brief Model Info containing path to the model or its content blob.
typedef struct
{
    /// @brief Path to the model file
    const wchar_t* path;

    /// @brief Model file content as a blob
    struct
    {
        const uint8_t* data;
        size_t size;
    } blob;
} KrispModelInfo;

/// @brief Return results of the API calls
typedef enum
{
    KrispRetValSuccess = 0,
    KrispRetValUnknowError = 1,
    KrispRetValInternalError = 2,
    KrispRetValInvalidInput = 3,
} KrispRetVal;

/// @brief The log levels.
typedef enum
{
    LogLevelTrace = 0,
    LogLevelDebug = 1,
    LogLevelInfo = 2,
    LogLevelWarn = 3,
    LogLevelErr = 4,
    LogLevelCritical = 5,
    LogLevelOff = 6
} KrispLogLevel;

#ifdef __cplusplus
} // extern "c"
#endif
