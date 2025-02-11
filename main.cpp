#include<bits/stdc++.h>
#include "System.hpp"
#include "toml.hpp"

using namespace std;

int main() {
    srand(time(0));
    System system;
    cout << ">>> Simulation for 307 莒光往板橋前站 <<<\n";

    system.init();
    system.simulation();
    system.performance();
}