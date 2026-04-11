#include "../homework1_types.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

static int g_test_failures = 0;

namespace {

void record_fail(const char* msg) {
    std::cerr << "FAIL: " << msg << '\n';
    ++g_test_failures;
}

bool near_eq(double a, double b, double eps) { return std::fabs(a - b) <= eps; }

void expect_vec(const char* ctx, const vector2d& v, double ex, double ey, double eps) {
    if (!near_eq(v.x, ex, eps) || !near_eq(v.y, ey, eps)) {
        std::cerr << "FAIL " << ctx << " got (" << v.x << ", " << v.y << ") expected (" << ex << ", " << ey
                  << ")\n";
        ++g_test_failures;
    }
}

void expect_maneuver(const char* ctx, bool got, bool want) {
    if (got != want) {
        std::cerr << "FAIL " << ctx << " maneuver flag got " << got << " want " << want << '\n';
        ++g_test_failures;
    }
}

target_config base_cfg() {
    target_config c{};
    c.xd = 100;
    c.yd = 100;
    c.zd = 100;
    c.targetX = 200;
    c.targetY = 200;
    c.attackSpeed = 10;
    c.accelerationPath = 10;
    std::memset(c.ammo_name, 0, sizeof(c.ammo_name));
    std::strncpy(c.ammo_name, "VOG-17", sizeof(c.ammo_name) - 1);
    return c;
}

} // namespace

void generate_flight_function_visuzualization(const target_config& cfg, const ammo_config& ammo, const char* filename, double dt);

void test_calculate_drop_parameters() {
    g_test_failures = 0;
    static constexpr double eps = 0.02;

    // VOG-17, default geometry (golden values from reference run)
    {
        target_config cfg = base_cfg();
        ammo_config ammo{};
        if (!detect_ammo_config("VOG-17", ammo)) {
            record_fail("detect VOG-17");
        } else {
            vector2d drop{};
            vector2d maneuver{};
            bool need = false;
            if (!calculate_drop_parameters(cfg, ammo, drop, maneuver, need)) {
                record_fail("calculate_drop_parameters VOG-17 should succeed");
            } else {
                expect_vec("VOG-17 drop", drop, 173.759, 173.759, eps);
                expect_maneuver("VOG-17", need, false);
            }

            generate_flight_function_visuzualization(cfg, ammo, "VOG-17_trajectory", 0.1);
        }
    }

    // GLIDING-VOG — lift changes horizontal travel
    {
        target_config cfg = base_cfg();
        ammo_config ammo{};
        if (!detect_ammo_config("GLIDING-VOG", ammo)) {
            record_fail("detect GLIDING-VOG");
        } else {
            vector2d drop{};
            vector2d maneuver{};
            bool need = false;
            if (!calculate_drop_parameters(cfg, ammo, drop, maneuver, need)) {
                record_fail("calculate_drop_parameters GLIDING-VOG should succeed");
            } else {
                expect_vec("GLIDING-VOG drop", drop, 157.656, 157.656, eps);
                expect_maneuver("GLIDING-VOG", need, false);
            }
            generate_flight_function_visuzualization(cfg, ammo, "GLIDING-VOG_trajectory", 0.1);
        }
    }

    // Long acceleration path — maneuver required (golden from reference run)
    {
        target_config cfg = base_cfg();
        cfg.accelerationPath = 150.f;
        ammo_config ammo{};
        if (!detect_ammo_config("VOG-17", ammo)) {
            record_fail("detect VOG-17 (maneuver case)");
        } else {
            vector2d drop{};
            vector2d maneuver{};
            bool need = false;
            if (!calculate_drop_parameters(cfg, ammo, drop, maneuver, need)) {
                record_fail("calculate_drop_parameters long accel should succeed");
            } else {
                expect_maneuver("long accel", need, true);
                expect_vec("long accel maneuver", maneuver, 67.6931, 67.6931, eps);
                expect_vec("long accel drop", drop, 173.759, 173.759, eps);
            }
            generate_flight_function_visuzualization(cfg, ammo, "VOG-17_long_accel_trajectory", 0.1);
        }
    }
}

void generate_flight_function_visuzualization(const target_config& cfg, const ammo_config& ammo, const char* filename, double dt) {
    vector2d drop;
    vector2d maneuver;
    bool need;
    bool success = calculate_drop_parameters(cfg, ammo, drop, maneuver, need);
    if (!success) {
        std::cerr << "calculate_drop_parameters failed" << std::endl;
        return;
    }

    json j;
    j["cfg"]= json::object({
        {"xd", cfg.xd},
        {"yd", cfg.yd},
        {"zd", cfg.zd},
        {"targetX", cfg.targetX},
        {"targetY", cfg.targetY},
        {"attackSpeed", cfg.attackSpeed},
        {"accelerationPath", cfg.accelerationPath},
        {"name", ammo.name}
    });
    j["ammo_config"] = json::object({
        {"name", ammo.name},
        {"m", ammo.m},
        {"d", ammo.d},
        {"l", ammo.l},
        {"type", ammo.type}
    });
    constexpr double ground_z = 0.0;
    j["result"] = json::object({
        {"drop", json::object({{"x", drop.x}, {"y", drop.y}, {"z", ground_z}})},
        {"maneuver", json::object({{"x", maneuver.x}, {"y", maneuver.y}, {"z", ground_z}})},
        {"need", need},
    });

    const double g = 9.81;
    const double acoef = a_koef(ammo.d, g, ammo.m, ammo.l, cfg.attackSpeed);
    const double bcoef = b_koef(g, ammo.m, ammo.d, ammo.l, cfg.attackSpeed);
    const double ccoef = c_koef(ammo.m, cfg.zd);

    std::cout << "Generating trajectory for " << filename << std::endl;

    vector2d direction = (vector2d(cfg.targetX, cfg.targetY)-drop).normalized();

    std::cout << "a_koef: " << acoef << std::endl;
    std::cout << "b_koef: " << bcoef << std::endl;
    std::cout << "c_koef: " << ccoef << std::endl;
    double t = 0;
    for (int i = 0; i < 2000; i++) {
        double Z = flight_vertical_function(t, acoef, bcoef, ccoef);
        vector2d current_pos = flight_horizontal_function(t, drop, direction, cfg.attackSpeed);
        j["trajectory"].push_back(json::object({{"x", current_pos.x}, {"y", current_pos.y}, {"z", Z}}));
        if (Z <= 0) {
            // trajectory is over the ground, stop
            break;
        }
        if (i % 1 == 0) {
            std::cout << "t: " << t << " Z: " << Z << " current_pos: " << current_pos.x << ", " << current_pos.y << std::endl;
        }
        t += dt;
    }

    std::string json_name = std::string(filename) + ".json";
    std::ofstream jout(json_name);
    if (jout) {
        std::cout << "Writing trajectory to " << json_name << std::endl;
        jout << j.dump(4);
        jout.close();
    }
}

int main() {
    test_calculate_drop_parameters();
    if (g_test_failures != 0) {
        std::cerr << g_test_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All calculate_drop_parameters tests passed.\n";
    return 0;
}
