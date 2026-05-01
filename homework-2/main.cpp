#include <asm-generic/errno.h>
#include <iostream>
#include <fstream>
#include <ostream>
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

    inline vector2d operator /(double scalar) const {
        return vector2d(x / scalar, y / scalar);
    }

    inline vector2d rotate(double angle) {
        double cos_angle = cos(angle);
        double sin_angle = sin(angle);
        double x_new = x * cos_angle - y * sin_angle;
        double y_new = x * sin_angle + y * cos_angle;
        return vector2d(x_new, y_new);
    }
};

enum ammo_type {
    FREE_FALL,
    PLANE,
};

struct ammo {
    const char *name;
    double m; // mass
    double d; // drag
    double l; // lift
    ammo_type type;
};

typedef vector2d target_waypoint;

struct target_trajectory {
    target_waypoint* points;
    int count;
};

struct drop_parameters {
    float xd, yd, zd;
    float targetX, targetY;
    float attackSpeed;
    float accelerationPath;
    char ammo_name[10];
};


namespace Config {

    struct task2_config {
        float xd, yd, zd; // drone coordinates (zd - height, m)
        float initialDir; // initial direction of the drone (radians, from the X axis)
        float attackSpeed; // attack speed of the drone (m/s)
        float accelerationPath; // length of acceleration/deceleration (m)
        char ammo_name[10]; // name of the ammo (see the ammo table)
        float arrayTimeStep; // time step of the array of target coordinates (s)
        float simTimeStep; // time step of the simulation (s)
        float hitRadius; // hit radius - permissible error of hitting (m)
        float angularSpeed; // angular speed of the drone (rad/s)
        float turnThreshold; // threshold angle for stopping (rad)
    };

    bool read_task2_config(task2_config& cfg) {
        std::ifstream input("input.txt");
        if (!input.is_open()) {
            std::cout << "read_task2_config: failed to open: " << strerror(errno) <<std::endl;
            return false;
        }
        input >> cfg.xd >> cfg.yd >> cfg.zd;
        input >> cfg.initialDir;
        input >> cfg.attackSpeed;
        input >> cfg.accelerationPath;
        input >> cfg.ammo_name;
        input >> cfg.arrayTimeStep;
        input >> cfg.simTimeStep;
        input >> cfg.hitRadius;
        input >> cfg.angularSpeed;
        input >> cfg.turnThreshold;

        if (input.fail()) {
            std::cout << "read_task2_config: failed to read: " << strerror(errno) <<std::endl;
            return false;
        }
        input.close();

        std::cout << "input: xd: " << cfg.xd << std::endl;
        std::cout << "input: yd: " << cfg.yd << std::endl;
        std::cout << "input: zd: " << cfg.zd << std::endl;
        std::cout << "input: initialDir: " << cfg.initialDir << std::endl;
        std::cout << "input: attackSpeed: " << cfg.attackSpeed << std::endl;
        std::cout << "input: accelerationPath: " << cfg.accelerationPath << std::endl;
        std::cout << "input: ammo_name: " << cfg.ammo_name << std::endl;
        std::cout << "input: arrayTimeStep: " << cfg.arrayTimeStep << std::endl;
        std::cout << "input: simTimeStep: " << cfg.simTimeStep << std::endl;
        std::cout << "input: hitRadius: " << cfg.hitRadius << std::endl;
        std::cout << "input: angularSpeed: " << cfg.angularSpeed << std::endl;
        std::cout << "input: turnThreshold: " << cfg.turnThreshold << std::endl;
        return true;
    }

    namespace ammo_configs {
        char bombNames[][15] = {"VOG-17", "M67", "RKG-3", "GLIDING-VOG", "GLIDING-RKG"};
        float bombM[] = {0.35f, 0.6f, 1.2f, 0.45f, 1.4f};
        float bombD[] = {0.07f, 0.10f, 0.10f, 0.10f, 0.10f};
        float bombL[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f};

        bool get_ammo_config(const char* name, ammo& cfg) {
            for (int i = 0; i < sizeof(bombNames) / sizeof(bombNames[0]); i++) {
                if (strcmp(name, bombNames[i]) == 0) {
                    cfg.name = bombNames[i];
                    cfg.m = bombM[i];
                    cfg.d = bombD[i];
                    cfg.l = bombL[i];
                    if (strcmp(bombNames[i],"VOG-17") == 0 ) {
                        cfg.type = FREE_FALL;
                    } else if (strcmp(bombNames[i],"M67") == 0) {
                        cfg.type = FREE_FALL;
                    } else if (strcmp(bombNames[i],"RKG-3") == 0) {
                        cfg.type = FREE_FALL;
                    } else if (strcmp(bombNames[i],"GLIDING-VOG") == 0) {
                        cfg.type = PLANE;
                    } else if (strcmp(bombNames[i],"GLIDING-RKG") == 0) {
                        cfg.type = PLANE;
                    }

                    return true;
                }
            }
            return false;
        }
    }

