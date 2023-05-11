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

#include "Consts.h"

#include <forward_list>
#include <string>
#include <vector>

struct Config;
class GenerationState;
class Star;

/// @brief Enumeration of gases of interest in planetary atmospheres
///
/// @note If this is changed, update Planet::GasString()
enum class Gas
{
    Hydrogen,
    Helium,
    Nitrogen,
    Oxygen,
    Neon,
    Argon,
    Krypton,
    Xenon,
    Ammonia,
    Water,
    CarbonDioxide,
    Ozone,
    Methane
};

/// @brief Enumeration of planet types
///
/// @note If there are changes here, don't forget to update Planet::PlanetTypeStr().
enum class PlanetType
{
    Unknown, //!< Undetermined type

    //--- Rocky planet types
    Rocky, //!< Catch-all for non-gaseous planets.  Terrestrial planets.

    AsteroidBelt, //!< A collection of boulder-to-mountain sized rocky and metallic objects.
    IcePlanet, //!< A colder terrestrial planet dominated by surface ices.
    Terrestrial, //!< A rocky planet with atmosphere and liquid hydrosphere.
    Ocean, //!< A terrestrial world characterized by near-total coverage in water.

    //--- Gaseous planet types
    Gaseous,    //!< Catch-all for a planet with a substantial volatile gas composition.  Used as an interim classification

    IceGiant,   //!< The smaller sized gaseous planets, also called Neptunian.  Uranus and Neptune fall into this category.  Roughly 2.0 (+/-0.7)x M(Earth) to 0.41 (+/-0.07)x M(Jupiter).
    GasGiant,   //!< Large planets primarily composed of hydrogen and helium.  Jupiter falls into this category; Saturn barely makes the cutoff, as well.  0.41x M(Jupiter) on up.
    BrownDwarf, //!< Large gas giants, but not large enough to sustain nuclear fusion of hydrogen.  13x to 80x M(Jupiter).
};

/// @brief Major components of an atmosphere
struct AtmosphereComponent
{
    Gas gas; //!< The gas
    float fraction; //!< The fraction of the atmosphere composed of this gas.

    ///@brief Less-than operator for sorting.
    ///@Note The sense is reversed so the operator acts as greater-than, so the highest fraction is listed first.
    bool operator<(const AtmosphereComponent& rhs) const { return fraction > rhs.fraction; }
};

/// @brief Represents a planet or moon.
class Planet
{
    public:
    /// @brief Null constructor
    Planet();

    /// @brief Constructor.  Sets the protoplanetary components of the planet.
    /// 
    /// Full evaluation and finalization is done in Planet::evaluate()
    /// @param sma Semi-major axis of the orbit, in AU.
    /// @param e Eccentricity of the orbit, in AU.
    /// @param dustMassComponent The mass of the dust component of the planet, in solar masses.
    /// @param gasMassComponent The mas of the volatile gas component of the planet, in solar masses.
    Planet(double sma, float e, double dustMassComponent, double gasMassComponent);

    ~Planet() { }

    /// @brief Add a moon to this planet's list of moons.
    /// 
    /// During the step of planetary system evaluation where this method is called,
    /// the moon list contains mass info only.
    /// The semi-major axis and eccentricity are unevaluated, so we just tack the new mass onto
    /// the front of the list.
    /// @param m The new Planet representing the moon.
    void addMoon(const Planet& m)
    {
        moon.emplace_front(m);
    }

    /// @brief Evaluate a planet that has been generated via accretion.
    ///
    /// This step computes derived values (radius, temperature, classification, etc).
    ///
    /// The following values are assumed to be initialized: sma, eccentricity, mass, dustMass, gasMass, moon.
    /// All other values are finalized during evaluation.
    /// @param centralStar The central star of the planetary system.
    /// @param planet The parent planet of a moon, or nullptr if this ia a planet that orbits the central star.
    /// @param planetNumber The ordinal index of the planet, with 1 the closest to the star (or planet, in the case of moons).
    /// @param config The Config.  Used to retrieve customization parameters.
    /// @param state The GenerationState.  Used for random number generation
    void evaluate(const Star* centralStar, const Planet* planet, int planetNumber, const Config* config, GenerationState* state);

    /// @brief Used during protoplanet collision.
    /// 
    /// This method replaces the current unevaluated planet (sma / eccentricity / mass) with a larger protoplanet,
    /// and it adds the current planet as a moon of the new planet.  This method should only be called during accretion,
    /// since it does not perform a total exchange of values.
    /// @param p The planet to exchange.
    void exchange(Planet& p);

    //--- Accessors.  Most of them are valid only after calling evaluate().

