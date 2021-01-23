#if !defined(DISPLAY_H)
#define DISPLAY_H

class DisplayClass
{
public:
    DisplayClass(int pin_lumi);
    void init();
    void update();
    void select(uint8_t index);
    void setLumi(uint16_t lumi);

private:
    uint8_t _current_menu;
    uint8_t _pin_lumi;
};

#endif // DISPLAY_H