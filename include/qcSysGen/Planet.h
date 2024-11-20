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

#include "Consts.h"
#include "Enums.h"

#include <forward_list>
#include <string>
#include <vector>

namespace qc
{

namespace SystemGenerator
{

/// @brief Major components of an atmosphere
struct AtmosphereComponent
{
    Gas gas; //!< The gas
    float fraction; //!< The fraction of the atmosphere composed of this gas.

    /// @brief Less-than operator for sorting.
    /// @param rhs The AtmosphereComponent to which we are comparing this.
    /// @note The sense is reversed so the operator acts as greater-than, so the highest fraction is listed first.
    /// @return true if rhs.fraction is greater than this->fraction; false otherwise.
    bool operator<(const AtmosphereComponent& rhs) const { return fraction > rhs.fraction; }
};

class Generator;
class Star;

/// @brief Represents one of the bodies orbiting the central Star of the SolarSystem.
///
/// This entity may represent a rocky planet, a gas giant, or a dwarf planet.
/// 
/// @todo Parameters: orbital position placement, sphere of influence
class Planet
{
    public:
    Planet() { reset(); }

    /// @brief Constructor.  Sets the protoplanetary components of the planet.
    /// 
    /// Full evaluation and finalization is done in Planet::evaluate()
    /// @param sma Semi-major axis of the orbit, in AU.
    /// @param e Eccentricity of the orbit, in AU.
    /// @param dustMassComponent The mass of the dust component of the planet, in solar masses.
    /// @param gasMassComponent The mas of the volatile gas component of the planet, in solar masses.
    Planet(double sma, float e, double dustMassComponent, double gasMassComponent)
    {
        reset();
        semimajorAxis = sma;
        eccentricity = e;
        dustMass = dustMassComponent;
        gasMass = gasMassComponent;
        totalMass = dustMass + gasMass;
    }

    ~Planet() { }

    // @brief Add a new moon to the moons list.
    // @param m The new moon to add
    //void addMoon(const Planet& m);

    /// @brief Evaluate a planet.
    /// 
    /// This method is not intended to evaluate moons - that requires a separate method.
    /// @param generator The Generator (used for random numbers).
    /// @param star The star at the center of the SolarSystem.
    void evaluate(Generator& generator, const Star& star);

    /// @brief Returns a vector of the major components of the planet's atmosphere.
    /// @return The atmospheric components vector.
    const std::vector<AtmosphereComponent>& getAtmo() const { return atmosphere; }

    /// @brief The percentage of the planet obscured by clouds.
    /// @return The cloud coverage, in the range [0.0, 1.0].
    float getCloudPercentage() const { return cloudCoverage; }

    /// @brief The density of the planet.
    /// @return Density, in g/cc.
    float getDensity() const { return density; }

    /// @brief The dust/rocky component of the planet's mass.
    /// @return Dust mass, in Solar masses.
    double getDustMassComponent() const { return dustMass; }

    /// @brief A broad classification of how similar the planet may be to earth conditions.
    /// 
    /// A value of 1.0 indicates very earth-like conditions, a value of 0.0 indicates
    /// little-to-no earth similarity.
    /// 
    /// Formulation from https://phl.upr.edu/projects/earth-similarity-index-esi.
    /// @return The ESI, in the range [0.0, 1.0].
    float getEarthSimilarityIndex() const { return earthSimilarityIndex; }

    /// @brief Returns the orbital eccentricity.
    /// @return Eccentricity.
    float getEccentricity() const { return eccentricity; }

    /// @brief The gaseous component of the planet's mass.
    /// @return Gas mass, in Solar masses.
    double getGasMassComponent() const { return gasMass; }

    /// @brief Percentage of the planet covered with liquid water.
    /// @return The hydrosphere percentage, in the range [0.0, 1.0].
    float getHydroPercentage() const { return hydrosphere; }

    /// @brief Percentage of the planet covered with ices.
    /// @return The ice percentage, in the range [0.0, 1.0].
    float getIcePercentage() const { return iceCoverage; }

    /// @brief Mass of the planet.
    /// @return Mass, in Solar masses.
    double getMass() const { return totalMass; }

    // @brief Access an ordered vector of moons orbiting this planet.
    // 
    // @note Editing the moons after evaluation is not recommended.  Make sure
    // to call evaluate() again.
    // @return The moons.
    //std::vector<Planet>& getMoons() { return moon; }

    /// @brief Return the name of the planet/moon.
    /// @return The name of the planet
    const std::string& getName() const { return name; }

    /// @brief Return the orbital zone classification of this planet.
    /// @return The OrbitalZone.
    OrbitalZone getOrbitalZone() const { return orbitalZone; }

    /// @brief Returns the enumerated classification of the planet type.
    /// @return The planet type.
    PlanetType getPlanetType() const { return type; }

    /// @brief Returns the radius of the planet.
    /// @return THe radius, in km.
    float getRadius() const { return radius; }

    /// @brief Semimajor axis of the orbit, in AU.
    /// @return SMA of the orbit, in AU
    double getSemimajorAxis() const { return semimajorAxis; }

