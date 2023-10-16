#include "inet/common/INETDefs.h"
#include <iostream>
#include <fstream>
#include <random>
#include <sqlite3.h>

using namespace inet;


namespace utils {

cNEDValue check_server_connect_address(cComponent *context, cNEDValue argv[], int argc)
{
    int chosen_idx = (int) argv[0];
    int parent_idx = (int) argv[1];
    int max_value = (int) argv[2];
    if (parent_idx == chosen_idx) {
        chosen_idx++;
    }

    return chosen_idx % max_value;
}

Define_NED_Function(check_server_connect_address,
        "int check_server_connect_address(int chosen_idx, int parent_idx, int max_value)"
        );

} // namespace utils

