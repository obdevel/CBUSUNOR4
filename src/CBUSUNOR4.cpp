
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

#include <CBUS.h>
#include "CBUSUNOR4.h"

CBUSUNOR4 *instance_ptr;                                      // pointer to CBUS object
void event_callback(can_callback_args_t *p_args);      // static callback function

//
/// constructor and destructor
//

CBUSUNOR4::CBUSUNOR4() {
  initMembers();
}

CBUSUNOR4::CBUSUNOR4(CBUSConfig *the_config) : CBUSbase(the_config) {
  initMembers();
}

void CBUSUNOR4::initMembers(void) {
  eventhandler = NULL;
  eventhandlerex = NULL;
  framehandler = NULL;
  instance_ptr = this;
  debug_on = false;
}

CBUSUNOR4::~CBUSUNOR4() {

}

//
/// initialise the CAN controller peripheral
//

bool CBUSUNOR4::begin(__attribute__((unused)) bool poll, __attribute__((unused)) SPIClass& spi) {

  _numMsgsSent = 0;
  _numMsgsRcvd = 0;

  if (debug_on) {
    Serial.println("CBUS: begin");
    Serial.flush();
  }

  rx_buffer = new circular_buffer_R4(_num_rx_buffers); // call setNumBuffers first, if required to override default size (16)

  if (debug_on) {
    Serial.print("CBUS: receive buffer created = ");
    Serial.println(rx_buffer != NULL);
    Serial.print("CBUS: size = ");
    Serial.println(rx_buffer->free_slots());
    Serial.flush();
  }

  can.set_debug(debug_on);                             // set CAN driver debug state
  can.set_can_bitrate(CANBITRATE);                     // set the CAN bus bitrate, fixed at 125kbps

  if (debug_on) {
    Serial.println("CBUS: bitrate set");
    Serial.flush();
  }

  can.set_callback(event_callback);                    // register our handler for CAN bus events

  if (debug_on) {
    Serial.println("CBUS: callback installed");
    Serial.flush();

    Serial.println("CBUS: calling CAN begin");
    Serial.flush();
  }

  bool ok = can.begin();                               // initialise CAN bus driver

  if (debug_on) {
    Serial.print("CBUS: CAN begin returns ");
    Serial.println(ok);
    Serial.flush();
  }

  return ok;
}

//
/// check for one or more messages available to be received fromthe receive buffer
//

bool CBUSUNOR4::available(void) {

  return rx_buffer->available();
}

//
/// must call available() first to ensure a message is available in the buffer
//

CANFrame CBUSUNOR4::getNextMessage(void) {

  CANFrame cf;

  ++_numMsgsRcvd;
  memcpy(&cf, rx_buffer->get(), sizeof(CANFrame));
  return cf;
}

//
/// send a CBUS message
//

bool CBUSUNOR4::sendMessage(CANFrame *msg, bool rtr, bool ext, byte priority) {

  // caller must populate the message data
  // this method will create the correct frame header (CAN ID and priority bits)
  // rtr and ext default to false unless arguments are supplied - see method definition in .h
  // priority defaults to 1011 low/medium

  bool ok;

  makeHeader(msg, priority);                      // default priority unless user overrides
  msg->rtr = rtr;
  msg->ext = ext;

  ok = sendMessageNoUpdate(msg);                  // send the message with no further changes

  // call user transmit handler
  if (transmithandler != nullptr) {
    (void)(*transmithandler)(msg);
  }

  return ok;
}

//
/// send a CBUS message, without updating the header
/// this may be useful to CAN message routers which wish to pass the message unaltered
//

bool CBUSUNOR4::sendMessageNoUpdate(CANFrame *msg) {

  int r;
  can_frame_t omsg;

  omsg.id = msg->id;
  omsg.data_length_code = msg->len;
  memcpy(omsg.data, msg->data, msg->len);

  omsg.type = (msg->rtr ? CAN_FRAME_TYPE_REMOTE : CAN_FRAME_TYPE_DATA);
  omsg.id_mode = (msg->ext ? CAN_ID_MODE_EXTENDED : CAN_ID_MODE_STANDARD);

  if (debug_on) {
    Serial.println("CBUS: calling send");
    Serial.flush();
  }

  r = can.send(&omsg);

  if (debug_on) {
    Serial.print("CBUS: return = ");
    Serial.println(r);
    Serial.flush();
  }

  if (r) {
    ++_numMsgsSent;
    return true;
  } else {
    return false;
  }
}

//
/// display the CAN bus status instrumentation
//

void CBUSUNOR4::printStatus(void) {
  // not implemented
}

//
/// reset the CAN transceiver
//

void CBUSUNOR4::reset(void) {
  // not implemented
}

//
/// set the size of the software receive buffer
//

void CBUSUNOR4::setNumBuffers(byte num_rx_buffers, byte num_tx_buffers) {

  _num_rx_buffers = num_rx_buffers;
  _num_tx_buffers = num_tx_buffers;          // transmit buffering not implemented; chip has numerous mailbox slots
}

//
/// enable debug output
//

void CBUSUNOR4::setDebug(bool new_state) {

  debug_on = new_state;
  Serial.print("CBUS: debug is ");
  Serial.println((debug_on ? "on" : "off"));

  return;
}

