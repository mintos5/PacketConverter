#include "../inc/MainApp.h"

int main(){
    using json = nlohmann::json;
    json test = json::parse("{ \"happy\": true, \"pi\": 3.141 }");
    std::cout << test.dump(4) << std::endl;
}
