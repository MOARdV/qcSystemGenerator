/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2024 Questionable Coding
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
#include "Planet.h"
#include "Star.h"

#include <random>

#include <forward_list>

namespace qc
{

namespace SystemGenerator
{

// Forward declarations
class SolarSystem;

/// @brief The Generator is the functional element used to generate random solar systems.
/// 
/// This class also encompasses the random number generator.
/// 
/// Because the random number generator is embedded within this class, it is needed for evaluating
/// manually-created solar systems.
/// 
/// @todo: proper sections in this documentation.
/// 
/// ## Accretion
/// 
/// The basic accretion algorithm applies when Config::protoplanetSeeds is empty and Config::generateBodeSeeds
/// is false.  Using this algorithm, a protoplanet is generated at a random
/// distance from the star.  It sweeps up any dust within its influence and accumulates it.  If there is
/// sufficient mass to exceed the protoplanet threshold, the protoplanet is added to the solar system as a
/// planetisimal.
/// 
/// @todo: Bibliography
/// 
/// @todo: finish documenting the process properly.
/// 
/// ## Bode Seeds
/// 
/// When Config::generateBodeSeeds is true, the accretion algorithm creates a series of protoplanet
/// seeds at distances based on the observed ratio of orbital distances found in our solar system.
/// These seeds are used to initialize the solar system with protoplanets.  Once these protoplanets have
/// been placed, any remaining dust is swept up using the accretion algorithm described above.
/// 
/// When using this mode, the first seed is always going to be in or near the habitable zone of the star
/// in order to increase the possibility of an Earth-like world being generated.  The remainder of the
/// seeds are placed in a randomized order, which makes the final solar system less deterministic - if
/// seeds were always consumed starting close to the sun, or far from the sun, all solar systems would
/// end up being generally similar.
/// 
/// ## Manual Seeds
/// 
/// If you want more control over the initial state of the solar system, you can specify one or more seeds
/// in Config::protoplanetSeeds.  When there are entries in this vector, those entries are used to initialize
/// the protoplanets of the solar system.  For instance, if you want a planet to form at in a specific orbit,
/// but you did not care about the details of the remainder of the solar system, you could add a single seed
/// to Config::protoplanetSeeds.  Once the protoplanets from Config::protoplanetSeeds are placed, the remainder
/// of the dust in the solar system will be consumed using accretion.
/// 
/// Manual seeds take priority over Config::generateBodeSeeds, so supplying entries in Config::protoplanetSeeds
/// will cause Generator to treat Config::generateBodeSeeds as false.
class Generator
{
    public:

    Generator() { mt.seed(seedVal); }
    ~Generator() { }

    /// @brief Generate a random solar system.
    /// 
    /// Any existing planets in `system` will be removed.  If Config::generateStar is true,
    /// the Star will also be replaced.
    /// 
    /// After this method has run, the SolarSystem will have been fully evaluated.
    /// 
    /// This generator uses the original accretion model, where each protoplanet is generated
    /// in sequence and dust is iteratively collected until the protoplanet has swept its lane.
    /// @param system The SolarSystem that will contain the results.
    /// @param config_ The Config that configures the generator.
    void generate(SolarSystem& system, const Config& config_);

    /// @brief Generate a random solar system.
    /// 
    /// Any existing planets in `system` will be removed.  If Config::generateStar is true,
    /// the Star will also be replaced.
    /// 
    /// After this method has run, the SolarSystem will have been fully evaluated.
    /// 
    /// This generator method uses a semi-parallel accretion algorithm.  Instead of each protoplanet
    /// being fully accreted before the next one protoplanet is evaluated, each of the initial protoplanets
    /// (those provided in Config::protoplanetSeeds or those created via Config::generateBodeSeeds, as
    /// well as those added because of Config::protoplanetCount) accrete dust once, and then each one
    /// accretes again.  This continues until no protoplanets are collecting dust.  At that stage,
    /// if there is still dust available, the sequential accretion algorithm used in generate() is used
    /// to sweep the remaining dust.
    /// @param system The SolarSystem that will contain the results.
    /// @param config_ The Config that configures the generator.
    void generate2(SolarSystem& system, const Config& config_);

    /// @brief Returns the percentage random variation in density to use generating a planet.
    /// 
    /// This variation allows for a little more variety in planetary sizes and characteristics.
    /// @return The density variation percentage, [0, 1].
    float getDensityVariation() const { return config.densityVariation; }

    /// @brief Returns the number of protoplanets that were successfully generated.
    /// 
    /// This number may or may not correspond to the final planet count.
    /// @return The number of viable protoplanets that were generated.
    uint32_t getProtoplanetCount() const { return protoPlanetCount; }

