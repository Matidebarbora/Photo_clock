#!/usr/bin/env python3
"""
Resize JPEG images to 320x240, adding black bars if aspect ratio doesn't match.
"""

import os
from PIL import Image

def resize_with_padding(input_path, output_path, target_size=(320, 240)):
    """
    Resize image to target size, adding black padding if needed.
    
    Args:
        input_path: Path to input image
        output_path: Path to save output image
        target_size: Tuple of (width, height)
    """
    try:
        # Open the image
        img = Image.open(input_path)
        
        # Convert to RGB if needed (handles RGBA, grayscale, etc.)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        # Calculate scaling to fit within target size while maintaining aspect ratio
        img.thumbnail(target_size, Image.Resampling.LANCZOS)
        
        # Create a new black image at target size
        new_img = Image.new('RGB', target_size, (0, 0, 0))
        
        # Calculate position to paste the resized image (center it)
        paste_x = (target_size[0] - img.width) // 2
        paste_y = (target_size[1] - img.height) // 2
        
        # Paste the resized image onto the black background
        new_img.paste(img, (paste_x, paste_y))
        
        # Save the result
        new_img.save(output_path, 'JPEG', quality=95)
        print(f"✓ Processed: {os.path.basename(input_path)}")
        
    except Exception as e:
        print(f"✗ Error processing {os.path.basename(input_path)}: {e}")

def process_directory(input_dir):
    """
    Process all JPEG images in a directory.
    
    Args:
        input_dir: Directory containing input images
    """
    # Create output directory inside the input directory
    output_dir = os.path.join(input_dir, 'converted_photos')
    os.makedirs(output_dir, exist_ok=True)
    
    # Find all JPEG files
    jpeg_extensions = ('.jpg', '.jpeg', '.JPG', '.JPEG')
    image_files = [f for f in os.listdir(input_dir) 
                   if f.endswith(jpeg_extensions) and os.path.isfile(os.path.join(input_dir, f))]
    
    if not image_files:
        print(f"No JPEG images found in {input_dir}")
        return
    
    print(f"Found {len(image_files)} images. Processing...\n")
    
    for filename in image_files:
        input_path = os.path.join(input_dir, filename)
        output_path = os.path.join(output_dir, filename)
        resize_with_padding(input_path, output_path)
    
    print(f"\n✓ Done! Resized images saved to: {output_dir}")

if __name__ == "__main__":
    print("=" * 60)
    print("JPEG Image Resizer - 320x240 with Black Padding")
    print("=" * 60)
    
    # Ask for input folder
    input_dir = input("\nEnter the folder path containing JPEG images: ").strip()
    
    # Remove quotes if user wrapped the path in quotes
    input_dir = input_dir.strip('"').strip("'")
    
    # Check if directory exists
    if not os.path.exists(input_dir):
        print(f"\n✗ Error: Directory '{input_dir}' does not exist!")
        input("\nPress Enter to exit...")
        exit(1)
    
    if not os.path.isdir(input_dir):
        print(f"\n✗ Error: '{input_dir}' is not a directory!")
        input("\nPress Enter to exit...")
        exit(1)
    
    print(f"\nProcessing images in: {input_dir}")
    process_directory(input_dir)
    
    input("\nPress Enter to exit...")
