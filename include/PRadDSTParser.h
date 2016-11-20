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
    DST_Update_All = 0,
    No_GEM_Ped_Update = 1 << 0,
    No_HyCal_Ped_Update = 1 << 1,
    No_HyCal_Cal_Update = 1 << 2,
    No_Run_Info_Update = 1 << 3,
    No_Epics_Map_Update = 1 << 4,
    DST_Update_None = 0xffffffff,
};
//============================================================================//

class PRadDSTParser
{
public:
    friend class PRadDataHandler;

public:
    // constructor
    PRadDSTParser(PRadDataHandler *h = nullptr);

    // copy/move constructors
    PRadDSTParser(const PRadDSTParser &that) = delete;
    PRadDSTParser(PRadDSTParser &&that) = delete;

    // detructor
    virtual ~PRadDSTParser();

    // copy/move assignment operators
    PRadDSTParser &operator =(const PRadDSTParser &rhs) = delete;
    PRadDSTParser &operator =(PRadDSTParser &&rhs) = delete;

    // public member functions
    void SetHandler(PRadDataHandler *h) {handler = h;};
    PRadDataHandler *GetHandler() const {return handler;};
    void OpenOutput(const std::string &path,
                    std::ios::openmode mode = std::ios::out | std::ios::binary);
    void OpenInput(const std::string &path,
                   std::ios::openmode mode = std::ios::in | std::ios::binary);
    void CloseOutput();
    void CloseInput();
    void SetMode(const uint32_t &bit) {update_mode = bit;};
    bool Read();
    PRadDSTInfo EventType() const {return type;};
    const EventData &GetEvent() const {return event;};
    const EPICS_Data &GetEPICSEvent() const {return epics_event;};

    // write information
    void WriteRunInfo() throw(PRadException);
    void WriteEvent() throw(PRadException);
    void WriteEPICS() throw(PRadException);
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
