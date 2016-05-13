#ifndef PRAD_EVIO_PARSER_H
#define PRAD_EVIO_PARSER_H

#include <stdint.h>
#include <datastruct.h>

class PRadDataHandler;

class PRadEvioParser
{
public:
    PRadEvioParser(PRadDataHandler* handler);
    void parseEventByHeader(PRadEventHeader *evtHeader);
    void parseADC1881M(const uint32_t *data);
    void parseGEMData(const uint32_t *data, const size_t &size, const int &fec_id);
    void parseTDCData(const uint32_t *data);
    void parseDSCData(const uint32_t *data);
    unsigned int GetCurrentEventNb() {return eventNb;};

private:
    PRadDataHandler *myHandler;
    unsigned int eventNb;
};

#endif
