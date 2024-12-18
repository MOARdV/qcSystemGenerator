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
#include <qcSysGen/Generator.h>

#include <qcSysGen/Star.h>
#include <qcSysGen/System.h>

#include <assert.h>

namespace
{

//----------------------------------------------------------------------------
// Apply the Blagg's formumation of the Bode Law using the specified parameters.
double BodeSequence(int n, double A, double B, float alpha, float beta)
{
    const float theta = alpha + n * beta;
    // f(theta) = 0.249 + 0.86 * ( cos(theta)/(3 - cos(2 * theta)) + 1/(6 - 4*cos(2 * (theta - pi/6))) )
    const double f = 0.249 + 0.86 * (cosf(theta) / (3.0 - cosf(2.0f * theta)) + 1.0 / (6.0 - 4.0 * cosf(theta - 0.52359877559829887307710723054658f)));

    return A * (B + f) * powf(1.7275f, float(n));
}

//----------------------------------------------------------------------------
// "reduced_mass" in the original accrete implementation.  Provides a number
// in the range [0, 1) based on the provided mass (in Solar masses).
inline double EffectLimitScalar(double mass)
{
    return pow(mass / (1.0 + mass), (1.0 / 4.0));
}

//----------------------------------------------------------------------------
// Returns the inner and outer effect limit for a given protoplanet.
std::pair<double, double> GetEffectLimits(double sma, double e, double mass)
{
    /// Mean eccentricity of the nebular dust in Dole 1969.
    /// The original value was 0.25, but it looks like the various versions afterwards used 0.2 instead.
    static constexpr double CloudEccentricity = 0.2;

    const double scalar = EffectLimitScalar(mass);

    return std::make_pair(sma * (1.0 - e) * (1.0 - scalar) / (1.0 + CloudEccentricity), sma * (1.0 + e) * (1.0 + scalar) / (1.0 - CloudEccentricity));
}

}

namespace qc
{

namespace SystemGenerator
{

//----------------------------------------------------------------------------
void Generator::accreteDust(Protoplanet& protoplanet)
{
    protoplanet.criticalMass = CriticalLimit(protoplanet.sma, protoplanet.eccentricity, stellarLuminosity);

#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(__FUNCTION__"(): sma @ %.3lf\n", protoplanet.sma);
    }
#endif

    // Amount of dust and gas collected (added to the protoplanet afterwards)
    double addedDustMass, addedGasMass;
    // Total mass collected (addedDustMass + addedGasMass)
    double addedMass = 0.0;
    
    double oldMass;

    // Accumulate dust.  When we exit this loop, addedMass is the total mass collected by the
    // protoplanet, and addedDustMass and addedGasMass are the additional dust and gas masses.
    do
    {
        const std::pair<double, double> effectLimits = GetEffectLimits(protoplanet.sma, protoplanet.eccentricity, protoplanet.mass + addedMass);
        protoplanet.r_inner = effectLimits.first;
        protoplanet.r_outer = effectLimits.second;

        oldMass = addedMass;
        addedMass = collectDust(protoplanet.mass + addedMass, addedDustMass, addedGasMass, protoplanet, availableDust.begin());

        // Keep trying to collect dust until we're not adding much per iteration.
    } while (addedMass > 0.0 && (addedMass - oldMass) >= 0.0001 * oldMass);
    
    // If any mass wass was added, add it to the protoplanet and update the dust lanes.
    if (addedMass > 0.0)
    {
        protoplanet.mass += addedMass;
        protoplanet.dustMass += addedDustMass;
        protoplanet.gasMass += addedGasMass;
        
        const std::pair<double, double> effectLimits = GetEffectLimits(protoplanet.sma, protoplanet.eccentricity, protoplanet.mass);
        protoplanet.r_inner = effectLimits.first;
        protoplanet.r_outer = effectLimits.second;

        updateDustLanes(protoplanet);
    }

