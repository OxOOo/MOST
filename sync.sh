set -ex

rsync -av --exclude data ./ MOST:~/most
