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

namespace qc
{

namespace SystemGenerator
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
double KothariRadius(double mass, double sma, bool forGasGiant, float materialZone)
{
    // nb: atomicWeight is A in Kothari's equation.  atomicNumber is Z.

    // Parameter alpha1 in Fogg 1985 equation 9 / Kothari 1936 equation 23.
    static constexpr double alpha1 = 6.485e12;

    // Parameter alpha2 in Fogg 1985 equation 9 / Kothari 1936 equation 23.
    static constexpr double alpha2 = 4.0032e-8;

    // Parameter beta in Fogg 1985 equation 9 / Kothari 1936 equation 23.
    static constexpr double beta = 5.71e12;

    int zoneIndex = (int)(materialZone) - 1;
    double interpolant = (materialZone - floorf(materialZone));

    if (forGasGiant)
    {
        zoneIndex += 3;
    }

    // [rocky Zone 1, rocky Zone 2, rocky Zone 3, gas Zone 1, gas Zone 2, gas Zone 3]
    // The original gas Zone 1 is questionable.  It leads to very high density inner zone
    // gas giants.  For now, I'll lower those weights to match Zone 2.
    static constexpr double AtomicWeight[6] = { 15.0, 10.0, 10.0, /*9.5*/ 2.47, 2.47, 7.0 };
    static constexpr double AtomicNumber[6] = { 8.0, 5.0, 5.0, /*4.5*/ 2.0, 2.0, 4.0 };

    const double A = Lerp(interpolant, AtomicWeight[zoneIndex], AtomicWeight[zoneIndex + 1]);
    const double Z = Lerp(interpolant, AtomicNumber[zoneIndex], AtomicNumber[zoneIndex + 1]);

    // Kothari 1936 eq 23, broken down
    static constexpr double numeratorT1 = 2.0 * beta / alpha1;
    const double numeratorT2 = 1.0 / pow(Z * A, (1.0 / 3.0));
    const double numeratorT3 = pow(SolarMassInGrams, (1.0 / 3.0));
    const double numerator = numeratorT1 * numeratorT2 * numeratorT3;

    static constexpr double denominatorT1 = alpha2 / alpha1;
    const double denominatorT2 = pow(A, (4.0 / 3.0)) / (Z * Z);
    const double denominatorT3 = pow(SolarMassInGrams, (2.0 / 3.0));
    const double denominatorT4 = pow(mass, (2.0 / 3.0));
    const double denominator = 1.0 + denominatorT1 * denominatorT2 * denominatorT3 * denominatorT4;

    const double radius = numerator / denominator * pow(mass, (1.0 / 3.0)) * KmPerCm;

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
}