    // If the protoplanet is heavier than the initial seed mass, let's try to turn it into a planet.
    if (protoplanet.mass > config.protoplanetSeedMass)
    {
#ifdef ALLOW_DEBUG_PRINTF
        if (config.verboseLogging && !availableDust.empty())
        {
            printf("Updated dust bands:\n");
            for (const auto& d : availableDust)
            {
                if (d.dustPresent || d.gasPresent)
                {
                    const char* whatsLeft = "cleared"; // Shouldn't be seen - empty bands aren't in the dust list.
                    if (d.dustPresent)
                    {
                        if (d.gasPresent)
                        {
                            whatsLeft = "dust and gas";
                        }
                        else
                        {
                            whatsLeft = "dust";
                        }
                    }
                    else if (d.gasPresent)
                    {
                        whatsLeft = "gas";
                    }

                    printf("%7.3lfAU - %7.3lfAU: %s\n",
                           d.innerEdge, d.outerEdge,
                           whatsLeft);
                }
            }
        }
#endif

        ++protoPlanetCount;
        coalescePlanetisimals(protoplanet);
    }
#ifdef ALLOW_DEBUG_PRINTF
    else if (config.verboseLogging)
    {
        printf(" ... No dust collected.  Discarding\n");
    }
#endif
}

//----------------------------------------------------------------------------
bool Generator::accreteDust2(Protoplanet& protoplanet)
{
    protoplanet.criticalMass = CriticalLimit(protoplanet.sma, protoplanet.eccentricity, stellarLuminosity);

#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(__FUNCTION__"(): sma @ %.3lf\n", protoplanet.sma);
    }
#endif

    const std::pair<double, double> effectLimits = GetEffectLimits(protoplanet.sma, protoplanet.eccentricity, protoplanet.mass);
    protoplanet.r_inner = effectLimits.first;
    protoplanet.r_outer = effectLimits.second;

    // Amount of dust and gas collected
    double addedDustMass, addedGasMass;
    const double addedMass = collectDust(protoplanet.mass, addedDustMass, addedGasMass, protoplanet, availableDust.begin());

    // If any mass wass was added, add it to the protoplanet and update the dust lanes.
    if (addedMass > 0.0)
    {
        protoplanet.mass += addedMass;
        protoplanet.dustMass += addedDustMass;
        protoplanet.gasMass += addedGasMass;


        const std::pair<double, double> effectLimits = GetEffectLimits(protoplanet.sma, protoplanet.eccentricity, protoplanet.mass);
        protoplanet.r_inner = effectLimits.first;
        protoplanet.r_outer = effectLimits.second;

        updateDustLanes(protoplanet);

        return true;
    }
    else
    {
        // This protoplanet is no longer collecting dust, so we can skip it in future iterations.
        protoplanet.active = false;

        return false;
    }
}

//----------------------------------------------------------------------------
void Generator::coalescePlanetisimals(const Protoplanet& protoplanet)
{
#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(__FUNCTION__"():\n");
    }