//
/// non-class callback function
//

void event_callback(can_callback_args_t *p_args) {

  CANFrame cf;

  switch (p_args->event) {

  case CAN_EVENT_TX_COMPLETE:                                                                        // 2048
    if (instance_ptr->debug_on) {
      Serial.println("EVT: tx complete");
      Serial.flush();
    }
    break;

  case CAN_EVENT_RX_COMPLETE:                                                                        // 1024

    if (instance_ptr->debug_on) {
      Serial.println("EVT: rx complete");
      Serial.flush();
    }

    // convert to CBUS library message format
    cf.id = p_args->frame.id;
    cf.len = p_args->frame.data_length_code;
    cf.rtr = (p_args->frame.type == CAN_FRAME_TYPE_REMOTE);
    cf.ext = (p_args->frame.id_mode == CAN_ID_MODE_EXTENDED);
    memcpy(cf.data, &p_args->frame.data, p_args->frame.data_length_code);

    // insert into receive buffer
    instance_ptr->rx_buffer->put(&cf);

    break;

  case CAN_EVENT_ERR_WARNING:          /* error warning CAN event */                                 // 2
  case CAN_EVENT_ERR_PASSIVE:          /* error passive CAN event */                                 // 4
  case CAN_EVENT_ERR_BUS_OFF:          /* error bus off CAN event */                                 // 8
  case CAN_EVENT_BUS_RECOVERY:         /* Bus recovery error CAN event */                            // 16
  case CAN_EVENT_MAILBOX_MESSAGE_LOST: /* overwrite/overrun error CAN event */                       // 32
  case CAN_EVENT_ERR_BUS_LOCK:         /* Bus lock detected (32 consecutive dominant bits). */       // 128
  case CAN_EVENT_ERR_CHANNEL:          /* Channel error has occurred. */                             // 256
  case CAN_EVENT_TX_ABORTED:           /* Transmit abort CAN event. */                               // 512
  case CAN_EVENT_ERR_GLOBAL:           /* Global error has occurred. */                              // 4096
  case CAN_EVENT_TX_FIFO_EMPTY:        /* Transmit FIFO is empty. */                                 // 8192
    if (instance_ptr->debug_on) {
      Serial.print("EVT: error = ");
      Serial.println(p_args->event);
      Serial.flush();
    }

    break;
  }

  return;
}

///
/// a circular buffer class
///

/// constructor and destructor

circular_buffer_R4::circular_buffer_R4(unsigned int num_items) {

  _head = 0;
  _tail = 0;
  _hwm = 0;
  _capacity = num_items;
  _size = 0;
  _puts = 0;
  _gets = 0;
  _overflows = 0;
  _full = false;
  _buffer = (CANFrame *)malloc(num_items * sizeof(CANFrame));
}

circular_buffer_R4::~circular_buffer_R4() {
  free(_buffer);
}

/// if buffer has one or more stored items

bool circular_buffer_R4::available(void) {

  return (_size > 0);
}

/// store an item to the buffer - overwrite oldest item if buffer is full
/// make sure another action is not interrupted

void circular_buffer_R4::put(const CANFrame *item) {

  synchronized {
    memcpy(&_buffer[_head], item, sizeof(CANFrame));

    // if the buffer is full, this put will overwrite the oldest item

    if (_full) {
      _tail = (_tail + 1) % _capacity;
      ++_overflows;
    }

    _head = (_head + 1) % _capacity;
    _full = _head == _tail;
    _size = size();
    _hwm = (_size > _hwm) ? _size : _hwm;
    ++_puts;
  }

  return;
}

/// retrieve the next item from the buffer
/// make sure to synchronise so we aren't interrupted

CANFrame *circular_buffer_R4::get(void) {

  static CANFrame cf;

  cf = {};

  // should always call ::available first to avoid returning empty message

  synchronized {
    if (_size > 0) {
      memcpy(&cf, &_buffer[_tail], sizeof(CANFrame));
      _full = false;
      _tail = (_tail + 1) % _capacity;
      _size = size();
      ++_gets;
    }
  }

  return &cf;
}

/// clear all items

void circular_buffer_R4::clear(void) {

  _head = 0;
  _tail = 0;
  _full = false;
  _size = 0;

  return;
}

/// return high water mark

unsigned int circular_buffer_R4::hwm(void) {

  return _hwm;
}

/// return full indicator

bool circular_buffer_R4::full(void) {

  return _full;
}

/// recalculate number of items in the buffer

unsigned int circular_buffer_R4::size(void) {

  unsigned int size = _capacity;

  if (!_full) {
    if (_head >= _tail) {
      size = _head - _tail;
    } else {
      size = _capacity + _head - _tail;
    }
  }

  _size = size;
  return _size;
}

/// return empty indicator

bool circular_buffer_R4::empty(void) {

  return (!_full && (_head == _tail));
}

/// return number of free slots

unsigned int circular_buffer_R4::free_slots(void) {

  return (_capacity - _size);
}

/// number of puts

unsigned int circular_buffer_R4::puts(void) {

  return _puts;
}

/// number of gets

unsigned int circular_buffer_R4::gets(void) {

  return _gets;
}

/// number of overflows

unsigned int circular_buffer_R4::overflows(void) {

  return _overflows;
}

///
