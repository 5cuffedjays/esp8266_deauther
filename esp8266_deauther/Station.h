/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
 */

#pragma once

#include "AccessPoint.h"
#include "StringList.h"
#include "config.h"
#include "MACList.h"

#ifndef MAX_STATIONS
#define MAX_STATIONS 0
#endif // ifndef MAX_ALIAS

// ========== Station ========== //

class Station {
    private:
        uint8_t mac[6];
        AccessPoint* ap;

        uint32_t pkts           = 0;
        int8_t rssi             = -127;
        SortedStringList probes = SortedStringList(MAX_PROBES);
        Station* next           = nullptr;

        uint32_t auths = 0;

    public:
        Station(const uint8_t* mac, AccessPoint* ap);

        const uint8_t* getMAC() const;
        String getMACString() const;
        const AccessPoint* getAccessPoint() const;
        uint32_t getPackets() const;
        int8_t getRSSI() const;
        String getSSIDString() const;
        String getBSSIDString() const;
        String getVendor() const;
        uint32_t getAuths() const;

        SortedStringList& getProbes();
        Station* getNext();

        void setAccessPoint(AccessPoint* ap);
        void setAuth(uint8_t auths);
        void setNext(Station* next);

        bool addProbe(const char* ssid, uint8_t len);
        bool addAuth(uint8_t num);
        void newPkt(int8_t rssi);

        void print(int id = -1, const result_filter_t* filter = nullptr);
};

// ========== StationList ========== //

class StationList {
    private:
        Station* list_begin = nullptr;
        Station* list_end   = nullptr;
        int list_size       = 0;
        int list_max_size;

        Station* list_h = nullptr;
        int list_pos    = 0;

        int compare(const Station* st, const uint8_t* mac) const;

    public:
        StationList(int max = MAX_STATIONS);
        ~StationList();

        bool push(const uint8_t* mac);
        void clear();

        Station* search(const uint8_t* mac);
        Station* get(int i);

        void begin();
        Station* iterate();

        bool available() const;
        int size() const;
        bool full() const;

        void printHeader();
        void printFooter();
        void print(const result_filter_t* filter = nullptr);
};