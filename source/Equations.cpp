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
#include <qcSysGen/Equations.h>

#include <qcSysGen/Planet.h>
#include <qcSysGen/Star.h>

namespace qcSystemGenerator
{

//----------------------------------------------------------------------------
double CriticalLimit(double sma, double eccentricity, double stellarLuminosity)
{
    static constexpr double B = 1.2e-5;

    const double perihelion = (sma - sma * eccentricity);
    const double term = perihelion * sqrt(stellarLuminosity);

    return B * pow(term, -0.75);
}

//----------------------------------------------------------------------------
double GasLife(double molecularMass, const Planet* planet)
{
    const double v = RMSVelocity(molecularMass, planet->exosphereTemperature()) * CmPerM;

    const double g = planet->surfaceAcceleration() * CmPerM;

    const double r = planet->r() * CmPerKm;

    const double t = (pow(v, 3.0) / (2.0 * pow(g, 2.0) * r)) * exp((3.0 * g * r) / pow(v, 2.0));

    return t * YearsPerSecond;
}

//----------------------------------------------------------------------------
double KothariRadius(double mass, double sma, bool forGasGiant, const Star* star)
{
    // nb: atomicWeight is A in Kothari's equation.  atomicNumber is Z.

    const float orbitZone = star->materialZone(sma);
    int zoneIndex = (orbitZone < 2.0f) ? 0 : 1;
    double interpolant = (orbitZone - floorf(orbitZone));

    if (forGasGiant)
    {
        zoneIndex += 3;
    }

    // [rocky Zone 1, rocky Zone 2, rocky Zone 3, gas Zone 1, gas Zone 2, gas Zone 3]
    constexpr double AtomicWeight[6] = { 15.0, 10.0, 10.0, 9.5, 2.47, 7.0 };
    constexpr double AtomicNumber[6] = { 8.0, 5.0, 5.0, 4.5, 2.0, 4.0 };

    const double atomicWeight = Lerp(interpolant, AtomicWeight[zoneIndex], AtomicWeight[zoneIndex + 1]);
    const double atomicNumber = Lerp(interpolant, AtomicNumber[zoneIndex], AtomicNumber[zoneIndex + 1]);

    const double ZA = atomicWeight * atomicNumber;

    double radius = (2.0 * K_B * pow(SolarMassInGrams, (1.0 / 3.0))) / (K_A1 * pow(ZA, (1.0 / 3.0)));

    double denominator = K_A2 * pow(atomicWeight, (4.0 / 3.0)) * pow(SolarMassInGrams, (2.0 / 3.0));
    denominator *= pow(mass, (2.0 / 3.0));
    denominator /= (K_A1 * (atomicNumber * atomicNumber));
    denominator += 1.0;

    radius /= denominator;
    radius = (radius * pow(mass, (1.0 / 3.0))) * KmPerCm;

    return radius;
}

//----------------------------------------------------------------------------
double Luminosity(double stellarMass)
{
    double n;

    if (stellarMass < 1.0)
    {
        n = 1.75 * (stellarMass - 0.1) + 3.325;
    }
    else
    {
        n = 0.5 * (2.0 - stellarMass) + 4.4;
    }

    return pow(stellarMass, n);
}

//----------------------------------------------------------------------------
double MinimumMolecularWeight(const Planet* planet, const Star* star)
{
    // We will search through various molecular masses to find the one that is
    // closest to the age of the planetary system.  We initialize the search to
    // the molecular limit of the planet, and check its gas life.
    const double goalAge = star->age();

    double molecularMass = MolecularLimit(planet->escapeVelocity(), planet->exosphereTemperature());
    double previousMass = molecularMass;

    double gasLife = GasLife(molecularMass, planet);

    if (gasLife > goalAge)
    {
        // Gas retention is high for the starting mass.  Reduce the weight to find the right range.
        while (gasLife > goalAge)
        {
            previousMass = molecularMass;
            molecularMass *= 0.5;

            gasLife = GasLife(molecularMass, planet);
        }
    }
    else
    {
        // Gas retention is low.  Increase the weight to find something that's retained.
        while (gasLife < goalAge)
        {
            previousMass = molecularMass;
            molecularMass *= 2.0;

            gasLife = GasLife(molecularMass, planet);
        }
        // molecularMass is the lower of the two values in the binary search below.
        std::swap(previousMass, molecularMass);
    }

    // Binary search between the two end points
    while (previousMass - molecularMass > 0.1)
    {
        const double midMass = (previousMass + molecularMass) * 0.5;
        gasLife = GasLife(midMass, planet);

        if (gasLife < goalAge)
        {
            molecularMass = midMass;
        }
        else
        {
            previousMass = midMass;
        }
    }

    return (previousMass + molecularMass) * 0.5;
}

//----------------------------------------------------------------------------
double Period(double distance, double mass1, double mass2)
{
    const double periodYears = sqrt((distance * distance * distance) / (mass1 + mass2));
    return periodYears * DaysPerYear;
}

//----------------------------------------------------------------------------
double VolumeDensity(double mass, double radius)
{
    const double volume = (4.0 * PI * pow((radius * CmPerKm), 3.0)) / 3.0;

    return (mass * SolarMassInGrams) / volume;
}

}
