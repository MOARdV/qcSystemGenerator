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
#include "System.h"

#include "GenerationState.h"

#include <assert.h>
#include <string>

//----------------------------------------------------------------------------
std::string RomanNumeral(int value)
{
    static const char* tens[] = { "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC" };
    static const char* ones[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" };

    const int tDigit = (value / 10) % 10;
    const int oDigit = (value % 10);

    std::string str(tens[tDigit]);
    str.append(ones[oDigit]);

    return str;
}

//----------------------------------------------------------------------------
std::string PlanetOrdinal(int planetOrd)
{
    std::string str(RomanNumeral(planetOrd));
    if (str.length() < 8u)
    {
        str.append(8u - str.length(), ' ');
    }

    return str;
}

//----------------------------------------------------------------------------
std::string MoonOrdinal(int planetOrd, int moonOrd)
{
    std::string str(" ");
    str.append(RomanNumeral(planetOrd));
    char s[4];
    sprintf_s(s, "%c", 'a' + moonOrd - 1);
    str.append(s);
    if (str.length() < 8u)
    {
        str.append(8u - str.length(), ' ');
    }

    return str;
}

//----------------------------------------------------------------------------
char AsciiArtType(PlanetType type)
{
    switch (type)
    {
        case PlanetType::IceGiant:
            return 'o';

        case PlanetType::GasGiant:
            return 'O';

        case PlanetType::AsteroidBelt:
            return ':';
        //case PlanetType::Goldilocks:
        //    return 'G';
        default:
        case PlanetType::Rocky:
            return '.';
    }
}

//----------------------------------------------------------------------------
std::string SMA(double sma)
{
    char smaStr[16];

    sprintf_s(smaStr, "%8.3lfAU  ", sma);
    return std::string(smaStr);
}

//----------------------------------------------------------------------------
std::string km(double sma)
{
    char smaStr[16];

    sprintf_s(smaStr, "%8.0lfkm  ", sma * KmPerAu);
    return std::string(smaStr);
}

//----------------------------------------------------------------------------
std::string Radius(double radius)
{
    char radiusStr[16];

    sprintf_s(radiusStr, "%9.0lfkm ", radius);
    return std::string(radiusStr);
}

//----------------------------------------------------------------------------
std::string MeanSurfaceConditions(float surfaceTempCelsius, float surfacePressureMb)
{
    char tempStr[64];

    sprintf_s(tempStr, "%4.0f*C %7.3fatm ", surfaceTempCelsius, surfacePressureMb / EarthSurfacePressure);
    return std::string(tempStr);
}

//----------------------------------------------------------------------------
void SpewPlanet(const Planet& pl, int planetOrdinal)
{
    std::string planetInfo;

    if (planetOrdinal >= 0)
    {
        planetInfo.append(MoonOrdinal(planetOrdinal, pl.ordinal())).append(" - ").append(Planet::PlanetTypeString(pl.planetType())).append(" moon");
    }
    else
    {
        planetInfo.append(PlanetOrdinal(pl.ordinal())).append(" - ").append(Planet::PlanetTypeString(pl.planetType()));
    }
    puts(planetInfo.c_str());
    planetInfo.clear();

    if (pl.planetType() != PlanetType::AsteroidBelt)
    {
        char buffer[64];

        planetInfo.append("\tSemi-major axis: ");
        if (planetOrdinal >= 0)
        {
            planetInfo.append(km(pl.sma()));
        }
        else
        {
            planetInfo.append(SMA(pl.sma()));
        }
        puts(planetInfo.c_str());
        planetInfo.clear();

        planetInfo.append("\tRadius: ").append(Radius(pl.r()));
        puts(planetInfo.c_str());
        planetInfo.clear();

        sprintf_s(buffer, "%4.1fg/cc", pl.density());
        planetInfo.append("\tDensity: ").append(buffer);
        puts(planetInfo.c_str());
        planetInfo.clear();

        if (!pl.isGaseous())
        {
            sprintf_s(buffer, "%4.2f", pl.esi());
            planetInfo.append("\tESI: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            sprintf_s(buffer, "%4.0f*C - %3.0f%% ocean, %3.0f%% ice", pl.surfaceTemperature() + KelvinToCelsius, 100.0f * pl.hydroPercentage(), 100.0f * pl.icePercentage());
            planetInfo.append("\tSurface Temp: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            sprintf_s(buffer, "%4.1fg", pl.surfaceGravity());
            planetInfo.append("\tSurface Gravity: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            sprintf_s(buffer, "%7.3fatm - %3.0f%% cloud coverage", pl.surfacePressure() / EarthSurfacePressure, 100.0f * pl.cloudPercentage());
            planetInfo.append("\tSurface Press: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            if (pl.esi() > 0.80f)
            {
                assert(pl.surfacePressure() > 0.0f);
                puts("\tAtmosphere:");

                bool firstTrace = true;
                for (auto gas = pl.beginAtmo(); gas != pl.endAtmo(); ++gas)
                {
                    bool showPercent = (gas->fraction >= 0.001f);
                    bool showPPM = (gas->fraction >= 0.000001f);

                    if (showPercent)
                    {
                        sprintf_s(buffer, "%5.1f%%", gas->fraction * 100.0f);
                        planetInfo.append("\t\t").append(Planet::GasString(gas->gas)).append(": ").append(buffer);
                        puts(planetInfo.c_str());
                        planetInfo.clear();
                    }
                    else if (showPPM)
                    {
                        sprintf_s(buffer, "%3.0fppm", gas->fraction*1000000.0f);
                        planetInfo.append("\t\t").append(Planet::GasString(gas->gas)).append(": ").append(buffer);
                        puts(planetInfo.c_str());
                        planetInfo.clear();
                    }
                    else
                    {
                        if (firstTrace)
                        {
                            firstTrace = false;
                            planetInfo.append("\t\t").append("Trace ");
                        }
                        else
                        {
                            planetInfo.append(", ");
                        }
                        planetInfo.append(Planet::GasString(gas->gas));
                    }
                }
                puts(planetInfo.c_str());
                planetInfo.clear();
            }
        }
    }
}

//----------------------------------------------------------------------------
int main(int, char**)
{
#if 0
    // Sanity checks
    Star sun(1.0f, 0.5f);
    const double totalMass = 3.0e-6;
    const double gasMass = 0.0;
    const double dustMass = totalMass - gasMass;
    Planet earth(1.0f, 0.0167f, dustMass, gasMass);

    Config cfg;
    cfg.stellarMass = 1.0f;
    cfg.seed = 1;
    cfg.computeGases = true;
    GenerationState state(&cfg);
    earth.evaluate(&sun, nullptr, 3, &cfg, &state);
#endif
    System system;

    Config config;
    //config.stellarMass = 1.01f;
    config.randomAxialTilt = true;
    config.generateBodeSeeds = true;
    config.generateMoons = true;
    config.computeGases = true;

    system.create(&config);

    const size_t numPlanets = system.size();

    OrbitalZone lastZone = OrbitalZone::Inner;

    char starClass[4];
    system.star().stellarClass(starClass, sizeof(starClass));
    printf("Central Star: %s - seed 0x%I64X\n %u protoplanets consumed\n\n", starClass, system.seed(), system.protoplanets());

    printf("=== INNER ZONE     %6.3lfAU ==================================================\n", 0.0);
    for (const auto& pl : system)
    {
        const OrbitalZone thisZone = system.star().zone(pl.sma());
        if (thisZone != lastZone)
        {
            if (thisZone == OrbitalZone::Habitable)
            {
                printf("=== HABITABLE ZONE %6.3lfAU ==================================================\n", system.star().innerHabitableZone());
            }
            else if (thisZone == OrbitalZone::Middle)
            {
                if (lastZone != OrbitalZone::Habitable)
                {
                    printf("=== HABITABLE ZONE %6.3lfAU ==================================================\n", system.star().innerHabitableZone());
                }
                printf("=== MIDDLE ZONE    %6.3lfAU ==================================================\n", system.star().outerHabitableZone());
            }
            else if (thisZone == OrbitalZone::Outer)
            {
                printf("=== OUTER ZONE     %6.3lfAU ==================================================\n", system.star().snowLine());
            }
            lastZone = thisZone;
        }

        std::string planetInfo;
        planetInfo.append(PlanetOrdinal(pl.ordinal()));
        planetInfo.append(2, ' ').append(1, AsciiArtType(pl.planetType())).append(2, ' ');
        planetInfo.append(SMA(pl.sma()));
        planetInfo.append(Radius(pl.r()));

        if (!pl.isGaseous())
        {
            planetInfo.append(MeanSurfaceConditions(pl.surfaceTemperature() + KelvinToCelsius, pl.surfacePressure()));
            char esi[16];
            sprintf_s(esi, "esi: %4.2f", pl.esi());
            planetInfo.append(esi);
        }

        puts(planetInfo.c_str());
        if (pl.sizeMoon() > 0)
        {
            std::for_each(pl.beginMoon(), pl.endMoon(), [&](const auto& m)
                {
                    std::string moonInfo(MoonOrdinal(pl.ordinal(), m.ordinal()));
                    moonInfo.append(2, ' ').append(1, AsciiArtType(m.planetType())).append(2, ' ');
                    moonInfo.append(km(m.sma()));
                    moonInfo.append(Radius(m.r()));
                    if (!m.isGaseous())
                    {
                        moonInfo.append(MeanSurfaceConditions(m.surfaceTemperature() + KelvinToCelsius, m.surfacePressure()));
                        char esi[16];
                        sprintf_s(esi, "esi: %4.2f", m.esi());
                        moonInfo.append(esi);
                    }

                    puts(moonInfo.c_str());
                });
        }
    }

    for (const auto& pl : system)
    {
        puts("\n==============================================================================");

        SpewPlanet(pl, -1);
        if (pl.sizeMoon() > 0)
        {
            std::for_each(pl.beginMoon(), pl.endMoon(), [&](const auto& m)
                {
                    puts("");
                    SpewPlanet(m, pl.ordinal());
                });
        }
    }

    return 0;
}
