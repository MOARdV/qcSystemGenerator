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
#include <qcSysGen/Planet.h>

#include <qcSysGen/Config.h>
#include <qcSysGen/Equations.h>
#include <qcSysGen/Generator.h>
#include <qcSysGen/Star.h>

#include <assert.h>
#include <numeric>

namespace
{

// Constants used during evaluation

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

using qc::SystemGenerator::Gas;

struct ChemicalTable
{
    Gas chemical;
    float atomicWeight; //!< Atomic weight of the atom / molecule
    float meltingPoint; //!< Melting point, in Kelvin.
    float boilingPoint; //!< Boiling point, in Kelvin.
    float density; //!< Density, g/cc.

    float abundE;
    float abundS;
    float reactivity;
    float maxIPP; //!< Maximum inspired partial pressure
};

using qc::SystemGenerator::MbPerMmhg;

/// @brief Table of atmospheric gases gleaned from Burrows 2006.  Max IPP values imported from Dole 1969.
static constexpr ChemicalTable gases[] =
{
        { Gas::Hydrogen, 1.0079f,  14.06f, 20.40f, 8.99e-05f, 0.00125893f, 27925.4f, 1.0f, 0.0f },
        { Gas::Helium, 4.0026f, 3.46f, 4.20f, 0.0001787f, 7.94328e-09f, 2722.7f, 0.0f, (61000.0f * MbPerMmhg) },
        { Gas::Nitrogen, 14.0067f, 63.34f, 77.40f, 0.0012506f, 1.99526e-05f, 3.13329f, 0.0f, (2330.0f * MbPerMmhg) },
        { Gas::Oxygen, 15.9994f, 54.80f, 90.20f, 0.001429f, 0.501187f, 23.8232f, 10.0f, (400.0f * MbPerMmhg) },
        { Gas::Neon, 20.1700f, 24.53f, 27.10f, 0.0009f, 5.01187e-09f, 3.4435e-5f, 0.0f, (3900.0f * MbPerMmhg) },
        { Gas::Argon, 39.9480f, 84.00f, 87.30f, 0.0017824f, 3.16228e-06f, 0.100925f, 0.0f, (1220.0f * MbPerMmhg) },
        { Gas::Krypton, 83.8000f, 116.60f, 119.70f, 0.003708f, 1e-10f, 4.4978e-05f, 0.0f, (350.0f * MbPerMmhg) },
        { Gas::Xenon, 131.3000f, 161.30f, 165.00f, 0.00588f, 3.16228e-11f, 4.69894e-06f, 0.0f, (160.0f * MbPerMmhg) },
        { Gas::Ammonia, 17.0000f, 195.46f, 239.66f, 0.001f, 0.002f, 0.0001f, 1.0f, (100.0f * MbPerMmhg) },
        { Gas::Water, 18.0000f, 273.16f, 373.16f, 1.000f, 0.03f, 0.001f, 0.0f, 0.0f },
        { Gas::CarbonDioxide, 44.0000f, 194.66f, 194.66f, 0.001f, 0.01f, 0.0005f, 0.0f, (7.0f * MbPerMmhg) },
        { Gas::Ozone, 48.0000f, 80.16f, 161.16f, 0.001f, 0.001f, 0.000001f, 2.0f, (0.10f * MbPerMmhg) },
        { Gas::Methane, 16.0000f,  90.16f, 109.16f, 0.010f, 0.005f, 0.0001f, 1.0f, (50000.0f * MbPerMmhg) }
};
static constexpr int gasesCount = _countof(gases);


//----------------------------------------------------------------------------
/// @brief Given surface atmospheric pressure, return the boiling point of water.
///
/// This function returns the boiling point of water in an atmosphere of
/// pressure 'surf_pressure', given in millibars.	The boiling point is
/// returned in units of Kelvin.  This is Fogg's eq.21.
/// @param surfacePressureMb Surface pressure, in mb.
/// @return Boiling point of water, in Kelvin.  0 if there is no atmospheric pressure.
float BoilingPoint(float surfacePressureMb)
{
    const double surfacePressureBars = surfacePressureMb * qc::SystemGenerator::BarPerMillibar;

    // TODO: Where do the magic numbers come from?
    return static_cast<float>(1.0 / ((log(surfacePressureBars) / -5050.5) + (1.0 / 373.0)));
}

//----------------------------------------------------------------------------
/// @brief Returns the effective temperature based on the specified albedo and relative distance from the ecosphere.
/// @param distanceRatio The normalized distance of the planet's SMA relative to the star's ecosphere.
/// @param albedo Planet's albedo, in the range [0, 1].
/// @return Effective temperature, in Kelvin.
float EffectiveTemperature(double distanceRatio, float albedo)
{
    return static_cast<float>(sqrtf(static_cast<float>(1.0 / distanceRatio))
                              * powf((1.0f - albedo) / (1.0f - qc::SystemGenerator::Albedo_Earth), 0.25f)
                              * qc::SystemGenerator::EarthEffectiveTemperature);
}

//----------------------------------------------------------------------------
/// @brief Returns a unitless opacity value used for determining the greenhouse effect
/// on the planet
/// @return Opacity.
float Opacity(float minMolecularWeight, float surfacePressure)
{
    float opticalDepth = 0.0f;

    // TODO: Can I make these functions, instead of if-else blocks?
    if ((minMolecularWeight >= 0.0f) && (minMolecularWeight < 10.0f))
    {
        opticalDepth = opticalDepth + 3.0f;
    }
    if ((minMolecularWeight >= 10.0f) && (minMolecularWeight < 20.0f))
    {
        opticalDepth = opticalDepth + 2.34f;
    }
    if ((minMolecularWeight >= 20.0f) && (minMolecularWeight < 30.0f))
    {
        opticalDepth = opticalDepth + 1.0f;
    }
    if ((minMolecularWeight >= 30.0f) && (minMolecularWeight < 45.0f))
    {
        opticalDepth = opticalDepth + 0.15f;
    }
    if ((minMolecularWeight >= 45.0f) && (minMolecularWeight < 100.0f))
    {
        opticalDepth = opticalDepth + 0.05f;
    }

    using qc::SystemGenerator::EarthSurfacePressure;
    if (surfacePressure >= (70.0f * EarthSurfacePressure))
    {
        opticalDepth = opticalDepth * 8.333f;
    }
    else if (surfacePressure >= (50.0f * EarthSurfacePressure))
    {
        opticalDepth = opticalDepth * 6.666f;
    }
    else if (surfacePressure >= (30.0f * EarthSurfacePressure))
    {
        opticalDepth = opticalDepth * 3.333f;
    }
    else  if (surfacePressure >= (10.0f * EarthSurfacePressure))
    {
        opticalDepth = opticalDepth * 2.0f;
    }
    else if (surfacePressure >= (5.0f * EarthSurfacePressure))
    {
        opticalDepth = opticalDepth * 1.5f;
    }

    return opticalDepth;
}

//----------------------------------------------------------------------------
/// @brief A smoothstep function
/// @todo Double-check that's how the function is used.
/// @param x Range over [0, 1].
/// @return A smoothed value between [0, 1].
inline float lim(float x)
{
    return x / sqrt(sqrt(1 + x * x * x * x));
}

//----------------------------------------------------------------------------
/// @brief Returns an interpolated value between max and min.
/// @todo Double-check that's how the function is used.
/// @param v The value to interpolate
/// @param max Max value
/// @param min Min value
/// @return The interpolated value
inline float soft(float v, float max, float min)
{
    const float dv = v - min;
    const float dm = max - min;
    return (lim(2.0f * dv / dm - 1.0f) + 1.0f) * 0.5f * dm + min;
}

}