    /// @brief Returns the percentage of the surface covered by clouds.
    /// @return A value in the range [0, 1].
    float cloudPercentage() const { return cloudCoverage; }

    /// @brief Returns the density of the planet.
    /// @return Density in g/cc.
    float density() const { return plDensity; }

    /// @brief The dust/rocky component of the planet's mass.
    /// @return Dust mass, in Solar masses.
    double dustMassComponent() const { return dustMass; }

    /// @brief Return the eccentricity of the planet's orbit.
    /// @return Eccentricity, [0, 1).
    float e() const { return eccentricity; }

    /// @brief Escape velocity of the planet.
    /// @return Escape velocity, in m/s.
    float escapeVelocity() const { return escapeVel; }

    /// @brief The Earth Similarity Index
    /// 
    /// An evaluation of how similar a planet's surface conditions are to Earth's.
    /// It is composed from planetary mass and radius, as well as escape velocity and
    /// surface temperature.  If Config::computeGases is true, the partial pressure
    /// of oxygen is also considered.
    /// 
    /// A value of 0 is not-at-all Earth-similar.  A value of 1 is identical to Earth.
    /// A score of > 0.8 is worth evaluating in greater depth for finding potential
    /// habitable planets.
    /// 
    /// The ESI is based on material published by the Planetary Habitability Laboratory
    /// managed by the University of Puerto Rico at Arecibo.
    /// @return The ESI, [0, 1].
    float esi() const { return earthSimilarityIndex; }

    /// @brief Exospheric temperature of the planet.
    /// @return Exospheric temperature, in Kelvin.
    float exosphereTemperature() const { return exosphereTemp; }

    /// @brief The gaseous component of the planet's mass.
    /// @return Gas mass, in Solar masses.
    double gasMassComponent() const { return gasMass; }

    /// @brief Return a string naming the Gas
    /// @param gas The Gas to name
    /// @return The corresponding string
    static std::string GasString(Gas gas);

    /// @brief Returns the percentage of the surface covered by liquid water
    /// @return A value in the range [0, 1].
    float hydroPercentage() const { return hydrosphere; }

    /// @brief Returns the percentage of the surface covered by ices.
    /// @return A value in the range [0, 1].
    float icePercentage() const { return iceCoverage; }

    /// @brief Is this planet a gaseous planet (gas giant, ice giant, etc)
    /// @return true if the planet is one of the gaseous types.
    bool isGaseous() const
    {
        return
            (type == PlanetType::Gaseous) ||
            (type == PlanetType::IceGiant) ||
            (type == PlanetType::GasGiant) ||
            (type == PlanetType::BrownDwarf);
    }

    /// @brief Mass of the planet.
    /// @return Mass, in Solar masses.
    double mass() const { return totalMass; }

    /// @brief Returns the planet's ordinal position in the planetary system (1 = closest to star),
    /// or the moon's ordinal position in the moon system (1 = closest to planet).
    /// @return The 1-based ordinal position.
    int ordinal() const { return planetNo; }

    /// @brief Returns the classification of the planet type.
    /// @return PlanetType
    PlanetType planetType() const { return type; }

    /// @brief Returns a string for the PlanetType.
    /// @return The string corresponding th the planet type.
    static std::string PlanetTypeString(PlanetType type);

    /// @brief Radius of the planet.
    /// @return Radius, in km.
    double r() const { return radius; }

    /// @brief The semi-major axis of the planet's orbit around its parent body.
    /// @return SMA in AU.
    double sma() const { return semiMajorAxis; }

    /// @brief Acceleration on the surface
    /// @return Surface acceleration, in m/(s^2).
    double surfaceAcceleration() const { return surfaceAccel; }

    /// @brief Returns the surface acceleration of gravity in G's.
    /// @return Acceleration in G's.
    double surfaceGravity() const { return surfaceAccel * AccelerationInGees; }

    /// @brief Returns the atmospheric pressure at the surface of the planet.
    /// @return Surface pressure, in millibars.
    float surfacePressure() const { return surfacePress; }

    /// @brief Returns the mean surface temperature of the planet.
    /// @return  Surface temp, in Kelvin.
    float surfaceTemperature() const { return meanSurfaceTemp; }

    //-- Moon system inspection methods.

    /// @brief Return an iterator for the first moon orbiting the planet.
    /// @return Beginning iterator
    std::forward_list<Planet>::const_iterator beginMoon() const { return moon.begin(); }

    /// @brief Return an iterator for the past-the-last moon orbiting the planet.
    /// @return End iterator
    std::forward_list<Planet>::const_iterator endMoon() const { return moon.end(); }