    /// @brief Returns the surface acceleration of gravity in G's.
    /// @return Acceleration in G's.
    double getSurfaceGravity() const { return surfaceAcceleration * AccelerationInGees; }

    /// @brief Return the surface atmospheric pressure.
    /// @return The surface pressure, in millibars.
    float getSurfacePressure() const { return surfacePressure; }

    /// @brief Return the mean surface temperature.
    /// @return Mean surface temperature, in Kelvin.
    float getSurfaceTemperature() const { return meanSurfaceTemperature; }

    /// @brief Returns the total mass of the planet's moons.
    /// @return Total mass, in solar masses.
    double getTotalMoonMass() const;

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

    /// @brief Zero all values, including the planet's name.
    /// @todo: remove reset() and simply use a constructor
    void reset()
    {
        name.clear();
        //moon.clear();
        semimajorAxis = 0.0;
        eccentricity = 0.0f;
        inclination = 0.0f;
        longitudeAscendingNode = 0.0f;
        argumentOfPeriapsis = 0.0f;
        meanAnomalyAtEpoch = 0.0;
        totalMass = dustMass = gasMass = 0.0;
        orbitalPeriod = 0.0;
        axialTilt = 0.0f;
        type = PlanetType::Unknown;
        rmsVelocity = 0.0f;
        exosphereTemperature = 0.0;
        minMolecularWeight = 0.0f;
        radius = 0.0f;
        escapeVelocity = 0.0f;
        surfaceAcceleration = 0.0f;
        boilingPoint = 0.0f;
        volatileGasInventory = 0.0;
        periapsis = apoapsis = 0.0;
        runawayGreenhouse = false;
        surfacePressure = 0.0f;
        evaluated = false;
    }

    /// @brief Set the name of the planet.
    /// 
    /// If no name is specified when the solar system is generated, the planet will inherit
    /// a name derived from the star identifying its ordinal placement in the system using
    /// Roman numerals - the closest planet to the star is (star name) I, the second planet is
    /// (star name) II, etc.
    /// @param name_ The new name of the planet.
    void setName(const std::string& name_) { name = name_; }

    private:
    friend class Generator;

    std::string name; //!< Name of the planet.

    //std::vector<Planet> moon; //!< Natural satellites of this planet.

    bool evaluated; //!< Has this planet been evaluated?

    //--- Keplerian Orbital parameters
    double semimajorAxis; //!< a - semimajor axis of the orbit (in AU!).  EDITABLE
    float eccentricity; //!< e - eccentricity of the orbit, [0, 1).  EDITABLE
    float inclination; //!< i - Inclination of the orbit above the ecliptic [0, pi).  EDITABLE
    float longitudeAscendingNode; //!< O (Omega, upper case) - or Right Ascension of the Ascending Node, measured from the vernal equinox.  EDITABLE
    float argumentOfPeriapsis; //!< w (Omega, lower case) - [0, 2 * pi).  EDITABLE
    double meanAnomalyAtEpoch; //!< M - where the body is at time = 0, [0, 2 * pi).  EDITABLE

    //--- Physical parameters
    double totalMass; //!< Mass of the planet, in solar masses.
    double dustMass; //!< Rocky portion of the mass, in solar masses.
    double gasMass;  //!< Gaseous portion of the mass, in solar masses.

    //--- Derived parameters
    double orbitalPeriod; //!< Orbital period around the parent, in Earth days.
    float axialTilt; //!< Angle between the planet's pole of rotation and the normal of the orbital plane, radians.  EDITABLE
    float orbitalDominance; //!< How thoroughly does a planet clear out its orbital neighborhood.  1.0 is the threshold between dwarf planets and planets.
    PlanetType type; //!< Type of planet.
    OrbitalZone orbitalZone; //!< Orbital zone classification of the planet.

    float dayLength; //!< Length of the local day, in hours.  EDITABLE
    float spinResonanceFactor; //!< If non-zero, the factor used to determine the spin resonance of the body's day.  If zero, the body does not have a resonant day length.
    bool resonant; //!< true if in resonant rotation

    double periapsis; //!< Minimum radius of the orbit, in AU.
    double apoapsis; //!< Maximum radius of the orbit, in AU.

    // Mass-derived values
    float radius; //!< Radius, km.   EDITABLE -- Note this will require some extra effort to do correctly.
    float escapeVelocity; //!< Escape velocity, m/s
    float surfaceAcceleration; //!< Surface acceleration from gravity, m/(s^2).
    float density; //!< Density, in g/cc.

    // Atmosphere
    float rmsVelocity; //!< RMS velocity of molecular nitrogen, in m/s.
    float minMolecularWeight; //!< The lightest molecular weight retianed by the planet, which can help suss out the atmospheric contents.
    bool runawayGreenhouse = false; //!< Is the atmosphere in a runaway greenhouse effect?
    float surfacePressure; //!< Atmospheric pressure, in millibars (mb).
    double volatileGasInventory; //!< Unitless parameter used to characterize surface pressure and hydrosphere percentage.
    std::vector<AtmosphereComponent> atmosphere; //!< Major components of the atmosphere