#endif

    PlanetList::iterator planet = planetList.begin();
    while (planet != planetList.end())
    {
        // Difference in semi-major axis
        const double diff = planet->getSemimajorAxis() - protoplanet.sma;

        const double ppEffectLimitScalar = EffectLimitScalar(protoplanet.mass);
        const double planetEffectLimitScalar = EffectLimitScalar(planet->getMass());

        double dist1, dist2;
        // Planet is more distant than protoplanet
        if (diff > 0.0)
        {
            dist1 = (protoplanet.sma * (1.0 + protoplanet.eccentricity) * (1.0 + ppEffectLimitScalar)) - protoplanet.sma;
            // x aphelion
            dist2 = planet->getSemimajorAxis()
                - (planet->getSemimajorAxis() * (1.0 - planet->getEccentricity()) * (1.0 - planetEffectLimitScalar));
        }
        else
        {
            dist1 = protoplanet.sma - (protoplanet.sma * (1.0 - protoplanet.eccentricity) * (1.0 - ppEffectLimitScalar));
            // x perihelion
            dist2 = (planet->getSemimajorAxis() * (1.0 + planet->getEccentricity()) * (1.0 + planetEffectLimitScalar))
                - planet->getSemimajorAxis();
        }

        if (((fabs(diff) <= fabs(dist1)) || (fabs(diff) <= fabs(dist2))))
        {
            // TODO: Double-check these equations
            const double newSMA = (planet->getMass() + protoplanet.mass) /
                ((planet->getMass() / planet->getSemimajorAxis()) + (protoplanet.mass / protoplanet.sma));

            double e2 = planet->getMass() * sqrt(planet->getSemimajorAxis()) * sqrt(1.0 - pow(planet->getEccentricity(), 2.0));
            e2 += protoplanet.mass * sqrt(protoplanet.sma) * sqrt(sqrt(1.0 - pow(protoplanet.eccentricity, 2.0)));
            e2 /= (planet->getMass() + protoplanet.mass) * sqrt(newSMA);
            e2 = std::max(0.0, 1.0 - pow(e2, 2.0));

            const float newE = static_cast<float>(sqrt(e2));

            if (config.generateMoonsOnCollision)
            {
#if 0
                if (protoplanet.mass < protoplanet.criticalMass)
                {
                    const double existingMoonMass = planet->getTotalMoonMass();

                    if ((protoplanet.mass * SolarMassToEarthMass) < 2.5
                        && (protoplanet.mass * SolarMassToEarthMass) > .0001
                        && existingMoonMass < (planet->getMass() * .05)
                        )
                    {
                        Planet moon(protoplanet.sma, protoplanet.eccentricity, protoplanet.dustMass, protoplanet.gasMass);

                        if (protoplanet.mass > planet->getMass())
                        {
#ifdef ALLOW_DEBUG_PRINTF
                            if (config.verboseLogging)
                            {
                                printf("... Protoplanet collision.  New planet @ %.3lfAU has captured existing planet @ %.3lfAU as a moon.\n",
                                       protoplanet.sma, planet->getSemimajorAxis());
                            }
#endif

                            // Swap the parameters for planet and this moon.  Add the old planet / new moon
                            // to the new planet's menagerie.
                            planet->exchange(moon);
                        }
                        else
                        {
#ifdef ALLOW_DEBUG_PRINTF
                            if (config.verboseLogging)
                            {
                                printf("... Protoplanet collision.  New planet @ %.3lfAU has become a moon of existing planet @ %.3lfAU.\n",
                                       protoplanet.sma, planet->getSemimajorAxis());
                            }
#endif
                            planet->addMoon(moon);
                        }

                        // Early return - the planet captured the protoplanet or vice-versa
                        return;
                    }
                }
#endif
            }

            // Protoplanet collision
            Protoplanet newProtoplanet;
            newProtoplanet.sma = newSMA;
            newProtoplanet.eccentricity = newE;
            newProtoplanet.mass = planet->getMass() + protoplanet.mass;
            newProtoplanet.dustMass = planet->getDustMassComponent() + protoplanet.dustMass;
            newProtoplanet.gasMass = planet->getGasMassComponent() + protoplanet.gasMass;

#ifdef ALLOW_DEBUG_PRINTF
            if (config.verboseLogging)
            {
                printf("... Protoplanet collision.  New planet @ %.3lfAU has merged with existing planet @ %.3lfAU.  Rechecking dust accretion.\n",
                       protoplanet.sma, planet->getSemimajorAxis());
            }
#endif

            // Remove planet from the list - accreteDust() will replace it with the merged protoplanet.
            planetList.remove_if([planet](Planet& p) { return (p.getSemimajorAxis() == planet->getSemimajorAxis()); });

            // Sweep the dustbands with the protoplanet that represents the merged mass.
            accreteDust(newProtoplanet);

            // Return early - finished, and we added the merged planetisimal in accreteDust
            return;
        }

        ++planet;
    }

    // The protoplanet didn't interact with anything already in the planetary system, so we need to add it as a new
    // body.
    Planet newPlanet(protoplanet.sma, static_cast<float>(protoplanet.eccentricity), protoplanet.dustMass, protoplanet.gasMass);
#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(" ... Adding new planet.\n");
    }
#endif
    if (planetList.empty() || planetList.front().getSemimajorAxis() > newPlanet.getSemimajorAxis())
    {
        planetList.emplace_front(newPlanet);
    }
    else
    {
        auto currentPlanet = planetList.begin();
        auto nextPlanet = currentPlanet;
        ++nextPlanet;

        while (currentPlanet != planetList.end())
        {
            if (nextPlanet == planetList.end())
            {
                planetList.emplace_after(currentPlanet, newPlanet);
                break;
            }
            else if (currentPlanet->getSemimajorAxis() < newPlanet.getSemimajorAxis() && nextPlanet->getSemimajorAxis() >= newPlanet.getSemimajorAxis())
            {
                planetList.emplace_after(currentPlanet, newPlanet);
                break;
            }
            ++currentPlanet;
            ++nextPlanet;
        }
    }
}

