#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>

#include "wfm.h"

using namespace std;

int main(int argc, char *argv[])
{
    std::string argv_str = argv[0];
    std::string base = argv_str.substr(0, argv_str.find_last_of("\\"));
    string file = base +"/ch1.wfm";

    Wfm *wfm = map_wfm(file.c_str());
    if (!wfm) {
        std::cerr << "Failed to map WFM." << std::endl;
        return 1;
    }

    print_wfm_info(wfm);
    calculate_and_print_voltages(wfm);

    write_wfm("test.wfm", wfm);

    unmap_wfm(wfm);

    return 0;
}

