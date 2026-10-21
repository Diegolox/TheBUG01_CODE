#pragma once

struct DatosBNO055 {
    float rumbo;
    float roll;
    float pitch;
    float accX;
    float accY;
    float accZ;
};

bool initIMU();
DatosBNO055 leerIMU();

