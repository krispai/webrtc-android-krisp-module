# WebRTC Android Krisp Module

## Usage Instructions

### 1. Include the library, import and instantiate the Krisp Module

1.1. include the `libwebrtc.aar` into the Android project.

1.2. `import org.webrtc.KrispAudioProcessingFactory`

1.3. `var audioProcessorModule = KrispAudioProcessingFactory()`

### 2 Load dependencies
2.1. Load the stdlib required by Krisp Audio SDK. Load it before using Krisp.


`System.loadLibrary("c++_shared")`

2.2. Load the jingle_peerconnection_so. The library is required to use WebRTC on Android. 

`System.loadLibrary("jingle_peerconnection_so")`


### 3. Load the Krisp Dynamic Library
Load the Krisp dynamic library once before initializing a model.
```
val krispDllpath = "libkrisp-audio-sdk.so"
val loaded = KrispAudioProcessingFactory.LoadKrisp(krispDllpath)
if (!loaded) {
    // report error, read the logs for the details
}
```

### 4. Initialize the Krisp model
#### 3.1. Using the model file path
You can load Krisp model specifying file path. For this scenario you should make sure the Android app has access to the file resource.

```
val modelFilePath = "krisp-nc-o-med-v7.kef"
var retValue = audioProcessorModule.Init(modelFilePath)
if (!retValue) {
    // report error, read the logs for the details
}
```
Make sure to specify correct file paths, these are hard coded sample values.

#### 3.2. Using the model data loaded to the memory
Alternatively, you can load Krisp model by specifying model data content loaded into the memory.
```
var modelData: ByteArray // = load the model into the memory
audioProcessorModule.Init(modelData)
if (!retValue) {
    return null
}
```

### 5. Enable, disable Krisp NC during runtime
to enable Krisp NC during runtime
`audioProcessorModule.Enable(true)`

to disable Krisp NC
`audioProcessorModule.Enable(false)`

### 6. Integrate Krisp Module into the WebRTC PeerConnectionFactory
```
PeerConnectionFactory
            .builder()
            .setAudioProcessingFactory(audioProcessorModule)
```

### 7. Unload Krisp when done
```
KrispAudioProcessingFactory.UnloadKrisp()
```


## Build Instructions

### 1. Checkout WebRTC Repository
```
mkdir workdir
cd workdir
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:$PATH
fetch --nohooks webrtc_android
cd src
git checkout branch-heads/6478
git checkout -b 6478-modified a18e38fed2307edd6382760213fa3ddf199fa181
cd ..
gclient sync
cd src
```

### 2. Checkout the WebRTC Android Krisp Module Into The Repo
```
git clone git@github.com:krispai/webrtc-android-krisp-module.git
```

### 3. Modify the sdk/andorid/BUILD.gn file
```
Add krisp_processor library into the dependency list of the rtc_shared_library(“libjingle_peerconnection_so”)
deps = [ “../../webrtc-android-krisp-module:krisp_processor”, ….]
Add krisp_java library into the dependency list of the dist_jar(“libwebrtc”), for creating public java modules
deps = [ “../../webrtc-android-krisp-module:krisp_java”, ….]
```
See the screenshot.


<img width="662" alt="BUILD gn-mod" src="https://github.com/user-attachments/assets/adb3d064-923a-4dcb-8c35-bb04a5501015">

### 4. Build the WebRTC Android library
```
python3  tools_webrtc/android/build_aar.py
```

### 5. Get the WebRTC Android library
```
ls libwebrtc.aar
```
