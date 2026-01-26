#include "krisp_sdk.h"

// required by dlopen, dlclose, dlsym
#include <dlfcn.h>
// required by syslog
#include <syslog.h>
// required by std::array
#include <array>
// required by std::call_once
#include <mutex>


namespace KrispSDK {

enum class KrispFunctionId
{
	krispGlobalInit = 0,
	krispGlobalDestroy = 1,
	krispCreateNcFloat = 2,
	krispDestroyNc = 3,
	krispProcessNcFloat = 4,
};

class KrispAudioSdkDllGate {
 public:
  static KrispAudioSdkDllGate* singleton() {
    std::call_once(_initFlag, []() {
      _singleton = new KrispAudioSdkDllGate;
    });
    return _singleton;
  }

  bool LoadDll(const char* krispDllPath) {
    dlerror();
    _dllHandle = dlopen(krispDllPath, RTLD_LAZY);
    if (!_dllHandle) {
      syslog(LOG_ERR,
             "KrispSDK::LoadDll: Failed to load the library = %s\n",
             krispDllPath);
      return false;
    }
    syslog(LOG_INFO, "Krisp DLL loaded: %s", krispDllPath);
    return LoadFunctions();
  }

  void UnloadDll() {
    if (_dllHandle) {
      dlclose(_dllHandle);
      _dllHandle = nullptr;
    }
    for (auto& functionPtr : _functionPointers) {
      functionPtr = nullptr;
    }
  }

	template<typename ReturnType, typename... Args>
	ReturnType InvokeFunction(KrispFunctionId functionId, Args... args)
	{
		dlerror();    
		if (!_dllHandle) {
			syslog(LOG_INFO, "InvokeFunction invalid DLL handle: %p", _dllHandle);
			return ReturnType(); 
		}
		using FuncType = ReturnType (*)(Args...);
		void * functionPtr = _functionPointers[static_cast<size_t>(functionId)];
		FuncType function = reinterpret_cast<FuncType>(functionPtr);
		return function(args...);
	}

private:
	bool LoadFunctions() 
	{
		dlerror();
		for (size_t functionId = 0; functionId < _functionCount; ++functionId)
		{
			const char * functionName = _functionNames[functionId];
			syslog(LOG_INFO,"KrispAudioSdkDllGate::LoadFunctions dlsym the %s", functionName);
			void * functionPtr = dlsym(_dllHandle, functionName);
			const char* dlsym_error = dlerror();
			if (dlsym_error) {
				syslog(LOG_ERR, "KrispAudioSdkDllGate::LoadFunctions Error finding symbol: %s", dlsym_error);
				return false;
			}
			_functionPointers[functionId] = functionPtr;
		}
		return true;
	}

	static KrispAudioSdkDllGate * _singleton;
	static std::once_flag _initFlag;
	void* _dllHandle = nullptr;
	static constexpr unsigned int _functionCount = 5;
	static constexpr std::array<const char *, _functionCount> _functionNames =
	{
		"krispGlobalInit",
		"krispGlobalDestroy",
		"krispCreateNcFloat",
		"krispDestroyNc",
		"krispProcessNcFloat"
	};
	std::array<void *, _functionCount> _functionPointers = {};
};

KrispAudioSdkDllGate * KrispAudioSdkDllGate::_singleton = nullptr;
std::once_flag KrispAudioSdkDllGate::_initFlag;

template<typename ReturnType, typename... Args>
ReturnType InvokeFunction(KrispFunctionId functionId, Args... args)
{
	return KrispAudioSdkDllGate::singleton()->InvokeFunction<ReturnType, Args...>(functionId, args...);
}

bool LoadDll(const char* krispDllPath)
{
	return KrispAudioSdkDllGate::singleton()->LoadDll(krispDllPath);
}

void UnloadDll()
{
	KrispAudioSdkDllGate::singleton()->UnloadDll();
}

KrispRetVal GlobalInit(const wchar_t* workingPath,
					  void (*logCallback)(const char*, KrispLogLevel),
					  KrispLogLevel logLevel)
{
	return InvokeFunction<KrispRetVal>(KrispFunctionId::krispGlobalInit, workingPath, logCallback, logLevel);
}

KrispRetVal GlobalDestroy()
{
	return InvokeFunction<KrispRetVal>(KrispFunctionId::krispGlobalDestroy);
}

krispNcHandle CreateNcFloat(const KrispNcSessionConfig* config)
{
	return InvokeFunction<krispNcHandle>(KrispFunctionId::krispCreateNcFloat, config);
}

KrispRetVal DestroyNcFloat(const krispNcHandle session)
{
	return InvokeFunction<KrispRetVal>(KrispFunctionId::krispDestroyNc, session);
}

KrispRetVal ProcessNcFloat(const krispNcHandle session,
						   const float* inputSamples,
						   size_t numInputSamples,
						   float* outputSamples,
						   size_t numOutputSamples,
						   float noiseSuppressionLevel,
						   KrispNcPerFrameStats* frameStats)
{
	return InvokeFunction<KrispRetVal>(KrispFunctionId::krispProcessNcFloat,
									   session,
									   inputSamples,
									   numInputSamples,
									   outputSamples,
									   numOutputSamples,
									   noiseSuppressionLevel,
									   frameStats);
}

} 