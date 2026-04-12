#include <asm-generic/errno.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <cmath>

class vector2d {
public:

    double x;
    double y;
 
    vector2d() : x(0), y(0) {}
    vector2d(double x, double y) : x(x), y(y) {}
    inline double length() const { return sqrt(x*x + y*y); }

    inline vector2d operator +(const vector2d& other) const {
        return vector2d(x + other.x, y + other.y);
    }
    
    inline vector2d operator -(const vector2d& other) const {
        return vector2d(x - other.x, y - other.y);
    }

    inline vector2d operator -() const {
        return vector2d(-x, -y);
    }

    inline vector2d normalized() const {
        double l = length();
        if (l == 0) {
            return vector2d(0, 0);
        }
        return vector2d(x / l, y / l);
    }

    inline vector2d operator *(double scalar) const {
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

bool read_target_config(target_config& cfg) {
    std::ifstream input("input.txt");
    if (!input.is_open()) {
        std::cout << "read_target_config: failed to open: " << strerror(errno) <<std::endl;
        return false;
    }
    input >> cfg.xd >> cfg.yd >> cfg.zd >> cfg.targetX >> cfg.targetY >> cfg.attackSpeed >> cfg.accelerationPath >> cfg.ammo_name;
    input.close();

    std::cout << "input: xd: " << cfg.xd << std::endl;
    std::cout << "input: yd: " << cfg.yd << std::endl;
    std::cout << "input: zd: " << cfg.zd << std::endl;
    std::cout << "input: targetX: " << cfg.targetX << std::endl;
    std::cout << "input: targetY: " << cfg.targetY << std::endl;
    std::cout << "input: attackSpeed: " << cfg.attackSpeed << std::endl;
    std::cout << "input: accelerationPath: " << cfg.accelerationPath << std::endl;
    std::cout << "input: ammo_name: " << cfg.ammo_name << std::endl;
    return true;
}

enum ammo_type {
    FREE_FALL,
    PLANE,
};

struct ammo_config {
    const char *name;
    double m; // mass
    double d; // drag
    double l; // lift
    ammo_type type;
};

bool detect_ammo_config(const char* name, ammo_config& cfg) {
    if (strcmp(name, "VOG-17") == 0) {
        cfg.name = "VOG-17";
        cfg.m = 0.35;
        cfg.d = 0.07;
        cfg.l = 0.0;
        cfg.type = FREE_FALL;
        return true;
    } else if (strcmp(name, "M67") == 0) {
        cfg.name = "M67";
        cfg.m = 0.6;
        cfg.d = 0.10;
        cfg.l = 0.0;
        cfg.type = FREE_FALL;
        return true;
    }
    else if (strcmp(name, "RKG-3") == 0) {
        cfg.name = "RKG-3";
        cfg.m = 1.2;
        cfg.d = 0.10;
        cfg.l = 0.0;
        cfg.type = FREE_FALL;
        return true;
    } else if (strcmp(name, "GLIDING-VOG") == 0) {
        cfg.name = "GLIDING-VOG";
        cfg.m = 0.45;
        cfg.d = 0.10;
        cfg.l = 1.0;
        cfg.type = PLANE;
        return true;
    } else if (strcmp(name, "GLIDING-RKG") == 0) {
        cfg.name = "GLIDING-RKG";
        cfg.m = 1.4;
        cfg.d = 0.10;
        cfg.l = 1.0;
        cfg.type = PLANE;
        return true;
    }
    return false;
}

double a_koef(double d, double g, double m, double l, double Vo) {
    // a = d·g·m − 2d²·l·V₀
    double d2 = d*d;
    return d * g * m - 2 * d2 * l * Vo;
}

double b_koef(double g, double m, double d, double l, double Vo) {
    // b = −3g·m² + 3d·l·m·V₀
    double m2 = m*m;
    return -3 * g * m2 + 3 * d * l * m * Vo;
}

double c_koef(double m, double Za) {
    // c = 6m²·Z₀
    double m2 = m*m;
    return 6 * m2 * Za;
}

double flight_vertical_function(double t, double a, double b, double c) {
    // a*t^3 + b*t + c
    return a * t*t*t + b * t*t + c;
}

vector2d flight_horizontal_function(double t,const vector2d& drop_pos,vector2d& direction, double velocity) {
    return drop_pos + direction * velocity * t;
}

double calculate_time_of_flight(double p, double b, double a, double phi) {
    // t = 2√(−p/3) · cos( (φ + 4π) / 3 ) − b / (3a)
    double t = 2 * sqrt(-p/3) * cos( (phi + 4 * M_PI) / 3 ) - b / (3 * a);
    return t;
}

double p_value(double b, double a) {
    // p = − b² / (3a²)
    return -b*b / (3 * a*a);
}

double q_value(double b, double a, double c) {
    // q = 2b³ / (27a³) + c / a
    return 2 * b*b*b / (27 * a*a*a) + c / a;
}

bool phi_value(double q, double p,double &phi) {
    // φ = arccos( 3q / (2p) · √(−3/p) )
    if (p == 0) {
        std::cout << "phi_value: p == 0: " << p << std::endl;
        return false;
    }
    double acos_argument = 3 * q / (2 * p) * sqrt(-3 / p);
    if (acos_argument > 1 || acos_argument < -1) {
        std::cout << "acos_argument is out of range: " << acos_argument << std::endl;
        return false;
    }
    phi = acos(acos_argument);
    return true;
}

// h = V₀t − t²d·V₀/(2m) 
//   + t³(6d·g·l·m − 6d²(l²-1)·V₀)/(36m²) +
//   + t⁴ (−6d²g·l·(1+l²+l⁴)m + 3d³l²(1+l²)V₀ + 6d³l⁴(1+l²)V₀)  / (36(1+l²)²m³) 
//   + t⁵(3d³g·l³m − 3d⁴l²(1+l²)V₀) / (36(1+l²)m⁴)
bool horizontal_distance_before_target(double Vo, double t, const ammo_config& ammo_cfg,double g, double& hdbts) {
    if (t <= 0) {
        std::cout << "time could not be zero or negative: " << t << std::endl;
        return false;
    }

    if (ammo_cfg.m <= 0) {
        std::cout << "mass could not be zero or negative: " << ammo_cfg.m << std::endl;
        return false;
    }

    double l = ammo_cfg.l;
    double l2 = l*l;
    double l3 = l2*l;
    double l4 = l2*l2;

    double t2 = t*t;
    double t3 = t2*t;
    double t4 = t2*t2;
    double t5 = t2*t3;
    
    double d = ammo_cfg.d;
    double d2 = d*d;
    double d3 = d2*d;
    double d4 = d2*d2;

    double m = ammo_cfg.m;
    double m2 = m*m;
    double m3 = m2*m;
    double m4 = m2*m2;

    double one_plus_l2_squared = (1 + l2)*(1 + l2);

    hdbts = Vo * t - t2 * d * Vo / (2 * m)
        + t3 * (6 * d * g * l * m - 6 * d2 * (l2 - 1) * Vo) / (36 * m2)
        + t4 * (-6 * d2 * g * l * (1 + l2 + l4) * m + 3 * d3 * l2 * (1 + l2) * Vo
        + 6 * d3 * l4 * (1 + l2) * Vo) / (36 * one_plus_l2_squared * m3)
        + t5 * (3 * d3 * g * l3 * m - 3 * d4 * l2 * (1 + l2) * Vo) / (36 * (1 + l2) * m4);

    return true;
}

bool solution_cardano(double b, double a, double c, double& result) {
    double p = p_value(b, a);
    double q = q_value(b, a, c);

    double phi;

    bool phi_success = phi_value(q, p, phi);
    if (!phi_success) {
        std::cout << "phi_value: No solution" << std::endl;
        return false;
    }

    result = calculate_time_of_flight(p, b, a, phi);
    return true;
}


// soultion of the equation of free flight with drag and lift: a*t^3 + b*t + c = 0 
//   (where a, b, c are coefficients, but 
//      a = d*g*m - 2*d^2*l*V₀, 
//      b = -3*g*m^2 + 3*d*l*m*V₀, 
//      c = 6*m^2*Z₀) Z₀ - height
bool time_solution(double g, double attackSpeed,double height, const ammo_config& ammo_cfg, double& timeOfFlight) {
    double a = a_koef(ammo_cfg.d, g, ammo_cfg.m, ammo_cfg.l, attackSpeed);
    double b = b_koef(g, ammo_cfg.m, ammo_cfg.d, ammo_cfg.l, attackSpeed);
    double c = c_koef(ammo_cfg.m, height);

    if (a == 0) {
        // need to solve linear equation
        if (b == 0) {
            std::cout << "linear equation: b == 0" << std::endl;
            return false;
        }
        timeOfFlight = -c / b;
    } else {
        bool cardano_success = solution_cardano(b, a, c, timeOfFlight);
        if (!cardano_success) {
            std::cout << "cardano_success: No solution" << std::endl;
            return false;
        }
    }
    return true;
}

vector2d calculate_maneuver_pos(const vector2d& target_to_drone_direction, const vector2d& target_pos, double hdbts, double accelerationPath) {
    return target_to_drone_direction*(hdbts+accelerationPath)+target_pos;
}

vector2d calculate_drop_pos(const vector2d& target_to_drone_direction, const vector2d& target_pos, double hdbts) {
    return target_to_drone_direction*(hdbts)+target_pos;
}

bool calculate_drop_parameters(const target_config& cfg, const ammo_config& ammo_cfg, vector2d &drop_pos,vector2d &maneuver_pos,bool &do_we_need_maneuver) {

    vector2d drone_pos = vector2d(cfg.xd, cfg.yd);
    double drone_height = cfg.zd;

    vector2d target_pos = vector2d(cfg.targetX, cfg.targetY);

    // time of flight:
    double t;
    bool success = time_solution(9.81, cfg.attackSpeed, drone_height, ammo_cfg, t);
    if (!success) {
        std::cout << "time_solution: No solution" << std::endl;
        return false;
    }

    // horizontal distance between drop point and target:
    double hdbts;
    success = horizontal_distance_before_target(cfg.attackSpeed, t, ammo_cfg, 9.81, hdbts);
    if (!success) {
        std::cout << "horizontal_distance_before_target: No solution" << std::endl;
        return false;
    }

    vector2d target_to_drone_vector = (drone_pos - target_pos);
    double D = target_to_drone_vector.length();
    vector2d target_to_drone_direction = target_to_drone_vector.normalized();

    drop_pos = calculate_drop_pos(target_to_drone_direction, target_pos, hdbts);
    maneuver_pos = calculate_maneuver_pos(target_to_drone_direction, target_pos, hdbts, cfg.accelerationPath);

    if (cfg.accelerationPath+hdbts > D) {
        do_we_need_maneuver = true;
    } else {
        do_we_need_maneuver = false;
    }
    return true;
}

void write_vector(const vector2d& vec,std::ofstream &output) {
    output << vec.x << " " << vec.y;
}

void write_result(const vector2d& drop_pos,const vector2d& maneuver_pos,bool do_we_need_maneuver) {
    std::ofstream output("output.txt");

    if (do_we_need_maneuver) {
        write_vector(maneuver_pos,output);
        output << " ";
    }
    write_vector(drop_pos,output);

    output.close();
}

bool read_configuration(target_config& cfg, ammo_config& ammo_cfg) {
    bool success = read_target_config(cfg);
    if (!success) {
        std::cout << "read_target_config: read config failed" << std::endl;
        return false;
    }
    success = detect_ammo_config(cfg.ammo_name, ammo_cfg);
    if (!success) {
        std::cout << "detect_ammo_config: No appropriate ammo config found" << std::endl;
        return false;
    }
    return true;
}

#ifndef _LIBRARY_

int main() {
    target_config cfg;
    ammo_config ammo_cfg;

    bool success = read_configuration(cfg, ammo_cfg);
    if (!success) {
        return 1;
    }

    vector2d drop_pos;
    vector2d maneuver_pos;
    bool do_we_need_maneuver;

    success = calculate_drop_parameters(cfg,ammo_cfg,drop_pos,maneuver_pos,do_we_need_maneuver);
    if (!success) {
        return 1;
    }

    write_result(drop_pos,maneuver_pos,do_we_need_maneuver);

    return 0;
}

#endif // _LIBRARY_

