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

#include "Config.h"
#include "Consts.h"
#include "Planet.h"
#include "Star.h"

#include <random>

namespace qc
{

namespace SystemGenerator
{

class GenerationState;

/// @brief Defines an optional seed value for generating a solar system.
///
/// The values are {radius, eccentricity}, where radius is the semi-major axis in
/// AU, and eccentricity is in the range [0, 1).  Eccentricity may be randomized by
/// using an illegal value (negative, or a value of 1.0 or greater).  Illegal radius
/// values cause the seed to be skipped when processing.
typedef std::pair<float, float> PlanetisimalSeed;

/// @brief Represents a solar system (star + planets).
/// 
/// The Config structure controls the behavior of the simulation.  Refer to it for details of
/// the parameters.
/// 
/// System creation is triggered using one of the create() methods.  The simpler method,
/// create(const Config* config), will generate planetary systems using the parameters listed
/// in the Config structure.  The second method, create(const Config* config, const std::vector<PlanetisimalSeed>& planetSeed),
/// behaves the same, but it applies the PlanetisimalSeed values in `planetSeed`.
/// 
/// When `planetSeed` contains values, or Config::generateBodeSeeds is true, then a series
/// of planetisimal seeds are applied to the dust cloud prior to the random distribution of
/// protoplanetary masses.  These seeds allow customizing the
/// layout of the solar system (in the former case), or to generate a layout that conforms to
/// Bode's Law (in the latter case).
/// 
/// These seeds are identical to the planetisimals that are generated normally.  However, by planting
/// them before the random planetisimals are generated, you can guarantee planets will accrue in
/// certain orbits.
/// 
/// Once those seeds are consumed, additional planetisimals are generated at random distances from
/// the star.  These planetisimals consume free dust, and they potentially collide with existing
/// planetisimals (or are captured to become moons) until no dust remains.
/// 
/// Once the free dust has been consumed, the planetoids are resolved into planets.  When the
/// create() method returns, planetList contains the planets of the solar system.
/// 
/// Bode seeds are an implementation based on Mary Blagg's proposed substitute for Bode's Law
/// (see Blagg 1913).  The implementation here is roughly based on her proposal, but it replaces
/// her second term {B + f(a + nb)} with GenerationState::randomNear(B, B * 0.1).  The primary reason
/// for this change is that I had no success getting the f(a + nb) function to return results that
/// matched the original paper's, which tells me either I fundamentally messed something up,
/// or I misunderstood the terms.  Either way, the randomNear function provides a value in the
/// neighborhood of B, which provides enough variation to that term to make a reasonable approximation.
/// 
/// The value 0.3 was determined by using the Blagg formulation (A * 1.7275^n) * (B), solving for B
/// for each planet in our solar system, and determining the mean and standard deviation of the resulting
/// values B.  The actual std.dev was around 0.3, but I'm narrowing the variation for now.
/// 
/// The values A and B are determined by selecting a random uniform value in the range 0.41-0.46 for A,
/// and solving B as the value Star::ecosphere() / A.  This guarantees that one planet will try to form in
/// the habitable zone, or very close to it.  Refer to the code in System::create() for more specifics.
/// 
/// @todo Save System, Load System, Print System (printf / FILE * stream).
class System
{
    public:
    System();
    ~System() {}

    //--- Solar system creation methods.

    /// @brief Create a solar system using the parameters in `appConfig`.
    /// 
    /// @param appConfig The generation configuration structure, with overrides applied.  If null, all defaults will be used.
    void create(const Config* appConfig);

    /// @brief Create a solar system using the parameters in `appConfig`, using
    /// `planetSeed` to seed the initial protoplanets.
    /// 
    /// The PlanetisimalSeed values are applied in the order they are listed.  All seeds are
    /// applied before the random accretion sequence begins.
    /// 
    /// Config::generateBodeSeeds is ignored if `planetSeed` is not empty.
    /// @param appConfig The generation configuration structure, with overrides applied.  If null, all defaults will be used.
    /// @param planetSeed A vector of 0 or more PlanetisimalSeed values.
    void create(const Config* appConfig, const std::vector<PlanetisimalSeed>& planetSeed);

    //-- Solar system inspection methods.

    /// @brief Return an iterator for the first planet in the system.
    /// @return Beginning iterator
    PlanetList::const_iterator begin() const { return planetList.begin(); }

    /// @brief Return an iterator for the past-the-last planet in the system.
    /// @return End iterator
    PlanetList::const_iterator end() const { return planetList.end(); }

    /// @brief Return the count of the number of protoplanets that were generated for the solar system.
    /// @return The number of protoplanets.
    uint32_t protoplanets() const { return protoplanetsGenerated; }

    /// @brief Return the RNG seed used to generate this system.
    /// @return The RNG seed
    uint64_t seed() const { return rngSeed; }

    /// @brief Returns the number of planets in the system.
    /// @return The planet count
    size_t size() const { return std::distance(planetList.begin(), planetList.end()); }

    /// @brief Access the central star of this planetary system
    /// @return The star
    const Star& star() const { return centralStar; }

    /// @brief The maximum randomly-generated mass of the star, in solar masses.
    static constexpr float MaxRandomSolarMass = 1.3f;

    /// @brief The minimum randomly-generated mass of the star, in solar masses.
    static constexpr float MinRandomSolarMass = 0.6f;

    private:

    /// @brief The value used to seed the random number generator.
    uint64_t rngSeed;

    /// @brief A count of the number of protoplanets that were used to generate the solar system.
    uint32_t protoplanetsGenerated;

    /// @brief The configuration state.
    Config config;

    /// @brief The planetary system's star.
    Star centralStar;

    /// @brief Record of all planets, from innermost to outermost.
    PlanetList planetList;

    /// @brief The guts of the distribution system.
    /// @param state The GenerationState.
    /// @param planetSeed A vector of 0 or more PlanetisimalSeed values.
    void distributePlanetaryMass(GenerationState& state, const std::vector<PlanetisimalSeed>& planetSeed);

    /// @brief After distributing planetary masses, evaluate the resulting planets and store final results.
    /// @param state The GenerationState
    void evaluatePlanets(GenerationState* state);
};

}
}
