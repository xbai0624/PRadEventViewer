//============================================================================//
// A class to parse data buffer from file or ET                               //
// Make sure the endianness is correct or change the code befire using it     //
// Multi-thread can be disabled in PRadDataHandler.h                          //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "PRadEvioParser.h"
#include "PRadDataHandler.h"
#include "ConfigParser.h"
#include <thread>
#include <sstream>
#include <iostream>
#include <iomanip>

#define HEADER_SIZE 2
#define MAX_BUFFER_SIZE 100000

using namespace std;

PRadEvioParser::PRadEvioParser(PRadDataHandler *handler)
: myHandler(handler), c_parser(new ConfigParser()), event_number(0)
{
}

PRadEvioParser::~PRadEvioParser()
{
    delete c_parser;
}

// Simple binary reading for evio format files
void PRadEvioParser::ReadEvioFile(const char *filepath)
{
    ifstream evio_in(filepath, ios::binary | ios::in);

    if(!evio_in.is_open()) {
        cerr << "Cannot open evio file " << filepath << endl;
        return;
    }

    evio_in.seekg(0, evio_in.end);
    int length = evio_in.tellg();
    evio_in.seekg(0, evio_in.beg);

    uint32_t *buffer = new uint32_t[MAX_BUFFER_SIZE];

    while(evio_in.tellg() < length && evio_in.tellg() != -1)
    {
        try {
            getEvioBlock(evio_in, buffer);
        } catch (PRadException &e) {
            cerr << e.FailureType() << ": " 
                 << e.FailureDesc() << endl;
            cerr << "Abort reading from file " << filepath << endl;
            break;
        }
    }

    delete [] buffer;
}

size_t PRadEvioParser::getEvioBlock(ifstream &in, uint32_t *buf) throw(PRadException)
{
#define CODA_BLOCK_SIZE 8

    streamsize buf_size = sizeof(uint32_t);

    // read the block size
    in.read((char*) &buf[0], buf_size);

    if(buf[0] > MAX_BUFFER_SIZE)
    {
        throw PRadException("Read Evio Block", "buffer size is not enough for the block (size " + to_string(buf[0]) + ")");
    }

    // read the whole block in
    in.read((char*) &buf[1], buf_size * (buf[0] - 1));

    size_t index = CODA_BLOCK_SIZE; // strip off block header

    while(index < buf[0])
    {
        ParseEventByHeader((PRadEventHeader *) &buf[index]);
        index += buf[index] + 1;
    }

    return buf[0];
}

