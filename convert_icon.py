from PIL import Image
import os

# Paths
png_path = r"C:\Users\Kylian\.gemini\antigravity\brain\5a5cc64f-3c53-49ff-81ee-6e247bbb79b8\gitingest_ui_icon_premium_1774916728516.png"
ico_path = r"c:\Users\Kylian\.gemini\antigravity\scratch\gitingest-ui\src\GitingestUI.ico"

print(f"Opening {png_path}...")
img = Image.open(png_path)

# Windows icons need several sizes
icon_sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
print(f"Saving to {ico_path}...")
img.save(ico_path, format="ICO", sizes=icon_sizes)
print("Icon conversion successful!")
