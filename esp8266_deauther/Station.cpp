/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
 */

#include "Station.h"
#include "strh.h"
#include "vendor.h"
#include "alias.h"
#include "debug.h"

// ========== Station ========== //

Station::Station(const uint8_t* mac, AccessPoint* ap) {
    memcpy(this->mac, mac, 6);
    this->ap = ap;
}

const uint8_t* Station::getMAC() const {
    return mac;
}

String Station::getMACString() const {
    return alias::get(mac);
}

const AccessPoint* Station::getAccessPoint() const {
    return ap;
}

uint32_t Station::getPackets() const {
    return pkts;
}

int8_t Station::getRSSI() const {
    return rssi;
}

String Station::getSSIDString() const {
    if (ap) {
        return ap->getSSIDString();
    } else {
        return String();
    }
}

String Station::getBSSIDString() const {
    if (ap) {
        return ap->getBSSIDString();
    }
    return String();
}

String Station::getVendor() const {
    return vendor::search(mac);
}

uint32_t Station::getAuths() const {
    return auths;
}

SortedStringList& Station::getProbes() {
    return probes;
}

Station* Station::getNext() {
    return next;
}

void Station::setAccessPoint(AccessPoint* ap) {
    this->ap = ap;
}

void Station::setAuth(uint8_t auths) {
    this->auths = auths;
}

void Station::setNext(Station* next) {
    this->next = next;
}

bool Station::addProbe(const char* ssid, uint8_t len) {
    return probes.push(ssid, len);
}

bool Station::addAuth(uint8_t num) {
    if ((auths >> num) & 0x01) {
        return false;
    } else {
        auths |= (uint32_t(1) << num);
        return true;
    }
}

void Station::newPkt(int8_t rssi) {
    ++pkts;
    this->rssi = rssi;
}

void Station::print(int id, const result_filter_t* f) {
    const AccessPoint* ap = getAccessPoint();

    if (f) {
        if (ap) {
            if ((((f->channels >> (ap->getChannel()-1)) & 1) == 0)) return;
            if (f->ssids && f->ssids->size() && !f->ssids->contains(ap->getSSID())) return;
            if (f->bssid && f->bssid->size() && !f->bssid->contains(ap->getBSSID())) return;
        } else {
            // IF (no ap) AND (channel filter OR ssid filter OR bssid filter) return;
            if (f->channels != 0x3FFF) return;
            if (f->ssids && f->ssids->size()) return;
            if (f->bssid && f->bssid->size()) return;
        }

        if (f->vendors && f->vendors->size() && !f->vendors->contains(getVendor())) return;
    }

    debug(strh::right(3, id<0 ? String('-') : String(id)));
    debug(' ');
    debug(strh::right(4, String(getPackets())));
    debug(' ');
    debug(strh::right(4, String(getRSSI())));
    debug(' ');
    debug(strh::left(8, getVendor()));
    debug(' ');
    debug(strh::left(17, getMACString()));
    debug(' ');
    debug(strh::left(34, getSSIDString()));
    debug(' ');
    debug(strh::left(17, getBSSIDString()));
    debug(' ');

    getProbes().begin();
    bool first = true;

    while (getProbes().available()) {
        if (!first) {
            debugln();
            debugF("                                                                                         ");
        }
        debug(/*strh::left(32, */ '"' + getProbes().iterate() + '"');
        first = false;
    }

    debugln();
}

// ========== StationList ========== //

int StationList::compare(const Station* st, const uint8_t* mac) const {
    return memcmp(st->getMAC(), mac, 6);
}

StationList::StationList(int max) : list_max_size(max) {}

StationList::~StationList() {
    clear();
}

bool StationList::push(const uint8_t* mac) {
    if ((list_max_size > 0) && (list_size >= list_max_size)) return false;

    Station* new_st = new Station(mac, nullptr);

    // Empty list -> insert first element
    if (!list_begin) {
        list_begin = new_st;
        list_end   = new_st;
        list_h     = list_begin;
    } else {
        // Insert at start
        if (compare(list_begin, mac) > 0) {
            new_st->setNext(list_begin);
            list_begin = new_st;
        }
        // Insert at end
        else if (compare(list_end, mac) < 0) {
            list_end->setNext(new_st);
            list_end = new_st;
        }
        // Insert somewhere in the middle (insertion sort)
        else {
            Station* tmp_c = list_begin;
            Station* tmp_p = nullptr;

            int res = compare(tmp_c, mac);

            while (tmp_c->getNext() && res < 0) {
                tmp_p = tmp_c;
                tmp_c = tmp_c->getNext();
                res   = compare(tmp_c, mac);
            }

            // Skip duplicates
            if (res == 0) {
                delete new_st;
                return false;
            } else {
                new_st->setNext(tmp_c);
                if (tmp_p) tmp_p->setNext(new_st);
            }
        }
    }

    ++list_size;
    return true;
}

void StationList::clear() {
    Station* tmp = list_begin;

    while (tmp) {
        Station* to_delete = tmp;
        tmp = tmp->getNext();
        delete to_delete;
    }

    list_begin = nullptr;
    list_end   = nullptr;
    list_size  = 0;

    list_h   = nullptr;
    list_pos = 0;
}

Station* StationList::search(const uint8_t* mac) {
    if ((list_size == 0) || (compare(list_begin, mac) > 0) || (compare(list_end, mac) < 0)) {
        return nullptr;
    }

    Station* tmp = list_begin;

    int res = compare(tmp, mac);

    while (tmp->getNext() && res < 0) {
        tmp = tmp->getNext();
        res = compare(tmp, mac);
    }

    return (res == 0) ? tmp : nullptr;
}

Station* StationList::get(int i) {
    if (i < list_pos) begin();

    while (list_h && list_pos<i) iterate();

    return list_h;
}

void StationList::begin() {
    list_h   = list_begin;
    list_pos = 0;
}

Station* StationList::iterate() {
    Station* tmp = list_h;

    if (list_h) {
        list_h = list_h->getNext();
        ++list_pos;
    }

    return tmp;
}

bool StationList::available() const {
    return list_h;
}

int StationList::size() const {
    return list_size;
}

bool StationList::full() const {
    return list_max_size > 0 && list_size >= list_max_size;
}

void StationList::printHeader() {
    debug(strh::right(3, "ID"));
    debug(' ');
    debug(strh::right(4, "Pkts"));
    debug(' ');
    debug(strh::right(4, "RSSI"));
    debug(' ');
    debug(strh::left(8, "Vendor"));
    debug(' ');
    debug(strh::left(17, "MAC-Address"));
    debug(' ');
    debug(strh::left(34, "AccessPoint-SSID"));
    debug(' ');
    debug(strh::left(17, "AccessPoint-BSSID"));
    debug(' ');
    debug(strh::left(34, "Probe-Requests"));
    debugln();

    debuglnF("================================================================================================================================");
}

void StationList::printFooter() {
    debuglnF("================================================================================================================================");
    debuglnF("Pkts = Recorded Packets");
    debuglnF("================================================================================================================================");

    debugln();
}

void StationList::print(const result_filter_t* filter) {
    debugF("Station (Client) List: ");
    debugln(size());
    debuglnF("-------------------------");

    printHeader();

    int i = 0;
    begin();

    while (available()) {
        iterate()->print(i, filter);
        ++i;
    }

    printFooter();
}