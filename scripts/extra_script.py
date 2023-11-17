Import("env")
import os
import gzip
from shutil import copyfileobj, rmtree
from pathlib import Path

def gzip_file(input_file, output_file):
    with open(input_file, 'rb') as f_in:
        with gzip.open(output_file, 'wb') as f_out:
            copyfileobj(f_in, f_out)

def process_directory(input_dir, output_dir):
    for root, dirs, files in os.walk(input_dir):
        relative_path = os.path.relpath(root, input_dir)
        output_root = os.path.join(output_dir, relative_path)

        Path(output_root).mkdir(parents=True, exist_ok=True)

        for file in files:
           # if file.endswith(('.html', '.css', '.js')):
                input_file_path = os.path.join(root, file)
                output_file_path = os.path.join(output_root, file + '.gz')
                gzip_file(input_file_path, output_file_path)
                print(f'Compressed: {input_file_path} -> {output_file_path}')



# Build web interface before building FS
def before_buildfs(source, target, env):
    env.Execute("cd data && yarn && PUBLIC_BASE_URL=\"\" yarn build")
    input_directory = 'data/dist'
    output_directory = 'data/build_gz'
    rmtree(output_directory)
    process_directory(input_directory, output_directory)

env.AddPreAction("$BUILD_DIR/littlefs.bin", before_buildfs)