void PRadEvioParser::ParseEventByHeader(PRadEventHeader *header)
{
    // first check event type
    switch(header->tag)
    {
    case CODA_Event:
    case CODA_Sync:
    case EPICS_Info:
        myHandler->StartofNewEvent(header->tag);
        break; // go on to process
    case CODA_Prestart:
    case CODA_Go:
    case CODA_End:
    default:
        return; // not interested event type
    }

    const uint32_t *buffer = (const uint32_t*) header;
    size_t evtSize = header->length;
    size_t dataSize = 0;
    size_t index = 0;
#ifdef MULTI_THREAD // definition in PRadDataHandler.h
    vector<thread> bank_threads;
#endif
    index += HEADER_SIZE; // skip event header

    while(index < evtSize)
    {
        PRadEventHeader* evtHeader = (PRadEventHeader*) &buffer[index];
        index += HEADER_SIZE; // header info is read
        if(evtHeader->length < 2)
           continue;

        dataSize = evtHeader->length - 1;
        // check the header, skip uninterested ones
        switch(evtHeader->type)
        {
        case EvioBank: // Bank type header for ROC
        case EvioBank_B:
            switch(evtHeader->tag)
            {
            case PRadTagE:  // Tagger E, ROC id 7
            case PRadSRS_2: // SRS, ROC id 9
            case PRadSRS_1: // SRS, ROC id 8
            case PRadROC_3: // Fastbus, ROC id 6
            case PRadROC_2: // Fastbus, ROC id 5
            case PRadROC_1: // Fastbus, ROC id 4
            case PRadTS: // VME, ROC id 2
            case EPICS_IOC:
                continue; // Interested in ROCs, to next header

            default: // unrecognized ROC
                // Skip the whole segment
                break;
            }
            break;

        case UnsignedInt_32bit: // uint32 data bank
            switch(evtHeader->tag)
            {
            default:
            case LIVE_BANK: // bank contains the live time
                break;
            case EVINFO_BANK: // Bank contains the event information
                event_number = buffer[index];
                break;
            case TI_BANK: // Bank 0x4, TI data, contains live time and event type information
                parseTIData(&buffer[index], dataSize, evtHeader->num);
                break;
            case TDC_BANK:
            case TAG_BANK:
#ifdef MULTI_THREAD
                bank_threads.push_back(thread(&PRadEvioParser::parseTDCV1190, this, &buffer[index], dataSize, evtHeader->num));
#else
                parseTDCV1190(&buffer[index], dataSize, evtHeader->num);
#endif
                break;
            case DSC_BANK:
                parseDSCData(&buffer[index], dataSize);
                break;
            case FASTBUS_BANK: // Bank 0x7, Fastbus data
                // Self defined crate data header
                if((buffer[index]&0xff0fff00) == ADC1881M_DATABEG) {
#ifdef MULTI_THREAD
                    // for LMS event, since every module is fired, each thread needs to modify the non-local
                    // variable E_total and the container for current event. Which means very frequent actions
                    // on mutex lock and unlock, it indeed undermine the performance
                    // TODO, separate thread for GEM and HyCal only, there won't be any shared object between
                    // these two sub-system
                    bank_threads.push_back(thread(&PRadEvioParser::parseADC1881M, this, &buffer[index]));
#else
                    parseADC1881M(&buffer[index]);
#endif
                } else {
                    cerr << "Incorrect Fastbus bank header!"
                         << "0x" << hex << setw(8) << setfill('0')
                         <<  buffer[index] << endl;
                }
                break;
            case GEM_BANK: // Bank 0x8, gem data, single FEC right now
//                parseGEMData(&buffer[index], dataSize, evtHeader->num);
                break;
            }
            break;

        case CharString_8bit: // string data bank
            switch(evtHeader->tag)
            {
            case EPICS_BANK: // epics information
#ifdef MULTI_THREAD
                bank_threads.push_back(thread(&PRadEvioParser::parseEPICS, this, &buffer[index]));
#else
                parseEPICS(&buffer[index]);
#endif
                break;
            case CONF_BANK: // configuration information
            default:
                break;
            }
            break;
        default:
            // Unknown header
            break;
        }

        index += dataSize; // Data are either processed or skipped above
    }

#ifdef MULTI_THREAD
    // wait for all threads finished
    for(auto &thread : bank_threads)
    {
        if(thread.joinable()) thread.join();
    }
#endif

    myHandler->EndofThisEvent(event_number); // inform handler the end of event
}

void PRadEvioParser::parseADC1881M(const uint32_t *data)
{
    // number of boards given by the self defined info word in CODA readout list
    const unsigned char boardNum = data[0]&0xFF;
    unsigned int index = 2, wordCount;
    ADC1881MData adcData;

    adcData.config.crate = (data[0]>>20)&0xF;

    // parse the data for all boards
    for(unsigned char i = 0; i < boardNum; ++i)
    {
        if(data[index] == ADC1881M_DATAEND) // self defined, end of crate word
            break;
        adcData.config.slot = (data[index]>>27)&0x1F;
        wordCount = (data[index]&0x7F) + index;
        while(++index < wordCount)
        {
            if(((data[index]>>27)&0x1F) == (unsigned int)adcData.config.slot) {
                adcData.config.channel = (data[index]>>17)&0x3F;
                adcData.val = data[index]&0x3FFF;
                myHandler->FeedData(adcData); // feed data to handler
            } else { // show the error message
                cerr << "*** MISMATCHED CRATE ADDRESS ***" << endl;
                cerr << "GEOGRAPHICAL ADDRESS = "
                     << "0x" << hex << setw(8) << setfill('0') // formating
                     << adcData.config.slot
                     << endl;
                cerr << "BOARD ADDRESS = "
                     << "0x" << hex << setw(8) << setfill('0')
                     << ((data[index]&0xf8000000)>>27)
                     << endl;
                cerr << "DATA WORD = "
                     << "0x" << hex << setw(8) << setfill('0')
                     << data[index]
                     << endl;
            }
        }
    }

}

