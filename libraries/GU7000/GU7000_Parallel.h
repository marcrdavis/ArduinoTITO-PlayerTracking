class GU7000_Parallel : public GU7000_Interface {

protected:
    unsigned RS_PIN:4;
    unsigned WR_PIN:4;
    unsigned RD_PIN:4;
    unsigned RESET_PIN:4;
    unsigned BUSY_PIN:4;
    unsigned D0_PIN:4;
    unsigned D1_PIN:4;
    unsigned D2_PIN:4;
    unsigned D3_PIN:4;
    unsigned D4_PIN:4;
    unsigned D5_PIN:4;
    unsigned D6_PIN:4;
    unsigned D7_PIN:4;
    unsigned jrb:2; // Pin#3 (JRB jumper): 0=BUSY, 1=RESET, 2=nothing
    
public:
    GU7000_Parallel(uint8_t jrb,
      uint8_t busy, uint8_t reset, uint8_t wr, uint8_t rd,
      uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
      uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7):
        jrb(2), BUSY_PIN(busy), RESET_PIN(reset), WR_PIN(wr),  RD_PIN(rd),
        D0_PIN(d0), D1_PIN(d1), D2_PIN(d2), D3_PIN(d3),
        D4_PIN(d4), D5_PIN(d5), D6_PIN(d6), D7_PIN(d7)
    {
        if ((jrb|0x20) == 'b') this->jrb = 0;
        if ((jrb|0x20) == 'r') this->jrb = 1;
    }
    
    void init() {        
        if (this->jrb == 1) {
            RAISE(RESET);
            DIRECTION(RESET, 1);
        }
        if (this->jrb != 0) {
            RAISE(RD);
            DIRECTION(RD, 1);
        }
        RAISE(WR);
        DIRECTION(BUSY, 0);
        DIRECTION(WR, 1);
        DIRECTION(D0, 1);
        DIRECTION(D1, 1);
        DIRECTION(D2, 1);
        DIRECTION(D3, 1);
        DIRECTION(D4, 1);
        DIRECTION(D5, 1);
        DIRECTION(D6, 1);
        DIRECTION(D7, 1);
    }
    
    void write(uint8_t data) {
        
        if (this->jrb == 0)
            while (CHECK(BUSY));
        else {
            bool ok;
            DIRECTION(D7,0);
            do {
                LOWER(RD);        
                _delay_us(.08);
                ok = CHECK(D7);
                RAISE(RD);
            } while (ok);
            DIRECTION(D7,1);
        }
		
		if (data & 0x01) RAISE(D0); else LOWER(D0);
        if (data & 0x02) RAISE(D1); else LOWER(D1);
        if (data & 0x04) RAISE(D2); else LOWER(D2);
        if (data & 0x08) RAISE(D3); else LOWER(D3);
        if (data & 0x10) RAISE(D4); else LOWER(D4);
        if (data & 0x20) RAISE(D5); else LOWER(D5);
        if (data & 0x40) RAISE(D6); else LOWER(D6);
        if (data & 0x80) RAISE(D7); else LOWER(D7);
        LOWER(WR);
        _delay_us(0.11);
        RAISE(WR);
        _delay_us(20);
    }
    
    void hardReset() {
        init();
        if (this->jrb == 1) {
        	LOWER(RESET);
        	_delay_ms(1);
        	RAISE(RESET);
        	_delay_ms(100);
        }
    }
};
