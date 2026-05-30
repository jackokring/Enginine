#!/usr/bin/env python3
import shutil
import zlib


def clean_xml_string(text):
    marker = "<?xml"
    index = text.find(marker)

    if index != -1:
        # Slice from the index to the end of the string
        return text[index:]
    return text  # Return original if marker isn't found


def compress_file(input_filename, output_filename, cleanXml):
    # Read the original file
    with open(input_filename, "rb") as f:
        data = f.read()

    # Clean the XML string
    if cleanXml:
        data = clean_xml_string(data.decode("utf-8", errors="ignore")).encode("utf-8")

    # Compress the data
    compressed_data = zlib.compress(data, level=9)

    # Write the compressed data to a new file
    with open(output_filename, "wb") as f:
        f.write(compressed_data)


# Usage
compress_file("presets.xml", "presets.zlib", True)
print(
    "File presets.xml compressed to presets.zlib (presets.xml can be saved from the plugin)."
)
print(
    "Edit background.xcf and save as background.png? (use gimp export or save as .png)."
)
# shutil.copyfile("background.png", "background.png")
