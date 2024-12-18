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

#include <vector>


// When defined, printfs may be inserted in the code for debug purposes.
#define ALLOW_DEBUG_PRINTF

namespace qc
{

namespace SystemGenerator
{

/// @brief Represents a seed value for a protoplanet during system generation.
///
/// Higher eccentricity values are able to collect more dust, leading to larger planets.
/// 
/// @note 1: Valid values for semiMajorAxis are in the range of the star's protoplanet zone,
/// as returned by Star::getProtoplanetZone().  Values outside that range are discarded.
/// 
/// @note 2: Valid values for `eccentricity` are in the range of [0, 0.9].  Values less than
/// zero or greater than 0.9 will be treated as "generate a random eccentricity".
struct ProtoplanetSeed
{
    double semiMajorAxis; //!< The semi-major axis of the orbit, in AU.
    float eccentricity; //!< The eccentricity of the orbit.  See note about invalid values.
};

/// @brief The Config structure contains the values that steer random solar
/// system generation.
/// 
/// These values are used to configure the Generator class during random solar
/// system generation.  Reasonable defaults are set for all of these values.
/// Read their descriptions for more detail.
/// 
/// The configuration is roughly broken up into three sections:
/// 
/// **Constants:**
/// 
/// These are values that were hard-coded in the original versions of the accretion software.
/// They are exposed here, and can be tweaked, to see what happens.  Successful solar systems
/// are *not* guaranteed.  Neither is avoiding floating point exceptions.  Alter these at your
/// peril.
/// 
/// **Switches:**
/// 
/// Allows various optional features to be switched on or off.
/// 
/// **Initial Seed:**
/// 
/// An optional std::vector containing protoplanet seeds.  When it is populated, it will be used to
/// scatter the initial protoplanets in the accretion disc.  Once all of those seeds have been placed,
/// the remainder of the planetary dust will be consumed using the normal accretion method.
/// 
/// This vector is consumed in the order it's provided, so the highest priority potential worlds should
/// be listed earlier.  It is always possible that a seed will not result in a planet if the corridor it
/// spawns in has already been cleared of dust - in that case, it most likely will be absorbed by an existing
/// planet.
/// 
/// Note that if this vector is not empty, it takes priority over `generateBodeSeeds`.
struct Config
{
    //--- Constants used to configure the generator.

    /// @brief Default mass of a protoplanet seed.
    ///
    /// Each protoplanet that is spawned during system creation starts with this mass.  In units of solar mass.
    static constexpr double ProtoplanetSeedMass = 1.0e-15;

    /// @brief The mass of a protoplanet seed.
    ///
    /// This is the initial mass that is used to accumulate dust and gas into a planet, in Solar Mass.
    double protoplanetSeedMass = ProtoplanetSeedMass;

    ///--- User-adjustable variables

    /// @brief Allow for some variation in the density of the planets.
    ///
    /// Changing the density will change the volume, and thus change the radius of the bodies that are
    /// generated.  Setting this value to 0.0 means each planet is sized deterministically.  Setting it
    /// to a positive value will vary the density of the planet by that percentage, so the actual volume
    /// of the planet will be in the range of [1 - densityVariation, 1 + densityVariation].
    /// 
    /// This value is clamped to the range [0.0, 0.1].
    float densityVariation = 0.025f;

    /// @brief Set the mean inclination of the planets of the solar system.
    ///
    /// Inclination is in degrees relative to the equator of the star.  Defaults are for the Solar
    /// System's planets.
    /// 
    /// This value will be normalized to the range [0, 180).
    /// 
    /// Resulting inclinations are also always positive in the range of [0, 180).
    /// @todo Convert to radians for consistency.
    float inclinationMean = 5.57f;

    /// @brief Set the standard deviation of the inclination of the planets of the solar system.
    ///
    /// Set this value to 0 to set all planets' inclinations to `inclinationMean`.
    /// 
    /// Default value is the stddev of the inclination of the planets of the Solar System.
    /// @todo Convert to radians for consistency.
    float inclinationStdDev = 1.23f;

    /// @brief The number of protoplanets to generate in Generator::generate2().
    ///
    /// These protoplanets are generated in addition to the protoplanets generated when Config::generateBodeSeeds
    /// is true or Config.protoplanetSeeds is not empty.  If these additional protoplanets are inadequate to
    /// sweep the neighborhood, then Generator::generate2() falls back to random protoplanets that sweep
    /// their neighborhood per the Generator::generate().
    uint32_t protoplanetCount = 20u;

    //--- Switches

    /// @brief When true, generate planetisimal seeds derived from Blagg's modification of Bode's Law.
    /// 
    /// When this switch is true, a list of protoplanet seeds is generated based on Blagg's modification
    /// of Bode's Law.  This list of protoplanets is used place the initial protoplanets.  Once this list
    /// is consumed, the remainder of the dust will be distributed using normal accretion.
    ///
    /// When false, the traditional accretion method is used, with random distribution of all protoplanets.
    /// 
    /// The modification of Bode's Law is derived from Blagg 1913.
    /// 
    /// The first planet in the seed list will always be somewhere in the habitable zone.  The remainder of the
    /// seeds will be correspond to the other entries found using the Blagg modification of Bode's Law.
    /// All elements after the first will be shuffled to allow for less deterministic results.
    /// 
    /// If the protoplanetSeeds vector is not empty, it will override this switch.
    bool generateBodeSeeds = false;

    /// @brief When true, moons may be synthesized around each planet using a Bode Law progression.
    /// @todo Support moon generation
    bool generateMoons = false;

    /// @brief When true, planetisimal collisions may result in moon capture instead of always merging the
    /// new protoplanet with the existing one.
    /// @todo Support moon capture
    bool generateMoonsOnCollision = false;

    /// @brief When true, generate a new random star for the solar system.
    ///
    /// When false, the star provided will be used, instead.
    /// 
    /// A random star will be limited to the range F5V to K9V, which is the range that the accretion algorithm
    /// works best with.
    bool generateStar = false;

    /// @brief When true, verbose logging is enabled.
    ///
    /// If ALLOW_DEBUG_PRINTF is defined, verbose logging is done to the console (printf).
    /// @todo Callback logging as well?
    bool verboseLogging = false;

    //--- Initial Seed

    /// @brief An optional vector of protoplanets to initialize the solar system.
    ///
    /// When this vector is not empty, it will determine the placement of the initial protoplanets that
    /// are generated for the solar system.
    std::vector<ProtoplanetSeed> protoplanetSeeds;
};

}
}
