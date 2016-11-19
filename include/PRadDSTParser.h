#ifndef PRAD_DST_PARSER_H
#define PRAD_DST_PARSER_H

#include <fstream>
#include <string>
#include "PRadException.h"
#include "PRadEventStruct.h"

class PRadDataHandler;
class PRadEPICSystem;
class PRadHyCalSystem;
class PRadGEMSystem;

//============================================================================//
// DST FILE RELATED ENUMS                                                     //
//============================================================================//
enum PRadDSTInfo
{
    // event types
    PRad_DST_Event = 0,
    PRad_DST_Epics,
    PRad_DST_Epics_Map,
    PRad_DST_Run_Info,
    PRad_DST_HyCal_Info,
    PRad_DST_GEM_Info,
    PRad_DST_Undefined,
};

enum PRadDSTHeader
{
    // headers
    PRad_DST_Header = 0xc0c0c0,
    PRad_DST_EvHeader = 0xe0e0e0,
};

enum PRadDSTMode
{
    // by default it updates all info
    DST_UPDATE_ALL = 0,
    NO_GEM_PED_UPDATE = 1 << 0,
    NO_HYCAL_PED_UPDATE = 1 << 1,
    NO_HYCAL_CAL_UPDATE = 1 << 2,
    NO_RUN_INFO_UPDATE = 1 << 3,
    NO_EPICS_MAP_UPDATE = 1 << 4,
    DST_UPDATE_NONE = 0xffffffff,
};
//============================================================================//

class PRadDSTParser
{
public:
    PRadDSTParser(PRadDataHandler *h);
    virtual ~PRadDSTParser();

    void OpenOutput(const std::string &path,
                    std::ios::openmode mode = std::ios::out | std::ios::binary);
    void OpenInput(const std::string &path,
                   std::ios::openmode mode = std::ios::in | std::ios::binary);
    void CloseOutput();
    void CloseInput();
    void SetMode(const uint32_t &bit) {update_mode = bit;};
    bool Read();
    PRadDSTInfo EventType() {return type;};
    EventData &GetEvent() {return event;};
    EPICS_Data &GetEPICSEvent() {return epics_event;};


    void WriteRunInfo() throw(PRadException);
    void WriteEvent(const EventData &data) throw(PRadException);
    void WriteEPICS(const EPICS_Data &data) throw(PRadException);
    void WriteEPICSMap(const PRadEPICSystem *epics) throw(PRadException);
    void WriteHyCalInfo(const PRadHyCalSystem *hycal) throw(PRadException);
    void WriteGEMInfo(const PRadGEMSystem *gem) throw(PRadException);

private:
    void readRunInfo() throw(PRadException);
    void readEvent(EventData &data) throw(PRadException);
    void readEPICS(EPICS_Data &data) throw(PRadException);
    void readEPICSMap(PRadEPICSystem *epics) throw(PRadException);
    void readHyCalInfo(PRadHyCalSystem *hycal) throw(PRadException);
    void readGEMInfo(PRadGEMSystem *gem) throw(PRadException);

private:
    PRadDataHandler *handler;
    std::ofstream dst_out;
    std::ifstream dst_in;
    int64_t input_length;
    EventData event;
    EPICS_Data epics_event;
    PRadDSTInfo type;
    uint32_t update_mode;
};

#endif
