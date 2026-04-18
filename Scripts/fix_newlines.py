import os

script_dir = os.path.dirname(os.path.abspath(__file__))
source_dir = os.path.join(script_dir, "../Source")

count = 0
for root, dirs, files in os.walk(source_dir):
    for file in files:
        if file.endswith((".h", ".cpp")):
            filepath = os.path.join(root, file)
            # 追記モード(a)で強制的に2回改行を挿入
            with open(filepath, "a", encoding="utf-8") as f:
                f.write("\n\n")
            count += 1

print(f"成功: {count}個のファイル末尾に安全用改行を追加しました！")
print("Visual Studio で再度ビルドを実行してください。")