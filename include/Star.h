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

#include "Equations.h"

#include <string>

struct Config;
class GenerationState;

//--- CONSTANTS - todo: scope them inside Star?

/// @brief Radius of the ecosphere, in AU.
static constexpr double EcosphereScalar = 1.0;

/// @brief Inner limit of dust formation, in AU.
static constexpr double InnerDustScalar = 0.0;

/// @brief Inner limit of the habitable zone, in AU.
static constexpr double InnerHabitableZoneScalar = 0.95;

/// @brief Inner limit of planetisimal formation, in AU.
static constexpr double InnerPlanetScalar = 0.3;

/// @brief Inner limit of dust Zone 2, in AU.
static constexpr double InnerZone2Scalar = 4.0;

/// @brief Inner limit of dust Zone 3, in AU.
static constexpr double InnerZone3Scalar = 14.0;

/// @brief The maximum age for a main sequence star with fully-formed planets, in years.
///
/// Note that this value is not the maximum possible age of the star.  It is simply the
/// upper limit for a star's maximum randomly-determined age, provided the star is not too
/// hot / shorter lived.
static constexpr double MaximumStellarAge = 6.0e9;

/// @brief The minimum age for a main sequence star with fully-formed planets, in years.
static constexpr double MinimumStellarAge = 1.0e9;

/// @brief The highest stellar mass that can be used in the Star class.
static constexpr float MaximumStellarMass = 2.18f;

/// @brief The lowest stellar mass that can be used in the Star class.
static constexpr float MinimumStellarMass = 0.57f;

/// @brief Outer limit of dust formation, in AU.
static constexpr double OuterDustScalar = 200.0;

/// @brief Outer limit of the habitable zone, in AU.
static constexpr double OuterHabitableZoneScalar = 1.37;

/// @brief Outer limit of planetisimal formation, in AU.
static constexpr double OuterPlanetScalar = 50.0;

/// @brief Outer limit of dust Zone 1, in AU.
static constexpr double OuterZone1Scalar = 5.0;

/// @brief Outer limit of dust Zone 2, in AU.
static constexpr double OuterZone2Scalar = 16.0;

/// @brief The snow line, where water ice can exist in a vacuum, in AU.
static constexpr double SnowLineScalar = 5.0;

//--- Enumerations

/// @brief A broad classification of the zones of a planetary system, based on position relative to the
/// habitable zone and the snow line.
enum class OrbitalZone
{
    Inner, //!< Between the star and the inner edge of the habitable zone.
    Habitable, //!< Between the bounds of the habitable zone.
    Middle, //!< Outside the habitable zone, inside the snow line.
    Outer, //!< Outside the snow line.
};

/// @brief Encapsulates the stats of the central star of the planetary sytem.
///
/// Main sequence stars are assumed.  Mass in the range of [0.6, 1.3] leads to
/// best results.  Masses outside that range may lead to odd results due to an
/// inaccurate estimation of stellar luminosity.
class Star
{
    public:
    /// @brief Null-initialize a star.
    Star():stellarMass(0.0f), stellarLuminosity(0.0f), stellarAge(0.0), lifespan(0.0), curtMass(0.0), sqrtLum(0.0), outerPlanetaryR(0.0) {}

    /// @brief Initialize a star based on the solar mass of the star.
    /// 
    /// The `random01` parameter is used to select the age, from MimimumStellarAge (0.0) to MaximumStellarAge (1.0).
    /// 
    /// @note Mass is not clamped in this constructor.  It should be within the bounds of [MinimumStellarMass, MaximumStellarMass].
    /// @param mass The mass of the star in solar masses.
    /// @param random01 A number in the range of [0, 1].
    Star(float mass, double random01);

    /// @brief Return the age of the star.
    /// @return Age of the star, in years.
    double age() const { return stellarAge; }

    /// @brief Initialize a star.
    /// 
    /// @note The mass of the star should be constrained to the range [0.6, 1.3] for good results.  The algorithm to
    /// convert mass to luminosity is untested outside that range.
    /// @param config The Config structure.
    /// @param state GenerationState.  Used strictly for random number generation.
    void init(const Config* config, GenerationState* state);

    /// @brief Radius of the ideal (Earth-equivalent) ecosphere, in AU.
    /// @return Ecosphere radius, in AU.
    double ecosphere() const
    {
        return sqrtLum * EcosphereScalar;
    }

    /// @brief Returns the minimum distance from the star where dust may be found.
    /// @return Inner dust limit, in AU.
    double innerDustLimit() const
    {
        return curtMass * InnerDustScalar;
    }

    /// @brief Returns the inner radius of the habitable zone.
    /// @return Inner habitable zone, in AU.
    double innerHabitableZone() const
    {
        return sqrtLum * InnerHabitableZoneScalar;
    }

    /// @brief Returns the minimum distance from the star where planetisimals may form.
    /// @return Inner planetary limit, in AU.
    double innerPlanetaryLimit() const
    {
        return curtMass * InnerPlanetScalar;
    }

