/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2021-2023 Questionable Coding
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

// Non-specific constants
// 
// TODO: Document where these values come from.
// TODO: Organize by where they're used.

/// @brief Major version of qcSystemGenerator
static constexpr uint32_t SysGenMajor = 1u;

/// @brief Minor version of qcSystemGenerator
static constexpr uint32_t SysGenMinor = 0u;

/// @brief Patch version of qcSystemGenerator
static constexpr uint32_t SysGenPatch = 0u;

/// @brief qcSystemGenerator version
static constexpr uint32_t SysGenVersion = (SysGenMajor << 22u) | (SysGenMinor << 12u) | (SysGenPatch);

/// @brief Albedo of clouds.
static constexpr float Albedo_Cloud = 0.52f;

/// @brief Albedo of the Earth.
static constexpr float Albedo_Earth = 0.3f;

/// @brief The mean albedo of our solar system's 4 gaseous worlds.
static constexpr float Albedo_GasGiant = 0.492f;

/// @brief Albedo of ice in an atmosphere.
static constexpr float Albedo_Ice = 0.7f;

/// @brief Albedo of ice outside of an atmosphere.
static constexpr float Albedo_IceAirless = 0.4f;

/// @brief Albedo of rock in an atmosphere.
static constexpr float Albedo_Rock = 0.15f;

/// @brief Albedo of rock outside of an atmosphere.
static constexpr float Albedo_RockAirless = 0.07f;

/// @brief Albedo of liquid water.
static constexpr float Albedo_Water = 0.04f;

/// @brief Convert acceleration in m/s^2 to G.
static constexpr double AccelerationInGees = 1.0 / 9.807;

/// @brief Upper limit for a planet to be considered an asteroid belt instead, in Earth masses.
static constexpr double AsteroidMassLimit = 0.001;

/// @brief Convert km -> AU
static constexpr double AuPerKm = 6.6845871222684454959959533702106e-9;

/// @brief Convert mb -> b
static constexpr float BarPerMillibar = 0.001f;

/// @brief The value raised by the orbit number exponent when evaluating Bode's Law.
///
/// In the original formulation, this value was 2.  Blagg 1913 proposed the more
/// precise value of 1.7275.
static constexpr float BodeProgression = 1.7275f;

/// @brief Transition from Jovian gas giant to brown dwarf, in Jovian mass.
static constexpr double BrownDwarfTransition = 13.0;

/// @brief Change in Earth's angular velocity, in radians/sec per year.
static constexpr double ChangeInEarthAngularVelocity = -1.3e-15;

/// @brief Convert km -> cm.
static constexpr double CmPerKm = 1000.0 * 100.0;

/// @brief Convrt cm -> m.
static constexpr double CmPerM = 100.0;

/// @brief Convert Earth years to Earth days
static constexpr double DaysPerYear = 365.256;

/// @brief Mean surface temperature of the Earth, in Kelvin.
///
/// This is composed of the frezing point of water (K) + mean earth temperature (C).
static constexpr float EarthAverageTemperature = 273.15f + 14.0f;

/// @brief Earth's axial tilt, in degrees.
static constexpr double EarthAxialTilt = 23.4;

/// @brief Density of the Earth in g/cc.
static constexpr double EarthDensity = 5.52;

/// @brief Escape velocity of Earth, in m/s.
static constexpr float EarthEscapeVelocity = 11186.0f;

/// @brief Effective temperature of the earth, in Kelvin.  Fogg originally used 255.
static constexpr double EarthEffectiveTemperature = 250.0;

/// @brief Temperature of Earth's exosphere, in Kelvin.
static constexpr double EarthExosphereTemperature = 1273.0;

/// @brief Percentage of Earth's surface covered with liquid water.
static constexpr double EarthHydrosphere = 0.708;

/// @brief Mass of the Earth in grams.
static constexpr double EarthMassInGrams = 5.977e27;

/// @brief Radius of the Earth in km.
static constexpr double EarthRadiusKm = 6378.0;

/// @brief Mean surface pressure of Earth's atmosphere, in millibars.
static constexpr double EarthSurfacePressure = 1013.25;

/// @brief Convert mb -> atm
static constexpr double AtmPerMb = 1.0 / EarthSurfacePressure;

/// @brief Partial pressure of oxygen at sea level, in millibars.
static constexpr float EarthPartialPressureOxygen = static_cast<float>(EarthSurfacePressure * 0.2095);

/// @brief Mass of water per square kilometer, in grams.
static constexpr double EarthWaterMassPerKm2 = 3.83e15;

/// @brief The freezing point of water at 1 atm, in Kelvin.
static constexpr double FreezingPointWater = 273.15;

/// @brief The ratio of RMS velocity to escape velocity.
///
/// The original accrete used 5.0.  Stargen used 6.0.  I don't know why the change.
static constexpr double GasRetentionThreshold = 5.0;

/// @brief The ratio of escape velocity to RMS velocity.
static constexpr double EscapeToRmsVelocity = 1.0 / GasRetentionThreshold;

/// @brief The gravity constant, in dyne (cm^2)/(gram^2).
static constexpr double GravityConstant = 6.672e-8;

/// @brief Center of the transition from Ice Giant to Gas Giant, in Jovian mass.
static constexpr double IceGiantTransition = 0.414;

/// @brief Conversion addend for Kelvin -> Celsius
static constexpr float KelvinToCelsius = -273.15f;

/// @brief Convert AU -> km
static constexpr double KmPerAu = 1.0 / AuPerKm;

/// @brief Conversion factor cm -> km.
static constexpr double KmPerCm = (1.0 / (1000.0 * 100.0));

/// @brief Convert mmHg -> mb.
static constexpr float MbPerMmhg = static_cast<float>(EarthSurfacePressure / 760.0);

///@brief The ideal gas constant.
static constexpr double MolarGasConstant = 8314.41;

/// @brief Convert m -> cm.
static constexpr double MPerCm = 0.01;

/// @brief Convert m -> km.
static constexpr double MPerKm = 1000.0;

/// @brief Pi
static constexpr double PI = 3.1415926535897932384626433832795;

/// @brief Number of radians in a circle.
static constexpr double RadiansPerCircle = 2.0 * PI;

/// @brief Convert hours -> seconds
static constexpr double SecondsPerHour = 60.0 * 60.0;

/// @brief Convert solar mass to earth mass.
static constexpr double SolarMassToEarthMass = 332775.64;

/// @brief Convert solar mass to Jupiter mass.
/// @todo Refine this value?
static constexpr double SolarMassToJovianMass = 1.0 / 1047.0;

/// @brief Mass of Sol, in grams.
static constexpr double SolarMassInGrams = 1.989e33;

/// @brief Center of the transition point from rocky world to gaseous world, in Solar masses.
///
/// From Chen, et al. 2017.  Original value is 2.04 +0.66/-0.59 M(Earth).
static constexpr double RockyTransition = 2.04 / SolarMassToEarthMass;

/// @brief Convert seconds -> years.  Currently uses 24.0 hours per day.
static constexpr double YearsPerSecond = 1.0 / (SecondsPerHour * 24.0 * DaysPerYear);

/// @brief Weight of molecular hydrogen.
static constexpr double Weight_MolecularHydrogen = 2.0;

/// @brief Weight of helium.
static constexpr double Weight_Helium = 4.0;

/// @brief Weight of water.
static constexpr double Weight_WaterVapor = 18.0;

/// @brief Weight of molecular nitrogen.
static constexpr double Weight_MolecularNitrogen = 28.0;
