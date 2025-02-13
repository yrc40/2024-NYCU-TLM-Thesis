#include "Bus.hpp"

Bus::Bus(int id, int headway) : id(id), headway(headway) {}

int Bus::getId() { return id; }

double Bus::getVol() { return vol; }

int Bus::getPax() { return pax; }

int Bus::getLocation() { return location; }

int Bus::getDwell() { return dwell; }

int Bus::getStopDwell() { return stopDwell; }

const int Bus::getCapacity() { return capacity; }

const int Bus::getHeadway() { return headway; }

double Bus::getNextVol() { return nextVol; }

double Bus::getArrivalRate() { return arrivalRate; }

double Bus::getDropRate() { return dropRate; }

void Bus::setVol(double v) { this->vol = v; }

void Bus::setPax(int p) { this->pax = p; }

void Bus::setLocation(int l) { this->location = l; }

void Bus::setDwell(int d) { this-> dwell = d; }

void Bus::setStopDwell(int d) { this->stopDwell = d; }

int Bus::getLastGo() { return lastGo; }

void Bus::setLastGo(int t) { this->lastGo = t; }

void Bus::setNextVol(double v) { this->nextVol = v; };

void Bus::setArrivalRate(double a) { this->arrivalRate = a; }

void Bus::setDropRate(double d) { this->dropRate = d; };