    /// @brief Returns the minimum distance from the star for dust Zone 2.
    /// @return Inner Zone 2 radius, in AU.
    double innerZone2() const
    {
        return sqrtLum * InnerZone2Scalar;
    }

    /// @brief Returns the minimum distance from the star for dust Zone 3.
    /// @return Inner Zone 3 radius, in AU.
    double innerZone3() const
    {
        return sqrtLum * InnerZone3Scalar;
    }

    /// @brief Return the luminosity of the star.
    /// @return Luminosity, Sol = 1.0.
    double luminosity() const { return stellarLuminosity; }

    /// @brief Return the mass of the star.
    /// @return Mass, Sol = 1.0.
    double mass() const { return stellarMass; }

    /// @brief Classifies the provided distance from the star into one of the three Zones of protoplanetary materials:
    /// 
    /// Zone I is the inner zone of the nebular dust where volatile ices can not form, and it contains primarily heavier elements.
    /// Zone II is the middle zone, where volatile ices can form and H2 and He are available in the gases, as well as the heavier elements.
    /// Zone III is the outer zone, where the gravitational forces are too low to retian H2 and He, leaving volatile ices and heavier elements.
    /// 
    /// The return value is a number in the range of 1.0 to 3.0.  Fractional values (eg, 2.4) indicate a location in the transitory region
    /// from one Zone to the next.
    /// 
    /// Zone classification from Pollard 1979 by way of Fogg 1985.
    /// @param radius The distance from the star, in AU.
    /// @return The zone, as described.
    float materialZone(double radius) const
    {
        if (radius < innerZone2())
        {
            return 1.0f;
        }
        else if (radius < outerZone1())
        {
            return 1.0f + static_cast<float>(InverseLerp(radius, innerZone2(), outerZone1()));
        }
        else if (radius < innerZone3())
        {
            return 2.0f;
        }
        else
        {
            // InverseLerp clamps to 1.0 for `radius` > outerZone2().
            return 2.0f + static_cast<float>(InverseLerp(radius, innerZone3(), outerZone2()));
        }
    }

    /// @brief Returns the maximum distance from the star where dust may be found.
    /// @return Outer dust limit, in AU.
    double outerDustLimit() const
    {
        return curtMass * OuterDustScalar;
    }

    /// @brief Returns the outer radius of the habitable zone.
    /// @return Outer habitable zone, in AU.
    double outerHabitableZone() const
    {
        return sqrtLum * OuterHabitableZoneScalar;
    }

    /// @brief Returns the maximum distanbce from the star where planetisimals may form.
    /// @return Outer planetary limit, in AU.
    double outerPlanetaryLimit() const
    {
        return outerPlanetaryR;
    }

    /// @brief Returns the maximum distance from the star for dust Zone 1.
    /// @return Outer Zone 1 radius, in AU.
    double outerZone1() const
    {
        return sqrtLum * OuterZone1Scalar;
    }

    /// @brief Returns the maximum distance from the star for dust Zone 2.
    /// @return Outer Zone 2 radius, in AU.
    double outerZone2() const
    {
        return sqrtLum * OuterZone2Scalar;
    }

    /// @brief Returns the radius that marks the snow line.
    /// @return Snow line radius, in AU.
    double snowLine() const
    {
        return sqrtLum * SnowLineScalar;
    }

    /// @brief Place the stellar class of this star in the provided string.
    /// 
    /// `output` will receive a three-character designation plus a null
    /// terminator, ie "G2V".
    /// 
    /// If `sizeofOutput` is less than 4, nothing is written.
    /// @param output The buffer to populate.
    /// @param sizeofOutput The size of the buffer.
    void stellarClass(char* output, size_t sizeofOutput) const;

    /// @brief Return a string containing the stellar class of the star,
    /// eg "G2V".
    /// @return The stellar class
    std::string stellarClass() const;

    /// @brief Classifies the radius into one of the orbital zones
    /// @param radius The distance from the star, in AU.
    /// @return The OrbitalZone in which the radius resides.
    OrbitalZone zone(double radius) const
    {
        if (radius < innerHabitableZone())
        {
            return OrbitalZone::Inner;
        }
        else if (radius < outerHabitableZone())
        {
            return OrbitalZone::Habitable;
        }
        else if (radius < snowLine())
        {
            return OrbitalZone::Middle;
        }
        else
        {
            return OrbitalZone::Outer;
        }
    }

    private:

    //--- Basic/stored stats

    /// @brief Mass of the star in solar masses.
    double stellarMass;

    /// @brief Luminosity of the star, Sol = 1.0.
    double stellarLuminosity;

    /// @brief Age of the star in years.
    double stellarAge;

    /// @brief Maximum age of the star, in years.
    double lifespan;

    //--- Derived stats

    /// @brief Mass ^ 1/3, used for some radius computations.
    double curtMass;

    /// @brief Luminosity ^ 1/2, used for some radius computations.
    double sqrtLum;

    /// @brief Outer planetary formation limit, in AU.  Recorded in case the caller specified an override value.
    double outerPlanetaryR;
};
