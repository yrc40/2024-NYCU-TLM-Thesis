#include<bits/stdc++.h>
#include "System.hpp"

using namespace std;

int main() {
    srand(time(0));
    System system("307g");
    /*for(int i = 0; i < 3; i++) {
        
    }*/
    system.readSche(0);
    system.init();
    system.simulation();
    system.performance();
}