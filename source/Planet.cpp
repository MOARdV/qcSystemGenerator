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
#include <qcSysGen/Planet.h>

#include <qcSysGen/Config.h>
#include <qcSysGen/Equations.h>
#include <qcSysGen/GenerationState.h>
#include <qcSysGen/Star.h>

#include <assert.h>
#include <numeric>

namespace
{

struct ChemicalTable
{
    qcSystemGenerator::Gas chemical;
    float atomicWeight; //!< Atomic weight of the atom / molecule
    float meltingPoint; //!< Melting point, in Kelvin.
    float boilingPoint; //!< Boiling point, in Kelvin.
    float density; //!< Density, g/cc.

    float abundE;
    float abundS;
    float reactivity;
    float maxIPP; //!< Maximum inspired partial pressure
};

using qcSystemGenerator::MbPerMmhg;

/// @brief Table of atmospheric gases gleaned from Burrows 2006.  Max IPP values imported from Dole 1969.
static constexpr ChemicalTable gases[] =
{
        { qcSystemGenerator::Gas::Hydrogen, 1.0079f,  14.06f, 20.40f, 8.99e-05f, 0.00125893f, 27925.4f, 1.0f, 0.0f },
        { qcSystemGenerator::Gas::Helium, 4.0026f, 3.46f, 4.20f, 0.0001787f, 7.94328e-09f, 2722.7f, 0.0f, (61000.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Nitrogen, 14.0067f, 63.34f, 77.40f, 0.0012506f, 1.99526e-05f, 3.13329f, 0.0f, (2330.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Oxygen, 15.9994f, 54.80f, 90.20f, 0.001429f, 0.501187f, 23.8232f, 10.0f, (400.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Neon, 20.1700f, 24.53f, 27.10f, 0.0009f, 5.01187e-09f, 3.4435e-5f, 0.0f, (3900.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Argon, 39.9480f, 84.00f, 87.30f, 0.0017824f, 3.16228e-06f, 0.100925f, 0.0f, (1220.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Krypton, 83.8000f, 116.60f, 119.70f, 0.003708f, 1e-10f, 4.4978e-05f, 0.0f, (350.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Xenon, 131.3000f, 161.30f, 165.00f, 0.00588f, 3.16228e-11f, 4.69894e-06f, 0.0f, (160.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Ammonia, 17.0000f, 195.46f, 239.66f, 0.001f, 0.002f, 0.0001f, 1.0f, (100.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Water, 18.0000f, 273.16f, 373.16f, 1.000f, 0.03f, 0.001f, 0.0f, 0.0f },
        { qcSystemGenerator::Gas::CarbonDioxide, 44.0000f, 194.66f, 194.66f, 0.001f, 0.01f, 0.0005f, 0.0f, (7.0f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Ozone, 48.0000f, 80.16f, 161.16f, 0.001f, 0.001f, 0.000001f, 2.0f, (0.10f * MbPerMmhg) },
        { qcSystemGenerator::Gas::Methane, 16.0000f,  90.16f, 109.16f, 0.010f, 0.005f, 0.0001f, 1.0f, (50000.0f * MbPerMmhg) }
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
    const double surfacePressureBars = surfacePressureMb * qcSystemGenerator::BarPerMillibar;

    // TODO: Where do the magic numbers come from?
    return static_cast<float>(1.0 / ((log(surfacePressureBars) / -5050.5) + (1.0 / 373.0)));
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

    using qcSystemGenerator::EarthSurfacePressure;
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

namespace qcSystemGenerator
{

//----------------------------------------------------------------------------
Planet::Planet() :
    type(PlanetType::Unknown),
    planetNo(0),
    semiMajorAxis(0.0),
    solarSMA(0.0),
    eccentricity(0.0f),
    solarEccentricity(0.0f),
    earthSimilarityIndex(0.0f),
    totalMass(0.0),
    resonant(false),
    spinResonanceFactor(0.0f),
    orbitalPeriod(0.0),
    day(0.0f),
    dustMass(0.0),
    gasMass(0.0),
    radius(0.0f),
    axialTilt(0.0f),
    albedo(0.0f),
    minMolecularWeight(0.0f),
    volatileGasInventory(0.0),
    plDensity(0.0f),
    runawayGreenhouse(false),
    rmsVelocity(0.0f),
    exosphereTemp(0.0f),
    escapeVel(0.0f),
    surfaceAccel(0.0f),
    surfacePress(0.0f),
    boilingPoint(0.0f),
    meanSurfaceTemp(0.0f),
    highTemp(0.0f),
    lowTemp(0.0f),
    maxTemp(0.0f),
    minTemp(0.0f),
    hydrosphere(0.0f),
    cloudCoverage(0.0f),
    iceCoverage(0.0f),
    star(nullptr)
{

}

//----------------------------------------------------------------------------
Planet::Planet(double sma, float e, double dustMassComponent, double gasMassComponent) :
    type(PlanetType::Unknown),
    planetNo(0),
    semiMajorAxis(sma),
    solarSMA(sma),
    eccentricity(e),
    solarEccentricity(e),
    earthSimilarityIndex(0.0f),
    totalMass(dustMassComponent + gasMassComponent),
    resonant(false),
    spinResonanceFactor(0.0f),
    orbitalPeriod(0.0),
    day(0.0f),
    dustMass(dustMassComponent),
    gasMass(gasMassComponent),
    radius(0.0f),
    axialTilt(0.0f),
    albedo(0.0f),
    minMolecularWeight(0.0f),
    volatileGasInventory(0.0),
    plDensity(0.0f),
    runawayGreenhouse(false),
    rmsVelocity(0.0f),
    exosphereTemp(0.0f),
    escapeVel(0.0f),
    surfaceAccel(0.0f),
    surfacePress(0.0f),
    boilingPoint(0.0f),
    meanSurfaceTemp(0.0f),
    highTemp(0.0f),
    lowTemp(0.0f),
    maxTemp(0.0f),
    minTemp(0.0f),
    hydrosphere(0.0f),
    cloudCoverage(0.0f),
    iceCoverage(0.0f),
    star(nullptr)
{

}

//----------------------------------------------------------------------------
float Planet::calculateAlbedo(GenerationState* state) const
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

    const bool airless = (surfacePress == 0.0f);
    const float waterAlbedo = waterFraction * ((airless) ? 0.0f : state->randomNear(Albedo_Water, Albedo_Water * 0.2f));
    const float iceAlbedo = iceFraction * ((airless) ? state->randomNear(Albedo_IceAirless, Albedo_IceAirless * 0.4f) : state->randomNear(Albedo_Ice, Albedo_Ice * 0.1f));
    const float rockAlbedo = rockFraction * ((airless) ? state->randomNear(Albedo_RockAirless, Albedo_RockAirless * 0.3f) : state->randomNear(Albedo_Rock, Albedo_Rock * 0.1f));
    const float cloudAlbedo = cloudCoverage * ((airless) ? 0.0f : state->randomNear(Albedo_Cloud, Albedo_Cloud * 0.2f));

    return waterAlbedo + iceAlbedo + rockAlbedo + cloudAlbedo;
}

//----------------------------------------------------------------------------
void Planet::calculateDayLength()
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

    // May not need radiusCm, since it's divided by earth radius, which can be km as well.
    const double deltaAngularVelocity = ChangeInEarthAngularVelocity *
        (plDensity / EarthDensity) *
        (radius / (EarthRadiusKm)) *
        (EarthMassInGrams / massGr) *
        pow(star->mass(), 2.0) *
        (1.0 / pow(semiMajorAxis, 6.0));
    const double angularVelocity = baseAngularVelocity + deltaAngularVelocity * star->age();

    if (angularVelocity <= 0.0)
    {
        day = static_cast<float>(yearHours);
    }
    else
    {
        day = static_cast<float>(RadiansPerCircle / (SecondsPerHour * angularVelocity));
    }

    spinResonanceFactor = 0.0f;
    if (day >= yearHours)
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

        day = static_cast<float>(spinResonanceFactor * yearHours);
    }
}

//----------------------------------------------------------------------------
void Planet::calculateEarthSimilarity()
{
    if (isGaseous() || type == PlanetType::AsteroidBelt)
    {
        // Early out - too dissimilar to consider.
        earthSimilarityIndex = 0.0f;
        return;
    }

    // Criteria for earth similarity.  All weights from
    // https://phl.upr.edu/projects/earth-similarity-index-esi

    const float NumberOfWeights = (!atmosphere.empty()) ? 5.0f : 4.0f;

    static constexpr float RadiusWeight = 0.57f;
    const float radiusRating = powf(1.0f - static_cast<float>(fabs(radius - EarthRadiusKm) / (radius + EarthRadiusKm)), RadiusWeight / NumberOfWeights);

    static constexpr float DensityWeight = 1.07f;
    const float densityRating = powf(1.0f - static_cast<float>(fabs(plDensity - EarthDensity) / (plDensity + EarthDensity)), DensityWeight / NumberOfWeights);

    static constexpr float EvWeight = 0.70f;
    const float escapeVelocityRating = powf(1.0f - fabsf(escapeVel - EarthEscapeVelocity) / (escapeVel + EarthEscapeVelocity), EvWeight / NumberOfWeights);

    static constexpr float TemperatureWeight = 5.58f;
    const float surfaceTempRating = powf(1.0f - fabsf(meanSurfaceTemp - EarthAverageTemperature) / (meanSurfaceTemp + EarthAverageTemperature), TemperatureWeight / NumberOfWeights);

    float oxygenRating = 1.0f;
    if (!atmosphere.empty())
    {
        // TODO: Refine this if I can find guidance on how the exponents were derived.
        const auto o2 = std::find_if(atmosphere.begin(), atmosphere.end(), [](const AtmosphereComponent& c) { return c.gas == qcSystemGenerator::Gas::Oxygen; });
        const float ppo = surfacePress * ((o2 == atmosphere.end()) ? 0.0f : o2->fraction);
        // Partial pressure of oxygen.  Exponent is still work-in-progress.
        static constexpr float PpoWeight = 2.5f;
        oxygenRating = powf(1.0f - fabsf(ppo - EarthPartialPressureOxygen) / (ppo + EarthPartialPressureOxygen), PpoWeight / NumberOfWeights);
    }

    earthSimilarityIndex = radiusRating * densityRating * escapeVelocityRating * surfaceTempRating * oxygenRating;
}

//----------------------------------------------------------------------------
void Planet::calculateGases()
{
    if (surfacePress > 0.0f)
    {
        //    long double* amount = (long double*)calloc((max_gas + 1), sizeof(long double));
        //    long double	totamount = 0;
        //    long double pressure = planet->surf_pressure / MILLIBARS_PER_BAR;
        const float pressure = surfacePress * BarPerMillibar;
        //    int			n = 0;
        //    int			i;
        const float starAgeOver2B = static_cast<float>(star->age() / 2.0e9);

        float totalAmount = 0.0f;

        for (int i = 0; i < gasesCount; ++i)
        {
            //        long double yp = gases[i].boil /
            //            (373. * ((log((pressure)+0.001) / -5050.5) +
            //                (1.0 / 373.)));
            const float yp = gases[i].boilingPoint /
                (373.0f * ((logf(pressure + 0.001f) / -5050.5f) + (1.0f / 373.0f)));

            if ((yp >= 0.0f && yp < lowTemp)
                && (gases[i].atomicWeight >= minMolecularWeight))
            {
                const float vrms = static_cast<float>(RMSVelocity(gases[i].atomicWeight, exosphereTemp));
                const float pvrms = powf(1.0f / (1.0f + vrms / escapeVel), static_cast<float>(star->age() / 1.0e9));
                float abund = gases[i].abundS; /* gases[i].abunde */
                float react = 1.0f;
                float fract = 1.0f;
                float pres2 = 1.0f;

                // TODO: Store 1.0f / (1.0f + reactivity) in the structures, instead of computing it every run
    //            if (strcmp(gases[i].symbol, "Ar") == 0)
                if (gases[i].chemical == qcSystemGenerator::Gas::Argon)
                {
                    react = 0.15f * static_cast<float>(star->age() / 4.0e9);
                }
                //            else if (strcmp(gases[i].symbol, "He") == 0)
                else if (gases[i].chemical == qcSystemGenerator::Gas::Helium)
                {
                    abund *= 0.001f + static_cast<float>(gasMass / totalMass);
                    pres2 = (0.75f + pressure);
                    react = powf(1.0f / (1.0f + gases[i].reactivity), starAgeOver2B * pres2);
                }
                //            else if ((strcmp(gases[i].symbol, "O") == 0 ||
                //                strcmp(gases[i].symbol, "O2") == 0) &&
                //                sun->age > 2e9 &&
                //                planet->surf_temp > 270 && planet->surf_temp < 400)
                else if (gases[i].chemical == qcSystemGenerator::Gas::Oxygen &&
                         star->age() > 2.0e9 &&
                         // TODO: Where do these come from?
                         meanSurfaceTemp > 270.0f && meanSurfaceTemp < 400.0f)
                {
                    /*	pres2 = (0.65 + pressure/2);			Breathable - M: .55-1.4 	*/
                    pres2 = (0.89f + pressure / 4.0f); /*	Breathable - M: .6 -1.8 	*/
                    react = powf(1.0f / (1.0f + gases[i].reactivity), static_cast<float>(pow(starAgeOver2B, 0.25)) * pres2);
                }
                //            else if (strcmp(gases[i].symbol, "CO2") == 0 &&
                //                sun->age > 2e9 &&
                //                planet->surf_temp > 270 && planet->surf_temp < 400)
                else if (gases[i].chemical == qcSystemGenerator::Gas::CarbonDioxide &&
                         star->age() > 2.0e9 &&
                         meanSurfaceTemp > 270.0f && meanSurfaceTemp < 400.0f)
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

                AtmosphereComponent c = { gases[i].chemical, abund * pvrms * react * fract };
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
void Planet::calculateSurfaceConditions(bool initialize, GenerationState* state)
{
    if (initialize)
    {
        albedo = Albedo_Earth;

        const float effectiveTemp = effectiveTemperature(albedo);
        const float greenhouseTemp = greenhouseRise(effectiveTemp);
        meanSurfaceTemp = effectiveTemp + greenhouseTemp;

        setTemperatureRange();
    }

    if (runawayGreenhouse && maxTemp < boilingPoint)
    {
        // Planet is too cool for runaway greenhouse, so re-evaluate some things
        runawayGreenhouse = false;

        calculateSurfacePressure(state);
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
        const double waterVapor = (0.00000001 * hydroMass) * exp(Q2_36 * (meanSurfaceTemp - EarthAverageTemperature));

        newCloudCover = std::min(1.0f, static_cast<float>(CloudCoverageFactor * waterVapor / surfaceArea));
    }

    // Burke 1985 notes:
    // Given the surface temperature of a planet (in Kelvin), this function
    // returns the fraction of the planet's surface covered by ice.  This is
    // Fogg's eq.24.  See Hart[24] in Icarus vol.33, p.28 for an explanation.
    // I have changed a constant from 70 to 90 in order to bring it more in
    // line with the fraction of the Earth's surface covered with ice, which
    // is approximatly .016 (=1.6%).
    float newIceCover = std::min(1.5f * newHydrosphere, powf(((328.0f - meanSurfaceTemp) / 90.0f), 5.0f));

    // Reconcile new results with state of the planet and apply sanity clamps
    newIceCover = std::max(0.0f, std::min(1.0f, newIceCover));

    if (newHydrosphere + newIceCover > 1.0f)
    {
        newHydrosphere = 1.0f - newIceCover;
    }

    if (runawayGreenhouse && surfacePress > 0.0f)
    {
        cloudCoverage = 1.0f;
    }

    if (highTemp >= boilingPoint
        && !initialize
        // Not sure why the integer conversions here, other than to avoid floating point vagaries
        && !((int)day == (int)(orbitalPeriod * HoursPerDay) || resonant))
    {
        // Boil-off
        hydrosphere = 0.0f;
        newHydrosphere = 0.0f;
        cloudCoverage = (minMolecularWeight > Weight_WaterVapor) ? 0.0f : 1.0f;
    }

    if (meanSurfaceTemp < FreezingPointWater - 3.0)
    {
        // Frozen
        hydrosphere = 0.0f;
        newHydrosphere = 0.0f;
    }

    if (initialize)
    {
        hydrosphere = newHydrosphere;
        cloudCoverage = newCloudCover;
        iceCoverage = newIceCover;
    }
    else
    {
        hydrosphere = (2.0f * hydrosphere + newHydrosphere) / 3.0f;
        cloudCoverage = (2.0f * cloudCoverage + newCloudCover) / 3.0f;
        iceCoverage = (2.0f * iceCoverage + newIceCover) / 3.0f;

        if (hydrosphere + iceCoverage > 1.0f)
        {
            hydrosphere = 1.0f - iceCoverage;
        }
    }

    const float newAlbedo = calculateAlbedo(state);

    if (initialize)
    {
        albedo = newAlbedo;
    }
    else
    {
        albedo = (2.0f * albedo + newAlbedo) / 3.0f;
    }

    // TODO: Consolidate into a single method?  effectiveTemperature is used only
    // in conjunction with greenhouseRise
    const float effectiveTemp = effectiveTemperature(albedo);
    const float greenhouseTemp = greenhouseRise(effectiveTemp);
    const float newSurfaceTemp = effectiveTemp + greenhouseTemp;

    if (initialize)
    {
        meanSurfaceTemp = newSurfaceTemp;
    }
    else
    {
        meanSurfaceTemp = (2.0f * meanSurfaceTemp + newSurfaceTemp) / 3.0f;
    }

    setTemperatureRange();
}

//----------------------------------------------------------------------------
void Planet::calculateSurfacePressure(GenerationState* state)
{
    volatileGasInventory = volatileInventory(state);
    if (volatileGasInventory > 0.0)
    {
        const double radiusRatio = EarthRadiusKm / radius;

        surfacePress = static_cast<float>(
            volatileGasInventory * surfaceGravity() *
            EarthSurfacePressure * BarPerMillibar /
            (radiusRatio * radiusRatio)
            );

        boilingPoint = BoilingPoint(surfacePress);
    }
    else
    {
        surfacePress = boilingPoint = 0.0f;
    }
}


//----------------------------------------------------------------------------
float Planet::effectiveTemperature(float albedo) const
{
    return static_cast<float>(sqrt(star->ecosphere() / solarSMA)
                              * powf((1.0f - albedo) / (1.0f - Albedo_Earth), 0.25f)
                              * EarthEffectiveTemperature);
}

//----------------------------------------------------------------------------
void Planet::evaluate(const Star* centralStar, const Planet* planet, int planetNumber, const Config* config, GenerationState* state)
{
    star = centralStar;
    planetNo = planetNumber;
    //atmosphere = nullptr;

    // Distance of this planet or moon from the star.
    solarSMA = (planet != nullptr) ? planet->sma() : semiMajorAxis;
    solarEccentricity = (planet != nullptr) ? planet->e() : eccentricity;

    orbitalPeriod = Period(semiMajorAxis, totalMass, (planet) ? planet->mass() : star->mass());

    axialTilt = (config->randomAxialTilt) ? state->randomTilt(solarSMA) : 0.0f;

    // Planet's SMA scaled such that the star's ecosphere is 1.0
    const double ecosphereRelativeSMA = solarSMA / star->ecosphere();
    exosphereTemp = static_cast<float>(EarthExosphereTemperature / (ecosphereRelativeSMA * ecosphereRelativeSMA));

    rmsVelocity = static_cast<float>(RMSVelocity(Weight_MolecularNitrogen, exosphereTemp));

    // Do some preliminary computations on the planet.  These steps will be used to see if the results of
    // accretion were sufficient to create a gaseous planet.  If the mass is below the critical limit,
    // then we know it's a rocky world.  Likewise, if less than 5% of the mass of the world is in gas, we
    // treat it as a rocky world.
    // TODO: Do I need to bother checking CriticalLimit?  Gas isn't retained unless we've passed it.
    // Or do I bother with the 5% threshold, since I see planets getting swept up by the gas/total mass test in the
    // next if block.
    if (dustMass > CriticalLimit(solarSMA, solarEccentricity, star->luminosity()) && (gasMass / mass()) > GaseousPlanetThreshold)
    {
        // Assume it's a successful gas giant
        radius = static_cast<float>(KothariRadius(totalMass, solarSMA, true, star));
        escapeVel = static_cast<float>(EscapeVelocity(totalMass, radius));
        surfaceAccel = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

        minMolecularWeight = static_cast<float>(MinimumMolecularWeight(this, star));

        const bool sufficientMolecularRetention = (minMolecularWeight <= 4.0f);
        const bool sufficientOverallMass = (totalMass > RockyTransition);
        type = (sufficientMolecularRetention && sufficientOverallMass) ? PlanetType::Gaseous : PlanetType::Rocky;
        if (type == PlanetType::Rocky)
        {
            printf("Gaseous planet demoted to rocky: %s molecular retention and %s mass\n",
                   sufficientMolecularRetention ? "sufficient" : "inadequate",
                   sufficientOverallMass ? "sufficient" : "inadequate");
        }
    }
    else
    {
        type = PlanetType::Rocky;
    }

    if (type == PlanetType::Rocky)
    {
        assert(!(gasMass / mass() > GaseousPlanetThreshold) || (dustMass > CriticalLimit(solarSMA, solarEccentricity, star->luminosity())));

        //const double earthMass = mass() * SolarMassToEarthMass;
        radius = static_cast<float>(KothariRadius(totalMass, solarSMA, isGaseous(), star));
        escapeVel = static_cast<float>(EscapeVelocity(totalMass, radius));
        surfaceAccel = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

        minMolecularWeight = static_cast<float>(MinimumMolecularWeight(this, star));

        // If this is a failed gaseous planet (too low of a gas mass ratio, or too low of a gas retention), account for H2 and He loss.
        if ((gasMass / totalMass) > IcePlanetThreshold && totalMass > RockyTransition)
        {
            printf("Re-evaluating rocky planet -> gas dwarf.  dustMass %s, gasRatio = %.3lf\n",
                   (dustMass > CriticalLimit(solarSMA, solarEccentricity, star->luminosity())) ? "critical" : "sub-critical",
                   gasMass / totalMass);

            const double h2Mass = gasMass * 0.85;
            const double h2Life = GasLife(Weight_MolecularHydrogen, this);
            bool lostMass = false;

            if (h2Life < star->age())
            {
                const double h2Loss = ((1.0 - (1.0 / exp(star->age() / h2Life))) * h2Mass);

                gasMass -= h2Loss;
                totalMass -= h2Loss;
                assert(gasMass >= 0.0);
                lostMass = true;
            }

            const double heMass = (gasMass - h2Mass) * 0.999;
            const double heLife = GasLife(Weight_Helium, this);
            if (heLife < star->age())
            {
                const double heLoss = ((1.0 - (1.0 / exp(star->age() / heLife))) * heMass);

                gasMass -= heLoss;
                totalMass -= heLoss;
                assert(gasMass >= 0.0);
                lostMass = true;
            }

            if (lostMass)
            {
                radius = static_cast<float>(KothariRadius(totalMass, solarSMA, false, star));
                escapeVel = static_cast<float>(EscapeVelocity(totalMass, radius));
                surfaceAccel = static_cast<float>(GravityConstant * (totalMass * SolarMassInGrams) / pow(radius * CmPerKm, 2.0) * MPerCm);

                minMolecularWeight = static_cast<float>(MinimumMolecularWeight(this, star));
            }

            // Last bit of checking - this may be a gaseous planet after all
            const double initialGreenhouseTemp = effectiveTemperature(GreenhouseTriggerAlbedo);
            runawayGreenhouse = (initialGreenhouseTemp > FreezingPointWater);

            calculateSurfacePressure(state);
            if (surfacePress > 6000.0f && // Surface pressure is at least 6ATM (approximately).
                minMolecularWeight <= 2.0f) // Retains hydrogen
            {
                type = PlanetType::Gaseous;
                // TODO: Clear out the rocky planet specific values
                runawayGreenhouse = false;
            }
        }
    }

    plDensity = static_cast<float>(VolumeDensity(totalMass, radius));

    calculateDayLength();

    if (type == PlanetType::Gaseous)
    {
        // Categorize the type of gaseous planet.  Cutoffs from
        // Chen, et al. 2017:
        const double jovianMass = totalMass * SolarMassToJovianMass;
        if (jovianMass > BrownDwarfTransition)
        {
            type = PlanetType::BrownDwarf;
        }
        else
        {
            // Neptunian/Jovian transition falls at M(Juptier) 0.414 (+0.057/-0.065).
            type = (jovianMass > IceGiantTransition) ? PlanetType::GasGiant : PlanetType::IceGiant;

            // Rocky / Gaseous = 2.04 (+0.66/-0.59)x M(Earth)
            if (totalMass < RockyTransition)
            {
                printf("! Ice Giant found with M(Earth) = %.2lf (floor should be 1.45 - 2.70)\n", totalMass * SolarMassToEarthMass);
            }
        }

        albedo = state->randomNear(Albedo_GasGiant, ThreeSigma_Albedo_GasGiant);

        // surface pressure
        // temperatures
        // hydrosphere / iceCoverage / cloudCoverage
    }
    else
    {
        assert(type == PlanetType::Rocky);

        const double distanceFactor = sqrt(star->ecosphere() / solarSMA);

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
        const double initialGreenhouseTemp = effectiveTemperature(GreenhouseTriggerAlbedo);
        runawayGreenhouse = (initialGreenhouseTemp > FreezingPointWater);

        calculateSurfacePressure(state);

        // Iterate surface conditions until they converge (hopefully).
        // Sets multiple components of the atmosphere / surface / temperature.
        iterateSurfaceConditions(state);

        if (config->computeGases &&
            maxTemp >= FreezingPointWater &&
            minTemp <= boilingPoint)
        {
            calculateGases();
        }

        // Select the type of planet
        if (surfacePress < 1.0f)
        {
            // Effectively no atmosphere (sub-1mb) - is it small enough to be an asteroid belt?
            type = (totalMass * SolarMassToEarthMass < AsteroidMassLimit && planet == nullptr) ? PlanetType::AsteroidBelt : PlanetType::Rocky;
        }
        else
        {
            calculateEarthSimilarity();

            if (hydrosphere > 0.95f)
            {
                type = PlanetType::Ocean;
            }
            else if (iceCoverage > 0.95f || meanSurfaceTemp < FreezingPointWater)
            {
                type = PlanetType::IcePlanet;
            }
            else if (hydrosphere > 0.05f)
            {
                type = PlanetType::Terrestrial;

                // What criteria for Goldilocks?
                if (earthSimilarityIndex > 0.80f)
                {
                    // Do some additional workup to determine suitability for habitation.
                }
            }
        }
    }

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
                printf("! A moon tried to form within the Roche limit!\n");
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
}

//----------------------------------------------------------------------------
void Planet::exchange(Planet& p)
{
    // Swap the existing planet with the new planet.  At this stage of evaluation, that entails swapping only a few values:
    std::swap(semiMajorAxis, p.semiMajorAxis);
    std::swap(eccentricity, p.eccentricity);
    std::swap(dustMass, p.dustMass);
    std::swap(gasMass, p.gasMass);
    std::swap(totalMass, p.totalMass);

    moon.emplace_front(p);
}

//----------------------------------------------------------------------------
const std::string& Planet::GasString(Gas gas)
{
    static const std::string gases[] =
    {
        "Hydrogen",
        "Helium"
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
float Planet::greenhouseRise(float effectiveTemperature) const
{
    const float opticalDepth = Opacity(minMolecularWeight, surfacePress);
    const float convectionFactor = EarthConvectionFactor * powf(surfacePress * static_cast<float>(AtmPerMb), 0.4f);

    return std::max(0.0f,
                    (powf(1.0f + 0.75f * opticalDepth, 0.25f) - 1.0f) * effectiveTemperature * convectionFactor);
}

//----------------------------------------------------------------------------
void Planet::iterateSurfaceConditions(GenerationState* state)
{
    // Set initial conditions:
    calculateSurfaceConditions(true, state);

    bool converged = false;
    float previousTemperature;
    static constexpr int MaxConvergenceIterations = 25;
    for (int i = 0; i < MaxConvergenceIterations; ++i)
    {
        previousTemperature = meanSurfaceTemp;
        calculateSurfaceConditions(false, state);

        // Do I want to use only absolute temperature?
        if (fabsf(previousTemperature - meanSurfaceTemp) < 0.25f)
        {
            // Converged closely enough
            converged = true;
            break;
        }
    }
    if (!converged)
    {
        printf("!!! Failed to converge planetary conditions in %d iterations; last delta was %f\n", MaxConvergenceIterations, fabsf(previousTemperature - meanSurfaceTemp));
    }
}

//----------------------------------------------------------------------------
const std::string& Planet::PlanetTypeString(PlanetType type)
{
    static const std::string planetType[] =
    {
        "Unknown",
        "Rocky",
        "AsteroidBelt",
        "IcePlanet",
        "Terrestrial",
        "Ocean",
        "Gaseous",
        "IceGiant",
        "GasGiant",
        "BrownDwarf"
    };

    return planetType[uint32_t(type)];
}

//----------------------------------------------------------------------------
void Planet::setTemperatureRange()
{
    const float maxT = meanSurfaceTemp + sqrtf(meanSurfaceTemp) * 10.0f;
    const float minT = meanSurfaceTemp / sqrtf(day + float(HoursPerDay));

    const float pressmod = 1.0f / sqrtf(1.0f + 20.0f * surfacePress * BarPerMillibar);
    const float ppmod = 1.0f / sqrtf(10.0f + 5.0f * surfacePress * BarPerMillibar);
    const float tiltmod = fabsf(cosf(axialTilt * static_cast<float>(PI / 180.0)) * powf(1.0f + solarEccentricity, 2.0f));
    const float daymod = 1.0f / (200.0f / day + 1.0f);
    const float mh = powf(1.0f + daymod, pressmod);
    const float  ml = powf(1.0f - daymod, pressmod);

    const float hi = mh * meanSurfaceTemp;
    const float lo = std::max(minT, ml * meanSurfaceTemp);
    const float sh = hi + pow((100.0f + hi) * tiltmod, sqrt(ppmod));
    const float wl = std::max(0.0f, lo - powf((150.0f + lo) * tiltmod, sqrt(ppmod)));

    highTemp = soft(hi, maxT, minT);
    lowTemp = soft(lo, maxT, minT);
    maxTemp = soft(sh, maxT, minT);
    minTemp = soft(wl, maxT, minT);
}

//----------------------------------------------------------------------------
double Planet::totalMoonMass() const
{
    auto sumMass = [](double ttl, const Planet& m) { return ttl + m.totalMass; };

    double ttlMass = std::accumulate(moon.begin(), moon.end(), 0.0, sumMass);
    return ttlMass;
}

//----------------------------------------------------------------------------
double Planet::volatileInventory(GenerationState* state) const
{
    const double velocityRatio = escapeVel / rmsVelocity;
    if (velocityRatio >= GasRetentionThreshold)
    {
        // Burrows 2006 adjusted Zone 1 from 100000 to 140000
        constexpr double Zone1Constant = 100000.0;
        // Burrows 2006 adjusted the divisor from 100 to 140.
        constexpr double StandardDivisor = 100.0;

        constexpr double proportionConstantByZone[] = { Zone1Constant, 75000.0, 250.0 };
        const double zone = star->materialZone(solarSMA);

        double proportionConstant;
        if (zone < 2.0)
        {
            proportionConstant = Lerp(zone - 1.0, proportionConstantByZone[0], proportionConstantByZone[1]);
        }
        else
        {
            proportionConstant = Lerp(zone - 2.0, proportionConstantByZone[1], proportionConstantByZone[2]);
        }

        const double massInEarths = totalMass * SolarMassToEarthMass;
        const double center = proportionConstant * massInEarths / star->mass();

        if (runawayGreenhouse || (gasMass / totalMass) > IcePlanetThreshold)
        {
            return state->randomAbout(center, 0.2);
        }
        else
        {
            return state->randomAbout(center / StandardDivisor, 0.2);
        }
    }
    else
    {
        return 0.0;
    }
}

}
