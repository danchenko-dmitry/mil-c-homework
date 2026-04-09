#include <asm-generic/errno.h>
#include <iostream>
#include <fstream>
#include <string.h>

struct target_config {
    float xd, yd, zd;
    float targetX, targetY;
    float attackSpeed;
    float accelerationPath;
    char ammo_name[10];
};

bool read_target_config(target_config& cfg) {
    std::ifstream input("input.txt");
    input >> cfg.xd >> cfg.yd >> cfg.zd >> cfg.targetX >> cfg.targetY >> cfg.attackSpeed >> cfg.accelerationPath >> cfg.ammo_name;
    input.close();
    return true;
}

void print_target_config(const target_config& cfg) {
    std::cout << "xd: " << cfg.xd << std::endl;
    std::cout << "yd: " << cfg.yd << std::endl;
    std::cout << "zd: " << cfg.zd << std::endl;
    std::cout << "targetX: " << cfg.targetX << std::endl;
    std::cout << "targetY: " << cfg.targetY << std::endl;
    std::cout << "attackSpeed: " << cfg.attackSpeed << std::endl;
    std::cout << "accelerationPath: " << cfg.accelerationPath << std::endl;
    std::cout << "ammo_name: " << cfg.ammo_name << std::endl;
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

void print_ammo_config(const ammo_config& cfg) {
    std::cout << "name: " << cfg.name << std::endl;
    std::cout << "m: " << cfg.m << std::endl;
    std::cout << "d: " << cfg.d << std::endl;
    std::cout << "l: " << cfg.l << std::endl;
    std::cout << "type: " << cfg.type << std::endl;
}

int main() {
    target_config cfg;
    read_target_config(cfg);

    // print the input
    print_target_config(cfg);

    // detect the ammo config
    ammo_config ammo_cfg;
    detect_ammo_config(cfg.ammo_name, ammo_cfg);    

    // print the ammo config
    print_ammo_config(ammo_cfg);

    return 0;
}