    /// @brief Returns the value used to seed the RNG.
    /// @return The seed values.
    uint64_t getSeed() const { return seedVal; }

    /// @brief Indicates whether verbose logging is enabled.
    /// @return True if we want verbose logging.
    bool getVerbose() const { return config.verboseLogging; }

    /// @brief Select a uniformly-distributed random number within the range
    /// [(1 - range) * center, (1 + range) * center].
    /// @tparam T_ The type of value to return (float or double)
    /// @param center The center / mean value.
    /// @param range The range about center, as a ratio.
    /// @return A random number in the range.
    template<class T_> T_ randomAbout(T_ center, T_ range)
    {
        return center * randomUniform((T_)(1) - range, (T_)(1) + range);
    }

    /// @brief Return a random eccentricity, per the accrete algorithm.
    /// 
    /// By setting the lower bound above 0, we don't have to worry about a result of 1.0.
    /// @return An eccentricity from 0.00 to about 0.20.
    float randomEccentricity()
    {
        static constexpr float EccentricityCoefficient = 0.077f;

        return 1.0f - powf(randomUniform(1.0f / 16.0f, 1.0f), EccentricityCoefficient);
    }

    /// @brief Returns a random number near the mean using a Gaussian distribution.
    /// @tparam T_ The type of value to return (float or double)
    /// @param mean The central value
    /// @param threeSigma 3x standard deviation.  97% of the values will be within +/- 3s of the mean.  The remainder will be with 2x 3s.
    /// @return A number near the `mean`.
    template<class T_> T_ randomNear(T_ mean, T_ threeSigma)
    {
        std::normal_distribution<T_> nd(mean, threeSigma * (T_)(1.0 / 3.0));
        return nd(mt);
    }

    /// @brief Return a random axial tilt, per the accrete algorithm
    /// @param sma The semi-major axis of the body that we're tilting.
    /// @param medianTilt The median axial tilt.
    /// @return A normalized axial tilt in the range [0, 180].
    float randomTilt(double sma, double medianTilt)
    {
        float tilt = static_cast<float>(pow(sma, 0.2) * randomAbout(medianTilt, 0.4));
        tilt = remainder(tilt, 360.0f);
        if (tilt > 180.0f)
        {
            tilt = 360.0f - tilt;
        }

        return tilt;
    }

    /// @brief Return a random angle on a circle.
    /// @return A floating-point value in the range of 0 to 2*PI.
    float randomTwoPi()
    {
        return randomUniform(0.0f, 6.283185307179586476925286766559f);
    }

    /// @brief Returns a floating-point number in the range [lowerBound, upperBound] using a uniform distribution.
    /// @tparam T_ The type of value to return (float or double)
    /// @param lowerBound The lower bound of the random value.
    /// @param upperBound The upper bound of the random value.
    /// @return A value in the prescribed range.
    template<class T_> T_ randomUniform(T_ lowerBound, T_ upperBound)
    {
        std::uniform_real_distribution<T_> rng(lowerBound, upperBound);
        return rng(mt);
    }

    /// @brief Returns a number in the range [lowerBound, upperBound] using a uniform distribution.
    /// @tparam T_ The type of integer value to return)
    /// @param lowerBound The lower bound of the random value.
    /// @param upperBound The upper bound of the random value.
    /// @return A value in the prescribed range.
    template<class T_> T_ randomUniformInt(T_ lowerBound, T_ upperBound)
    {
        std::uniform_int_distribution<T_> rng(lowerBound, upperBound);
        return rng(mt);
    }

    /// @brief Seed the random number generator.
    /// @param seedVal_ The seed value.
    void seed(uint64_t seedVal_) { seedVal = seedVal_; mt.seed(seedVal); }

    private:

    /// @brief Represents a band of dust during accrual.
    struct Dust
    {
        /// @brief Inner edge of the band, in AU.
        double innerEdge;

        /// @brief Outer edge of the band, in AU.
        double outerEdge;

        /// @brief Is dust present in this band?
        bool dustPresent;

        /// @brief Is gas present in this band?
        bool gasPresent;

        /// @brief Initializes a dustband
        /// @param inner Inner edge of the band, in AU.
        /// @param outer Outer edge of the band, in AU.
        /// @param hasDust Whether the dustband contains dust
        /// @param hasGas Whether the dustband contains gas
        Dust(double inner, double outer, bool hasDust, bool hasGas) :innerEdge(inner), outerEdge(outer), dustPresent(hasDust), gasPresent(hasGas) {}

