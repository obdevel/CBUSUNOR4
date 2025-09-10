
/*

  Copyright (C) Duncan Greenwood 2017 (duncan_greenwood@hotmail.com)

  This work is licensed under the:
      Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
   To view a copy of this license, visit:
      http://creativecommons.org/licenses/by-nc-sa/4.0/
   or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

   License summary:
    You are free to:
      Share, copy and redistribute the material in any medium or format
      Adapt, remix, transform, and build upon the material

    The licensor cannot revoke these freedoms as long as you follow the license terms.

    Attribution : You must give appropriate credit, provide a link to the license,
                  and indicate if changes were made. You may do so in any reasonable manner,
                  but not in any way that suggests the licensor endorses you or your use.

    NonCommercial : You may not use the material for commercial purposes. **(see note below)

    ShareAlike : If you remix, transform, or build upon the material, you must distribute
                 your contributions under the same license as the original.

    No additional restrictions : You may not apply legal terms or technological measures that
                                 legally restrict others from doing anything the license permits.

   ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

    This software is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE

*/

#pragma once

// header files

#include <CBUS.h>               // abstract base class
#include <UNOR4CAN.h>           // Arduino library for Uno and Nano R4 CAN bus

/// constants

static const uint32_t CANBITRATE = 125000UL;                // 125Kb/s - fixed for CBUS
static const byte NUM_RX_BUFFS = 16;                        // default value
static const byte NUM_TX_BUFFS = 0;                         // not used

/// class definitions

/// circular buffer class for CAN messages

class circular_buffer_R4 {

public:

  circular_buffer_R4(unsigned int num_items);
  ~circular_buffer_R4();
  bool available(void);
  void put(const CANFrame *cf);
  CANFrame *get(void);
  bool full(void);
  void clear(void);
  bool empty(void);
  unsigned int size(void);
  unsigned int free_slots(void);
  unsigned int puts();
  unsigned int gets();
  unsigned int hwm(void);
  unsigned int overflows(void);

private:

  bool _full;
  unsigned int _head, _tail, _capacity, _size, _hwm, _puts, _gets, _overflows;
  CANFrame *_buffer;
};

/// an implementation of the abstract base CBUS class
/// using the on-chip CAN peripheral of the UNO or Nano R4

class CBUSUNOR4 : public CBUSbase {

public:

  CBUSUNOR4();
  CBUSUNOR4(CBUSConfig *the_config);
  ~CBUSUNOR4();

  // these methods are declared virtual in the base class and must be implemented by the derived class
  bool begin(bool poll = false, SPIClass& spi = SPI);
  bool available(void);
  CANFrame getNextMessage(void);
  bool sendMessage(CANFrame *msg, bool rtr = false, bool ext = false, byte priority = DEFAULT_PRIORITY);   // note default arguments
  bool sendMessageNoUpdate(CANFrame *msg);
  void reset(void);
  void printStatus(void);

  // these methods are specific to this implementation
  // they are not declared or implemented by the base CBUS class
  void setNumBuffers(byte num_rx_buffers, byte num_tx_buffers = 0);
  void setDebug(bool new_state);

  UNOR4CAN can;
  circular_buffer_R4 *rx_buffer;        // message receive buffer
  bool debug_on;

private:

  void initMembers(void);
  int _num_rx_buffers = NUM_RX_BUFFS, _num_tx_buffers = NUM_TX_BUFFS;
};

///

