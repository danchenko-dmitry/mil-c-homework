#pragma once

#include <cmath>

class vector2d {
public:
    double x;
    double y;

    vector2d() : x(0), y(0) {}
    vector2d(double x, double y) : x(x), y(y) {}
    inline double length() const { return sqrt(x * x + y * y); }

    inline vector2d operator+(const vector2d& other) const {
        return vector2d(x + other.x, y + other.y);
    }

    inline vector2d operator-(const vector2d& other) const {
        return vector2d(x - other.x, y - other.y);
    }

    inline vector2d operator-() const { return vector2d(-x, -y); }

    inline vector2d normalized() const {
        double l = length();
        if (l == 0) {
            return vector2d(0, 0);
        }
        return vector2d(x / l, y / l);
    }

    inline vector2d operator*(double scalar) const {
        return vector2d(x * scalar, y * scalar);
    }
};

struct target_config {
    float xd, yd, zd;
    float targetX, targetY;
    float attackSpeed;
    float accelerationPath;
    char ammo_name[10];
};

enum ammo_type {
    FREE_FALL,
    PLANE,
};

struct ammo_config {
    const char* name;
    double m;
    double d;
    double l;
    ammo_type type;
};

bool calculate_drop_parameters(const target_config& cfg, const ammo_config& ammo_cfg,
    vector2d& drop_pos, vector2d& maneuver_pos, bool& do_we_need_maneuver);

double flight_vertical_function(double t, double a, double b, double c);
vector2d flight_horizontal_function(double t,const vector2d& drop_pos,vector2d& direction, double velocity);

bool detect_ammo_config(const char* name, ammo_config& cfg);

double a_koef(double d, double g, double m, double l, double Vo);
double b_koef(double g, double m, double d, double l, double Vo);
double c_koef(double m, double Za);
double flight_function(double t, double a, double b, double c);