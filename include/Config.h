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

/// @brief Configuration / override parameter structure.
///
/// When this structure is instantiated, all values are set to defaults.  Any of them may be overridden prior to calling
/// System::create().  Look at the documentations for the member values for explanations of their use.
struct Config
{
/// @brief Mean eccentricity of the nebular dust.
    static constexpr double CloudEccentricity = 0.2;

    ///@brief A in Dole's paper
    static constexpr double DustDensity = 2.0e-3;

    /// @brief Mass of protoplanet seed.  Each protoplanet that is spawned during system creation starts with this mass.  In solar mass.
    static constexpr double ProtoplanetSeedMass = 1.0e-15;

    /// @brief Initialize configuration values to their defaults.
    Config() :
        seed(0),
        overrideOuterPlanetLimit(0.0),
        cloudEccentricity(CloudEccentricity),
        dustDensity(DustDensity),
        protoplanetSeedMass(ProtoplanetSeedMass),
        stellarMass(0.0f),
        computeGases(false),
        generateBodeSeeds(false),
        generateMoons(false),
        randomAxialTilt(false)
    { }

    uint64_t seed; //!< RNG seed.  When 0, a seed is generated based on current system time.
    
    /// @brief Override for the maximum planet formation distance.
    ///
    /// If positive, and less than Star::outerPlanetaryLimit(), this override is used.  Otherwise, it is ignored.
    double overrideOuterPlanetLimit;
    double cloudEccentricity; //!< The mean eccentricity of dust in the planetary dust cloud.  Value will be clamped to [0, 0.99].
    double dustDensity; //!< Density of the planetary dust cloud.
    double protoplanetSeedMass; //!< The mass of a protoplanet seed.  This is the initial mass that is used to accumulate dust and gas into a planet.

    /// @brief The mass of the central star, in Solar masses.
    ///
    /// A value of 0 or a negative value will trigger a random stellar mass in the range of 0.6 (a K9V) to 1.3 (an F5V).
    /// 
    /// Currently, masses from 0.57 (M0V) to 2.8 (A0V) are supported, but masses hotter than roughly 1.3 (F5V) may not
    /// provide good results.  Values outside the supported range will be clamped.
    float stellarMass;

    bool computeGases; //!< When true, synthesize an atmospheric composition.
    bool generateBodeSeeds; //!< When true, planetisimal seeds based on the Blagg modification of Bode's Law will be used to initialize the planetary system.
    bool generateMoons; //!< When true, planetisimal collisions may result in moon capture instead of always merging planetisimals.
    bool randomAxialTilt; //!< When true, axial tilt is randomly computed; when false, all planets and moons have a 0 tilt.
};
