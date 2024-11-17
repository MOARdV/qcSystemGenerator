/**
 StellarInfo.h: auto-generated file containing the needed entries from
 http://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt

 Table entries for O0V - O2V are cloned from O3V to provide simpler indexing

 This file is intended to be included within the qc::SystemGenerator namespace, and
 not used externally from the Star class.
**/

#pragma once

struct StellarInfo_t
{
    float logT; //!< log(10) of effective temperature, Kelvin.
    float logL; //!< log(10) of luminance, Sol = 0.0.
    float radius; //!< Stellar radius, in Solar radii.
    float mass; //!< Stellar mass, in Solar masses.
};

static constexpr StellarInfo_t stellarInfo[] =
{
    { 4.652f,  5.82f, 13.430f, 59.000f }, // O0V
    { 4.652f,  5.82f, 13.430f, 59.000f }, // O1V
    { 4.652f,  5.82f, 13.430f, 59.000f }, // O2V
    { 4.652f,  5.82f, 13.430f, 59.000f }, // O3V
    { 4.632f,  5.65f, 12.130f, 48.000f }, // O4V
    { 4.617f,  5.54f, 11.450f, 43.000f }, // O5V
    { 4.597f,  5.36f, 10.270f, 35.000f }, // O6V
    { 4.569f,  5.18f,  9.420f, 28.000f }, // O7V
    { 4.545f,  4.99f,  8.470f, 23.600f }, // O8V
    { 4.522f,  4.82f,  7.720f, 20.200f }, // O9V
    { 4.497f,  4.65f,  7.160f, 17.700f }, // B0V
    { 4.415f,  4.13f,  5.710f, 11.800f }, // B1V
    { 4.314f,  3.43f,  4.060f,  7.300f }, // B2V
    { 4.230f,  2.99f,  3.610f,  5.400f }, // B3V
    { 4.215f,  2.89f,  3.460f,  5.100f }, // B4V
    { 4.196f,  2.77f,  3.360f,  4.700f }, // B5V
    { 4.161f,  2.57f,  3.270f,  4.300f }, // B6V
    { 4.146f,  2.48f,  2.940f,  3.920f }, // B7V
    { 4.090f,  2.19f,  2.860f,  3.380f }, // B8V
    { 4.029f,  1.86f,  2.490f,  2.750f }, // B9V
    { 3.987f,  1.58f,  2.193f,  2.180f }, // A0V
    { 3.968f,  1.49f,  2.136f,  2.050f }, // A1V
    { 3.944f,  1.38f,  2.117f,  1.980f }, // A2V
    { 3.934f,  1.23f,  1.861f,  1.860f }, // A3V
    { 3.917f,  1.13f,  1.794f,  1.930f }, // A4V
    { 3.908f,  1.09f,  1.785f,  1.880f }, // A5V
    { 3.898f,  1.05f,  1.775f,  1.830f }, // A6V
    { 3.890f,  1.00f,  1.750f,  1.770f }, // A7V
    { 3.880f,  0.96f,  1.747f,  1.810f }, // A8V
    { 3.869f,  0.92f,  1.747f,  1.750f }, // A9V
    { 3.859f,  0.86f,  1.728f,  1.610f }, // F0V
    { 3.846f,  0.79f,  1.679f,  1.500f }, // F1V
    { 3.834f,  0.71f,  1.622f,  1.460f }, // F2V
    { 3.829f,  0.67f,  1.578f,  1.440f }, // F3V
    { 3.824f,  0.62f,  1.533f,  1.380f }, // F4V
    { 3.816f,  0.56f,  1.473f,  1.330f }, // F5V
    { 3.803f,  0.43f,  1.359f,  1.250f }, // F6V
    { 3.798f,  0.39f,  1.324f,  1.210f }, // F7V
    { 3.791f,  0.29f,  1.221f,  1.180f }, // F8V
    { 3.782f,  0.22f,  1.167f,  1.130f }, // F9V
    { 3.773f,  0.13f,  1.100f,  1.060f }, // G0V
    { 3.768f,  0.08f,  1.060f,  1.030f }, // G1V
    { 3.761f,  0.01f,  1.012f,  1.000f }, // G2V
    { 3.757f, -0.01f,  1.002f,  0.990f }, // G3V
    { 3.754f, -0.04f,  0.991f,  0.985f }, // G4V
    { 3.753f, -0.05f,  0.977f,  0.980f }, // G5V
    { 3.748f, -0.10f,  0.949f,  0.970f }, // G6V
    { 3.744f, -0.13f,  0.927f,  0.950f }, // G7V
    { 3.739f, -0.17f,  0.914f,  0.940f }, // G8V
    { 3.731f, -0.26f,  0.853f,  0.900f }, // G9V
    { 3.723f, -0.34f,  0.813f,  0.880f }, // K0V
    { 3.713f, -0.39f,  0.797f,  0.860f }, // K1V
    { 3.708f, -0.43f,  0.783f,  0.820f }, // K2V
    { 3.684f, -0.55f,  0.755f,  0.780f }, // K3V
    { 3.663f, -0.69f,  0.713f,  0.730f }, // K4V
    { 3.647f, -0.76f,  0.701f,  0.700f }, // K5V
    { 3.633f, -0.86f,  0.669f,  0.690f }, // K6V
    { 3.613f, -1.00f,  0.630f,  0.640f }, // K7V
    { 3.601f, -1.06f,  0.615f,  0.620f }, // K8V
    { 3.594f, -1.10f,  0.608f,  0.590f }, // K9V
    { 3.585f, -1.16f,  0.588f,  0.570f }, // M0V
    { 3.563f, -1.39f,  0.501f,  0.500f }, // M1V
    { 3.551f, -1.54f,  0.446f,  0.440f }, // M2V
    { 3.535f, -1.79f,  0.361f,  0.370f }, // M3V
    { 3.507f, -2.14f,  0.274f,  0.230f }, // M4V
    { 3.486f, -2.52f,  0.196f,  0.162f }, // M5V
    { 3.449f, -2.98f,  0.137f,  0.102f }, // M6V
    { 3.428f, -3.19f,  0.120f,  0.090f }, // M7V
    { 3.410f, -3.28f,  0.114f,  0.085f }, // M8V
    { 3.377f, -3.52f,  0.102f,  0.079f }, // M9V
};

static constexpr size_t stellarInfoCount = _countof(stellarInfo);
