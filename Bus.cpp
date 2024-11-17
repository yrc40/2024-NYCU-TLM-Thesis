#include "Bus.hpp"

Bus::Bus(int id) : id(id) {}

int Bus::getId() { return id; }

int Bus::getVol() { return vol; }

int Bus::getPax() { return pax; }

int Bus::getLoaction() { return location; }

int Bus::getDwell() { return dwell; }

const int Bus::getCapacity() { return capacity; }

void Bus::setVol(int v) { this->vol = v; }

void Bus::setPax(int p) { this->pax = p; }

void Bus::setLocation(int l) { this->location = l; }

void Bus::setDwell(int d) { this-> dwell = d; }