//----------------------------------------------------------------------------
double Generator::collectDust(double lastMass, double& additionalDustMass, double& additionalGasMass, const Protoplanet& protoplanet, Generator::AvailableDust::iterator dustband)
{
    additionalDustMass = 0.0;
    additionalGasMass = 0.0;

    if (dustband == availableDust.end())
    {
        // Break the recursion - we're at the end of the list.
        return 0.0;
    }

    if ((dustband->outerEdge <= protoplanet.r_inner) || (dustband->innerEdge >= protoplanet.r_outer))
    {
        // Skip this dustband - this dustband is outside the range of the effect radius.
        return collectDust(lastMass, additionalDustMass, additionalGasMass, protoplanet, ++dustband);
    }

    // Where do these values come from?
    // Per acrete.cc - "See Sagan's article for insight into changing them."
    // Per Dole 1969, they were picked because the tended to generate planetary systems similar to our Solar System.

    // A is the density of the dust cloud in solar masses per cubic AU.
    //
    // Dole 1969 used 0.0015, most of the later implementations use 0.002.
    static constexpr double A = 0.0020;

    // Alpha moves where the highest density of the dust tends to be.  Per Issacman 1977,
    // 5.0 is roughly 5.8AU (Jupiter) for a G2V.  Larger values will tend to make outer
    // planets smaller.  The equation is extremely sensitive to these values,
    // and degenerate conditions are common if they change too much.
    static constexpr double Alpha = 5.0;

    // N is the denominator of the exponent in Dole 1969's dust density equation.  Issacman 1977
    // varied it to see how it affected the outcome.
    static constexpr double N = 3.0;

    const double dustDensity = A * sqrt(stellarMass) * exp(-Alpha * pow(protoplanet.sma, 1.0 / N));
    const double tempDensity = (dustband->dustPresent) ? dustDensity : 0.0;

    double massDensity;
    double gasDensity;
    if ((lastMass < protoplanet.criticalMass) || (!dustband->gasPresent))
    {
        massDensity = tempDensity;
        gasDensity = 0.0;
    }
    else
    {
        // Gas-to-dust ratio
        static constexpr double K = 50.0;

        massDensity = K * tempDensity / (1.0 + sqrt(protoplanet.criticalMass / lastMass) * (K - 1.0));
        gasDensity = massDensity - tempDensity;
        assert(gasDensity >= 0.0);
    }

    const double bandWidth = protoplanet.r_outer - protoplanet.r_inner;

    const double outerTemp = std::max(0.0, protoplanet.r_outer - dustband->outerEdge);

    double width = bandWidth - outerTemp;

    const double innerTemp = std::max(0.0, dustband->innerEdge - protoplanet.r_inner);

    width = width - innerTemp;

    const double effectLimitScalar = EffectLimitScalar(lastMass);

    const double area = 4.0 * PI * pow(protoplanet.sma, 2.0) * effectLimitScalar * (1.0 - protoplanet.eccentricity * (outerTemp - innerTemp) / bandWidth);

    const double volume = area * width;

    const double newMass = volume * massDensity;

    additionalGasMass = volume * gasDensity;

    additionalDustMass = newMass - additionalGasMass;
    assert(additionalDustMass >= 0.0);

    // Recurse to the next dust band.
    double nextDustMass, nextGasMass;
    const double nextMass = collectDust(lastMass, nextDustMass, nextGasMass, protoplanet, ++dustband);

    additionalDustMass += nextDustMass;
    additionalGasMass += nextGasMass;

    return newMass + nextMass;
}

//----------------------------------------------------------------------------
void Generator::generate(SolarSystem& system, const Config& config_)
{
    system.planet.clear();
    availableDust.clear();
    protoPlanetCount = 0;

    config = config_;

    // Sanity clamps:
    config.inclinationMean = fabsf(config.inclinationMean);
    while (config.inclinationMean >= 180.0f)
    {
        config.inclinationMean -= 180.0f;
    }
    config.inclinationStdDev = fabsf(config.inclinationStdDev);

#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(__FUNCTION__"():\n");
    }
#endif
    if (config.generateStar)
    {
        generateStar(system);
    }
    else
    {
        // Make sure the star's evaluataed before we start using it.
        system.star.evaluate(this);
        if (config.verboseLogging)
        {
            char st[6];
            system.star.getStellarClass(st, sizeof(st));
            printf("using supplied star %s\n", st);
        }
    }

    const Star& star = system.star;

    // Store shadow values
    protoplanetZone = star.getProtoplanetZone();
    stellarLuminosity = star.getLuminosity();
    stellarMass = star.getMass();

    std::vector<ProtoplanetSeed> protoplanetSeeds;
    if (!config.protoplanetSeeds.empty())
    {
#ifdef ALLOW_DEBUG_PRINTF
        if (config.verboseLogging)
        {
            printf("%Iu protoplanet seeds provided in Config\n", config.protoplanetSeeds.size());
        }
#endif
        protoplanetSeeds.assign(config.protoplanetSeeds.begin(), config.protoplanetSeeds.end());

        // Assign random eccentricity where needed:
        for (auto& s : protoplanetSeeds)
        {
            if (s.eccentricity < 0.0f || s.eccentricity > 0.9f)
            {
                s.eccentricity = randomEccentricity();
            }
        }
    }
    else if (config.generateBodeSeeds)
    {
        generateBodeSeeds(protoplanetSeeds, star);
    }

    // Initialize dust bands
    const BandLimit_t& dustZone = star.getDustZone();
    availableDust.emplace_front(Dust(dustZone.first, dustZone.second, true, true));
    dustRemains = true;

    // Apply seeds
