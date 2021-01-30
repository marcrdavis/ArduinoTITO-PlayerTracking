/*
Updated as of 6/24/15 to match Arduino 1.6.5 SofwareSerial library improvements.
Updated by Cody Johnson of Noritake Co., Inc.
*/
#ifndef GU7000_SERIAL_ASYNC_H
#define GU7000_SERIAL_ASYNC_H

#include <Noritake_VFD_GU7000.h>
#include <avr/pgmspace.h>
#include <util/delay_basic.h>

//template <unsigned NORITAKE_VFD_BAUD>
class GU7000_Serial_Async : public GU7000_Interface{

private:
	Noritake_VFD_GU7000 GU7000;
	uint16_t _rx_delay_centering;
	uint16_t _rx_delay_intrabit;
	uint16_t _rx_delay_stopbit;
	uint16_t _tx_delay;
	// private static method for timing
	static inline void tunedDelay(uint16_t delay){ 
		_delay_loop_2(delay);
	}
	//Helper method to aid in delay calculations.
	uint16_t subtract_cap(uint16_t num, uint16_t sub) {
	  if (num > sub)
		return num - sub;
	  else
		return 1;
	}
	uint16_t _inverse_logic: 1;
	volatile uint8_t *_receivePortRegister;
	uint8_t _transmitBitMask;
	volatile uint8_t *_transmitPortRegister;
protected:
    unsigned OUT_PIN:4;
    unsigned BUSY_PIN:4;
    unsigned RESET_PIN:4;

	unsigned DEBUG_PIN:8;
    
public:
	
    GU7000_Serial_Async(long baudRate, uint8_t out, uint8_t busy, uint8_t reset):
        OUT_PIN(out), BUSY_PIN(busy), RESET_PIN(reset),
		_rx_delay_centering(0),  _rx_delay_intrabit(0),  _rx_delay_stopbit(0),  _tx_delay(0)
    {
		_rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;
		
		//Set TX
		_transmitBitMask = digitalPinToBitMask(out);
		uint8_t port = digitalPinToPort(out);
		_transmitPortRegister = portOutputRegister(port);
		
		//Updated Calculations from Arduino SofwareSerial
		uint16_t bit_delay = (F_CPU / baudRate) / 4;
		_tx_delay = subtract_cap(bit_delay, 15 / 4);

		if(digitalPinToPCICR(out)){
			#if GCC_VERSION > 40800
				_rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);
				
				_rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);
				
				_rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);
				
			#else
				_rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 97 + 29 - 11) / 4);
				
				_rx_delay_intrabit = subtract_cap(bit_delay, 11 / 4);
				
				_rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (44 + 17) / 4);
				
			#endif
		}
	}
	
    void init() {
        RAISE(RESET);
		DIRECTION(OUT,1);
    	DIRECTION(RESET,1);
    	DIRECTION(BUSY,0);
		//set model class		
		if(getModelClass==7003 || getModelClass==7903 || getModelClass==7053) {
			_inverse_logic = 0;
			RAISE(OUT);
		}else{
			_inverse_logic = 1;
			LOWER(OUT);
		}
    }
    void tx_pin_write(uint8_t pin_state){
		if (pin_state == LOW)
			*_transmitPortRegister &= ~_transmitBitMask;
		else
			*_transmitPortRegister |= _transmitBitMask;
	}

    void write(uint8_t data) {
		
		//Local variables to help ensure the best timing possible for transmit.
		bool inv = _inverse_logic;
		uint16_t delay = _tx_delay;
		volatile uint8_t *reg = _transmitPortRegister;
		uint8_t reg_mask = _transmitBitMask;
		uint8_t inv_mask = ~_transmitBitMask;
		uint8_t oldSREG = SREG;
		// Check for busy signal
		if (!inv)
			while (CHECK(BUSY));
		else
			while (!CHECK(BUSY));
		
		if(inv){
			data = ~data;
		}
		
		cli();  // turn off interrupts for a clean transmit

		// Write the start bit
		if (inv)
			*reg |= reg_mask;
		  else
			*reg &= inv_mask;
		
		tunedDelay(delay);
		
		// Write each of the 8 bits
		for (uint8_t i = 8; i > 0; --i) {
			
			if (data & 1)
				*reg |= reg_mask; // send 1
			else
				*reg &= inv_mask; // send 0
			
			tunedDelay(delay);
    		data >>= 1;
    	}

		if (inv)
			*reg &= inv_mask;
		else
			*reg |= reg_mask;

		tunedDelay(delay);
		SREG = oldSREG; // turn interrupts back on
	}

   
    void hardReset() {
        init();
    	LOWER(RESET);
    	_delay_ms(1);
    	RAISE(RESET);
    	_delay_ms(100);
		if(getModelClass==7003 || getModelClass==7903|| getModelClass==7053)
    		while (CHECK(BUSY));
		else
			while (!CHECK(BUSY));
		
	}
};

#endif