namespace qc
{

namespace SystemGenerator
{

#if 0
//----------------------------------------------------------------------------
void Planet::addMoon(const Planet& m)
{
    for (PlanetVector::const_iterator i = moon.begin(); i != moon.end(); ++i)
    {
        if (m.getSemimajorAxis() < i->getSemimajorAxis())
        {
            moon.insert(i, m);
            return;
        }
    }
    moon.emplace_back(m);
}
#endif

//----------------------------------------------------------------------------
float Planet::calculateAlbedo(Generator& generator) const
{
    float waterFraction = hydrosphere;
    float iceFraction = iceCoverage;

    // 1 - waterFraction - iceFraction can be slightly negative due to floating-point
    // precision issues, so we clamp the lower limit to 0.
    float rockFraction = std::max(0.0f, 1.0f - waterFraction - iceFraction);

    float components = 0.0f + ((waterFraction > 0.0f) ? 1.0f : 0.0f) + ((iceFraction > 0.0f) ? 1.0f : 0.0f) + ((rockFraction > 0.0f) ? 1.0f : 0.0f);
    assert(components > 0.0f);

    const float cloudAdjustment = cloudCoverage / components;

    waterFraction = std::max(0.0f, waterFraction - cloudAdjustment);
    iceFraction = std::max(0.0f, iceFraction - cloudAdjustment);
    rockFraction = std::max(0.0f, rockFraction - cloudAdjustment);

    const bool airless = (surfacePressure == 0.0f);
    const float waterAlbedo = waterFraction * ((airless) ? 0.0f : generator.randomNear(Albedo_Water, Albedo_Water * 0.2f));
    const float iceAlbedo = iceFraction * ((airless) ? generator.randomNear(Albedo_IceAirless, Albedo_IceAirless * 0.4f) : generator.randomNear(Albedo_Ice, Albedo_Ice * 0.1f));
    const float rockAlbedo = rockFraction * ((airless) ? generator.randomNear(Albedo_RockAirless, Albedo_RockAirless * 0.3f) : generator.randomNear(Albedo_Rock, Albedo_Rock * 0.1f));
    const float cloudAlbedo = cloudCoverage * ((airless) ? 0.0f : generator.randomNear(Albedo_Cloud, Albedo_Cloud * 0.2f));

    return waterAlbedo + iceAlbedo + rockAlbedo + cloudAlbedo;
}

//----------------------------------------------------------------------------
void Planet::calculateDayLength(const EvaluationState& evaluationState)
{
    // TODO: This doesn't work for moons

    const double massGr = totalMass * SolarMassInGrams;
    const double yearHours = orbitalPeriod * HoursPerDay;

    // TODO: Where does this magic number come from?  How is it derived, and can it be
    // computed numerically instead of being treated as a hard-coded value?
    const double k2 = (isGaseous()) ? 0.24 : 0.33;
    const double baseAngularVelocity = sqrt(2.0 * J * (massGr) / (k2 * pow(radius * CmPerKm, 2.0)));

    // This next calculation determines how much the planet's rotation is
    // slowed by the presence of the star.

    const double deltaAngularVelocity = ChangeInEarthAngularVelocity *
        (density / EarthDensity) *
        (radius / EarthRadiusKm) *
        (EarthMassInGrams / massGr) *
        pow(evaluationState.stellarMass, 2.0) *
        (1.0 / pow(semimajorAxis, 6.0));
    const double angularVelocity = baseAngularVelocity + deltaAngularVelocity * evaluationState.stellarAge;

    if (angularVelocity <= 0.0)
    {
        dayLength = static_cast<float>(yearHours);
    }
    else
    {
        dayLength = static_cast<float>(RadiansPerCircle / (SecondsPerHour * angularVelocity));
    }

    spinResonanceFactor = 0.0f;
    if (dayLength >= yearHours)
    {
        resonant = true;
        if (eccentricity > 0.1)
        {
            spinResonanceFactor = static_cast<float>((1.0 - eccentricity) / (1.0 + eccentricity));
        }
        else
        {
            spinResonanceFactor = 1.0f;
        }

        dayLength = static_cast<float>(spinResonanceFactor * yearHours);
    }
}

//----------------------------------------------------------------------------
float Planet::calculateEarthSimilarity() const
{
    if (isGaseous() || type == PlanetType::AsteroidBelt)
    {
        // Early out - too dissimilar to consider.
        return 0.0f;
    }

    // Criteria for earth similarity.  All weights from
    // https://phl.upr.edu/projects/earth-similarity-index-esi

    const float NumberOfWeights = (!atmosphere.empty()) ? 5.0f : 4.0f;

    static constexpr float RadiusWeight = 0.57f;
    const float radiusRating = powf(1.0f - static_cast<float>(fabs(radius - EarthRadiusKm) / (radius + EarthRadiusKm)), RadiusWeight / NumberOfWeights);

    static constexpr float DensityWeight = 1.07f;
    const float densityRating = powf(1.0f - static_cast<float>(fabs(density - EarthDensity) / (density + EarthDensity)), DensityWeight / NumberOfWeights);

    static constexpr float EvWeight = 0.70f;
    const float escapeVelocityRating = powf(1.0f - fabsf(escapeVelocity - EarthEscapeVelocity) / (escapeVelocity + EarthEscapeVelocity), EvWeight / NumberOfWeights);

    static constexpr float TemperatureWeight = 5.58f;
    const float surfaceTempRating = powf(1.0f - fabsf(meanSurfaceTemperature - EarthAverageTemperature) / (meanSurfaceTemperature + EarthAverageTemperature), TemperatureWeight / NumberOfWeights);

    float oxygenRating = 1.0f;
    if (!atmosphere.empty())
    {
        // TODO: Refine this if I can find guidance on how the exponents were derived.
        const auto o2 = std::find_if(atmosphere.begin(), atmosphere.end(), [](const AtmosphereComponent& c) { return c.gas == Gas::Oxygen; });
        const float ppo = surfacePressure * ((o2 == atmosphere.end()) ? 0.0f : o2->fraction);
        // Partial pressure of oxygen.  Exponent is still work-in-progress.
        static constexpr float PpoWeight = 2.5f;
        oxygenRating = powf(1.0f - fabsf(ppo - EarthPartialPressureOxygen) / (ppo + EarthPartialPressureOxygen), PpoWeight / NumberOfWeights);
    }

    return radiusRating * densityRating * escapeVelocityRating * surfaceTempRating * oxygenRating;
}

//----------------------------------------------------------------------------
void Planet::calculateGases(const EvaluationState& evaluationState)
{
    atmosphere.clear();

    if (surfacePressure > 0.0f)
    {
        const float pressure = surfacePressure * BarPerMillibar;
        const float starAgeOver2B = static_cast<float>(evaluationState.stellarAge / 2.0e9);

        float totalAmount = 0.0f;

        for (int i = 0; i < gasesCount; ++i)
        {
            const float yp = gases[i].boilingPoint /
                (373.0f * ((logf(pressure + 0.001f) / -5050.5f) + (1.0f / 373.0f)));

            if ((yp >= 0.0f && yp < lowTemperature)
                && (gases[i].atomicWeight >= minMolecularWeight))
            {
                const float vrms = static_cast<float>(RMSVelocity(gases[i].atomicWeight, exosphereTemperature));
                const float pvrms = powf(1.0f / (1.0f + vrms / escapeVelocity), static_cast<float>(evaluationState.stellarAge / 1.0e9));
                float abund = gases[i].abundS;
                float react = 1.0f;
                float fract = 1.0f;
                float pres2 = 1.0f;

                // TODO: Store 1.0f / (1.0f + reactivity) in the structures, instead of computing it every run
                if (gases[i].chemical == Gas::Argon)
                {
                    react = 0.15f * static_cast<float>(evaluationState.stellarAge / 4.0e9);
                }
                else if (gases[i].chemical == Gas::Helium)
                {
                    abund *= 0.001f + static_cast<float>(gasMass / totalMass);
                    pres2 = (0.75f + pressure);
                    react = powf(1.0f / (1.0f + gases[i].reactivity), starAgeOver2B * pres2);
                }
                else if (gases[i].chemical == Gas::Oxygen &&
                         evaluationState.stellarAge > 2.0e9 &&
                         // TODO: Where do these come from?
                         meanSurfaceTemperature > 270.0f && meanSurfaceTemperature < 400.0f)
                {
                    pres2 = (0.89f + pressure / 4.0f); /*	Breathable - M: .6 -1.8 	*/
                    react = powf(1.0f / (1.0f + gases[i].reactivity), static_cast<float>(pow(starAgeOver2B, 0.25)) * pres2);
                }
                else if (gases[i].chemical == Gas::CarbonDioxide &&
                         evaluationState.stellarAge > 2.0e9 &&
                         meanSurfaceTemperature > 270.0f && meanSurfaceTemperature < 400.0f)
                {
                    pres2 = 0.75f + pressure;
                    react = powf(1.0f / (1.0f + gases[i].reactivity), static_cast<float>(pow(starAgeOver2B, 0.5)) * pres2);
                    react *= 1.5f;
                }
                else
                {
                    pres2 = 0.75f + pressure;
                    react = powf(1.0f / (1.0f + gases[i].reactivity), starAgeOver2B * pres2);
                }

                const float fraction = (1.0f - (minMolecularWeight / gases[i].atomicWeight));

                const AtmosphereComponent c = { gases[i].chemical, abund * pvrms * react * fract };
                if (c.fraction > 0.0f)
                {
                    atmosphere.emplace_back(c);
                    totalAmount += c.fraction;
                }
            }
        }

        for (auto& comp : atmosphere)
        {
            comp.fraction = comp.fraction / totalAmount;
        }
        std::sort(atmosphere.begin(), atmosphere.end());
    }
}

//----------------------------------------------------------------------------
void Planet::calculateSurfacePressure(Generator& generator, const EvaluationState& evaluationState)
{
    volatileGasInventory = volatileInventory(generator, evaluationState);
    if (volatileGasInventory > 0.0)
    {
        const double radiusRatio = EarthRadiusKm / radius;

        surfacePressure = static_cast<float>(
            volatileGasInventory * getSurfaceGravity() *
            EarthSurfacePressure * BarPerMillibar /
            (radiusRatio * radiusRatio)
            );

        boilingPoint = BoilingPoint(surfacePressure);
    }
    else
    {
        surfacePressure = boilingPoint = 0.0f;
    }
}

//----------------------------------------------------------------------------
void Planet::evaluate(Generator& generator, const Star& star)
{
    // TODO: Don't overwrite manually-set values.  Which ones are those, and how do I split them out?
    // TODO: Synthesize dust mass / gas mass for manual worlds.
#ifdef ALLOW_DEBUG_PRINTF
    if (generator.getVerbose())
    {
        printf(__FUNCTION__"() @ %.3lfAU:\n", semimajorAxis);
    }
#endif

    EvaluationState evaluationState;
    evaluationState.ecosphereRatio = semimajorAxis / star.getEcosphere();
    evaluationState.stellarMass = star.getMass();
    evaluationState.stellarAge = star.getAge();
    evaluationState.materialZone = star.getMaterialZone(semimajorAxis);

    orbitalPeriod = Period(semimajorAxis, totalMass, star.getMass());

    periapsis = semimajorAxis * (1.0f - eccentricity);
    apoapsis = semimajorAxis * (1.0f + eccentricity);

    orbitalDominance = OrbitalDominance(totalMass, semimajorAxis);

    axialTilt = generator.randomTilt(semimajorAxis);

    exosphereTemperature = static_cast<float>(EarthExosphereTemperature / (evaluationState.ecosphereRatio * evaluationState.ecosphereRatio));

    rmsVelocity = static_cast<float>(RMSVelocity(Weight_MolecularNitrogen, exosphereTemperature));

    // Do some preliminary computations on the planet.  These steps will be used to see if the results of
    // accretion were sufficient to create a gaseous planet.  If the mass is below the critical limit,
    // then we know it's a rocky world.  Likewise, if less than 5% of the mass of the world is in gas, we
    // treat it as a rocky world.
    // TODO: Do I need to bother checking CriticalLimit?  Gas isn't retained unless we've passed it.
    // Or do I bother with the 5% threshold, since I see planets getting swept up by the gas/total mass test in the
    // next if block.
    if (dustMass > CriticalLimit(semimajorAxis, eccentricity, star.getLuminosity()) && (gasMass / totalMass) > GaseousPlanetThreshold)
    {
        // Assume it's a successful gas giant
        radius = static_cast<float>(KothariRadius(totalMass, semimajorAxis, true, evaluationState.materialZone));
        escapeVelocity = static_cast<float>(EscapeVelocity(totalMass, radius));
        surfaceAcceleration = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

        minMolecularWeight = static_cast<float>(minimumMolecularWeight(star.getAge()));

        const bool sufficientMolecularRetention = (minMolecularWeight <= 4.0f);
        const bool sufficientOverallMass = (totalMass > RockyTransition);
        type = (sufficientMolecularRetention && sufficientOverallMass) ? PlanetType::Gaseous : PlanetType::Rocky;
#ifdef ALLOW_DEBUG_PRINTF
        if (generator.getVerbose())
        {
            printf(" ... Planet looks like a gas giant? %s molecular retention, %s overall mass.\n",
                   sufficientMolecularRetention ? "adequate" : "insufficient",
                   sufficientOverallMass ? "adequate" : "insufficient");
        }
#endif
    }
    else
    {
        type = PlanetType::Rocky;
    }

    if (type == PlanetType::Rocky)
    {
        assert(!(gasMass / totalMass > GaseousPlanetThreshold) || (dustMass > CriticalLimit(semimajorAxis, eccentricity, star.getLuminosity())));

        const double earthMass = totalMass * SolarMassToEarthMass;
        radius = static_cast<float>(KothariRadius(totalMass, semimajorAxis, false, evaluationState.materialZone));
        escapeVelocity = static_cast<float>(EscapeVelocity(totalMass, radius));
        surfaceAcceleration = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

        minMolecularWeight = static_cast<float>(minimumMolecularWeight(star.getAge()));

        // If this is a failed gaseous planet (too low of a gas mass ratio, or too low of a gas retention), account for H2 and He loss.
        if ((gasMass / totalMass) > IcePlanetThreshold && totalMass > RockyTransition)
        {
#ifdef ALLOW_DEBUG_PRINTF
            if (generator.getVerbose())
            {
                printf(" ... Re-evaluating rocky planet as a gas dwarf.  dustMass %s, gasRatio = %.3lf\n",
                       (dustMass > CriticalLimit(semimajorAxis, eccentricity, star.getLuminosity())) ? "critical" : "sub-critical",
                       gasMass / totalMass);
            }
#endif

            const double h2Mass = gasMass * 0.85;
            const double h2Life = getGasLife(Weight_MolecularHydrogen);
            bool lostMass = false;

            if (h2Life < star.getAge())
            {
                const double h2Loss = ((1.0 - (1.0 / exp(star.getAge() / h2Life))) * h2Mass);

                gasMass -= h2Loss;
                totalMass -= h2Loss;
                assert(gasMass >= 0.0);
                lostMass = true;
            }

            const double heMass = (gasMass - h2Mass) * 0.999;
            const double heLife = getGasLife(Weight_Helium);
            if (heLife < star.getAge())
            {
                const double heLoss = ((1.0 - (1.0 / exp(star.getAge() / heLife))) * heMass);

                gasMass -= heLoss;
                totalMass -= heLoss;
                assert(gasMass >= 0.0);
                lostMass = true;
            }

            if (lostMass)
            {
                radius = static_cast<float>(KothariRadius(totalMass, semimajorAxis, false, evaluationState.materialZone));
                escapeVelocity = static_cast<float>(EscapeVelocity(totalMass, radius));
                surfaceAcceleration = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

                minMolecularWeight = static_cast<float>(minimumMolecularWeight(star.getAge()));
                orbitalDominance = OrbitalDominance(totalMass, semimajorAxis);
            }

            // Last bit of checking - this may be a gaseous planet after all
            const double initialGreenhouseTemp = EffectiveTemperature(evaluationState.ecosphereRatio, GreenhouseTriggerAlbedo);
            runawayGreenhouse = (initialGreenhouseTemp > FreezingPointWater);

            calculateSurfacePressure(generator, evaluationState);
            if (surfacePressure > 6000.0f && // Surface pressure is at least 6ATM (approximately).
                minMolecularWeight <= 2.0f) // Retains hydrogen
            {
#ifdef ALLOW_DEBUG_PRINTF
                if (generator.getVerbose())
                {
                    printf(" ... Re-re-evaluating planet as a gas dwarf.\n");
                }
#endif
                type = PlanetType::Gaseous;

                radius = static_cast<float>(KothariRadius(totalMass, semimajorAxis, true, evaluationState.materialZone));
                escapeVelocity = static_cast<float>(EscapeVelocity(totalMass, radius));
                surfaceAcceleration = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

                minMolecularWeight = static_cast<float>(minimumMolecularWeight(star.getAge()));

                // TODO: Clear out the rocky planet specific values
                runawayGreenhouse = false;
            }
        }
    }

    density = static_cast<float>(VolumeDensity(totalMass, radius));

    calculateDayLength(evaluationState);

    if (type == PlanetType::Gaseous)
    {
        // Categorize the type of gaseous planet.  Cutoffs from
        // Chen, et al. 2017:
        const double jovianMass = totalMass * SolarMassToJovianMass;
        const double earthMass = totalMass * SolarMassToEarthMass;
        if (jovianMass > BrownDwarfTransition)
        {
            type = PlanetType::BrownDwarf;
#ifdef ALLOW_DEBUG_PRINTF
            if (generator.getVerbose())
            {
                printf(" ... Planet is a brown dwarf (MJ = %.4lf, threshold is %.1lf).\n",
                       jovianMass, BrownDwarfTransition);
            }
#endif
        }
        else
        {
            // Neptunian/Jovian transition falls at M(Juptier) 0.414 (+0.057/-0.065).
            type = (jovianMass > IceGiantTransition) ? PlanetType::GasGiant : PlanetType::IceGiant;

#ifdef ALLOW_DEBUG_PRINTF
            if (generator.getVerbose())
            {
                printf(" ... Planet is a %s (MJ = %lf; ME = %lf).\n",
                       (jovianMass > IceGiantTransition) ? "Gas giant" : "Ice Giant",
                       jovianMass, earthMass);
            }
#endif
            // Rocky / Gaseous = 2.04 (+0.66/-0.59)x M(Earth)
            //if (totalMass < RockyTransition)
            //{
            //    sprintf_s(text, "! Ice Giant found with M(Earth) = %.2lf (floor should be 1.45 - 2.70)\n", totalMass * SolarMassToEarthMass);
            //    state->emitCallback(text);
            //}
        }

        albedo = generator.randomNear(Albedo_GasGiant, ThreeSigma_Albedo_GasGiant);

        // surface pressure
        // temperatures
        // hydrosphere / iceCoverage / cloudCoverage
    }
    else
    {
        assert(type == PlanetType::Rocky);

        // From the stargen documentation Burrows 2006 for greenhouse evaluation:
        // Old grnhouse:
        //	Note that if the orbital radius of the planet is greater than or equal
        //	to R_inner, 99% of it's volatiles are assumed to have been deposited in
        //	surface reservoirs (otherwise, it suffers from the greenhouse effect).
        //--------------------------------------------------------------------------
        //	if ((orb_radius < r_greenhouse) && (zone == 1))
        //
        //--------------------------------------------------------------------------
        //	The new definition is based on the inital surface temperature and what
        //	state water is in. If it's too hot, the water will never condense out
        //	of the atmosphere, rain down and form an ocean. The albedo used here
        //	was chosen so that the boundary is about the same as the old method	
        //	Neither zone, nor r_greenhouse are used in this version   JLB
        const double initialGreenhouseTemp = EffectiveTemperature(evaluationState.ecosphereRatio, GreenhouseTriggerAlbedo);
        runawayGreenhouse = (initialGreenhouseTemp > FreezingPointWater);

        calculateSurfacePressure(generator, evaluationState);

        // Iterate surface conditions until they converge (hopefully).
        // Sets multiple components of the atmosphere / surface / temperature.
        iterateSurfaceConditions(generator, evaluationState);

        // The original criteria were minTemp < boiling, maxTemp > freezing.
        // That can lead to planets with atmospheres that aren't evaluated.
        // But evaluation seems a bit wonky?
        // What about a first-stab ESI rating?  If it may be similar, let's
        // break out the gases.
        if (calculateEarthSimilarity() > 0.50f &&
            minTemperature <= boilingPoint)
        {
#ifdef ALLOW_DEBUG_PRINTF
            if (generator.getVerbose())
            {
                printf(" ... Evaluating atmosphere\n");
            }
#endif
            calculateGases(evaluationState);
        }

        // Select the type of planet
        if (surfacePressure < 1.0f)
        {
            // Effectively no atmosphere (sub-1mb) - is it small enough to be an asteroid belt?
            if (totalMass * SolarMassToEarthMass < AsteroidMassLimit)
            {
                type = PlanetType::AsteroidBelt;
            }
            else if (orbitalDominance <= 1.0f)
            {
                type = PlanetType::DwarfPlanet;
            }
            else
            {
                type = PlanetType::Rocky;
            }
#ifdef ALLOW_DEBUG_PRINTF
            if (generator.getVerbose())
            {
                const std::string& tStr = PlanetTypeString(type);
                printf(" ... Planet w/o atmo - classified as %s\n", tStr.c_str());
            }
#endif
        }
        else
        {
            earthSimilarityIndex = calculateEarthSimilarity();

            if (orbitalDominance <= 1.0f)
            {
#ifdef ALLOW_DEBUG_PRINTF
                if (generator.getVerbose())
                {
                    printf(" ... Dwarf planet - esi %.2f\n", earthSimilarityIndex);
                }
#endif
                type = PlanetType::DwarfPlanet;
            }
            else if (hydrosphere > 0.95f)
            {
#ifdef ALLOW_DEBUG_PRINTF
                if (generator.getVerbose())
                {
                    printf(" ... Ocean planet - esi %.2f\n", earthSimilarityIndex);
                }
#endif
                type = PlanetType::Ocean;
            }
            else if (iceCoverage > 0.95f || meanSurfaceTemperature < FreezingPointWater)
            {
#ifdef ALLOW_DEBUG_PRINTF
                if (generator.getVerbose())
                {
                    printf(" ... Ice planet - esi %.2f\n", earthSimilarityIndex);
                }
#endif
                type = PlanetType::IcePlanet;
            }
            else if (hydrosphere > 0.05f)
            {
#ifdef ALLOW_DEBUG_PRINTF
                if (generator.getVerbose())
                {
                    printf(" ... Terrestrial planet - esi %.2f\n", earthSimilarityIndex);
                }
#endif

                type = PlanetType::Terrestrial;

                // What criteria for Goldilocks?
                if (earthSimilarityIndex > 0.80f)
                {
                    // Do some additional workup to determine suitability for habitation.
                }
            }
#ifdef ALLOW_DEBUG_PRINTF
            else if (generator.getVerbose())
            {
                printf(" ... Rocky planet - esi %.2f\n", earthSimilarityIndex);
            }
#endif
        }
    }

    // Keeping this around for reference, although it will move over to Generator::generate()
#if 0
    if (!moon.empty())
    {
        assert(planet == nullptr);
        int moonOrdinal = 0;
        const double hillSphere = semiMajorAxis * KmPerAu * pow((totalMass / (3.0 * star->mass())), (1.0 / 3.0));

        // Evaluate the first moon's density, so we can generate a Bode's Law progression of orbital radii.
        const double moonRadius = KothariRadius(moon.front().mass(), semiMajorAxis, false, star);
        const float moonDensity = static_cast<float>(VolumeDensity(moon.front().mass(), moonRadius));
        const double rocheLimit = 2.44 * radius * pow(plDensity / moonDensity, 1.0 / 3.0);

        // The moon generation process in Burrows 2006 placed moons randomly between 1.5x the Roche limit
        // and 0.5x the Hill sphere.  I will use these as constraints for generating the Bode's Law progression.

        // Determine A: A number around 0.41 to 0.46
        const float A = state->randomUniform(0.41f, 0.46f);
        // Determine B: A number such that A * B = 3.0*rocheLimit
        const float B = static_cast<float>(rocheLimit * 3.0 / A);
        const float intervalStdDev = B * 0.1f;

        std::vector<double> radiusKm;
        radiusKm.emplace_back(A * state->randomNear(B, intervalStdDev));

        // Iterate towards the planet
        float f = -1.0f;
        double sma = A * powf(BodeProgression, f) * state->randomNear(B, intervalStdDev);
        while (sma >= rocheLimit)
        {
            radiusKm.emplace_back(sma);

            f -= 1.0f;
            sma = A * powf(BodeProgression, f) * state->randomNear(B, intervalStdDev);
        }

        // Iterate away from the planet
        f = 1.0f;
        sma = A * powf(BodeProgression, f) * state->randomNear(B, intervalStdDev);
        while (sma <= 0.5 * hillSphere)
        {
            radiusKm.emplace_back(sma);

            f += 1.0f;
            sma = A * powf(BodeProgression, f) * state->randomNear(B, intervalStdDev);
        }

        // Randomize the order of the orbits
        for (size_t i = 0; i < radiusKm.size(); ++i)
        {
            std::swap(radiusKm[i], radiusKm[state->randomInt<size_t>(1u, radiusKm.size() - 1u)]);
        }

        for (auto& m : moon)
        {
            // Make sure this moon will go into a valid orbit:
            const double moonRadius = KothariRadius(m.mass(), semiMajorAxis, false, star);
            const float moonDensity = static_cast<float>(VolumeDensity(m.mass(), moonRadius));
            const double rocheLimit = 2.44 * radius * pow(plDensity / moonDensity, 1.0 / 3.0);

            if (rocheLimit < radiusKm[moonOrdinal])
            {
                m.semiMajorAxis = radiusKm[moonOrdinal] * AuPerKm;
                m.eccentricity = state->randomEccentricity();
                m.evaluate(star, this, moonOrdinal + 1, config, state);
                ++moonOrdinal;
            }
            else
            {
                // A moon can't form here.  What do I do with it?
                sprintf_s(text, "! A moon tried to form within the Roche limit!\n");
                state->emitCallback(text);
                m.semiMajorAxis = 0.0;
                m.eccentricity = 0.0f;
            }
        }

        // Cull moons with 0 SMA
        std::vector<Planet> mp;
        for (auto& m : moon)
        {
            if (m.semiMajorAxis > 0.0)
            {
                mp.emplace_back(m);
            }
        }

        // This isn't the most efficient...
        std::sort(mp.begin(), mp.end(), [](const Planet& a, const Planet& b) { return a.sma() < b.sma(); });

        moon.clear();
        moon.assign(mp.begin(), mp.end());
    }
#endif

    evaluated = true;
}

//----------------------------------------------------------------------------
#if 0
void Planet::exchange(Planet& p)
{
    // Swap the existing planet with the new planet.  At this stage of evaluation, that entails swapping only a few values:
    std::swap(semimajorAxis, p.semimajorAxis);
    std::swap(eccentricity, p.eccentricity);
    std::swap(dustMass, p.dustMass);
    std::swap(gasMass, p.gasMass);
    std::swap(totalMass, p.totalMass);

    moon.insert(moon.begin(), p);
}
#endif

//----------------------------------------------------------------------------
const std::string& Planet::GasString(Gas gas)
{
    static const std::string gases[] =
    {
        "Hydrogen",
        "Helium",
        "Nitrogen",
        "Oxygen",
        "Neon",
        "Argon",
        "Krypton",
        "Xenon",
        "Ammonia",
        "Water Vapor",
        "Carbon Dioxide",
        "Ozone",
        "Methane"
    };

    return gases[uint32_t(gas)];
}

//----------------------------------------------------------------------------
double Planet::getGasLife(double molecularMass) const
{
    const double v = RMSVelocity(molecularMass, exosphereTemperature) * CmPerM;

    const double g = surfaceAcceleration * CmPerM;

    const double r = radius * CmPerKm;

    const double t = (pow(v, 3.0) / (2.0 * pow(g, 2.0) * r)) * exp((3.0 * g * r) / pow(v, 2.0));

    return t * YearsPerSecond;
}

//----------------------------------------------------------------------------
double Planet::getTotalMoonMass() const
{
    double ttlMass = 0.0;

#if 0
    for (const auto& m : moon)
    {
        ttlMass += m.getMass();
    }
#endif

    return ttlMass;
}

//----------------------------------------------------------------------------
float Planet::greenhouseRise(float effectiveTemperature) const
{
    const float opticalDepth = Opacity(minMolecularWeight, surfacePressure);
    const float convectionFactor = EarthConvectionFactor * powf(surfacePressure * static_cast<float>(AtmPerMb), 0.4f);

    return std::max(0.0f,
                    (powf(1.0f + 0.75f * opticalDepth, 0.25f) - 1.0f) * effectiveTemperature * convectionFactor);
}

//----------------------------------------------------------------------------
void Planet::initializeSurfaceConditions(const EvaluationState& evaluationState)
{
    albedo = Albedo_Earth;

    const float effectiveTemp = EffectiveTemperature(evaluationState.ecosphereRatio, albedo);
    const float greenhouseTemp = greenhouseRise(effectiveTemp);
    meanSurfaceTemperature = effectiveTemp + greenhouseTemp;

    setTemperatureRange();

    hydrosphere = cloudCoverage = iceCoverage = 0.0f;
}

//----------------------------------------------------------------------------
void Planet::iterateSurfaceConditions(Generator& generator, const EvaluationState& evaluationState)
{
    // Set initial conditions:
    initializeSurfaceConditions(evaluationState);
    updateSurfaceConditions(generator, evaluationState);

    bool converged = false;
    float previousTemperature;
    static constexpr int MaxConvergenceIterations = 25;
    for (int i = 0; i < MaxConvergenceIterations; ++i)
    {
        previousTemperature = meanSurfaceTemperature;
        updateSurfaceConditions(generator, evaluationState);

        // Do I want to use only absolute temperature?
        if (fabsf(previousTemperature - meanSurfaceTemperature) < 0.25f)
        {
            // Converged closely enough
            converged = true;
            break;
        }
    }

#ifdef ALLOW_DEBUG_PRINTF
    if (generator.getVerbose())
    {
        printf(" ... !!! Failed to converge planetary conditions in %d iterations; last delta was %f\n", MaxConvergenceIterations, fabsf(previousTemperature - meanSurfaceTemperature));
    }
#endif
}

//----------------------------------------------------------------------------
float Planet::minimumMolecularWeight(double stellarAge) const
{
    // We will search through various molecular masses to find the one that is
    // closest to the age of the planetary system.  We initialize the search to
    // the molecular limit of the planet, and check its gas life.

    double molecularMass = MolecularLimit(escapeVelocity, exosphereTemperature);
    double previousMass = molecularMass;

    double gasLife = getGasLife(molecularMass);

    if (gasLife > stellarAge)
    {
        // Gas retention is high for the starting mass.  Reduce the weight to find the right range.
        while (gasLife > stellarAge)
        {
            previousMass = molecularMass;
            molecularMass *= 0.5;

            gasLife = getGasLife(molecularMass);
        }
    }
    else
    {
        // Gas retention is low.  Increase the weight to find something that's retained.
        while (gasLife < stellarAge)
        {
            previousMass = molecularMass;
            molecularMass *= 2.0;

            gasLife = getGasLife(molecularMass);
        }
        // molecularMass is the lower of the two values in the binary search below.
        std::swap(previousMass, molecularMass);
    }

    // Binary search between the two end points
    while (previousMass - molecularMass > 0.1)
    {
        const double midMass = (previousMass + molecularMass) * 0.5;
        gasLife = getGasLife(molecularMass);

        if (gasLife < stellarAge)
        {
            molecularMass = midMass;
        }
        else
        {
            previousMass = midMass;
        }
    }

    return static_cast<float>(previousMass + molecularMass) * 0.5f;
}

//----------------------------------------------------------------------------
const std::string& Planet::PlanetTypeString(PlanetType type)
{
    static const std::string planetType[] =
    {
        "Unknown",
        "Rocky Planet",
        "Asteroid Belt",
        "Dwarf Planet",
        "Ice Planet",
        "Terrestrial Planet",
        "Ocean Planet",
        "Gaseous",
        "Ice Giant",
        "Gas Giant",
        "Brown Dwarf"
    };

    return planetType[uint32_t(type)];
}

//----------------------------------------------------------------------------
void Planet::setTemperatureRange()
{
    const float maxT = meanSurfaceTemperature + sqrtf(meanSurfaceTemperature) * 10.0f;
    const float minT = meanSurfaceTemperature / sqrtf(dayLength + float(HoursPerDay));

    const float pressmod = 1.0f / sqrtf(1.0f + 20.0f * surfacePressure * BarPerMillibar);
    const float ppmod = 1.0f / sqrtf(10.0f + 5.0f * surfacePressure * BarPerMillibar);
    const float tiltmod = fabsf(cosf(axialTilt) * powf(1.0f + eccentricity, 2.0f));
    const float daymod = 1.0f / (200.0f / dayLength + 1.0f);
    const float mh = powf(1.0f + daymod, pressmod);
    const float  ml = powf(1.0f - daymod, pressmod);

    const float hi = mh * meanSurfaceTemperature;
    const float lo = std::max(minT, ml * meanSurfaceTemperature);
    const float sh = hi + pow((100.0f + hi) * tiltmod, sqrt(ppmod));
    const float wl = std::max(0.0f, lo - powf((150.0f + lo) * tiltmod, sqrt(ppmod)));

    highTemperature = soft(hi, maxT, minT);
    lowTemperature = soft(lo, maxT, minT);
    maxTemperature = soft(sh, maxT, minT);
    minTemperature = soft(wl, maxT, minT);
}

//----------------------------------------------------------------------------
void Planet::updateSurfaceConditions(Generator& generator, const EvaluationState& evaluationState)
{
    if (runawayGreenhouse && maxTemperature < boilingPoint)
    {
        // Planet is too cool for runaway greenhouse, so re-evaluate some things
        runawayGreenhouse = false;

        calculateSurfacePressure(generator, evaluationState);
    }

    // Burke 1985 notes:
    // This function is Fogg's eq.22.	 Given the volatile gas inventory and
    // planetary radius of a planet (in Km), this function returns the
    // fraction of the planet covered with water.
    float newHydrosphere = std::min(1.0f,
                                    static_cast<float>((EarthHydrosphere * volatileGasInventory / 1000.0) * pow(EarthRadiusKm / radius, 2.0)));

    // Burke 1985 notes:
    // Given the surface temperature of a planet (in Kelvin), this function
    // returns the fraction of cloud cover available.  This is Fogg's eq.23.
    // See Hart in "Icarus" (vol 33, pp23 - 39, 1978) for an explanation.
    // This equation is Hart's eq.3.
    // I have modified it slightly using constants and relationships from
    // Glass's book "Introduction to Planetary Geology", p.46.
    // The 'CLOUD_COVERAGE_FACTOR' is the amount of surface area on Earth
    // covered by one Kg. of cloud.
    float newCloudCover;
    if (minMolecularWeight > Weight_WaterVapor)
    {
        newCloudCover = 0.0f;
    }
    else
    {
        // TODO: Look up the papers listed in Burke 1985 to see if I can reconcile this
        // equation.
        const double surfaceArea = 4.0 * PI * (radius * radius);

        // Mass of water, in grams
        const double hydroMass = newHydrosphere * surfaceArea * EarthWaterMassPerKm2;

        // Mass of water vapor - original variable is listed as kg, but I don't see why it is, since we divided
        // mass in grams by 100,000,000
        const double waterVapor = (0.00000001 * hydroMass) * exp(Q2_36 * (meanSurfaceTemperature - EarthAverageTemperature));

        newCloudCover = std::min(1.0f, static_cast<float>(CloudCoverageFactor * waterVapor / surfaceArea));
    }

    // Burke 1985 notes:
    // Given the surface temperature of a planet (in Kelvin), this function
    // returns the fraction of the planet's surface covered by ice.  This is
    // Fogg's eq.24.  See Hart[24] in Icarus vol.33, p.28 for an explanation.
    // I have changed a constant from 70 to 90 in order to bring it more in
    // line with the fraction of the Earth's surface covered with ice, which
    // is approximatly .016 (=1.6%).
    float newIceCover = std::min(1.5f * newHydrosphere, powf(((328.0f - meanSurfaceTemperature) / 90.0f), 5.0f));

    // Reconcile new results with state of the planet and apply sanity clamps
    newIceCover = std::max(0.0f, std::min(1.0f, newIceCover));

    if (newHydrosphere + newIceCover > 1.0f)
    {
        newHydrosphere = 1.0f - newIceCover;
    }

    if (runawayGreenhouse && surfacePressure > 0.0f)
    {
        cloudCoverage = 1.0f;
    }

    if (highTemperature >= boilingPoint
        // Not sure why the integer conversions here, other than to avoid floating point vagaries
        && !((int)dayLength == (int)(orbitalPeriod * HoursPerDay) || resonant))
    {
        // Boil-off
        hydrosphere = 0.0f;
        newHydrosphere = 0.0f;
        cloudCoverage = (minMolecularWeight > Weight_WaterVapor) ? 0.0f : 1.0f;
    }

    if (meanSurfaceTemperature < FreezingPointWater - 3.0)
    {
        // Frozen
        hydrosphere = 0.0f;
        newHydrosphere = 0.0f;
    }

    hydrosphere = (2.0f * hydrosphere + newHydrosphere) / 3.0f;
    cloudCoverage = (2.0f * cloudCoverage + newCloudCover) / 3.0f;
    iceCoverage = (2.0f * iceCoverage + newIceCover) / 3.0f;

    if (hydrosphere + iceCoverage > 1.0f)
    {
        hydrosphere = 1.0f - iceCoverage;
    }

    const float newAlbedo = calculateAlbedo(generator);

    albedo = (2.0f * albedo + newAlbedo) / 3.0f;

    // TODO: Consolidate into a single method?  effectiveTemperature is used only
    // in conjunction with greenhouseRise
    const float effectiveTemp = EffectiveTemperature(evaluationState.ecosphereRatio, albedo);
    const float greenhouseTemp = greenhouseRise(effectiveTemp);
    const float newSurfaceTemp = effectiveTemp + greenhouseTemp;

    meanSurfaceTemperature = (2.0f * meanSurfaceTemperature + newSurfaceTemp) / 3.0f;

    setTemperatureRange();
}

//----------------------------------------------------------------------------
double Planet::volatileInventory(Generator& generator, const EvaluationState& evaluationState) const
{
    const double velocityRatio = escapeVelocity / rmsVelocity;
    if (velocityRatio >= GasRetentionThreshold)
    {
        // Burrows 2006 adjusted Zone 1 from 100000 to 140000
        constexpr double Zone1Constant = 100000.0;
        // Burrows 2006 adjusted the divisor from 100 to 140.
        constexpr double StandardDivisor = 100.0;

        constexpr double proportionConstantByZone[] = { Zone1Constant, 75000.0, 250.0 };

        double proportionConstant;
        if (evaluationState.materialZone < 2.0f)
        {
            proportionConstant = Lerp(evaluationState.materialZone - 1.0, proportionConstantByZone[0], proportionConstantByZone[1]);
        }
        else
        {
            proportionConstant = Lerp(evaluationState.materialZone - 2.0, proportionConstantByZone[1], proportionConstantByZone[2]);
        }

        const double massInEarths = totalMass * SolarMassToEarthMass;
        const double center = proportionConstant * massInEarths / evaluationState.stellarMass;

        if (runawayGreenhouse || (gasMass / totalMass) > IcePlanetThreshold)
        {
            return generator.randomAbout(center, 0.2);
        }
        else
        {
            return generator.randomAbout(center / StandardDivisor, 0.2);
        }
    }
    else
    {
        return 0.0;
    }
}

}
}
