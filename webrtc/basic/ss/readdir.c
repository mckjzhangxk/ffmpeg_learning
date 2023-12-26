#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

void read_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;

    // 打开目录
    dir = opendir(path);

    if (dir == NULL) {
        perror("无法打开目录");
        exit(EXIT_FAILURE);
    }

    // 读取目录中的文件
    while ((entry = readdir(dir)) != NULL) {
        printf("%d %s\n", entry->d_type,entry->d_name);
    }

    // 关闭目录
    closedir(dir);
}

int main() {
    const char *path = "/Users/zhanggxk/project/ffmpegLearning/webrtc/cmake-build-debug";
    read_directory(path);

    return 0;
}
