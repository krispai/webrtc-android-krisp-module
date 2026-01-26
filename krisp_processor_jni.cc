#include "krisp_processor.hpp"

#include <syslog.h>
#include <cstring>
#include <memory>

#include "sdk/android/src/jni/jni_helpers.h"


namespace jni_zero {
template <typename T>
class JavaParamRef;
}

namespace Krisp {
class Module {
public:
	explicit Module(std::unique_ptr<NativeKrispModule> module)
		: module_(std::move(module)) {}

	jlong GetAudioProcessorModule(JNIEnv* env);
	jboolean Init(JNIEnv* env, const jni_zero::JavaParamRef<jstring>& modelPathRef);
	jboolean InitWithData(JNIEnv* env, const jni_zero::JavaParamRef<jbyteArray>& modelDataRef);
	void Enable(JNIEnv* env, jboolean disable);
	jboolean IsEnabled(JNIEnv* env);
	void Destroy(JNIEnv* env);

private:
	std::unique_ptr<NativeKrispModule> module_;
};
}

#if __has_include("webrtc-android-krisp-module/generated_krisp_jni/KrispAudioProcessingFactory_jni.h")
#include "webrtc-android-krisp-module/generated_krisp_jni/KrispAudioProcessingFactory_jni.h"
#endif

namespace Krisp
{

#if defined(__GNUC__)
#define JNI_UNUSED __attribute__((unused))
#else
#define JNI_UNUSED
#endif

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModule(JNIEnv* env)
{
	auto module = NativeKrispModule::Create();
	return webrtc::jni::jlongFromPointer(new Module(std::move(module)));
}

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModuleWithModelPath(
	JNIEnv* env,
	const webrtc::JavaParamRef<jstring>& modelPathRef)
{
	const char *modelFilePath = env->GetStringUTFChars(modelPathRef.obj(), nullptr);
	auto module = NativeKrispModule::CreateWithModelPath(modelFilePath);
	env->ReleaseStringUTFChars(modelPathRef.obj(), modelFilePath);
	if (!module) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(new Module(std::move(module)));
}

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModuleWithModelData(
	JNIEnv* env,
	const webrtc::JavaParamRef<jbyteArray>& modelDataRef)
{
	jbyteArray javaByteArray = modelDataRef.obj();
	jsize javaModelSize = env->GetArrayLength(javaByteArray);
	jbyte *javaModelData = env->GetByteArrayElements(javaByteArray, nullptr);
	auto module = NativeKrispModule::CreateWithModelData(
		javaModelData, static_cast<unsigned int>(javaModelSize));
	env->ReleaseByteArrayElements(javaByteArray, javaModelData, JNI_ABORT);
	if (!module) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(new Module(std::move(module)));
}

jlong Module::GetAudioProcessorModule(JNIEnv* env)
{
	if (!module_ || !module_->apm) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(module_->apm.get());
}

jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_LoadKrisp(
	JNIEnv* env,
	const webrtc::JavaParamRef<jstring>& krispDllPath)
{
	const char *dllPath = env->GetStringUTFChars(krispDllPath.obj(), nullptr);
	bool retValue = LoadKrisp(dllPath);
	env->ReleaseStringUTFChars(krispDllPath.obj(), dllPath);
	return static_cast<jboolean>(retValue);
}

jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_UnloadKrisp(JNIEnv* env)
{
	return static_cast<jboolean>(UnloadKrisp());
}

void Module::Enable(JNIEnv* env, jboolean disable)
{
	if (!module_ || !module_->apm) {
		return;
	}
	module_->apm->SetRuntimeSetting(
		webrtc::AudioProcessing::RuntimeSetting::CreateCaptureOutputUsedSetting(disable));
}

jboolean Module::IsEnabled(JNIEnv* env)
{
	if (!module_ || !module_->proc) {
		return false;
	}
	return module_->proc->IsEnabled();
}

jboolean Module::Init(JNIEnv* env, const jni_zero::JavaParamRef<jstring>& modelPathRef)
{
	if (!module_ || !module_->proc) {
		return false;
	}
	jstring javaModelPath = modelPathRef.obj();
	const char *modelFilePath = env->GetStringUTFChars(javaModelPath, nullptr);
	bool retValue = module_->proc->Init(modelFilePath);
	env->ReleaseStringUTFChars(javaModelPath, modelFilePath);
	return static_cast<jboolean>(retValue); 
}

jboolean Module::InitWithData(JNIEnv* env,
	const jni_zero::JavaParamRef<jbyteArray>& modelDataRef)
{
	if (!module_ || !module_->proc) {
		return false;
	}
	jbyteArray javaByteArray = modelDataRef.obj();
	jsize javaModelSize = env->GetArrayLength(javaByteArray);
	jbyte *javaModelData = env->GetByteArrayElements(javaByteArray, nullptr);
	size_t arraySize = static_cast<unsigned int>(javaModelSize);
	std::unique_ptr<char[]> modelData(new char[arraySize]);
	std::memcpy(modelData.get(), javaModelData, arraySize);
	bool retValue = module_->proc->Init(modelData.get(), arraySize);
	env->ReleaseByteArrayElements(javaByteArray, javaModelData, JNI_ABORT);
	return static_cast<jboolean>(retValue); 
}

void Module::Destroy(JNIEnv* env)
{
	delete this;
}

#undef JNI_UNUSED

}
