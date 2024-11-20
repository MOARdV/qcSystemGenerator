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

#include "Equations.h"

#include <string>

namespace qc
{

namespace SystemGenerator
{

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

class Generator;

/// @brief Describes the inner/outer limit of various bands around the star, ie
/// the Habitable Zone, or Inner Planets, etc.
typedef std::pair<double, double> BandLimit_t;


/// @brief Provides enumerations of the (for now) main sequence star types.
///
/// Any star type may be specified, but accretion results are *not* guaranteed at the extremes of
/// this sequence.  The accretion implementation is intended to work in the range of F5V to K9V.
/// 
/// O class stars do not support O0V, O1V, or O2V types.  The type will be clamped at O3V.
enum class StarClassification : uint32_t
{
    O_V, //!< Main sequence class O.  Can support types O3V - O9V.  Super-rare.  Probably should not use.
    B_V, //!< Main sequence class B.
    A_V, //!< Main sequence class A.
    F_V, //!< Main sequence class F.  Warmest star type that works well with accretion (F5V or cooler).
    G_V, //!< Main sequence class G.  Earth's sun is G2V.
    K_V, //!< Main sequence class K.  The coolest temperature stars that work well with accretion.
    M_V, //!< Main sequence class M.  Red dwarfs.
};

/// @brief Encapsulates a star's classification.
typedef std::pair<StarClassification, int32_t> StarType_t;

/// @brief Encapsulates the data describing a star.
///
/// A star is primarily defined by the star type (enumerated as StarClassification) and an integer subtype in the range
/// of [0, 9].  Most of the other parameters are derived from those values.
/// 
/// Mass, radius, and luminosity are all Sol-normalized values (Sol = 1.0).
/// 
/// The age of the star may be specified any time after setting the stellar type.  If the age is still the
/// default (0.0) when evaluate() is called, an arbitrary middling age will be selected at random.  Otherwise,
/// the age will be clamped to reasonable values.  Note that setting the age /after/ evaluate() is called will
/// bypass the sanity checks, and could lead to Bad Things happening in equations that rely on the solar age.
/// 
/// The name of the star may be specified before adding it to a solar system.  If the name is not specified,
/// the star will inherit the name of the solar system.
/// 
/// @todo: Evaluate whether I should allow floats instead of ints for type.  It matters more on the ends of the
/// scale, which are not really the intended range for accretion, but could be used for other configurations.
class Star
{
    public:
    //--- Constants
    static constexpr double MinimumStellarAge = 1.0e9; //!< Minimum age allowed for solar system generation.
    static constexpr double MaximumStellarAge = 6.0e9; //!< Maximum age allowed for solar system generation.  Note shorter-lived stars will have lower values.

    //--- Methods
    /// @brief Null constructor.  Initializes the star to a G2V.
    Star() { reset(); }

    /// @brief Basic constructor to initialize the type/subtype.
    /// @param type_ The primary classification of the star.
    /// @param subtype_ The subtype of the star.  Valid ranges are 0 to 9, inclusive.
    Star(StarClassification type_, int32_t subtype_) { setType(type_, subtype_); }

    /// @brief Evaluate the derived traits of the star.
    ///
    /// If the star's age has not been specified (it is still the default age of
    /// 0.0), then the age will be set to halfway between the star's minimum age
    /// amd maximum age.
    /// 
    /// If the star's age was specified, it is clamped to the range between
    /// minimum and maximum.
    /// 
    /// The `generator` parameter is an optional parameter that allows some values to
    /// be randomized to add some variation to the star.  If `generator` is not null,
    /// the following behaviors change:
    /// 
    /// If the age is unspecified, then a random age between 25% and 75% of the
    /// star's min/max age will be selected at random.
    /// 
    /// @todo How about the other values?  Randomize mass and luminosity slightly?
    /// 
    /// @note If a star is not evaluated prior to adding it to a SolarSystem, the
    /// `generator` parameter will be null.
    /// @param generator An optional pointer to the random system generator.  May be null.
    void evaluate(Generator* generator = nullptr);

    /// @brief Return the age of the star.
    /// @return Age of the star, in years.
    double getAge() const { return ageYears; }

