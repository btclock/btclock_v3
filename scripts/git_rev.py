import subprocess

revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")
)

try:
    tag = (
        subprocess.check_output(["git", "describe", "--tags", "--exact-match"])
        .strip()
        .decode("utf-8")
    )
    git_tag_define = '\'-DGIT_TAG=\"%s\"\'' % tag
except subprocess.CalledProcessError:
    git_tag_define = ''
    
print("'-DGIT_REV=\"%s\"'" % revision)
if git_tag_define:
    print(git_tag_define)