#ifdef ALLOW_DEBUG_PRINTF
    if (!protoplanetSeeds.empty() && config.verboseLogging)
    {
        printf("Applying protoplanet seeds:\n");
    }
#endif

    for (const auto& s : protoplanetSeeds)
    {
        if (s.semiMajorAxis >= protoplanetZone.first && s.semiMajorAxis <= protoplanetZone.second && dustRemains)
        {
            Protoplanet protoplanet;
            protoplanet.sma = s.semiMajorAxis;
            protoplanet.eccentricity = s.eccentricity;
            protoplanet.mass = protoplanet.dustMass = config.protoplanetSeedMass;

            accreteDust(protoplanet);
        }
#ifdef ALLOW_DEBUG_PRINTF
        else
        {
            if (config.verboseLogging)
            {
                printf("Discarded protoplanet at SMA %.3lf: outside of protoplanet zone\n", s.semiMajorAxis);
            }
        }
#endif
    }

#ifdef ALLOW_DEBUG_PRINTF
    if (dustRemains && config.verboseLogging)
    {
        printf("Consuming remaining dust:\n");
    }
#endif
    while (dustRemains)
    {
        Protoplanet protoplanet;
        protoplanet.sma = randomUniform(protoplanetZone.first, protoplanetZone.second);
        protoplanet.eccentricity = randomEccentricity();
        protoplanet.mass = protoplanet.dustMass = config.protoplanetSeedMass;

        accreteDust(protoplanet);
    }

    // Generate moons
    if (config.generateMoons)
    {
        // TODO: Generate moons
    }


    // Copy the planet list across to the solar system.
    system.planet.reserve(std::distance(planetList.begin(), planetList.end()));
    for (auto& p : planetList)
    {
        // Finalize values for the planet:
        p.inclination = randomNear(config.inclinationMean, 3.0f * config.inclinationStdDev);
        p.inclination = fabsf(p.inclination);
        while (p.inclination >= 180.0f)
        {
            p.inclination -= 180.0f;
        }
        p.longitudeAscendingNode = randomTwoPi();
        p.argumentOfPeriapsis = randomTwoPi();
        p.meanAnomalyAtEpoch = randomTwoPi(); // mean anomaly is a double, but I think promoting a float is good enough for initializing it here.

        system.planet.emplace_back(p);
    }

    system.evaluate(*this);
}

//----------------------------------------------------------------------------
void Generator::generate2(SolarSystem& system, const Config& config_)
{
    system.planet.clear();
    availableDust.clear();
    protoPlanetCount = 0;

    config = config_;

    // Sanity clamps:
    config.inclinationMean = fabsf(config.inclinationMean);
    while (config.inclinationMean >= 180.0f)
    {
        config.inclinationMean -= 180.0f;
    }
    config.inclinationStdDev = fabsf(config.inclinationStdDev);

#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(__FUNCTION__"():\n");
    }
