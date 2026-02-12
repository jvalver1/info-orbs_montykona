"""Download flag images from Flagpedia CDN and convert to JPG for embedding.
Uses tight crop to remove any padding/alpha borders."""
import urllib.request
import os
from io import BytesIO

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

FLAGS = {
    "gb": "https://flagcdn.com/w80/gb.png",
    "es": "https://flagcdn.com/w80/es.png",
    "br": "https://flagcdn.com/w80/br.png",
    "us": "https://flagcdn.com/w80/us.png",
    "eg": "https://flagcdn.com/w80/eg.png",
}

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "images", "flags")

def download_and_convert():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    if not HAS_PIL:
        print("ERROR: Pillow (PIL) is required. Install with: pip install Pillow")
        return False
    
    for code, url in FLAGS.items():
        output_path = os.path.join(OUTPUT_DIR, f"{code}.jpg")
        print(f"Downloading {code} from {url}...")
        
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
            response = urllib.request.urlopen(req)
            png_data = response.read()
            
            # Open PNG
            img = Image.open(BytesIO(png_data))
            
            # Convert to RGBA first to handle any transparency
            if img.mode != "RGBA":
                img = img.convert("RGBA")
            
            # Find the bounding box of non-transparent pixels to crop tightly
            bbox = img.getbbox()
            if bbox:
                img = img.crop(bbox)
            
            # Resize to exactly 40x30 (flag display area)
            img = img.resize((40, 30), Image.LANCZOS)
            
            # Convert RGBA to RGB with the screen background color (black)
            bg = Image.new("RGB", img.size, (0, 0, 0))
            bg.paste(img, mask=img.split()[3])
            img = bg
            
            # Save as high-quality JPG to minimize compression artifacts
            img.save(output_path, "JPEG", quality=95)
            file_size = os.path.getsize(output_path)
            print(f"  Saved {output_path} ({file_size} bytes, {img.size[0]}x{img.size[1]})")
        except Exception as e:
            print(f"  ERROR downloading {code}: {e}")
            return False
    
    print("\nAll flags downloaded and converted successfully!")
    return True

if __name__ == "__main__":
    download_and_convert()
