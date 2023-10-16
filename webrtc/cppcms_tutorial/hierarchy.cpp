#include <cppcms/application.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppcms/applications_pool.h>
#include <iostream>
#include <stdlib.h>


class numbers : public cppcms::application {
public:
    numbers(cppcms::service &srv) : cppcms::application(srv)
    {
        dispatcher().assign("",&numbers::all,this);
        mapper().assign("");

        dispatcher().assign("/odd",&numbers::odd,this);
        mapper().assign("odd","/odd");

        dispatcher().assign("/even",&numbers::even,this);
        mapper().assign("even","/even");

        dispatcher().assign("/prime",&numbers::prime,this);
        mapper().assign("prime","/prime");
    }

    void prime()
    {
        response().out() << "2,3,5,7,...";
    }
    void odd()
    {
        response().out() << "1,3,5,7,9,...";
    }
    void even()
    {
        response().out() << "2,4,6,8,10,...";
    }
    void all()
    {
        //url("odd") 表示相对当前路径,当前是 /hello/number, url(odd)=>/hello/number/odd
        //url("/letters")表示的是绝对路径，当前是 /hello/number,url("/letter")=>/hello/letter
        response().out()
                << "<a href='" << url("/")       << "'>Top</a><br>"
                << "<a href='" << url("/letters")<< "'>Letters</a><br>"
                << "<a href='" << url(".")       << "'>All Numbers</a><br>"
                << "<a href='" << url("odd")     << "'>Odd Numbers</a><br>"
                << "<a href='" << url("even")    << "'>Even Numbers</a><br>"
                << "<a href='" << url("prime")   << "'>Prime Numbers</a><br>"
                << "1,2,3,4,5,6,7,8,9,10,...";
    }
};


class letters:public cppcms::application{
public:
    letters(cppcms::service &srv) : cppcms::application(srv)
    {
        dispatcher().assign("",&letters::all,this);
        mapper().assign("");

        dispatcher().assign("/capital",&letters::capital,this);
        mapper().assign("capital","/capital");

        dispatcher().assign("/small",&letters::small,this);
        mapper().assign("small","/small");

    }
    void all(){
        response().out()
                << "<a href='" << url("/")       << "'>Top</a><br>"
                << "<a href='" << url("/numbers")<< "'>Numbers</a><br>"
                << "<a href='" << url(".")       << "'>All Letters</a><br>"
                << "<a href='" << url("capital") << "'>Capital Letters</a><br>"
                << "<a href='" << url("small")   << "'>Small Letters</a><br>"
                << "Aa, Bb, Cc, Dd,...";
    }

    void capital()
    {
        response().out() << "A,B,C,D,...";
    }
    void small()
    {
        response().out() << "a,b,c,d,...";
    }
};


class myapp: public cppcms::application {
public:

    myapp(cppcms::service &srv) :cppcms::application(srv)
    {
        //  正则表达式  /numbers(/(.*))?  字表达是 是在 /numbers 和？之间的部分，以/开头，后面接任意符号
        // 例如 /number/odd  ==>  /odd
        // 例如 /letters/capital  ==>  /capital

        //然后交给对应的子应用  去匹配处理

        attach( new numbers(srv),
                "numbers", "/numbers{1}", // mapping
                "/numbers(/(.*))?", 1);   // dispatching
        attach( new letters(srv),
                "letters", "/letters{1}", // mapping
                "/letters(/(.*))?", 1);   // dispatching

        dispatcher().assign("",&myapp::describe,this);
        mapper().assign(""); // default URL

        //前缀 一定要注意
        mapper().root("/hello");
    }

    void describe()
    {
        response().out()
                << "<a href='" << url("/numbers")<< "'>Numbers</a><br>"
                << "<a href='" << url("/letters")<< "'>Letters</a><br>"
                << "<a href='" << url("/numbers/odd")<< "'>Odd Numbers</a><br>";
    }
};


/**
 * 教程  http://cppcms.com/wikipp/en/page/cppcms_1x_tut_hierarchy
 *
 * 你可以创建多个 app,一个主的，多个从的,通过 attach方法把把 路径与从 app关联起来
 * */

int main(int argc,char ** argv)
{
    try {
        cppcms::service app(argc,argv);
        app.applications_pool().mount(cppcms::applications_factory<myapp>());
        app.run();
    }
    catch(std::exception const &e) {
        std::cerr<<e.what()<<std::endl;
    }
}