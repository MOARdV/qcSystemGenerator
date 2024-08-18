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

#include <qcSysGen/GenerationState.h>

#include <qcSysGen/Equations.h>
#include <qcSysGen/Planet.h>

#include <assert.h>

namespace qcSystemGenerator
{

//----------------------------------------------------------------------------
GenerationState::GenerationState(const Config* config) :
    seedValue(config->seed),
    protoplanetSeedMass(config->protoplanetSeedMass),
    star(nullptr),
    uniform01(0.0, 1.0)
{
    mt.seed(config->seed);

    cloudEccentricity = config->cloudEccentricity;
    baseDustDensity = config->dustDensity;

    availableDust.clear();

    dustRemains = false;
    planetList.clear();

    doMoons = config->generateMoons;

    callback = config->callback;
}

//----------------------------------------------------------------------------
void GenerationState::accreteDust(Protoplanet& protoplanet)
{
    double oldMass;
    double newMass = protoplanet.mass;

    protoplanet.criticalMass = CriticalLimit(protoplanet.sma, protoplanet.eccentricity, star->luminosity());

    double dustMass, gasMass;
    do
    {
        oldMass = newMass;

        protoplanet.reducedMass = pow(oldMass / (1.0 + oldMass), (1.0 / 4.0));
        protoplanet.r_inner = innerEffectLimit(protoplanet.sma, protoplanet.eccentricity, protoplanet.reducedMass);
        protoplanet.r_outer = outerEffectLimit(protoplanet.sma, protoplanet.eccentricity, protoplanet.reducedMass);

        newMass = collectDust(oldMass, dustMass, gasMass, protoplanet, availableDust.begin());
    } while (newMass > 0.0 && abs(newMass - oldMass) >= 0.0001 * oldMass);

    protoplanet.mass += newMass;
    protoplanet.dustMass += dustMass;
    protoplanet.gasMass += gasMass;

    updateDustLanes(protoplanet);

    if (protoplanet.mass > protoplanetSeedMass)
    {
        coalescePlanetisimals(protoplanet);
    }
}

//----------------------------------------------------------------------------
void GenerationState::coalescePlanetisimals(Protoplanet& protoplanet)
{
    bool finished = false;

    PlanetList::iterator planet = planetList.begin();
    while (planet != planetList.end())
    {
        // Difference in semi-major axis
        const double diff = planet->sma() - protoplanet.sma;

        // TODO: Sort of reducedMass.  The way it's used seems wonky.  Here, the reduced mass is computed
        // using an exiting planet's mass.  But, during accretion, it uses the protoplanet's mass.  It's
        // been that way since accrete, but it seems odd.
        double dist1, dist2;
        if (diff > 0.0)
        {
            dist1 = (protoplanet.sma * (1.0 + protoplanet.eccentricity) * (1.0 + protoplanet.reducedMass)) - protoplanet.sma;
            // x aphelion
            protoplanet.reducedMass = pow((planet->mass() / (1.0 + planet->mass())), (1.0 / 4.0));
            dist2 = planet->sma()
                - (planet->sma() * (1.0 - planet->e()) * (1.0 - protoplanet.reducedMass));
        }
        else
        {
            dist1 = protoplanet.sma - (protoplanet.sma * (1.0 - protoplanet.eccentricity) * (1.0 - protoplanet.reducedMass));
            // x perihelion
            protoplanet.reducedMass = pow((planet->mass() / (1.0 + planet->mass())), (1.0 / 4.0));
            dist2 = (planet->sma() * (1.0 + planet->e()) * (1.0 + protoplanet.reducedMass))
                - planet->sma();
        }

        if (((fabs(diff) <= fabs(dist1)) || (fabs(diff) <= fabs(dist2))))
        {
            // TODO: Double-check these equations
            const double newSMA = (planet->mass() + protoplanet.mass) /
                ((planet->mass() / planet->sma()) + (protoplanet.mass / protoplanet.sma));

            double e2 = planet->mass() * sqrt(planet->sma()) * sqrt(1.0 - pow(planet->e(), 2.0));
            e2 += protoplanet.mass * sqrt(protoplanet.sma) * sqrt(sqrt(1.0 - pow(protoplanet.eccentricity, 2.0)));
            e2 /= (planet->mass() + protoplanet.mass) * sqrt(newSMA);
            e2 = std::max(0.0, 1.0 - pow(e2, 2.0));
            if (e2 >= 1.0)
            {
                if (callback)
                {
                    char text[256];
                    sprintf_s(text, "Planetoid collision should have resulted in planetoid escape (eccentricity = %lg)\n", sqrt(e2));
                    callback(text);
                }
                e2 = 0.0;
            }

            const float newE = static_cast<float>(sqrt(e2));

            if (doMoons)
            {
                if (protoplanet.mass < protoplanet.criticalMass)
                {
                    double existingMoonMass = planet->totalMoonMass();

                    if ((protoplanet.mass * SolarMassToEarthMass) < 2.5
                        && (protoplanet.mass * SolarMassToEarthMass) > .0001
                        && existingMoonMass < (planet->mass() * .05)
                        )
                    {
                        Planet moon(protoplanet.sma, protoplanet.eccentricity, protoplanet.dustMass, protoplanet.gasMass);

                        if (protoplanet.mass > planet->mass())
                        {
                            // Swap the parameters for planet and this moon.  Add the old planet / new moon
                            // to the new planet's menagerie.
                            planet->exchange(moon);
                        }
                        else
                        {
                            planet->addMoon(moon);
                        }

                        // Early return - the planet captured the protoplanet or vice-versa
                        return;
                    }
                }
            }

            // Protoplanet collision
            Protoplanet newProtoplanet;
            newProtoplanet.sma = newSMA;
            newProtoplanet.eccentricity = newE;
            newProtoplanet.mass = planet->mass() + protoplanet.mass;
            newProtoplanet.dustMass = planet->dustMassComponent() + protoplanet.dustMass;
            newProtoplanet.gasMass = planet->gasMassComponent() + protoplanet.gasMass;

            // Remove planet from the list - accreteDust() will replace it with the merged protoplanet.
            planetList.remove_if([planet](Planet& p) { return (p.sma() == planet->sma()); });

            // Sweep the dustbands with the protoplanet that represents the merged mass.
            accreteDust(newProtoplanet);

            // Return early - finished, and we added the merged planetisimal in accreteDust
            return;
        }

        ++planet;
    }

    if (!finished)
    {
        // The protoplanet didn't interact with anything already in the planetary system, so we need to add it as a new
        // body.
        Planet newPlanet(protoplanet.sma, static_cast<float>(protoplanet.eccentricity), protoplanet.dustMass, protoplanet.gasMass);

        if (planetList.empty() || planetList.front().sma() > newPlanet.sma())
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
                else if (currentPlanet->sma() < newPlanet.sma() && nextPlanet->sma() >= newPlanet.sma())
                {
                    planetList.emplace_after(currentPlanet, newPlanet);
                    break;
                }
                ++currentPlanet;
                ++nextPlanet;
            }
        }
    }
}

