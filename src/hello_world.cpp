#include <iostream>
#include <boost/python.hpp>
using namespace boost::python;


void greet()
{
    std::cout << "Hello World" << std::endl;
}

int main()
{
    greet();
    return 0;
}


BOOST_PYTHON_MODULE(Greet)
{
    def("greet", greet);
}
