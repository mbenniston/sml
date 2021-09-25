#include <iostream>
#include <fstream>
#include <sstream>
#include <sml.hpp>

int main()
{
    std::ifstream ss{ "../examples/simple.sml" };

    sml::Node node;
    ss >> node;

    std::cout << node;
  
    return 0;
}