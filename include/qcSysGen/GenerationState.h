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
#include "System.h"

#include <forward_list>

namespace qc
{

namespace SystemGenerator
{

/// @brief Represents a band of dust during accrual.
struct Dust
{
    /// @brief Inner edge of the band, in AU.
    double innerEdge;

    /// @brief Outer edge of the band, in AU.
    double outerEdge;

    /// @brief Is dust present in this band?
    bool dustPresent : 1;

    /// @brief Is gas present in this band?
    bool gasPresent : 1;

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
    double gasMass; //!< Mass of the gas component of the protoplanet, in solar masses.

    double reducedMass; //!< Reduced mass used and updated in GeneratorState::accreteDust().
    double r_inner; //!< Inner radius, in AU, derived from reducedMass.
    double r_outer; //!< Outer radius, in AU, derived from reducedMass.

    Protoplanet() :sma(0.0), eccentricity(0.0f), mass(0.0),
        criticalMass(0.0), dustMass(0.0), gasMass(0.0),
        reducedMass(0.0), r_inner(0.0), r_outer(0.0)
    { }
};

/// @brief Parameters used during planetary system generation.
///
/// Most values are initialized using the init() method.  Manual overrides may be applied afterwards,
/// prior to calling finalize().
/// 
/// The intended use sequence is:
/// 1. Instantiate the structure.
/// 2. Call init()
/// 3. Edit any public parameters, such as overriding the outer planet limit.
/// 4. Call finalize().
/// 5. Pass GenerationState around as needed.
/// 
/// During planetary accretion, the numeric parameters are treated as constants.  Only the Dust container is intended
/// to change.
class GenerationState
{
    public:
    /// @brief Initialize the generation state from the provided Config.
    /// @param config The config describing how the solar system will be generated.
    GenerationState(const Config* config);
    ~GenerationState() {}

    //--- Initialization

    /// @brief Finalize the state and store a pointer to the star.
    ///
    /// Practically, all this step does is initialize the dust band state structure.
    /// @param centralStar The star around which we're accreting dust.
    void finalize(const Star* centralStar)
    {
        star = centralStar;
        availableDust.emplace_front(Dust(star->innerDustLimit(), star->outerDustLimit(), true, true));
        dustRemains = true;
    }

    //--- System Generation

    /// @brief Loop that collects all available dust within reach of a given protoplanet.
    /// 
    /// Each iteration of the loop checks for available dust and gas based on the accumulated
    /// mass of the protoplanet.  The loop ends when the change in mass after an iteration is
    /// less than 0.01%.  The new mass is then resolved against existing planets.
    /// 
    /// Note that the values in protoplanet may change from the initial state as dust accumulates
    /// and collisions occur.
    /// @param protoplanet The Protoplanet structure to test (SMA, e, mass, and criticalLimit must be prepopulated).
    void accreteDust(Protoplanet& protoplanet);

    /// @brief Returns true if any dust remains for planetary accretion.
    /// @return true if there's still dust
    bool dustAvailable() const
    {
        return dustRemains;
    }

    /// @brief Returns true if any dust is available within the effect limits of the protoplanet
    /// @param protoplanet The pre-configured protoplanet to test (SMA, e, and mass must be valid).
    /// @return true if any dust is available.
    bool dustAvailable(const Protoplanet& protoplanet) const
    {
        if (protoplanet.sma <= 0.0)
        {
            return false;
        }

        const double innerLimit = innerEffectLimit(protoplanet.sma, protoplanet.eccentricity, protoplanet.mass);
        const double outerLimit = outerEffectLimit(protoplanet.sma, protoplanet.eccentricity, protoplanet.mass);
        bool available = false;
        auto band = availableDust.begin();

        while (band != availableDust.end() && band->outerEdge < innerLimit)
        {
            ++band;
        }

        if (band != availableDust.end())
        {
            available = band->dustPresent;
        }

        while (band != availableDust.end() && band->innerEdge < outerLimit)
        {
            available = available || band->dustPresent;
            ++band;
        }

        return available;
    }

    /// @brief Returns the inner effect limit for a given protoplanet.
    /// @param sma The semi-major axis of the orbit, in AU.
    /// @param e The eccentricity of the orbit.
    /// @param mass The mass of the protoplanet, with Sol = 1.0.
    /// @return The inner radius, in AU.
    double innerEffectLimit(double sma, double e, double mass) const
    {
        return sma * (1.0 - e) * (1.0 - mass) / (1.0 + cloudEccentricity);
    }

    /// @brief Move the generated planet list.  Used after accretion to transfer the
    /// planets to System.
    /// @return The r-reference to the planet list.
    PlanetList&& movePlanets() { return std::move(planetList); }