    // Temperatures
    // TODO: How big of a difference is there between max/min and high/low?
    float albedo; //!< Mean planetary albedo.
    float exosphereTemperature; //!< Temperature of the exosphere, Kelvin.
    float boilingPoint; //!< Boiling point of water, Kelvin.
    float meanSurfaceTemperature; //!< Mean surface temperature, in Kelvin.
    float maxTemperature; //!< Summer/Day temperature, in Kelvin.
    float minTemperature; //!< Winter/Night temperature, in Kelvin.
    float highTemperature; //!< Day-time temperature, in Kelvin.
    float lowTemperature; //!< Night-time temperature, in Kelvin.

    // Habitability characteristics
    float hydrosphere; //!< Percentage of the surface consisting of liquid water
    float iceCoverage; //!< Percentage of the surface consisting of frozen water
    float cloudCoverage; //!< Percentage of the surface obscured by clouds.
    float earthSimilarityIndex; //!< Rating of habitability potential relative to Earth [0.0, 1.0]

    //--- Methods

    /// @brief A structure containing transient values needed during evaluation of a planet.
    struct EvaluationState
    {
        double ecosphereRatio; //!< The stellarSMA divided by the star's ecosphere radius (1.0 = ideal ecosphere distance)
        double stellarMass; //!< Mass of the parent star.
        double stellarAge; //!< Age of the parent star.
        float materialZone; //!< The material band the planet/moon occupies (I, II, or III).  Fractional values indicate a location in the transition between two bands.
    };

    /// @brief Returns an updated albedo based on current surface conditions.  Used in the
    /// iterative surface conditions calculations.
    /// @param state The GenerationState, for the random number generator.
    /// @return Updated albedo, [0, 1].
    float calculateAlbedo(Generator& generator) const;

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
    void calculateDayLength(const EvaluationState& evaluationState);

    /// @brief Calculates the Earth Similarity Index.
    ///
    /// Computes the product of several criteria which collectively measure how similar
    /// various conditions are to Earth.  A score of 1 is most Earth-similar, a score
    /// of 0 is least Earth-similar.  Formulation from https://phl.upr.edu/projects/earth-similarity-index-esi
    /// @return The ESI, in the range [0, 1].
    float calculateEarthSimilarity() const;

    /// @brief For rocky planets, work up a description of the atmosphere.
    void calculateGases(const EvaluationState& evaluationState);

    /// @brief Compute the atmosphere's surface pressure.
    /// 
    /// Updates volatileGasInventory, surfacePressure, and boilingPoint.
    /// @param generator The Generator (used for random numbers).
    /// @param evaluationState Transient values used during generation
    void calculateSurfacePressure(Generator& generator, const EvaluationState& evaluationState);

    // @brief Used during protoplanet collision.
    // 
    // This method replaces the current unevaluated planet (sma / eccentricity / mass) with a larger protoplanet,
    // and it adds the current planet as a moon of the new planet.  This method should only be called during accretion,
    // since it does not perform a total exchange of values.
    // @param p The planet to exchange.
    //void exchange(Planet& p);

    /// @brief Returns the number of years for 1/e of the given gas to escape the
    /// planet.
    /// @param molecularMass Molecular mass of the gas in question.
    /// @return Years required for 1/e of the gas to escape.
    double getGasLife(double molecularMass) const;

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

    void initializeSurfaceConditions(const EvaluationState& evaluationState);

    /// @brief Use an iterative process to determine surface temperature,
    /// albedo, hydrosphere coverage, cloud coverage, etc.
    /// @param generator The Generator (used for random numbers).
    void iterateSurfaceConditions(Generator& generator, const EvaluationState& evaluationState);

    /// @brief Converges on the minimum molecular weight retained by a world.
    /// 
    /// This function accounts for gaseous escape over time using an iterative process
    /// to converge on a result.
    /// 
    /// Depends on values for escapeVelocity, exosphereTemperature,
    /// radius, and surfaceAcceleration.
    /// @param stellarAge The age of the star, in years.
    /// @return The lightest retained molecular weight.
    float minimumMolecularWeight(double stellarAge) const;

    /// @brief Set the range of temperatures likely to be found on the planet.
    ///
    /// Updates highTemperature, lowTemperature, maxTemp, minTemp.
    void setTemperatureRange();

    /// @brief One step of the surface conditions computations.
    /// @param generator The Generator (used for random numbers).
    void updateSurfaceConditions(Generator& generator, const EvaluationState& evaluationState);

    /// @brief Returns a unitless volatile inventory that is used for atmosphere and hydrosphere computations
    /// 
    /// Implements Fogg's eq.17.  The 'inventory' returned is unitless.
    /// @param generator The Generator (used for random numbers).
    /// @param evaluationState Transient values used during generation
    /// @return Volatile Inventory
    double volatileInventory(Generator& generator, const EvaluationState& evaluationState) const;
};


typedef std::vector<Planet> PlanetVector;
typedef std::forward_list<Planet> PlanetList;

}

}
