#include "System.hpp"

System::System() : route(mileageCmp()) {}

System::System(string routeName) : routeName(routeName), route(mileageCmp()) {
    eventSet[1] = [this](Event* e) { this->arriveAtStop(e); };
    eventSet[2] = [this](Event* e) { this->deptFromStop(e); };
    eventSet[3] = [this](Event* e) { this->arriveAtLight(e); };
    eventSet[4] = [this](Event* e) { this->deptFromLight(e); };
    eventSet[5] = [this](Event* e) { this->paxArriveAtStop(e); };
}

void System::init() {
    /*TODO: read parameter*/
    /*fleet、headway*/
    /*Stop, Light(assume 500/1)*/
    cout << "eneter init" << endl;
    ifstream file( "./data/" + routeName + ".csv");
    string line;

    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        Stop* stop = new Stop;

        std::getline(ss, field, ',');
        cout << field << " ";
        stop->id = std::stoi(field);
        std::getline(ss, field, ',');
        cout << field << " ";
        stop->stopName = field;
        std::getline(ss, field, ',');
        cout << field << " ";
        stop->mileage = std::stoi(field);
        std::getline(ss, field, ',');
        cout << field << " ";
        stop->pax = 0; 
        std::getline(ss, field, ',');
        cout << field << "\n";
        stop->note = field;

        route.push(stop);
    }

    file.close();

}

void System::arriveAtStop(Event* e) {
    
    cout << fleet[e->getBusID()]->getPax();
}

void System::deptFromStop(Event* e) {
    
}

void System::arriveAtLight(Event* e) {
    
}

void System::deptFromLight(Event* e) {
   
}

void System::paxArriveAtStop(Event* e) {
   
}