#endif
    if (config.generateStar)
    {
        generateStar(system);
    }
    else
    {
        // Make sure the star's evaluataed before we start using it.
        system.star.evaluate(this);
        if (config.verboseLogging)
        {
            char st[6];
            system.star.getStellarClass(st, sizeof(st));
            printf("using supplied star %s\n", st);
        }
    }

    const Star& star = system.star;

    // Store shadow values
    protoplanetZone = star.getProtoplanetZone();
    stellarLuminosity = star.getLuminosity();
    stellarMass = star.getMass();

    std::vector<ProtoplanetSeed> protoplanetSeeds;
    std::vector<Protoplanet> protoplanets;
    if (!config.protoplanetSeeds.empty())
    {
#ifdef ALLOW_DEBUG_PRINTF
        if (config.verboseLogging)
        {
            printf("%Iu protoplanet seeds provided in Config\n", config.protoplanetSeeds.size());
        }
#endif
        protoplanetSeeds.assign(config.protoplanetSeeds.begin(), config.protoplanetSeeds.end());

        // Assign random eccentricity where needed:
        for (auto& s : protoplanetSeeds)
        {
            if (s.eccentricity < 0.0f || s.eccentricity > 0.9f)
            {
                s.eccentricity = randomEccentricity();
            }
        }
    }
    else if (config.generateBodeSeeds)
    {
        generateBodeSeeds(protoplanetSeeds, star);
    }

    for (const auto& s : protoplanetSeeds)
    {
        if (s.semiMajorAxis >= protoplanetZone.first && s.semiMajorAxis <= protoplanetZone.second)
        {
            Protoplanet protoplanet;
            protoplanet.sma = s.semiMajorAxis;
            if (s.eccentricity < 0.0f || s.eccentricity > 0.9f)
            {
                protoplanet.eccentricity = randomEccentricity();
            }
            else
            {
                protoplanet.eccentricity = s.eccentricity;
            }
            protoplanet.mass = protoplanet.dustMass = config.protoplanetSeedMass;
            protoplanets.emplace_back(protoplanet);
        }
#ifdef ALLOW_DEBUG_PRINTF
        else if (config.verboseLogging)
        {
            printf("Discarded protoplanet at SMA %.3lf: outside of protoplanet zone\n", s.semiMajorAxis);
        }
#endif
    }

    for (uint32_t i = 0; i < config.protoplanetCount; ++i)
    {
        Protoplanet protoplanet;
        protoplanet.sma = randomUniform(protoplanetZone.first, protoplanetZone.second);
        protoplanet.eccentricity = randomEccentricity();
        protoplanet.mass = protoplanet.dustMass = config.protoplanetSeedMass;
        protoplanets.emplace_back(protoplanet);
    }

    // Initialize dust bands
    const BandLimit_t& dustZone = star.getDustZone();
    availableDust.emplace_front(Dust(dustZone.first, dustZone.second, true, true));
    dustRemains = true;

    // Apply seeds
#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf("Applying %Iu protoplanet seeds:\n", protoplanets.size());
    }
#endif

    bool anyAccrued;
    uint32_t iteratorCount = 0;
    do
    {
#ifdef ALLOW_DEBUG_PRINTF
        if (config.verboseLogging)
        {
            printf("Accretion iteration %u:\n", iteratorCount);
        }
#endif
        anyAccrued = false;
        uint32_t idx = 0;
        for (auto& protoplanet : protoplanets)
        {
            if (protoplanet.active)
            {
                if (accreteDust2(protoplanet))
                {
                    anyAccrued = true;
                }
                else
                {
                    protoplanet.active = false;
#ifdef ALLOW_DEBUG_PRINTF
                    if (config.verboseLogging)
                    {
                        printf(" ... protoplanet %3u has stopped accreting.\n", idx);
                    }
#endif
                }
            }

            ++idx;
        }
        ++iteratorCount;
    } while (anyAccrued);

#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf("%u accrual cycles before all protoplanets stopped accreting.\n", iteratorCount);
    }
#endif
    
    for (auto& protoplanet : protoplanets)
    {
        if (protoplanet.mass > config.protoplanetSeedMass)
        {
            coalescePlanetisimals(protoplanet);
        }
    }

#ifdef ALLOW_DEBUG_PRINTF
    if (dustRemains && config.verboseLogging)
    {
        printf("Consuming remaining dust:\n");
    }
#endif
    
    while (dustRemains)
    {
        // Fall back to serial accretion to sweep up remnants.
        Protoplanet protoplanet;
        protoplanet.sma = randomUniform(protoplanetZone.first, protoplanetZone.second);
        protoplanet.eccentricity = randomEccentricity();
        protoplanet.mass = protoplanet.dustMass = config.protoplanetSeedMass;

        accreteDust(protoplanet);
    }
    
    // Generate moons
    if (config.generateMoons)
    {
        // TODO: Generate moons
    }


    // Copy the planet list across to the solar system.
    system.planet.reserve(std::distance(planetList.begin(), planetList.end()));
    for (auto& p : planetList)
    {
        // Finalize values for the planet:
        p.inclination = randomNear(config.inclinationMean, 3.0f * config.inclinationStdDev);
        p.inclination = fabsf(p.inclination);
        while (p.inclination >= 180.0f)
        {
            p.inclination -= 180.0f;
        }
        p.longitudeAscendingNode = randomTwoPi();
        p.argumentOfPeriapsis = randomTwoPi();
        p.meanAnomalyAtEpoch = randomTwoPi(); // mean anomaly is a double, but I think promoting a float is good enough for initializing it here.

        system.planet.emplace_back(p);
    }

    system.evaluate(*this);
}

