import os
import re
from collections import Counter

# --- 設定：あなたの環境のWavetableフォルダ ---
WAV_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "../Assets/Wavetables"))

def analyze():
    if not os.path.exists(WAV_DIR):
        print(f"エラー: フォルダが見つかりません -> {WAV_DIR}")
        return

    print(f"--- 分析開始: {WAV_DIR} ---\n")
    
    all_words = []
    file_count = 0

    for root, dirs, files in os.walk(WAV_DIR):
        for f in files:
            if f.lower().endswith(".wav"):
                file_count += 1
                # 1. 拡張子を消す
                name = os.path.splitext(f)[0]
                
                # 2. 記号（_ や -）や数字をスペースに置き換えて単語に分ける
                # 大文字小文字の区切り（CamelCase）にも対応
                words = re.sub(r'([a-z])([A-Z])', r'\1 \2', name) # camelCaseを分離
                words = re.sub(r'[^a-zA-Z]', ' ', words)       # 記号と数字を消す
                
                for word in words.split():
                    if len(word) > 1: # 1文字の単語（aとか）は無視
                        all_words.append(word.capitalize())

    # 頻度をカウント
    word_counts = Counter(all_words)
    
    print(f"総ファイル数: {file_count} 個")
    print(f"抽出された単語の種類: {len(word_counts)} 種\n")
    print("【出現頻度ランキング（TOP 50）】")
    print("-" * 30)
    
    # 上位50個を表示
    for word, count in word_counts.most_common(50):
        # 明らかに不要そうな単語は除外して眺める
        if word.upper() in ["WAV", "WT", "WAVETABLE"]: continue
        print(f"{word.ljust(15)} : {count} 回")

    print("-" * 30)
    print("\nこのリストの中で、音色の特徴を表している単語をTAG候補にしましょう。")

if __name__ == "__main__":
    analyze()