    /// @brief Returns the outer effect limit for a given protoplanet.
    /// @param sma The semi-major axis of the orbit, in AU.
    /// @param e The eccentricity of the orbit.
    /// @param mass The mass of the protoplanet, with Sol = 1.0.
    /// @return The inner radius, in AU.
    double outerEffectLimit(double sma, double e, double mass) const
    {
        return sma * (1.0 + e) * (1.0 + mass) / (1.0 - cloudEccentricity);
    }

    /// @brief Select a uniformly-distributed random number within the range
    /// [(1 - range) * center, (1 + range) * center].
    /// @param center The center / mean value.
    /// @param range The range about center, as a ratio.
    /// @return A random number in the range.
    double randomAbout(double center, double range)
    {
        return center * randomUniform(1.0 - range, 1.0 + range);
    }

    /// @brief Return a random eccentricity, per the accrete algorithm.
    /// @return An eccentricity from 0 to 0.99
    float randomEccentricity()
    {
        static constexpr float EccentricityCoefficient = 0.077f;

        return std::min(0.99f, 1.0f - powf(randomUniform(0.0f, 1.0f), EccentricityCoefficient));
    }

    /// @brief Return a random integer-type value
    /// @tparam T_ The integer type
    /// @param lowerBound Lower bound of the range.
    /// @param upperBound Upper bound of the range.
    /// @return An integer in the range [lowerBound, upperBound].
    template<class T_> T_ randomInt(T_ lowerBound, T_ upperBound)
    {
        std::uniform_int_distribution<T_> rng(lowerBound, upperBound);
        return rng(mt);
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
    /// @return A normalized axial tilt in the range [0, 180].
    float randomTilt(double sma)
    {
        float tilt = static_cast<float>(pow(sma, 0.2) * randomAbout(EarthAxialTilt, 0.4));
        tilt = remainder(tilt, 360.0f);
        if (tilt > 180.0f)
        {
            tilt = 360.0f - tilt;
        }

        return tilt;
    }

    /// @brief Returns a number in the range [lowerBound, upperBound] using a uniform distribution.
    /// @tparam T_ The type of value to return (float or double)
    /// @param lowerBound The lower bound of the random value.
    /// @param upperBound The upper bound of the random value.
    /// @return A value in the prescribed range.
    template<class T_> T_ randomUniform(T_ lowerBound, T_ upperBound)
    {
        std::uniform_real_distribution<T_> rng(lowerBound, upperBound);
        return rng(mt);
    }

    /// @brief Safe callback emission method.
    /// 
    /// If `callback` is null, nothing happens.
    /// 
    /// `text` is expected to be a null-terminated string containing a newline character prior to the terminator.
    /// @param text The text that is emitted.
    void emitCallback(const char* text) const
    {
        if (callback)
        {
            callback(text);
        }
    }

    private:
    /// @brief The central star
    const Star* star;

    /// @brief The mean eccentricity of the dust cloud.
    double cloudEccentricity;

    /// @brief The density of the cloud.
    double baseDustDensity;

    /// @brief The mass of a protoplanet seed.  Used to detect if a protoplanet collected dust.
    double protoplanetSeedMass;

    /// @brief The value used to see the random number generator.
    uint64_t seedValue;

    /// @brief The 64-bit Mersenne twister engine used to provide random numbers.
    std::mt19937_64 mt;

    /// @brief Uniform number distribution in the range [0, 1].
    std::uniform_real_distribution<double> uniform01;

    /// @brief The dust available for accretion.
    AvailableDust availableDust;

    /// @brief Is there dust available in AvailableDust?
    bool dustRemains;

    /// @brief The list of planets as generated by coalescePlanetisimals()
    PlanetList planetList;

    /// @brief (optional) callback to emit console-style text.
    Config::ConsoleOutput_cb callback = nullptr;

    /// @brief Should we generate moons?
    bool doMoons;

    /// @brief Create a new planet in `planetList`, or merge the protoplanet with an existing planet.
    /// @param protoplanet The protoplanet to process.
    void coalescePlanetisimals(Protoplanet& protoplanet);

    /// @brief Recursively sweep the available dust bands for all available dust and gas.
    /// 
    /// protoplanet's values are updated during collection.
    /// @param mass The accumulated mass of dust.
    /// @param[out] dustMass The amount of dust collected.
    /// @param[out] gasMass The amount of gas collected.
    /// @param protoplanet The protoplanet that is collecting dust.
    /// @param dustband The dust band to inspect.
    double collectDust(double mass, double& dustMass, double& gasMass, Protoplanet& protoplanet, AvailableDust::iterator dustband);

    /// @brief Remove dust (and gas, where applicable) from the dustbands.  Merge identical, adjacent dustbands.
    /// @param protoplanet The protoplanet that is collecting dust and gas.
    void updateDustLanes(const Protoplanet& protoplanet);
};

}
}