// temporary decoder for gem data, not finished
void PRadEvioParser::parseGEMData(const uint32_t *data, const size_t &size, const int &fec_id)
{
#define GEMDATA_APVBEG 0x00434441 //&0x00ffffff
#define GEMDATA_FECEND 0xfafafafa
// gem data structure
// single FEC
// 3 words header
// 0xff|ffffff
// apv |  frame counter, same in one event
// 0xff|434441
// apv |  fix number
// 0xffffffff
// unknown
// 0xffff|ffff
// data  |  data

    GEMAPVData gemData;
    gemData.FEC = fec_id;

    for(size_t i = 0; i < size; ++i)
    {
        if((data[i]&0xffffff) == GEMDATA_APVBEG) {
            gemData.APV = (data[i]>>24)&0xff;
            i += 2;
            for(; (data[i]&0xffffff) != data[0] && data[i] != GEMDATA_FECEND; ++i) {
                gemData.val.first = data[i]&0xffff;
                gemData.val.second = (data[i]>>16)&0xffff;
                myHandler->FeedData(gemData); // two adc values are sent in one package to reduce the number of function calls
            }
            // APV ends
        }
    }
}

// parse CAEN V767 Data
void PRadEvioParser::parseTDCV767(const uint32_t *data, const size_t &size, const int &roc_id)
{
    if(!(data[0]&V767_HEADER_BIT)) {
        cerr << "Unrecognized V767 header word: "
             << "0x" << hex << setw(8) << setfill('0') << data[0]
             << endl;
        return;
    }
    if(!(data[size-1]&V767_END_BIT)) {
        cerr << "Unrecognized V767 EOB word: "
             << "0x" << hex << setw(8) << setfill('0') << data[size-1]
             << endl;
        return;
    }

    TDCV767Data tdcData;
    tdcData.config.crate = roc_id;
    tdcData.config.slot = data[0]>>27;
    for(size_t i = 1; i < size - 1; ++i)
    {
        if(data[i]&V767_INVALID_BIT) {
            cerr << "Event: "<< dec << event_number
                 << ", invalid data word: "
                 << "0x" << hex << setw(8) << setfill('0') << data[i]
                 << endl;
            continue;
        }
        tdcData.config.channel = (data[i]>>24)&0x7f;
        tdcData.val = data[i]&0xfffff;
        myHandler->FeedData(tdcData);
    }
}

// parse CAEN V1190 Data
void PRadEvioParser::parseTDCV1190(const uint32_t *data, const size_t &size, const int &roc_id)
{
    TDCV1190Data tdcData;
    tdcData.config.crate = roc_id;

    for(size_t i = 0; i < size; ++i)
    {
        switch(data[i]>>27)
        {
        case V1190_GLOBAL_HEADER:
            if(roc_id == PRadTS) {
                tdcData.config.slot = 0; // geo address not supported in this crate
            } else {
                tdcData.config.slot = data[i]&0x1f;
            }
            break;
        case V1190_TDC_MEASURE:
            tdcData.config.channel = (data[i]>>19)&0x7f;
            tdcData.val = (data[i]&0x7ffff);
            myHandler->FeedData(tdcData);
            break;
        case V1190_TDC_ERROR:
/*
            cerr << "V1190 Error Word: "
		 << "0x" << hex << setw(8) << setfill('0') << data[i]
                 << endl;
            break;
*/
        case V1190_GLOBAL_TRAILER:
        case V1190_FILLER_WORD:
        default:
            break;
        }
    }
}

