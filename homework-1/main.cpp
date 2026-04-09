#include <iostream>
#include <fstream>
#include <string>

struct config {
    float xd, yd, zd;
    float targetX, targetY;
    float attackSpeed;
    float accelerationPath;
    char ammo_name[10];
};

bool read_config(config& cfg) {
    std::ifstream input("input.txt");
    input >> cfg.xd >> cfg.yd >> cfg.zd >> cfg.targetX >> cfg.targetY >> cfg.attackSpeed >> cfg.accelerationPath >> cfg.ammo_name;
    input.close();
    return true;
}

void print_config(const config& cfg) {
    std::cout << "xd: " << cfg.xd << std::endl;
    std::cout << "yd: " << cfg.yd << std::endl;
    std::cout << "zd: " << cfg.zd << std::endl;
    std::cout << "targetX: " << cfg.targetX << std::endl;
    std::cout << "targetY: " << cfg.targetY << std::endl;
    std::cout << "attackSpeed: " << cfg.attackSpeed << std::endl;
    std::cout << "accelerationPath: " << cfg.accelerationPath << std::endl;
    std::cout << "ammo_name: " << cfg.ammo_name << std::endl;
}

int main() {
    config cfg;
    read_config(cfg);

    // print the input
    print_config(cfg);

    return 0;
}

