#include <bits/stdc++.h>
#include "System.hpp"
#include "toml.hpp"

using namespace std;

int main() {
    srand(time(0));
    System system;
    system.init();
    system.simulation();
    system.performance();
}