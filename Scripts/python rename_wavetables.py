import os
import librosa
import numpy as np
import shutil
import uuid

# --- 設定 ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
WAV_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "../Assets/Wavetables"))
OUTPUT_H_FILE = os.path.normpath(os.path.join(SCRIPT_DIR, "../Source/Generated/WavetableData_Generated.h"))

# 11種類のTAG定義（定義順を優先度として使用）
TAG_RULES = [
    ("Basic", ["Sine", "Saw", "Square", "Triangle", "Tri", "Basics", "Simple"]),
    ("Analog", ["Analog", "Moog", "Retro", "Vintage", "Warm", "Pwm", "Pulse"]),
    ("Modern", ["Esw", "Serum", "Digital", "Fm", "Sync", "Hyper", "Super", "Modern"]),
    ("Vocal", ["Vocal", "Voice", "Choir", "Formant", "Bot", "Mouth"]),
    ("Real", ["Real", "Acoustic", "Guitar", "Brass", "String", "Mallet", "World"]),
    ("Agressive", ["Growl", "Acid", "Dist", "Hard", "Heavy", "Nasty", "Agressive"]),
    ("Sub", ["Sub", "Deep", "Low", "Solid"]),
    ("Metallic", ["Metallic", "Bell", "Tin", "Glass", "Spectral", "Sah"]),
    ("Atmospheric", ["Chill", "Frost", "World", "Sweep", "Arp", "Evolve", "Atmospheric"]),
    ("Experimental", ["Experimental", "Chaos", "Weird", "Glitch"]),
    ("FX", ["FX", "Riser", "Hit", "Perc"])
]

def analyze_hybrid(filepath, filename):
    detected_tags = []
    f_lower = filename.lower()
    
    # 1. 名称分析
    for tag_name, keywords in TAG_RULES:
        if any(kw.lower() in f_lower for kw in keywords):
            detected_tags.append(tag_name)
    
    # 2. スペクトラル分析 (librosa)
    try:
        y, sr = librosa.load(filepath, sr=None, duration=1.0)
        centroid = np.mean(librosa.feature.spectral_centroid(y=y, sr=sr))
        flatness = np.mean(librosa.feature.spectral_flatness(y=y))
        
        if centroid < 500:
            detected_tags.append("Sub")
        if flatness > 0.1:
            detected_tags.append("Agressive")
    except Exception:
        pass

    # タグの重複排除とTAG_RULES順でのソート
    unique_tags = []
    for tag_name, _ in TAG_RULES:
        if tag_name in detected_tags and tag_name not in unique_tags:
            unique_tags.append(tag_name)
    
    # 上位2つに制限
    return unique_tags[:2] if unique_tags else ["Others"]

def process_library():
    if not os.path.exists(WAV_DIR):
        print(f"Error: フォルダが見つかりません: {WAV_DIR}")
        return

    print("--- 1. ファイル分析開始 ---")
    raw_files = [f for f in os.listdir(WAV_DIR) if f.lower().endswith(".wav")]
    analysis_results = []
    
    for f in raw_files:
        old_path = os.path.join(WAV_DIR, f)
        tags = analyze_hybrid(old_path, f)
        analysis_results.append({
            "old_path": old_path,
            "tags": tags,
            "tag_key": "_".join(tags)
        })

    # --- 2. リネーム用の一時ディレクトリ処理 ---
    # 名前衝突を避けるため、全ファイルを一時的な名前へリネーム
    print("--- 2. コリジョン回避用の一時リネーム ---")
    temp_rename_map = []
    for res in analysis_results:
        temp_name = str(uuid.uuid4()) + ".tmp"
        temp_path = os.path.join(WAV_DIR, temp_name)
        os.rename(res["old_path"], temp_path)
        res["temp_path"] = temp_path

    # --- 3. 新しいファイル名の決定と実行 ---
    print("--- 3. 構造化リネーム実行 ---")
    counters = {}
    final_files = []
    final_tags_str = []

    # タググループごとに処理して連番を振る
    for res in analysis_results:
        key = res["tag_key"]
        counters[key] = counters.get(key, 0) + 1
        
        new_filename = f"{key}{counters[key]}.wav"
        new_path = os.path.join(WAV_DIR, new_filename)
        
        os.rename(res["temp_path"], new_path)
        
        final_files.append(new_filename)
        # C++用のタグ文字列（ブラウザでパースするため全タグを'|'で結合）
        final_tags_str.append("|".join(res["tags"]))

    # リストをファイル名順でソート（C++側のインデックス整合性のため）
    sorted_indices = sorted(range(len(final_files)), key=lambda k: final_files[k])
    sorted_names = [final_files[i] for i in sorted_indices]
    sorted_tags = [final_tags_str[i] for i in sorted_indices]

    # --- 4. C++ヘッダーの生成 ---
    print(f"--- 4. ヘッダー生成: {os.path.basename(OUTPUT_H_FILE)} ---")
    with open(OUTPUT_H_FILE, "w", encoding="utf-8") as out_f:
        out_f.write("#pragma once\n\n")
        out_f.write("namespace EmbeddedWavetables {\n")
        # バックスラッシュ対策のためRaw文字列を使用
        out_f.write(f"    static const char* wavetablesDir = R\"({WAV_DIR})\";\n")
        out_f.write(f"    constexpr int numTables = {len(sorted_names)};\n")
        out_f.write(f"    constexpr int frameSize = 2048;\n\n")
        
        names_joined = ",\n        ".join([f'"{n}"' for n in sorted_names])
        out_f.write(f"    static const char* allNames[] = {{\n        {names_joined}\n    }};\n\n")
        
        tags_joined = ",\n        ".join([f'"{t}"' for t in sorted_tags])
        out_f.write(f"    static const char* allTags[] = {{\n        {tags_joined}\n    }};\n")
        out_f.write("}\n")

    print(f"\n完了！ {len(sorted_names)} 個のファイルをリネームし、名簿を更新しました。")

if __name__ == "__main__":
    # 実行前に確認
    confirm = input(f"{WAV_DIR} 内のファイルを直接リネームします。よろしいですか？ (y/n): ")
    if confirm.lower() == 'y':
        process_library()
    else:
        print("中止しました。")