//----------------------------------------------------------------------------
void Generator::generateBodeSeeds(std::vector<ProtoplanetSeed>& protoplanetSeeds, const Star& star)
{
#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf("Generating Bode seeds:\n");
    }
#endif

    // The Blagg formulation of Bode's Law is:
    // SMA (AU) = A * (B + f(a + nb)) * 1.7275 ^ n
    // where:
    // f(theta) = 0.249 + 0.86 * ( cos(theta)/(3 - cos(2 * theta)) + 1/(6 - 4*cos(2 * (theta - pi/6))) )
    // A = 0.4162
    // B = 2.025
    // a =  84.9*
    // b =  56.6*
    // n = integer (-2, -1, 0, 1, 2, ...)
    //
    // This formulation is pretty accurate for the solar system - within 5% on all of the major planets except
    // Neptune (which is 5.6%).  It also is reasonable for the moons of the gas giants with tweaking of the constants.
    //
    // For this simulation's sake, we'll vary A and B within a small range (+/- 5%), with additional scaling
    // of A based on the ideal ecosphere of the central star.  This way, we can more-or-less guarantee that the first
    // protoplanet (n = 0) is within or close to the ecosphere.

    // A in the Blagg formulation, initially 0.4162.
    // We scale that value by the ecosphere radius, and then apply a random
    // Gaussian distribution within a few percent of that result.
    const double A = 0.4162 * star.getEcosphere() * randomNear(1.0f, 0.04f);

    // B in the Blagg formulation, initially 2.025.
    const double B = 2.025 * randomNear(1.0f, 0.04f);

    // alpha in the Blagg formulation.  For our solar system, it's 84.9 degrees (~1.4818).
    // We'll use a completely random value here.
    const float a = randomTwoPi();

    // beta in the Blagg formulation.  For our solar system, it's 56.6 degrees (~0.9879).
    const float b = 0.9879f;

    // Generate the seeds:
    ProtoplanetSeed s;

    s.semiMajorAxis = BodeSequence(0, A, B, a, b);
    s.eccentricity = randomEccentricity();

    protoplanetSeeds.emplace_back(s);
#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        printf(" ... n =  0 - SMA = %.3lf, ecc = %.3f\n", s.semiMajorAxis, s.eccentricity);
    }
#endif

    int32_t n = 1;
    bool added = false;
    do
    {
        added = false;
        s.semiMajorAxis = BodeSequence(-n, A, B, a, b);
        s.eccentricity = randomEccentricity();
        if (s.semiMajorAxis >= protoplanetZone.first)
        {
            protoplanetSeeds.emplace_back(s);
            added = true;
#ifdef ALLOW_DEBUG_PRINTF
            if (config.verboseLogging)
            {
                printf(" ... n = %2d - SMA = %.3lf, ecc = %.3f\n", -n, s.semiMajorAxis, s.eccentricity);
            }
#endif
        }

        s.semiMajorAxis = BodeSequence(n, A, B, a, b);
        s.eccentricity = randomEccentricity();
        if (s.semiMajorAxis <= protoplanetZone.second)
        {
            protoplanetSeeds.emplace_back(s);
            added = true;
#ifdef ALLOW_DEBUG_PRINTF
            if (config.verboseLogging)
            {
                printf(" ... n = %2d - SMA = %.3lf, ecc = %.3f\n", n, s.semiMajorAxis, s.eccentricity);
            }
#endif
        }

        ++n;
    } while (added);

    // Reorder them?
    size_t i = 1u;
    while (i < protoplanetSeeds.size() - 1u)
    {
        const size_t otherIdx = randomUniformInt(1ull, protoplanetSeeds.size() - 1u);
        if (i != otherIdx)
        {
#ifdef ALLOW_DEBUG_PRINTF
            if (config.verboseLogging)
            {
                printf(" ... Swapping [%Iu] and [%Iu]\n", i, otherIdx);
            }
#endif
            std::swap(protoplanetSeeds[i], protoplanetSeeds[otherIdx]);
        }
        ++i;
    }
}

//----------------------------------------------------------------------------
void Generator::generateStar(SolarSystem& system)
{
    // Range of stellar masses that provide the best results.
    static constexpr float MinStellarMass = 0.59f;
    static constexpr float MaxStellarMass = 1.30f;

    const float starMass = randomUniform(MinStellarMass, MaxStellarMass);
    const StarType_t type = GetStarType(starMass);

    Star star(type.first, type.second);
    star.setName(system.star.getName());
    star.evaluate(this);

    system.add(star);
#ifdef ALLOW_DEBUG_PRINTF
    if (config.verboseLogging)
    {
        char st[6];
        star.getStellarClass(st, sizeof(st));
        printf("Generated random star %s\n", st);
    }
#endif
}