    /// @brief Get the valid range for dust forming around this star.
    /// @return The min/max valid dust distance, in AU.
    const BandLimit_t& getDustZone() const { return dustZone; }

    /// @brief Returns the ideal semimajor axis for an earth-like planet.
    /// @return Ideal SMA, in AU.
    double getEcosphere() const { return ecosphere; }

    /// @brief Get the habitable zone for this star.
    /// @return The min/max habitable zone distance, in AU.
    const BandLimit_t& getHabitableZone() const { return habitableZone; }

    /// @brief Returns the luminosity of the star in units of Solar luminosity.
    /// @return Stellar luminosity, Sol = 1.0.
    double getLuminosity() const { return luminositySolar; }

    /// @brief Returns the mass of the star in units of Solar mass.
    /// @return Stellar mass, Sol = 1.0.
    double getMass() const { return massSolar; }

    /// @brief Get the name of the star.
    /// @return The name.  If no name was specified, this return value will be an empty string.
    const std::string& getName() const { return name; }

    /// @brief Get the valid range for protoplanets forming around this star.
    /// @return The min/max valid protoplanet distance, in AU.
    const BandLimit_t& getProtoplanetZone() const { return protoplanetZone; }

    /// @brief Get the minimum distance from the star where water ice may exist in a vacuum.
    /// @return The snow line, in AU.
    double getSnowLine() const { return snowLine; }

    /// @brief Return the radius of the star in Solar radii.
    /// @return Solar radius of the star.
    double getSolarRadius() const { return radiusSolar; }

    /// @brief Get the star's classification and subtype.
    /// @return The star's type.
    StarType_t&& getStarType() const { return std::make_pair(type, subtype); }

    /// @brief Given a mass, return the star type that corresponds best with that mass.
    /// 
    /// Values outside the supported range will be clamped.
    /// @note Only main sequence stars are currently supported.
    /// @param mass The mass of the star, in Solar masses.
    /// @return The stellar classification for that star.
    static StarType_t GetStarType(double mass);

    /// @brief Place the stellar class of this star in the provided string.
    /// 
    /// `output` will receive a three-character designation plus a null
    /// terminator, ie "G2V".
    /// 
    /// If `sizeofOutput` is less than 4, nothing is written.
    /// @param output The buffer to populate.
    /// @param sizeofOutput The size of the buffer.
    void getStellarClass(char* output, size_t sizeofOutput) const;

    /// @brief Return a string containing the stellar class of the star,
    /// eg "G2V".
    /// @param out The string that will receive the stellar class text.
    void getStellarClass(std::string& out) const;

    /// @brief Return the temperature of the star in Kelvin.
    /// @return Temperature in Kelvin.
    double getTemperature() const { return temperatureKelvin; }

    /// @brief Classifies the provided distance from the star into one of the three Zones of protoplanetary materials:
    /// 
    /// Zone I is the inner zone of the nebular dust where volatile ices can not form, and it contains primarily heavier elements.
    /// Zone II is the middle zone, where volatile ices can form and H2 and He are available in the gases, as well as the heavier elements.
    /// Zone III is the outer zone, where the gravitational forces are too low to retain H2 and He, leaving volatile ices and heavier elements.
    /// 
    /// The return value is a number in the range of 1.0 to 3.0.  Fractional values (eg, 2.4) indicate a location in the transitory region
    /// from one Zone to the next.
    /// 
    /// Zone classification from Pollard 1979 by way of Fogg 1985.
    /// @param sma The distance from the star, in AU.
    /// @return The zone, as described.
    float getMaterialZone(double sma) const
    {
        if (sma < zone2.first)
        {
            return 1.0f;
        }
        else if (sma < zone1.second)
        {
            return 1.0f + static_cast<float>(InverseLerp(sma, zone2.first, zone1.second));
        }
        else if (sma < zone3.first)
        {
            return 2.0f;
        }
        else
        {
            // InverseLerp clamps to 1.0 for `radius` > outerZone2().
            return 2.0f + static_cast<float>(InverseLerp(sma, zone3.first, zone2.second));
        }
    }