        /// @brief Sorting operator.
        /// @param rhs Comparand.
        /// @return true if this Dust structure is closer to the star than `rhs`.
        bool operator <(const Dust& rhs) const { return innerEdge < rhs.innerEdge; }
    };

    /// @brief Type definition for a list of Dust.
    typedef std::forward_list<Dust> AvailableDust;

    /// @brief Contains the state relevant to a protoplanet during the accretion process.
    ///
    /// The caller must initialize sma, e, and mass, then use System::criticalLimit() to set
    /// criticalMass.  All other values are initialized and modified within GeneratorState::accreteDust().
    struct Protoplanet
    {
        //--- Configuration values provided to accreteDust()
        double sma; //!< Semi-major axis of the protoplanet.
        double mass; //!< Total mass of the protoplanet, in solar masses.
        float eccentricity; //!< Eccentricity of the protoplanet.

        //--- Computed / updated during accretion.
        double criticalMass; //!< Critical mass for gas retention (gas giant formation), in solar masses.  Derived from sma and e.
        double dustMass; //!< Mass of the dust component of the protoplanet, in solar masses.
        double gasMass; //!< Mass of the gas component of the protoplanet, in solar masses.  "Gas" in this context is hydrogen and helium.

        double r_inner; //!< Inner effect limit radius, in AU.
        double r_outer; //!< Outer effect limit radius, in AU.
        
        bool active; //!< Is this protoplanet still actively accruing?  Used for parallel accretion.

        Protoplanet() :sma(0.0), eccentricity(0.0f), mass(0.0),
            criticalMass(0.0), dustMass(0.0), gasMass(0.0),
            active(true),
            r_inner(0.0), r_outer(0.0)
        { }
    };

    /// @brief The dust available for accretion.
    AvailableDust availableDust;

    bool dustRemains = false; //!< Does any dust remain for accretion?

    Config config; //!< Copy of the config values.

    BandLimit_t protoplanetZone; //!< Shadow copy of the Star's protoplanet zone

    double stellarLuminosity = 0.0; //!< Shadow copy of the Star's luminosity

    double stellarMass = 0.0; //!< Shadow copy of the Star's mass

    /// @brief The 64-bit Mersenne twister engine used to provide random numbers.
    std::mt19937_64 mt;

    /// @brief The list of planets as generated by coalescePlanetisimals()
    PlanetList planetList;

    /// @brief Value used to seed the Mersenne twister engine.
    uint64_t seedVal = 5489u;

    /// @brief Count of how many protoplanets contributed to the solar system.
    uint32_t protoPlanetCount = 0;

    // Accrete dust.  This implementation grows the protoplanet until it's swept
    // all available dust.  Once the dust has been accreted, this method calls
    // coalescePlanetisimals() to attempt to convert the protoplanet into a planet.
    void accreteDust(Protoplanet& protoplanet);

    // Accrete dust, single step.  This implementation only collects dust once, instead
    // of growing the protoplanet until it's swept its neighborhood.  Used in generate2().
    bool accreteDust2(Protoplanet& protoplanet);

    // Attempt to convert the protoplanet into a planet.  First, each existing planet is tested
    // to see if the protoplanet may have collided with it.  If not, a new planet is formed.
    // If there was a collision, a new protoplanet is formed using post-colliision mass and
    // orbital characteristics, and it goes through collectDust() to sweep its neighborhood.
    void coalescePlanetisimals(const Protoplanet& protoplanet);

    /// @brief Execute one iteration to collect dust from the dustband.  Recurse to the next dust band to continue collecting.
    /// @param lastMass The amount of dust added in the previous step.
    /// @param additionalDustMass The amount of dust to add to the body.
    /// @param additionalGasMass The amount of gas to add to the body.
    /// @param protoplanet The protoplanet in question
    /// @param dustband The dustband to ocllect dust from
    /// @return Net increase in mass (dust mass + gas mass)
    double collectDust(double lastMass, double& additionalDustMass, double& additionalGasMass, const Protoplanet& protoplanet, AvailableDust::iterator dustband);

    // Generate a sequence of protoplanet seeds based on Blagg's modification of Bode's Law.
    // The first seed in the returned vector is always closest to the ideal habitable zone.
    // The remainder of the seeds are randomly sorted to allow for more variation in the results.
    void generateBodeSeeds(std::vector<ProtoplanetSeed>& protoplanetSeeds, const Star& star);

    // Generate a random star for the solar system.
    void generateStar(SolarSystem& system);

    // Clear dust and gas (as appropriate) from the availableDust list.
    void updateDustLanes(const Protoplanet& protoplanet);
};

}
}
