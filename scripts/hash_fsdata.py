import os
from pathlib import Path
import hashlib

def calculate_file_hash(file_path):
    hasher = hashlib.sha256()
    with open(file_path, 'rb') as file:
        for chunk in iter(lambda: file.read(4096), b''):
            hasher.update(chunk)
    return hasher.hexdigest()

# def calculate_directory_hash(directory_path):
#     file_hashes = []
#     for root, dirs, files in os.walk(directory_path):
#         for file_name in sorted(files):  # Sorting based on filenames
#             if not file_name.startswith('.'):  # Skip dotfiles
#                 file_path = os.path.join(root, file_name)
#                 file_hash = calculate_file_hash(file_path)
#                 file_hashes.append((file_path, file_hash))

#     combined_hash = hashlib.sha256()
#     for file_path, _ in sorted(file_hashes):  # Sorting based on filenames
#         print(f"{file_path}: {file_hash}")
#         file_hash = calculate_file_hash(file_path)
#         combined_hash.update(file_hash.encode('utf-8'))

#     return combined_hash.hexdigest()


# def calculate_directory_hash(directory_path):
#     combined_hash = hashlib.sha256()
#     for root, dirs, files in os.walk(directory_path):
#         for file_name in sorted(files):  # Sorting based on filenames
#             if not file_name.startswith('.'):  # Skip dotfiles
#                 file_path = os.path.join(root, file_name)
#                 with open(file_path, 'rb') as file:
#                     print(f"{file_path}")
#                     for chunk in iter(lambda: file.read(4096), b''):
#                         combined_hash.update(chunk)

#     return combined_hash.hexdigest()

# def calculate_directory_hash(directory_path):
#     combined_content = b''
#     for root, dirs, files in os.walk(directory_path):
#         for file_name in sorted(files):  # Sorting based on filenames
#             if not file_name.startswith('.'):  # Skip dotfiles
#                 file_path = os.path.join(root, file_name)
#                 with open(file_path, 'rb') as file:
#                     print(f"{file_path}")
#                     combined_content += file.read()

#     combined_hash = hashlib.sha256(combined_content).hexdigest()
#     return combined_hash

def calculate_directory_hash(directory_path):
    file_paths = []
    for root, dirs, files in os.walk(directory_path):
        for file_name in files:
            if not file_name.startswith('.'):  # Skip dotfiles
                file_paths.append(os.path.join(root, file_name))

    combined_content = b''
    for file_path in sorted(file_paths):  # Sorting based on filenames
        with open(file_path, 'rb') as file:
            print(f"{file_path}")
            combined_content += file.read()

    combined_hash = hashlib.sha256(combined_content).hexdigest()
    return combined_hash

data_directory = 'data/build'
directory_hash = calculate_directory_hash(data_directory)

print(f"Hash of files in {data_directory}: {directory_hash}")