    struct targets_config {
        target_trajectory* trajectories;
        int count;
    };

    bool read_targets(targets_config& cfg, int target_count,int point_count,const char* filename) {
        std::ifstream input(filename);
        if (!input.is_open()) {
            std::cout << "read_targets: failed to open: " << strerror(errno) <<std::endl;
            return false;
        }

        cfg.trajectories = new target_trajectory[target_count];
        cfg.count = target_count;

        for (int i = 0; i < target_count; i++) {
            cfg.trajectories[i].points = new target_waypoint[point_count];
            cfg.trajectories[i].count = point_count;
            for (int j = 0; j < point_count; j++) {
                input >> cfg.trajectories[i].points[j].x;
                if (input.fail()) {
                    std::cout << "read_targets: failed to read x point: " << strerror(errno) <<std::endl;
                    return false;
                }
            }
            for (int j = 0; j < point_count; j++) {
                input >> cfg.trajectories[i].points[j].y;
                if (input.fail()) {
                    std::cout << "read_targets: failed to read y point: " << strerror(errno) <<std::endl;
                    return false;
                }
            }
        }
        return true;
    }

    bool read_target_trajectory(targets_config& cfg) {
        return read_targets(cfg,5,60,"targets.txt");
    }
} // namespace Config

namespace Ballistic {

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
    bool horizontal_distance_before_target(double Vo, double t, const ammo& ammo_cfg,double g, double& hdbts) {
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
    bool time_solution(double g, double attackSpeed,double height, const ammo& ammo_cfg, double& timeOfFlight) {
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

    bool calculate_drop_parameters(const drop_parameters& cfg, const ammo& ammo_cfg, vector2d &drop_pos,vector2d &maneuver_pos,bool &do_we_need_maneuver) {

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

} // namespace Ballistic

namespace IO {

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
}

struct target {
    int id;
    vector2d pos;
    vector2d velocity;
    vector2d prev_pos;
};

void simulate_target_movement(target& target, float t, Config::targets_config& targets_cfg,Config::task2_config& cfg) {
    if (targets_cfg.trajectories->count >= target.id || target.id < 0) {
        std::cout << "invalid target id. Id is out of range: " << target.id << std::endl;
        return;
    }
    
    target_trajectory& trajectory = targets_cfg.trajectories[target.id];

    int idx = (int)floor(t / cfg.arrayTimeStep) % trajectory.count;
    int next = (idx + 1) % trajectory.count;
    float frac = (t - idx * cfg.arrayTimeStep) / cfg.arrayTimeStep;

    if (idx >= trajectory.count || idx < 0 || next >= trajectory.count || next < 0) {
        std::cout << "invalid index. Index is out of range: " << idx << " or " << next << std::endl;
        return;
    }

    target.prev_pos = target.pos;
    target.velocity = (target.pos - target.prev_pos) / cfg.simTimeStep;

    target.pos = trajectory.points[idx] + (trajectory.points[next] - trajectory.points[idx]) * frac;
}

void simulation(Config::targets_config& targets_cfg,Config::task2_config& cfg) {
    int target_count = targets_cfg.count;
    target *targets = new target[target_count];

    for (int i = 0; i < target_count; i++) {
        targets[i].id = i;
        targets[i].pos = targets_cfg.trajectories[i].points[0];
        targets[i].prev_pos = targets[i].pos;
    }
    
    float t = 0;
    const int max_steps = 200;

    for (int step = 0; step < max_steps; step++) {
        for (int i = 0; i < target_count; i++) {
            simulate_target_movement(targets[i], t, targets_cfg, cfg);
        }

        t += cfg.simTimeStep;
    }

    delete[] targets;
}

class drone {
    vector2d pos;
    double height;
    double velocity;
    vector2d direction;

    double timeOfFlightBomb;
    double dropRadius;

    struct config {
        float hitRadius;
        float angularSpeed;
        float turnThreshold;
        float attackSpeed;
        const char* ammo_name;
        ammo ammo_cfg;
        float simTimeStep;
        float acceleration; // acceleration/deceleration ability (m/s^2)
    } config;


    enum state_t {
        // STOPPED, we don't need to stop the drone. As we have TURNING state.
        ACCELERATING,
        DECELERATING,
        TURNING,
        MOVING, // the same as STOPPED state.
    };

    drone(Config::task2_config& cfg) {
        this->pos = vector2d(cfg.xd, cfg.yd);
        this->height = cfg.zd;
        this->velocity = 0.0;
        this->direction = vector2d(cos(cfg.initialDir), sin(cfg.initialDir));
        ammo ammo_cfg;
        bool success = Config::ammo_configs::get_ammo_config(cfg.ammo_name,ammo_cfg);
        if (!success) {
            std::cout << "failed to get ammo: " << cfg.ammo_name << std::endl;
            return;
        }

        success = calulate_time_and_drop_radious(config.attackSpeed, height, config.ammo_cfg, timeOfFlightBomb, dropRadius);
        if (!success) {
            std::cout << "calulate_time_and_drop_radious: No solution" << std::endl;
            return;
        }

        this->config.hitRadius = cfg.hitRadius;
        this->config.angularSpeed = cfg.angularSpeed;
        this->config.turnThreshold = cfg.turnThreshold;
        this->config.attackSpeed = cfg.attackSpeed;
        this->config.ammo_name = cfg.ammo_name;
        this->config.ammo_cfg = ammo_cfg;
        this->config.simTimeStep = cfg.simTimeStep;
        this->config.acceleration = cfg.attackSpeed*cfg.attackSpeed/(2*cfg.accelerationPath);

    }

    state_t what_should_i_do_next(target *target,double &my_angle,double &pursuit_angle) {
        pursuit_angle = get_pursuit_angle(target, my_angle);
        if (abs(pursuit_angle) < config.turnThreshold) {
            if (velocity >= config.attackSpeed) {
                return MOVING;
            }
            return ACCELERATING;
        }

        if (velocity <= 0.0f) {
            return TURNING;
        }

        return DECELERATING;
    }

    double calculate_time_to_reach_point(vector2d target_point, vector2d target_velocity) {
        vector2d drone_to_target_vector = (target_point - this->pos);
        double distance_to_target = (drone_to_target_vector).length();

        double angle_to_target = atan2(drone_to_target_vector.y, drone_to_target_vector.x);
        double angle_difference = angle_to_target - atan2(this->direction.y, this->direction.x);

        double time_to_target;
        if (abs(angle_difference) > config.turnThreshold) {
            // need to get decceleration & acceleration time.
            double deceleration_time = velocity / config.acceleration;
            vector2d decelerated_point = this->direction*config.acceleration*deceleration_time*deceleration_time/2+pos;

            vector2d target_pos_in_deceleration_time = target_point + target_velocity*deceleration_time;

            double acceleration_time = (config.attackSpeed) / config.acceleration; // or sqrt(2*config.accelerationPath/config.acceleration)

            vector2d target_pos_in_acceleration_time = target_pos_in_deceleration_time + target_velocity*acceleration_time;

            vector2d acceleration_point = pos + direction*config.acceleration*acceleration_time*acceleration_time/2;

            double time_of_linear_movement = 0;

            if ((target_pos_in_acceleration_time-decelerated_point).length()>(acceleration_point-decelerated_point).length()) {
                // our target far than acceleration path.
                time_of_linear_movement = (target_pos_in_acceleration_time-acceleration_point).length() / config.attackSpeed;
            }

            time_to_target = deceleration_time + acceleration_time + time_of_linear_movement;
        } else {
            double time_to_acceleration = (config.attackSpeed - velocity) / config.acceleration;
            double distance_to_acceleration = velocity*time_to_acceleration+config.acceleration*time_to_acceleration*time_to_acceleration/2;
            
            if (distance_to_acceleration > distance_to_target) {
                time_to_target = time_to_acceleration;
            } else {
                double linear_movement_time = (distance_to_target-distance_to_acceleration) / config.attackSpeed;
                time_to_target = time_to_acceleration + linear_movement_time;
            }
        }
        return time_to_target;
    }

    inline int get_pursuit_target(target *targets, int target_count,double &nearest_time_to_target,vector2d &nearest_pursuit_pos) {
        nearest_time_to_target = std::numeric_limits<double>::max();
        int nearest_target_index = -1;
        for (int i = 0; i < target_count; i++) {

            vector2d pursuit_pos = targets[i].pos;

            vector2d target_velocity = (targets[i].prev_pos - targets[i].pos) / config.simTimeStep;

            double time_to_target = 0;
            
            for (int j = 0; j < 10; j++) { // iterative calculation of the target position, as we don't know real time to target.
                time_to_target = calculate_time_to_reach_point(pursuit_pos, target_velocity);
                pursuit_pos = targets[i].pos + target_velocity * time_to_target;
            }

            if (time_to_target < nearest_time_to_target) {
                nearest_time_to_target = nearest_time_to_target;
                nearest_pursuit_pos = pursuit_pos;
                nearest_target_index = i;
            }
        }
        return nearest_target_index;
    }

    bool calulate_time_and_drop_radious(double attackSpeed, double drone_height, const ammo& ammo_cfg,double &time,double &drop_radius) {
        bool success = Ballistic::time_solution(9.81, attackSpeed, drone_height, ammo_cfg, time);
        if (!success) {
            std::cout << "time_solution: No solution" << std::endl;
            return false;
        }

        // horizontal distance between drop point and target:
        double hdbts;
        success = Ballistic::horizontal_distance_before_target(attackSpeed, time, ammo_cfg, 9.81, drop_radius);
        if (!success) {
            std::cout << "horizontal_distance_before_target: No solution" << std::endl;
            return false;
        }
        return true;
    }

    inline double get_pursuit_angle(target *target,double &my_angle) {
        vector2d target_to_drone_vector = (target->pos - pos);
        my_angle = atan2(direction.y, direction.x);

        double target_angle = atan2(target_to_drone_vector.y, target_to_drone_vector.x);

        double angle_difference = target_angle - my_angle;

        while (angle_difference >  M_PI) angle_difference -= 2.0 * M_PI;
        while (angle_difference < -M_PI) angle_difference += 2.0 * M_PI;

        return angle_difference;
    }

    inline bool relentness_pursuit(target *target) {
        double my_angle;
        double angle = get_pursuit_angle(target, my_angle);
        if (abs(angle) > config.turnThreshold) {
            return false;
        }

        angle += my_angle;

        direction = direction.rotate(angle);
        return true;
    }

    double limit_angle_rotation(double angle_difference,double max_angle_to_rotate) {
        double angle_to_rotate;
        if (angle_difference > 0) {
            angle_to_rotate = std::min(angle_difference, max_angle_to_rotate);
        } else {
            angle_to_rotate = -std::min(-angle_difference, max_angle_to_rotate);
        }
        return angle_to_rotate;
    }

    void make_step(state_t next_state,double pursuit_angle,double my_angle) {
        double max_angle_to_rotate = config.turnThreshold;
        switch (next_state) {
            case ACCELERATING:
                velocity += config.acceleration * config.simTimeStep;
                if (velocity > config.attackSpeed) {
                    velocity = config.attackSpeed;
                }
                break;
            case DECELERATING:
                velocity -= config.acceleration * config.simTimeStep;
                if (velocity < 0 ){
                    velocity = 0;
                }
                break;
            case TURNING: 
                max_angle_to_rotate = config.angularSpeed * config.simTimeStep;
                break;
        }
        double angle_to_rotate = limit_angle_rotation(pursuit_angle-my_angle, max_angle_to_rotate);
        direction = direction.rotate(angle_to_rotate);
        pos = pos + direction.normalized() * velocity * config.simTimeStep;
    }

    bool step(float t,target *targets,int target_count) {
        double my_angle;
        double pursuit_angle;

        target *target = choose_nearest_target(targets, target_count);

        state_t next_state = what_should_i_do_next(target, my_angle, pursuit_angle);

        make_step(next_state,pursuit_angle,my_angle);

        return can_we_drop_the_bomb_on_target(target);
    }
    
    bool can_we_drop_the_bomb_on_target(target *target) {
        if (velocity < config.attackSpeed) {
            return false;
        }

        vector2d hit_pos = pos + direction * config.attackSpeed * config.simTimeStep;

        double distance = (hit_pos - target->pos).length();
        if (distance <= config.hitRadius) {
            return true;
        }
        return false;
    }
};

int main() {
    Config::task2_config cfg{};
    if (!Config::read_task2_config(cfg)) {
        return 1;
    }

    ammo ammo_cfg;

    bool success = Config::ammo_configs::get_ammo_config(cfg.ammo_name,ammo_cfg);
    if (success == false) {
        std::cout << "failed to get ammo: " << cfg.ammo_name << std::endl;
        return 1;
    }

    double time_of_flight;
    success = Ballistic::time_solution(9.81,cfg.attackSpeed,cfg.zd,ammo_cfg,time_of_flight);
    if (success == false) {
        std::cout << "failed to get time solution" << std::endl;
        return 1;  
    }

    Config::targets_config targets_cfg{};
    if (!Config::read_target_trajectory(targets_cfg)) {
        return 1;
    }

    // Keep runtime bounded: simulate a short horizon.
    simulation(targets_cfg, cfg);
    return 0;
}


