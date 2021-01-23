#if !defined(ACCEL_H)
#define ACCEL_H

void update_accel(void);

class AccelClass
{
public:
    AccelClass() {}
    void init();
    void update();
    void print_debug(Print& p);
};

#endif // ACCEL_H