#include <iostream>

#include "shared.h"
#include "irq.h"

int main(void) {

    std::cout << "global: " << shared::g_data << std::endl;
    std::cout << "foo(): " << irq::foo() << std::endl;


    return 0;
}
