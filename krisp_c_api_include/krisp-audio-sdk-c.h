///
/// Copyright Krisp, Inc
///
#pragma once

#include "krisp-audio-api-definitions-c.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @brief Initializes the global data needed for the SDK
/// @param[in] workingPath The path to the working directory. Can be empty for using default execution directory.
/// @param[in] logCallback The callback to call when a log message is emitted.
/// @param[in] logLevel Log level.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispGlobalInit(
    const wchar_t* workingPath, void (*logCallback)(const char*, KrispLogLevel), KrispLogLevel logLevel);

/// @brief Frees all the global resources allocated by SDK.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispGlobalDestroy();

/// @brief Populates the versionInfo structure with API version information upon successful completion.
/// @param[in,out] versionInfo The structure that gets populated upon successful completion of this call.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispGetVersion(KrispVersionInfo* versionInfo);

#ifdef __cplusplus
} // extern "c"
#endif