//----------------------------------------------------------------------------
double GenerationState::collectDust(double lastMass, double& dustMass, double& gasMass, Protoplanet& protoplanet, AvailableDust::iterator dustband)
{
    dustMass = 0.0;
    gasMass = 0.0;

    if (dustband == availableDust.end())
    {
        // Break the recursion - we're at the end of the list.
        return 0.0;
    }

    if ((dustband->outerEdge <= protoplanet.r_inner) || (dustband->innerEdge >= protoplanet.r_outer))
    {
        // Skip this dustband - this dustband is outside the range of the effect radius.
        return collectDust(lastMass, dustMass, gasMass, protoplanet, ++dustband);
    }

    // TODO: Where do these values come from?
    static constexpr double Alpha = 5.0;
    static constexpr double N = 3.0;

    const double dustDensity = baseDustDensity * exp(-Alpha * pow(protoplanet.sma, 1.0 / N));
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

    const double area = 4.0 * PI * pow(protoplanet.sma, 2.0) * protoplanet.reducedMass * (1.0 - protoplanet.eccentricity * (outerTemp - innerTemp) / bandWidth);

    const double volume = area * width;

    const double newMass = volume * massDensity;

    gasMass = volume * gasDensity;

    dustMass = newMass - gasMass;
    assert(dustMass >= 0.0);

    // Recurse to the next dust band.
    double nextDustMass, nextGasMass;
    const double nextMass = collectDust(lastMass, nextDustMass, nextGasMass, protoplanet, ++dustband);

    dustMass += nextDustMass;
    gasMass += nextGasMass;

    return newMass + nextMass;
}

//----------------------------------------------------------------------------
void GenerationState::updateDustLanes(const Protoplanet& protoplanet)
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
            // The dustband covers the entire region, so we must split it in three
            // pieces (inner band, outer band, and the band that covers the inner to outer
            // radius).
            Dust middleBand(protoplanet.r_inner, protoplanet.r_outer, false, (currentBand->gasPresent) ? gasRemains : false);
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
        else if (currentBand->innerEdge < protoplanet.r_outer && currentBand->outerEdge > protoplanet.r_outer)
        {
            // This dustband straddles the outer limit of the region we touched.  We split the
            // region and add the new region after the current dustband.
            Dust newBand(protoplanet.r_outer, currentBand->outerEdge, currentBand->dustPresent, currentBand->gasPresent);

            //currentBand->innerEdge is unchanged
            currentBand->outerEdge = protoplanet.r_outer;
            currentBand->dustPresent = false;
            currentBand->gasPresent = (currentBand->gasPresent) ? gasRemains : false;

            availableDust.emplace_after(currentBand, newBand);
            ++currentBand;
        }
        else if (currentBand->innerEdge < protoplanet.r_inner && currentBand->outerEdge > protoplanet.r_inner)
        {
            // This dustband straddles the inner limit of the region we touched.  We split currentBand
            // into two chunks.

            // New band covers the distance from r_inner to the original dustband's outer edge.
            // The old dustband now covers the distance up to r_inner.
            Dust newBand(protoplanet.r_inner, currentBand->outerEdge, false, (currentBand->gasPresent) ? (gasRemains) : false);

            //currentBand->innerEdge is unchanged.
            currentBand->outerEdge = protoplanet.r_inner;
            //currentBand->dustPresent is unchanged.
            //currentBand->gasPresent is unchanged.

            availableDust.emplace_after(currentBand, newBand);
            ++currentBand;
        }
        else if (currentBand->innerEdge >= protoplanet.r_inner && currentBand->outerEdge <= protoplanet.r_outer)
        {
            // This node is contained entirely within the bounds.  Update its contents
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
        if (currentBand->dustPresent && currentBand->outerEdge >= star->innerPlanetaryLimit() && currentBand->innerEdge <= star->outerPlanetaryLimit())
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
