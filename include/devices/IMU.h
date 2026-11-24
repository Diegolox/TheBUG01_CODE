#pragma once

struct DatosBNO055 {
    float rumbo;
    float roll;
    float pitch;
    float accX;
    float accY;
    float accZ;
};

struct GiroBNO055 {
    float x;
    float y;
    float z;
};

bool initIMU(bool modoInercial = false);
DatosBNO055 leerIMU();
GiroBNO055 leerGiroIMU();

// Devuelve false si falla la lectura I2C. Unidades: grados y grados/s.
bool leerBalanceIMU(float &rollDeg, float &giroYDegPorSegundo);
