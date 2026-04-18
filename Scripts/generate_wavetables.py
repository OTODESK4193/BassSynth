import os
import librosa
import numpy as np

# --- 設定 ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
WAV_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "../Assets/Wavetables"))
OUTPUT_H_FILE = os.path.normpath(os.path.join(SCRIPT_DIR, "../Source/Generated/WavetableData_Generated.h"))

# 議論に基づいた全11種類のTAG定義
TAG_RULES = {
    "Basic": ["Sine", "Saw", "Square", "Triangle", "Tri", "Basics", "Simple"],
    "Analog": ["Analog", "Moog", "Retro", "Vintage", "Warm", "Pwm", "Pulse"],
    "Modern": ["Esw", "Serum", "Digital", "Fm", "Sync", "Hyper", "Super", "Modern"],
    "Vocal": ["Vocal", "Voice", "Choir", "Formant", "Bot", "Mouth"],
    "Real": ["Real", "Acoustic", "Guitar", "Brass", "String", "Mallet", "World"],
    "Agressive": ["Growl", "Acid", "Dist", "Hard", "Heavy", "Nasty", "Agressive"],
    "Sub": ["Sub", "Deep", "Low", "Solid"],
    "Metallic": ["Metallic", "Bell", "Tin", "Glass", "Spectral", "Sah"],
    "Atmospheric": ["Chill", "Frost", "World", "Sweep", "Arp", "Evolve", "Atmospheric"],
    "Experimental": ["Experimental", "Chaos", "Weird", "Glitch"],
    "FX": ["FX", "Riser", "Hit", "Perc"]
}

def analyze_hybrid(filepath, filename):
    tags = set()
    f_lower = filename.lower()
    
    # 1. 名称分析 (Keyword matching)
    for tag, keywords in TAG_RULES.items():
        if any(kw.lower() in f_lower for kw in keywords):
            tags.add(tag)
    
    # 2. 高度なスペクトラル分析 (librosaによる解析)
    try:
        # 音声ファイルをロード
        y, sr = librosa.load(filepath, sr=None, duration=1.0)
        
        # スペクトル重心（音の重心・明るさ）
        centroid = np.mean(librosa.feature.spectral_centroid(y=y, sr=sr))
        
        # スペクトルフラットネス（ノイズ・歪み感の強さ）
        flatness = np.mean(librosa.feature.spectral_flatness(y=y))
        
        # 数値に基づく自動タグ付け補正
        if centroid < 500: # 重心が低い
            tags.add("Sub")
        if flatness > 0.1: # 歪みが強い
            tags.add("Agressive")
            
    except Exception:
        pass # 解析失敗時は名称分析のみ採用
    
    return "|".join(list(tags)) if tags else "Others"

def generate():
    if not os.path.exists(WAV_DIR):
        print(f"Error: フォルダが見つかりません: {WAV_DIR}")
        return

    files = sorted([f for f in os.listdir(WAV_DIR) if f.lower().endswith(".wav")])
    
    print(f"--- ハイブリッド分析開始 (全 {len(files)} ファイル) ---")
    
    with open(OUTPUT_H_FILE, "w", encoding="utf-8") as out_f:
        out_f.write("#pragma once\n\n")
        out_f.write("namespace EmbeddedWavetables {\n")
        out_f.write(f"    static const char* wavetablesDir = R\"({WAV_DIR})\";\n")
        out_f.write(f"    constexpr int numTables = {len(files)};\n\n")
        
        # ファイル名リスト
        names_str = ", ".join([f'"{f}"' for f in files])
        out_f.write(f"    static const char* allNames[] = {{ {names_str} }};\n")
        
        # タグリストの生成
        tags_list = []
        for f in files:
            t = analyze_hybrid(os.path.join(WAV_DIR, f), f)
            tags_list.append(t)
            
        tags_str = ", ".join([f'"{t}"' for t in tags_list])
        out_f.write(f"    static const char* allTags[] = {{ {tags_str} }};\n")
        out_f.write("}\n")
    
    print(f"\n成功！名簿を作成しました。")

if __name__ == "__main__":
    generate()