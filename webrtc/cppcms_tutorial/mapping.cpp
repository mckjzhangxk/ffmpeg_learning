#include <cppcms/application.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppcms/applications_pool.h>
#include <iostream>
#include <stdlib.h>

//  export DYLD_LIBRARY_PATH=/Users/zhanggxk/project/bin/cppcms/lib
// http://127.0.0.1:9080/hello

/*
 * 教程 http://cppcms.com/wikipp/en/page/cppcms_1x_tut_url_mapping
 *
 *  讲解了 url与cppcms::application的成员函数映射的方式
 *
 *  0) cppcms的设计思想还是基于 cgi的方式
 *  把一个url理解成为SCRIPT_NAME PATH_INFO QUERY_STRING三大部分
 *  例如 /foo/bar.php/test?x=10 i
 *
 * SCRIPT_NAME = /foo/bar.php
 * PATH_INFO = /test
 * QUERY_STRING = x=10
 *
 *  1)对于cppcms，只有一个SCRIPT_NAME,cgi的 url解析规则
 *  例如 /myapp/page/10?x=10
 *   SCRIPT_NAME=/myapp
 *   PATH_INFO=/page/10
 *   QUERY_STRING = x=10
 *
 *   2） 涉及到的重要cppcms类
 *   url_mapper:  设置接受的path,可以包括placeholder
 *   url_dispatcher： 把path 解析参数，并且关联 app的成员函数，
 * */
class MyApp : public cppcms::application {
public:
    MyApp(cppcms::service &srv) :cppcms::application(srv)
    {
        //先定义一个/smile的 path
        mapper().assign("smile","/smile");
        //然后dispatch到 MyApp::smile,
        dispatcher().assign("/smile",&MyApp::smile, this);

        //定义一个/number/{1},{1}表示第一个placeholder
        mapper().assign("number","/number/{1}");
        //dispatch到 MyApp::number,并且解析正则表达式，传递subexpression 成为MyApp::number的第一个参数
        dispatcher().assign("/number/(\\d+)",&MyApp::number, this,1);



        //定义默认的path
        mapper().assign("");
        //dispatch到MyApp::welcome
        dispatcher().assign("",&MyApp::welcome,this);

        //程序的根节点是hello,或者可以认为是所有的PATHINFO的前缀
        mapper().root("/hello");
    }
    void number(std::string num)
    {
        int no = atoi(num.c_str());
        response().out() << "The number is " << no << "<br/>\n";
        //url(/) 表示 PATH_INFO=/
        response().out() << "<a href='" << url("/") << "'>Go back</a>";
    }
    void smile()
    {
        response().out() << ":-) <br/>\n";
        //url(/) 表示 PATH_INFO=/
        response().out() << "<a href='" << url("/") << "'>Go back</a>";
    }
    void welcome()
    {
        //url(/number,15) 表示 PATH_INFO=/number/15
        //url(/smile) 表示 PATH_INFO=/smile
        response().out() <<
                         "<h1> Welcome To Page with links </h1>\n"
                         "<a href='" << url("/number",1)  << "'>1</a><br>\n"
                         "<a href='" << url("/number",15) << "'>15</a><br>\n"
                         "<a href='" << url("/smile") << "' >:-)</a><br>\n";
    }
};
//./hello -c config.js  

int main(int argc,char *argv[]){
    try {
        cppcms::service srv(argc,argv);
        srv.applications_pool().mount(cppcms::applications_factory<MyApp>());
        srv.run();
    }catch (std::exception const &e){
        std::cerr << e.what() << std::endl;
    }

}