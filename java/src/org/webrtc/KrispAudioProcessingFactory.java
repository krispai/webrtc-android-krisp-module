package org.webrtc;
import org.webrtc.AudioProcessingFactory;

public class KrispAudioProcessingFactory implements AudioProcessingFactory {
  private long nativeModule;
    private boolean destroyed;

    private long requireNativeModule(String caller) {
        if (destroyed) {
            throw new IllegalStateException("KrispAudioProcessingFactory is destroyed");
        }
        if (nativeModule == 0) {
            throw new IllegalStateException("Call Init method before " + caller);
        }
        return nativeModule;
    }

  @Override
  public long createNative() {
        if (destroyed) {
            throw new IllegalStateException("KrispAudioProcessingFactory is destroyed");
        }
        if (nativeModule == 0) {
            throw new IllegalStateException("Call Init method before createNative()");
        }
    return nativeGetAudioProcessorModule(nativeModule);
  }

  public static boolean LoadKrisp(String dllPath) {
      return nativeLoadKrisp(dllPath);
  }

  public static boolean UnloadKrisp() {
      return nativeUnloadKrisp();
  }

  public boolean Init(String modelPath) {
        if (destroyed) {
            throw new IllegalStateException("KrispAudioProcessingFactory is destroyed");
        }
        if (nativeModule == 0) {
            nativeModule = nativeCreateModuleWithModelPath(modelPath);
            return nativeModule != 0;
        }
      return nativeInit(nativeModule, modelPath);
  }

    public boolean Init(byte[] modelData) {
        if (destroyed) {
            throw new IllegalStateException("KrispAudioProcessingFactory is destroyed");
        }
        if (nativeModule == 0) {
            nativeModule = nativeCreateModuleWithModelData(modelData);
            return nativeModule != 0;
        }
      return nativeInitWithData(nativeModule, modelData);
  }

  public void Enable(boolean enable) {
        long module = requireNativeModule("Enable");
        nativeEnable(module, enable);
  }

  public boolean IsEnabled() {
        long module = requireNativeModule("IsEnabled");
        return nativeIsEnabled(module);
  }

  public void Destroy() {
        long module = requireNativeModule("Destroy");
        nativeDestroy(module);
      nativeModule = 0;
        destroyed = true;
  }

  private static native void nativeEnable(long nativeModule, boolean disable);

  private static native boolean nativeIsEnabled(long nativeModule);

  private static native boolean nativeInit(long nativeModule, String modelPath);

    private static native boolean nativeInitWithData(long nativeModule, byte[] modelData);

  private static native boolean nativeLoadKrisp(String dllPath);

  private static native boolean nativeUnloadKrisp();

  private static native void nativeDestroy(long nativeModule);

  private static native long nativeCreateModule();

  private static native long nativeGetAudioProcessorModule(long nativeModule);

    private static native long nativeCreateModuleWithModelPath(String modelPath);

    private static native long nativeCreateModuleWithModelData(byte[] modelData);
}
