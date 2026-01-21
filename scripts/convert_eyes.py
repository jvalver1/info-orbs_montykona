from PIL import Image
import os

# Paths to generated images
base_path = r"C:\Users\juanjov\.gemini\antigravity\brain\e6932e02-fbd2-4d2e-b5b6-4a5d40f73f76"
eye_white_png = os.path.join(base_path, "photorealistic_eye_white_with_skin_1769017519265.png")
iris_png = os.path.join(base_path, "photorealistic_iris_blue_1769017536614.png")
nose_png = os.path.join(base_path, "photorealistic_nose_1769017551395.png")
eyelid_png = os.path.join(base_path, "photorealistic_closed_eyelid_with_skin_1769018091438.png")

# Target directory
target_dir = r"d:\OneDrive\ARDUINO\02 - PlatformIO\info-orbs_montykona\images\eyes"
if not os.path.exists(target_dir):
    os.makedirs(target_dir)

def convert_resize(src, dest, size, flip=False):
    img = Image.open(src)
    # Convert to RGB if necessary
    if img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info):
        background = Image.new('RGB', img.size, (0, 0, 0))
        # Handle alpha if exists
        if 'A' in img.mode:
            background.paste(img, mask=img.split()[3])
        else:
            background.paste(img)
        img = background
    elif img.mode != 'RGB':
        img = img.convert('RGB')
    
    img = img.resize(size, Image.Resampling.LANCZOS)
    if flip:
        img = img.transpose(Image.FLIP_LEFT_RIGHT)
    
    img.save(dest, "JPEG", quality=90)
    print(f"Saved {dest} ({size[0]}x{size[1]}){' flipped' if flip else ''}")

convert_resize(eye_white_png, os.path.join(target_dir, "eye_white_R.jpg"), (240, 240))
convert_resize(eye_white_png, os.path.join(target_dir, "eye_white_L.jpg"), (240, 240), flip=True)
convert_resize(iris_png, os.path.join(target_dir, "iris.jpg"), (80, 80))
convert_resize(nose_png, os.path.join(target_dir, "nose.jpg"), (240, 240))
convert_resize(eyelid_png, os.path.join(target_dir, "eyelid.jpg"), (240, 240))
convert_resize(eyelid_png, os.path.join(target_dir, "eyelid_mirrored.jpg"), (240, 240), flip=True)