    /// @brief Classifies the radius into one of the orbital zones.
    /// 
    /// These zones do not correspond to the material zones.
    /// @param sma The distance from the star, in AU.
    /// @return The OrbitalZone in which the radius resides.
    OrbitalZone getOrbitalZone(double sma) const
    {
        if (sma < habitableZone.first)
        {
            return OrbitalZone::Inner;
        }
        else if (sma < habitableZone.second)
        {
            return OrbitalZone::Habitable;
        }
        else if (sma < snowLine)
        {
            return OrbitalZone::Middle;
        }
        else
        {
            return OrbitalZone::Outer;
        }
    }

    /// @brief Reset all values to defaults.
    void reset()
    {
        name.clear();
        type = StarClassification::G_V;
        subtype = 2;
        evaluated = false;
        ageYears = 0.0;
        temperatureKelvin = 0.0f;
        luminositySolar = 0.0;
        radiusSolar = 0.0;
        massSolar = 0.0;
        ecosphere = 0.0;
        snowLine = 0.0;
        habitableZone = std::make_pair(0.0, 0.0);
        dustZone = protoplanetZone = zone1 = zone2 = zone3 = std::make_pair(0.0, 0.0);
        evaluated = false;
    }

    /// @brief Set the age of the star, in years.
    /// 
    /// If left at default, a middling age will be selected when evaluate() is called.
    /// Otherwise, it is clamped to a valid range.
    /// 
    /// @note: This value may be changed after evaluation, but that is not advised, since
    /// the age of the star affects planetary development.  Using absurd values could lead
    /// to undesirable side-effects.  Setting the age prior to evaluate() will guarantee
    /// sane values are selected.
    /// @param ageYears_ The age of the star, in years.
    void setAge(double ageYears_) { ageYears = ageYears_; }

    /// @brief Set the name of the star.
    /// 
    /// If no name is specified when the solar system is generated, the star will inherit
    /// the solar system's name.
    /// 
    /// The name may be changed once a Star has been added to a SolarSystem using SolarSystem::setStarName().
    /// @param name_ The new name of the star.
    void setName(const std::string& name_) { name = name_; }

    /// @brief Set the stellar type of this star.  Resets the derived traits.
    /// 
    /// Invalid subtype_ values will cause setType to fail, and reset the star to defaults.
    /// 
    /// Class O_V stars have valid subtype_ in the range of 3 to 9.  0 to 2 are invalid.
    /// @param type_ The primary classification of the star.
    /// @param subtype_ The subtype of the star.  Valid ranges are 0 to 9, inclusive.
    void setType(StarClassification type_, int32_t subtype_)
    {
        reset();
        if (subtype_ < 0 || subtype_ > 9)
        {
            return;
        }
        else if (type_ == StarClassification::O_V && subtype_ < 3)
        {
            return;
        }

        type = type_;
        subtype = subtype_;
    }

    private:
    //--- Primary stellar traits
    std::string name;

    StarClassification type = StarClassification::G_V; //!< Star type (classification/sequence)
    int32_t subtype = 2; //!< Star subtype.

    //--- Derived stellar traits
    bool evaluated = false; //!< Have the derived traits been evaluated?

    double ageYears = 0.0; //!< Age of the star, in years.

    float temperatureKelvin = 0.0f; //!< Temperature of the star's surface, in Kelvin.

    double luminositySolar = 0.0; //!< Luminosity of the star, in Solar luminosity.

    double radiusSolar = 0.0; //!< Radius of the star, in Solar radii.

    double massSolar = 0.0; //!< Mass of the star, in Solar masses.

    double ecosphere = 0.0; //!< Distance from the star best suited for Earth-like conditions, in AU.

    double snowLine = 0.0; //!< Minimum distance from the star where water ice may exist in a vaccuum, in AU.

    BandLimit_t habitableZone; //!< The habitable zone of the star, in AU.

    BandLimit_t dustZone; //!< The distance where dust may form, in AU.
    BandLimit_t protoplanetZone; //!< The distance where protoplanets may accrete, in AU.

    BandLimit_t zone1; //!< Zone 1 region, in AU.
    BandLimit_t zone2; //!< Zone 2 region, in AU.
    BandLimit_t zone3; //!< Zone 3 region, in AU.
};

}

}
