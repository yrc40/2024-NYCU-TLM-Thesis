#include "Bus.hpp"

Bus::Bus(int id, int headway) : id(id), headway(headway) {}

int Bus::getId() { return id; }

float Bus::getVol() { return vol; }

int Bus::getPax() { return pax; }

int Bus::getLoaction() { return location; }

int Bus::getDwell() { return dwell; }

const int Bus::getCapacity() { return capacity; }

const int Bus::getHeadway() { return headway; }

void Bus::setVol(float v) { this->vol = v; }

void Bus::setPax(int p) { this->pax = p; }

void Bus::setLocation(int l) { this->location = l; }

void Bus::setDwell(int d) { this-> dwell = d; }

int Bus::getLastStop() { return lastStop; }

void Bus::setLastStop(int t) { this->lastStop = t; }
