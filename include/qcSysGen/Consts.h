/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2021-2024 Questionable Coding
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
****************************************************************************/
#pragma once

#include <cstdint>

namespace qc
{

namespace SystemGenerator
{

//--- Version

/// @brief Major version of qcSystemGenerator
static constexpr uint32_t SysGenMajor = 0u;

/// @brief Minor version of qcSystemGenerator
static constexpr uint32_t SysGenMinor = 9u;

/// @brief Patch version of qcSystemGenerator
static constexpr uint32_t SysGenPatch = 0u;

/// @brief qcSystemGenerator version
static constexpr uint32_t SysGenVersion = (SysGenMajor << 22u) | (SysGenMinor << 12u) | (SysGenPatch);


//--- Reference Values

/// @brief Mean surface pressure of Earth's atmosphere, in millibars.
static constexpr float EarthSurfacePressureMb = 1013.25f;

/// @brief The gravity constant, in dyne (cm^2)/(gram^2).
static constexpr double GravityConstant = 6.672e-8;

///@brief The ideal gas constant.
static constexpr double MolarGasConstant = 8314.41;

/// @brief Pi
static constexpr double PI = 3.1415926535897932384626433832795;

/// @brief Number of radians in a circle.
static constexpr double RadiansPerCircle = 2.0 * PI;

/// @brief Mass of Sol, in grams.
static constexpr double SolarMassInGrams = 1.989e33;

/// @brief Radius of Sol, in km.
static constexpr double SolarRadiusKm = 695700.0;


//--- Conversion Factors

/// @brief Convert m/s^2 -> G
static constexpr double AccelerationInGees = 1.0 / 9.807;

/// @brief Convert mb -> atm
static constexpr float AtmPerMb = 1.0f / EarthSurfacePressureMb;

/// @brief Convert km -> AU
static constexpr double AuPerKm = 6.6845871222684454959959533702106e-9;

/// @brief Convert mb -> b
static constexpr float BarPerMillibar = 0.001f;

/// @brief Convert km -> cm
static constexpr double CmPerKm = 1000.0 * 100.0;

/// @brief Convert m -> cm
static constexpr double CmPerM = 100.0;

/// @brief Convert Earth years -> Earth days
static constexpr double DaysPerYear = 365.256;

/// @brief Convert sidereal earth day -> hours
static constexpr double HoursPerDay = 23.9344696;

/// @brief Conversion addend for Kelvin -> Celsius
static constexpr float KelvinToCelsius = -273.15f;

/// @brief Convert AU -> km
static constexpr double KmPerAu = 1.0 / AuPerKm;

/// @brief Convert cm -> km
static constexpr double KmPerCm = (1.0 / (1000.0 * 100.0));

/// @brief Convert mmHg -> mb
static constexpr float MbPerMmhg = EarthSurfacePressureMb / 760.0f;

/// @brief Convert m -> cm
static constexpr double MPerCm = 0.01;

/// @brief Convert m -> km
static constexpr double MPerKm = 1000.0;

/// @brief Convert hours -> seconds
static constexpr double SecondsPerHour = 60.0 * 60.0;

/// @brief Convert solar mass -> earth mass.
static constexpr double SolarMassToEarthMass = 332775.64;

/// @brief Convert solar mass -> Jupiter mass.
static constexpr double SolarMassToJovianMass = 1047.0;

/// @brief Convert seconds -> years.
static constexpr double YearsPerSecond = 1.0 / (SecondsPerHour * HoursPerDay * DaysPerYear);

}
}
