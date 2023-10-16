#include <stdio.h>
#include <string.h>
#include <curl/curl.h>


int notify() {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        // 设置目标URL
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.0.4.116:8000/api/v1/targetinfo/test");

        // 设置POST请求
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // 设置请求头
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 设置JSON数据
        const char *json_data = "{\"username\":\"aaa\",\"email\":\"sss\"}";
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(json_data));

        // 执行请求
        res = curl_easy_perform(curl);

        // 检查请求是否成功
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // 清理资源
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

    }
}

int main(void) {



    for (int i = 0; i < 4; ++i) {

        notify();

    }


    return 0;
}
