#include "Bus.hpp"

Bus::Bus(int id, int headway) : id(id), headway(headway) {}

int Bus::getId() { return id; }

double Bus::getVol() { return vol; }

int Bus::getPax() { return pax; }

int Bus::getLocation() { return location; }

int Bus::getDwell() { return dwell; }

const int Bus::getCapacity() { return capacity; }

const int Bus::getHeadway() { return headway; }

double Bus::getNextVol() { return nextVol; }

void Bus::setVol(double v) { this->vol = v; }

void Bus::setPax(int p) { this->pax = p; }

void Bus::setLocation(int l) { this->location = l; }

void Bus::setDwell(int d) { this-> dwell = d; }

int Bus::getLastGo() { return lastGo; }

void Bus::setLastGo(int t) { this->lastGo = t; }

void Bus::setNextVol(double v) { this->nextVol = v; };