void PRadEvioParser::parseDSCData(const uint32_t *data, const size_t &size)
{
#define REF_PULSER_FREQ 500000
#define GATED_TDC_GROUP 3
#define GATED_TRG_GROUP 19
#define UNGATED_TDC_GROUP 35
#define UNGATED_TRG_GROUP 51

#define FCUP_OFFSET 100.0
#define FCUP_SLOPE 906.2

    if(size < 72) {
        cerr << "Unexpected scalar data bank size: " << size << endl;
        return;
    }

    unsigned int gated_counts[8];
    unsigned int ungated_counts[8];
    unsigned int pulser_read = data[7 + UNGATED_TRG_GROUP];

    auto scale_to_ref = [](const unsigned int &num, const unsigned int &ref_read, const unsigned int &ref_freq = REF_PULSER_FREQ)
                        {
                            if(!ref_read)
                                return num;

                            uint64_t tmp = num;
                            tmp *= ref_freq;
                            tmp /= ref_read;
                            return (unsigned int)tmp;
                        };

    for(int i = 0; i < 8; ++i)
    {
        gated_counts[i] = scale_to_ref(data[i + GATED_TRG_GROUP], pulser_read);
        ungated_counts[i] = scale_to_ref(data[i + UNGATED_TRG_GROUP], pulser_read);
    }

    myHandler->UpdateScalarGroup(8, gated_counts, ungated_counts);

    // calculate beam charge
    double beam_current = ((double)ungated_counts[6] - FCUP_OFFSET)/FCUP_SLOPE;
    double beam_charge = beam_current * (double)pulser_read/(double)REF_PULSER_FREQ;
    double live_time = 1. - (double)data[7 + GATED_TRG_GROUP]/(double)pulser_read;

    myHandler->AccumulateBeamCharge(live_time * beam_charge);
}

void PRadEvioParser::parseTIData(const uint32_t *data, const size_t &size, const int &roc_id)
{
    // update trigger type
    myHandler->UpdateTrgType(bit_to_trigger(data[2]>>24));

    if(roc_id == PRadTS) {// we will be more interested in the TI-master
        // check block header first
        if((data[0] >> 27) != 0x10) {
            cerr << "Unexpected TI block header: "
                 << "0x" << hex << setw(8) << setfill('0') << data[0]
                 << endl;
            return;
        }
        // then check the bank size
        if(size != 9) {
            cerr << "Unexpected TI-master bank size: "
                 << size << endl;
            return;
        }
        JLabTIData tiData;
        tiData.time_low = data[4];
        tiData.time_high = data[5] & 0xffff;
        tiData.latch_word = data[6] & 0xff;
        tiData.lms_phase = (data[8] >> 16) & 0xff;
        myHandler->FeedData(tiData);   
    }
}

void PRadEvioParser::parseEPICS(const uint32_t *data)
{
    c_parser->OpenBuffer((char*) data);

    while(c_parser->ParseLine())
    {
        if(c_parser->NbofElements() == 2) {
            float number = c_parser->TakeFirst().Float();
            string name = c_parser->TakeFirst();
            myHandler->UpdateEPICS(name, number);
        }
    }
}

PRadTriggerType PRadEvioParser::bit_to_trigger(const unsigned int &bit)
{
    int trg = 0;
    for(; (bit >> trg) > 0; ++trg)
    {
        if(trg >= MAX_Trigger) {
            return Undefined;
        }
    }

    return (PRadTriggerType) trg; 
}

unsigned int PRadEvioParser::trigger_to_bit(const PRadTriggerType &trg)
{
    if(trg == NotFromTI)
        return 0;
    else
        return 1 << (int) trg;
}