    /// @brief Returns the number of moons orbiting the planet.
    /// @return The moon count.
    size_t sizeMoon() const { return std::distance(moon.begin(), moon.end()); }

    /// @brief Returns the total mass of the planet's moons.
    /// @return Total mass, in solar masses.
    double totalMoonMass() const;

    //-- Atmosphere inspection methods.

    /// @brief Begin iterator for the vector of atmospheric gases.
    /// @return Begin iterator
    std::vector<AtmosphereComponent>::const_iterator beginAtmo() const { return atmosphere.begin(); }

    /// @brief End iterator for the vector of atmospheric gases.
    /// @return End iterator
    std::vector<AtmosphereComponent>::const_iterator endAtmo() const { return atmosphere.end(); }

    /// @brief Returns the number of gases recorded in the atmosphere.
    /// @return 
    size_t sizeAtmo() const { return atmosphere.size(); }

    private:

    /// @brief Factor used to convert water vapor mass to cloud coverage, km^2 / kg
    static constexpr double CloudCoverageFactor = 1.839e-8;

    /// @brief Parameter used for determinine the effect of greenhouse gases on temperature.
    ///
    /// from Hart, eq.20
    static constexpr float EarthConvectionFactor = 0.43f;

    /// @brief The minimum ratio of gas mass to total mass to qualify as a gaseous planet
    static constexpr double GaseousPlanetThreshold = 0.05;

    /// @brief Albedo used to determine if the planet has a greenhouse effect.
    static constexpr float GreenhouseTriggerAlbedo = 0.20f;

    /// @brief The minimum ratio of gas mass to total mass to qualify as an ice planet
    static constexpr double IcePlanetThreshold = 0.000001;

    /// @brief Used in planetary day length computations.  cm^2/s^2 * g
    static constexpr double J = 1.46e-19;

    /// @brief Used to determine cloud coverage.  1/Kelvin (~=14.33)
    static constexpr double Q2_36 = 0.0698;

    /// @brief 3x the standard deviation of the solar system's 4 gaseous worlds' albedo.
    static constexpr float ThreeSigma_Albedo_GasGiant = 0.1185f;

    int planetNo; //!< Planet or moon ordinal number (1 is closest to parent)

    PlanetType type; //!< Classification of the planet type.

    double semiMajorAxis;  //!< Semi-major axis of the planet's orbit, in AU.  From the star for a planet, from the planet for a moon.
    double solarSMA; //!< Semi-major axis of the planet's orbit around the star.  For a moon, the SMA of the moon's parent, in AU.

    float eccentricity; //!< Eccentricity of the planet's orbit.
    float solarEccentricity; //!< For a moon, the eccentricity of the parent planet's orbit.  For a planet, same as eccentricity.

    double totalMass; //!< Mass of the planet, in solar masses.
    double dustMass; //!< Rocky portion of the mass, in solar masses.
    double gasMass;  //!< Gaseous portion of the mass, in solar masses.

    float radius; //!< Equatorial radius, in km.
    float plDensity; //!< Density, in g/cc.

    float albedo; //!< Albedo of the planet
    float hydrosphere; //!< Fraction of the surface covered with liquid water, [0, 1].
    float cloudCoverage; //!< Fraction of the surface covered with clouds, [0, 1].
    float iceCoverage; //!< Fraction of the surface covered with ice, [0, 1].

    float axialTilt; //!< Axial tilt, in units of degrees.

    double orbitalPeriod; //!< Length of the orbital period, in Earth days.
    float day; //!< Length of the local day, in hours.

    bool resonant; //!< true if in resonant rotation
    float spinResonanceFactor; //!< If non-zero, the factor used to determine the spin resonance of the body's day.  If zero, the body does not have a resonant day length.

    float escapeVel; //!< Escape velocity of the body, in m/s.
    float surfaceAccel; //!< Surface acceleration, in m/(s^2)

    float rmsVelocity; //!< RMS velocity of molecular nitrogen, in m/s.
    float minMolecularWeight; //!< The lightest molecular weight retianed by the planet, which can help suss out the atmospheric contents.
    double volatileGasInventory; //!< Unitless parameter used to characterize surface pressure and hydrosphere percentage.
    float surfacePress; //!< Atmospheric pressure, in millibars (mb).
    bool runawayGreenhouse; //!< Does runaway greenhouse affect the planet?

    float boilingPoint; //!< The boiling point of water, in Kelvin.
    float exosphereTemp; //!< Exosphere tempearture, in Kelvin.
    float meanSurfaceTemp; //!< surface temperature, in Kelvin
    float highTemp; //!< Day-time temperature, in Kelvin.
    float lowTemp; //!< Night-time temperature, in Kelvin.
    float maxTemp; //!<// Summer/Day temperature, in Kelvin.
    float minTemp; //!< Winter/Night temperature, in Kelvin.