//----------------------------------------------------------------------------
void Generator::updateDustLanes(const Protoplanet& protoplanet)
{
    // The protoplanet will sweep up all of the dust.
    dustRemains = false;

    // The protoplane twill sweep up all of the gas only if it exceeds the critical mass.
    const bool gasRemains = (protoplanet.mass < protoplanet.criticalMass);

    AvailableDust::iterator currentBand = availableDust.begin();

    while (currentBand != availableDust.end())
    {
        if (currentBand->innerEdge < protoplanet.r_inner && currentBand->outerEdge > protoplanet.r_outer)
        {
            // This dustband contains the entire swept region, so we must split it in three
            // pieces (inner band, outer band, and the band that covers the inner to outer
            // radius).
            Dust middleBand(protoplanet.r_inner, protoplanet.r_outer, false, currentBand->gasPresent && gasRemains);
            Dust outerBand(protoplanet.r_outer, currentBand->outerEdge, currentBand->dustPresent, currentBand->gasPresent);

            //currentBand->innerEdge is unchanged.
            currentBand->outerEdge = protoplanet.r_inner;
            // currentBand->dustPresent is unchanged.
            // currentBand->gasPresent is unchanged.

            availableDust.emplace_after(currentBand, middleBand);
            ++currentBand;
            // now points at the middle band.

            availableDust.emplace_after(currentBand, outerBand);
            ++currentBand;
            // now points at the outer band.
        }
        else if (currentBand->innerEdge < protoplanet.r_outer && currentBand->outerEdge >= protoplanet.r_outer)
        {
            // This dustband straddles the outer limit of the swept region.  We split the
            // dustband and add the new dustband after the current dustband.
            Dust newBand(protoplanet.r_outer, currentBand->outerEdge, currentBand->dustPresent, currentBand->gasPresent);

            //currentBand->innerEdge is unchanged
            currentBand->outerEdge = protoplanet.r_outer;
            currentBand->dustPresent = false;
            currentBand->gasPresent = currentBand->gasPresent && gasRemains;

            availableDust.emplace_after(currentBand, newBand);
            ++currentBand;
        }
        else if (currentBand->innerEdge <= protoplanet.r_inner && currentBand->outerEdge > protoplanet.r_inner)
        {
            // This dustband straddles the inner limit of the region we swept.  We split currentBand
            // into two chunks.

            // New band covers the distance from r_inner to the original dustband's outer edge.
            // The old dustband now covers the distance up to r_inner.
            Dust newBand(protoplanet.r_inner, currentBand->outerEdge, false, currentBand->gasPresent && gasRemains);

            //currentBand->innerEdge is unchanged.
            currentBand->outerEdge = protoplanet.r_inner;
            //currentBand->dustPresent is unchanged.
            //currentBand->gasPresent is unchanged.

            availableDust.emplace_after(currentBand, newBand);
            ++currentBand;
        }
        else if (currentBand->innerEdge >= protoplanet.r_inner && currentBand->outerEdge <= protoplanet.r_outer)
        {
            // This dustband is contained entirely within the swept bounds.  Update its contents.
            if (currentBand->gasPresent)
            {
                currentBand->gasPresent = gasRemains;
            }
            currentBand->dustPresent = false;
        }
        else
        {
            // There's a hole in the algorithm if this fires
            assert(currentBand->outerEdge < protoplanet.r_inner || currentBand->innerEdge > protoplanet.r_outer);
        }
        ++currentBand;
    }

    // See if dust remains and coalesce identical adjacent dustbands (dust presence and gas presence match).
    currentBand = availableDust.begin();
    while (currentBand != availableDust.end())
    {
        if (currentBand->dustPresent && currentBand->outerEdge >= protoplanetZone.first && currentBand->innerEdge <= protoplanetZone.second)
        {
            dustRemains = true;
        }

        AvailableDust::iterator nextBand = currentBand;
        ++nextBand;
        if (nextBand != availableDust.end())
        {
            if (currentBand->dustPresent == nextBand->dustPresent && currentBand->gasPresent == nextBand->gasPresent)
            {
                currentBand->outerEdge = nextBand->outerEdge;
                availableDust.erase_after(currentBand);

                // Don't advance the current band iterator, in case we can coalesce multiple bands.
            }
            else
            {
                ++currentBand;
            }
        }
        else
        {
            ++currentBand;
        }
    }
}

}
}
