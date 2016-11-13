//============================================================================//
// Basic DAQ channel unit                                                     //
//                                                                            //
// Chao Peng                                                                  //
// 12/11/2016                                                                 //
//============================================================================//

#include "PRadDAQChannel.h"



//============================================================================//
// Constructors, Destructors, Assignment Operators                            //
//============================================================================//

// constructors
PRadDAQChannel::PRadDAQChannel(ChannelAddress addr, bool d)
: ch_id(-1), ch_name("Undefined"), ch_address(addr), ch_dead(d)
{
    // place holder
}

PRadDAQChannel::PRadDAQChannel(int id, ChannelAddress addr, bool d)
: ch_id(id), ch_name("Undefined"), ch_address(addr), ch_dead(d)
{
    // place holder
}

PRadDAQChannel::PRadDAQChannel(std::string name, ChannelAddress addr, bool d)
: ch_id(-1), ch_name(name), ch_address(addr), ch_dead(d)
{
    // place holder
}

PRadDAQChannel::PRadDAQChannel(int id, std::string name, ChannelAddress addr, bool d)
: ch_id(id), ch_name(name), ch_address(addr), ch_dead(d)
{
    // place holder
}

// destructor
PRadDAQChannel::~PRadDAQChannel()
{
    // place holder
}
