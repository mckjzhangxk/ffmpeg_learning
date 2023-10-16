#include <cppcms/application.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppcms/applications_pool.h>
#include <iostream>


#include <iostream>
#include <sstream>

struct person {
    std::string name;
    double salary;
    std::vector<std::string> kids_names;
};



namespace cppcms {
    namespace json {
        template<>
        struct traits<person> {
            static person get(value const &v)
            {
                person p;
                if(v.type()!=is_object)
                    throw bad_value_cast();

                p.name=v.get<std::string>("name");
                p.salary=v.get<double>("salary");
                p.kids_names=
                        v.get<std::vector<std::string> >("kids_names");
                return p;
            }
            static void set(value &v,person const &in){
                v.set("name",in.name);
                v.set("salary",in.salary);
                v.set("kids_names",in.kids_names);
            }
        };
    } // json
} // cppcms
/**
 *
 * 教程 http://cppcms.com/wikipp/en/page/cppcms_1x_json
 *
 * */

int main(int argc,char *argv[]){
    cppcms::json::value my_object;

    my_object["name"]="Moshe";
    my_object["salary"]=1000.0;
    my_object["kids_names"][0]="Yossi";
    my_object["kids_names"][1]="Yonni";
    my_object["data"]["weight"]=85;
    my_object["data"]["height"]=1.80;


    std::cout <<  my_object<<std::endl;



    std::cout << "kids_names[0]="<< my_object["kids_names"][0]<<std::endl;

    double salary = my_object["salary"].number();
    salary=my_object.get<double>("salary");

    double height=my_object["data"]["height"].number();
    height=my_object.get<double>("data.height");

    std::string name = my_object["name"].str();
    name=my_object.get<std::string>("name");

    ///C++与 json对象的互相转换
    //实现 cppcms::json::traits 的 static  get,static set方法
    //json=>c++
    person cpp_persion=my_object.get_value<person>();
    std::cout << "kids_names[0]="<< cpp_persion.kids_names[0]<<std::endl;

    //c++=>json
    my_object["cpp_persion"]=cpp_persion;
    std::cout <<  my_object<<std::endl;



    //json=>string
    std::ostringstream oss;
    oss <<  my_object;
    std::string output = oss.str();
    std::cout << output << std::endl;

}