    std::vector<AtmosphereComponent> atmosphere; //!< Major components of the atmosphere

    float earthSimilarityIndex; //!< How Earth-like are the surface conditions?

    //int minorMoons; //!< Count of lesser moons that weren't worth working up in the moon list.
    std::forward_list<Planet> moon; //!< Significant moons, sorted closest-to-farthest.

    const Star* star; //!< The central star

    /// @brief Returns an updated albedo based on current surface conditions.  Used in the
    /// iterative surface conditions calculations.
    /// @param state The GenerationState, for the random number generator.
    /// @return Updated albedo, [0, 1].
    float calculateAlbedo(GenerationState* state) const;

    /// @brief Computes the length of the day, and determines day-to-year resonance.
    ///
    /// Per the original documentation for this function (note that variable names are
    /// changed in this implementation, and values are not returned, since they're
    /// components of this object):
    ///
    /// Fogg's information for this routine came from Dole "Habitable Planets
    /// for Man", Blaisdell Publishing Company, NY, 1964.  From this, he came
    /// up with his eq.12, which is the equation for the 'base_angular_velocity'
    /// below.  He then used an equation for the change in angular velocity per
    /// time (dw/dt) from P. Goldreich and S. Soter's paper "Q in the Solar
    /// System" in Icarus, vol 5, pp.375-389 (1966).  Using as a comparison the
    /// change in angular velocity for the Earth, Fogg has come up with an
    /// approximation for our new planet (his eq.13) and take that into account.
    /// This is used to find 'change_in_angular_velocity' below.
    ///
    /// Input parameters are mass (in solar masses), radius (in Km), orbital
    /// period (in days), orbital radius (in AU), density (in g/cc),
    /// eccentricity, and whether it is a gas giant or not.
    /// The length of the day is returned in units of hours.
    void calculateDayLength();

    /// @brief Calculates the Earth Similarity Index.
    ///
    /// Computes the product of several criteria which collectively measure how similar
    /// various conditions are to Earth.  A score of 1 is most Earth-similar, a score
    /// of 0 is least Earth-similar.  Formulation from https://phl.upr.edu/projects/earth-similarity-index-esi
    void calculateEarthSimilarity();

    /// @brief For rocky planets, work up a description of the atmosphere.
    void calculateGases();

    /// @brief One step of the surface conditions computations.
    /// @param initialize Initialize conditions (for the first pass).
    /// @param state The GenerationState, for the random number generator.
    void calculateSurfaceConditions(bool initialize, GenerationState* state);

    /// @brief Compute the atmosphere's surface pressure.
    /// 
    /// Updates volatileGasInventory, surfacePressure, and boilingPoint.
    /// @param state The GenerationState, for the random number generator.
    void calculateSurfacePressure(GenerationState* state);

    /// @brief Returns the effective temperature based on the specified albedo.
    /// @param albedo Planet's albedo, in the range [0, 1].
    /// @return Effective temperature, in Kelvin.
    float effectiveTemperature(float albedo) const;

    /// @brief Returns the temperature effect of greenhouse gases.
    /// 
    /// From Burrows 2006:
    /// This is Fogg's eq.20, and is also Hart's eq.20 in his "Evolution of
    /// Earth's Atmosphere" article.  The effective temperature given is in
    /// units of Kelvin, as is the rise in temperature produced by the
    /// greenhouse effect, which is returned.
    /// tuned this by changing a pow(x,.25) to pow(x,.4) to match Venus - JLB
    /// @param effectiveTemperature The effective temperature at the time of evaluation
    /// @return Temperature increase in Kelvin.
    float greenhouseRise(float effectiveTemperature) const;

    /// @brief Use an iterative process to determine surface temperature,
    /// albedo, hydrosphere coverage, cloud coverage, etc.
    /// @param state The GenerationState, for the random number generator.
    void iterateSurfaceConditions(GenerationState* state);

    /// @brief Set the range of temperatures likely to be found on the planet.
    ///
    /// Updates highTemp, lowTemp, maxTemp, minTemp.
    void setTemperatureRange();

    /// @brief Returns a unitless volatile inventory that is used for atmosphere and hydrosphere computations
    /// 
    /// Implements Fogg's eq.17.  The 'inventory' returned is unitless.
    /// @param state The GenerationState, for the random number generator.
    /// @return Volatile Inventory
    double volatileInventory(GenerationState* state) const;
};

/// @brief Convenience type definition for std::forward_list<Planet>.
typedef std::forward_list<Planet> PlanetList;
