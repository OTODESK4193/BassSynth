cmake_minimum_required(VERSION 3.22)

project(BassSynth VERSION 1.0.0 LANGUAGES C CXX)

# VST3の互換エラー回避
set(JUCE_VST3_AUTO_MANIFEST OFF)
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)
add_compile_definitions(JUCE_USE_WINDOWS_MEDIA_FORMAT=1)

# 1. C++20の有効化
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. JUCEディレクトリの指定
set(JUCE_DIR "C:/JUCE")
add_subdirectory(${JUCE_DIR} build_juce)

# 3. プラグインの定義
juce_add_plugin(BassSynth
    COMPANY_NAME "OTODESK"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE 
    VST3_AUTO_MANIFEST FALSE
    PLUGIN_MANUFACTURER_CODE "Otdk"
    PLUGIN_CODE "Bsyn"
    FORMATS VST3 Standalone
    PRODUCT_NAME "BassSynth"
    CHANNEL_CONFIGS 0,2
)

# 4. コンパイルオプション（SIMD最適化と文字コード設定）
target_compile_options(BassSynth PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Release>:/O2 /fp:fast> /arch:AVX2 /wd4819 /utf-8
    >
)

# 5. ソースコードの追加（不要なファイルを削除し、新UIを追加）
target_sources(BassSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h

    # DSP Group
    Source/DSP/WavetableOscillator.h
    Source/DSP/WavetableData.h
    Source/DSP/LadderFilter.h
    Source/DSP/SineShaper.h

    # UI Group
    Source/UI/AbletonLookAndFeel.h
    Source/UI/WavetableBrowser.h
    Source/UI/DualScopeComponent.h

    # Logic Group
    Source/Logic/MonoVoiceManager.h
    Source/Logic/AdsrEnvelope.h
 "Source/DSP/SpectralMorphProcessor.h" "Source/DSP/DualFilterEngine.h" "Source/DSP/MsegEngine.h" "Source/Logic/Mseg.h" "Source/DSP/ZeroLatencyLimiter.h")

# 6. インクルードパス
target_include_directories(BassSynth PRIVATE
    Source
    Source/DSP
    Source/UI
    Source/Logic
    Source/Generated
)

# 7. JUCEモジュールのリンク
target_link_libraries(BassSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_core        
    juce::juce_events      
)

# 8. JUCE 8 固有の設定
juce_generate_juce_header(BassSynth)

if(WIN32)
    target_compile_definitions(BassSynth PRIVATE JUCE_DIRECT2D=1)
endif()